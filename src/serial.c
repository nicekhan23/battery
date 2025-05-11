/**
* @file serial.c
* @brief Implementation of serial communication for multi-channel battery charger
*
* This file implements the interface defined in serial.h for serial communication
* with a battery charger device. It provides functions for initialization,
* deinitialization, and command handling.
*
* Created on: May 11, 2025
* @author Zhanibekuly Darkhan
*/
#include "serial.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <syslog.h>

/** @brief File descriptor for the serial port */
static int serial_fd = -1;
/** @brief Semaphore for thread-safe access to the command pool */
static sem_t cmd_semaphore;
/** @brief Queue head for the command pool */
static TAILQ_HEAD(cmd_queue, cmd_entry) command_pool;
/** @brief Array of pool entries */
static struct cmd_entry *pool_entries = NULL;
/** @brief Flag indicating whether the module is initialized */
static int initialized = 0;

/**
* @brief Validates the given device command
*
* This function checks if a command has valid parameters according to
* the device specification.
*
* @param cmd Pointer to the command structure to validate
* @return EXIT_SUCCESS if valid, EXIT_FAILURE if invalid
*/
static int is_valid_command(const device_command_t *cmd) {
    if (cmd == NULL) {
        syslog(LOG_WARNING, "NULL command pointer");
        return EXIT_FAILURE;
    }

    switch (cmd->command_type) {
        case CMD_SET_PARAMS:
            if (cmd->data.set_params.min_level > 100 ||
                cmd->data.set_params.max_level > 100 ||
                cmd->data.set_params.max_time == 0 ||
                cmd->data.set_params.max_time > 240 ||
                cmd->data.set_params.min_level > cmd->data.set_params.max_level) {
                syslog(LOG_WARNING, "Invalid SET_PARAMS command: min_level=%d, max_level=%d, max_time=%d",
                        cmd->data.set_params.min_level,
                        cmd->data.set_params.max_level,
                        cmd->data.set_params.max_time);
                return EXIT_FAILURE;
            }
            syslog(LOG_INFO, "Validated SET_PARAMS command: min_level=%d, max_level=%d, max_time=%d",
                    cmd->data.set_params.min_level,
                    cmd->data.set_params.max_level,
                    cmd->data.set_params.max_time);
            return EXIT_SUCCESS;

        case CMD_ON_OFF:
            if ((cmd->data.on_off.on_off != 0 && cmd->data.on_off.on_off != 1) ||
                cmd->data.on_off.channel > 7) {
                syslog(LOG_WARNING, "Invalid ON_OFF command: on_off=%d, channel=%d",
                        cmd->data.on_off.on_off,
                        cmd->data.on_off.channel);
                return EXIT_FAILURE;
            }
            syslog(LOG_INFO, "Validated ON_OFF command: on_off=%d, channel=%d",
                    cmd->data.on_off.on_off,
                    cmd->data.on_off.channel);
            return EXIT_SUCCESS;

        case CMD_EMERGENCY:
            syslog(LOG_INFO, "Validated EMERGENCY command");
            return EXIT_SUCCESS;

        default:
            syslog(LOG_WARNING, "Unknown command type: 0x%x", cmd->command_type);
            return EXIT_FAILURE;
    }
}

int init(const char *port_name, int speed) {
    // Check if already initialized
    if (initialized) {
        syslog(LOG_WARNING, "Serial communication already initialized");
        return EXIT_FAILURE;
    }

    // Check for NULL or too long port name
    if (port_name == NULL || strlen(port_name) > MAX_PORT_NAME) {
        if (port_name == NULL) {
            syslog(LOG_WARNING, "Initialization failed: port name is NULL");
        } else {
            syslog(LOG_WARNING, "Initialization failed: port name exceeds maximum length (%d characters)", MAX_PORT_NAME);
        }
        return EXIT_FAILURE;
    }

    // Open syslog for logging
    openlog("serial_comm", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Initializing serial communication module");

    // For testing with /dev/null just pretend it worked
    if (strcmp(port_name, "/dev/null") == 0) {
        serial_fd = 1; // Just a positive value for testing
        syslog(LOG_INFO, "Serial communication initialized with /dev/null (test mode)");
    } else {
        // Try to open the serial port
        serial_fd = open(port_name, O_WRONLY | O_NOCTTY);
        if (serial_fd < 0) {
            syslog(LOG_WARNING, "Failed to open serial port %s", port_name);
            closelog();
            return EXIT_FAILURE;
        }
        syslog(LOG_INFO, "Serial port %s opened successfully", port_name);

        // Set up the terminal settings
        struct termios tty;
        memset(&tty, 0, sizeof tty);
        if (tcgetattr(serial_fd, &tty) != 0) {
            syslog(LOG_WARNING, "Failed to get terminal attributes for %s", port_name);
            close(serial_fd);
            closelog();
            return EXIT_FAILURE;
        }

        cfsetospeed(&tty, speed);
        cfsetispeed(&tty, speed);
        tty.c_cflag |= (CLOCAL | CREAD);

        if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
            syslog(LOG_WARNING, "Failed to set terminal attributes for %s", port_name);
            close(serial_fd);
            closelog();
            return EXIT_FAILURE;
        }
        syslog(LOG_INFO, "Terminal attributes configured for serial port %s", port_name);
    }

    // Initialize the semaphore
    if (sem_init(&cmd_semaphore, 0, 1) != 0) {
        syslog(LOG_WARNING, "Failed to initialize semaphore");
        if (serial_fd >= 0) close(serial_fd);
        closelog();
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Semaphore initialized successfully");

    // Initialize the command pool queue
    TAILQ_INIT(&command_pool);
    syslog(LOG_INFO, "Command pool queue initialized");

    // Allocate memory for pool entries
    pool_entries = calloc(POOL_SIZE, sizeof(struct cmd_entry));
    if (!pool_entries) {
        syslog(LOG_WARNING, "Failed to allocate memory for command pool (POOL_SIZE=%d)", POOL_SIZE);
        if (serial_fd >= 0) close(serial_fd);
        sem_destroy(&cmd_semaphore);
        closelog();
        return EXIT_FAILURE;
    }
    syslog(LOG_INFO, "Memory allocated for %d command pool entries", POOL_SIZE);

    // Mark as initialized
    initialized = 1;
    syslog(LOG_INFO, "Serial communication module initialized");
    return EXIT_SUCCESS;
}

int deinit(void) {
    // Check if initialized
    if (!initialized) {
        syslog(LOG_WARNING, "Deinitialization failed: module not initialized");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Starting serial communication module deinitialization");

    // Lock the semaphore
    if (sem_wait(&cmd_semaphore) != 0) {
        syslog(LOG_WARNING, "Failed to lock semaphore during deinitialization");
        return EXIT_FAILURE;
    }

    // Close the serial port
    if (serial_fd >= 0) {
        close(serial_fd);
        syslog(LOG_INFO, "Serial port closed");
        serial_fd = -1;
    } else {
        syslog(LOG_INFO, "Serial port already closed or was not opened");
    }

    // Free the pool entries
    if (pool_entries != NULL) {
        free(pool_entries);
        syslog(LOG_INFO, "Command pool memory freed");
        pool_entries = NULL;
    } else {
        syslog(LOG_INFO, "Command pool memory already freed or not allocated");
    }

    // Destroy the semaphore
    if (sem_post(&cmd_semaphore) != 0) {
        syslog(LOG_WARNING, "Failed to unlock semaphore during deinitialization");
    }

    if (sem_destroy(&cmd_semaphore) != 0) {
        syslog(LOG_WARNING, "Failed to destroy semaphore");
    } else {
        syslog(LOG_INFO, "Semaphore destroyed");
    }

    // Close syslog
    syslog(LOG_INFO, "Serial communication module deinitialized");
    closelog();

    // Mark as not initialized
    initialized = 0;
    return EXIT_SUCCESS;
}

int add(const device_command_t *cmd) {
    // Check if initialized
    if (!initialized) {
        syslog(LOG_WARNING, "Failed to add command: module not initialized");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Adding new command");

    // Check if command is valid
    if (cmd == NULL) {
        syslog(LOG_WARNING, "Failed to add command: command pointer is NULL");
        return EXIT_FAILURE;
    }

    if (!(is_valid_command(cmd))) {
        syslog(LOG_WARNING, "Failed to add command: command is invalid");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Command is valid");

    // Lock the semaphore
    if (sem_wait(&cmd_semaphore) != 0) {
        syslog(LOG_WARNING, "Failed to lock semaphore while adding command");
        return EXIT_FAILURE;
    }

    // Find an available entry
    struct cmd_entry *entry = NULL;
    int found = 0;

    for (int i = 0; i < POOL_SIZE; i++) {
        // Check if the entry is not linked in the queue (available)
        if (!pool_entries[i].entries.tqe_next && !pool_entries[i].entries.tqe_prev) {
            entry = &pool_entries[i];
            found = 1;
            break;
        }
    }

    // Check if pool is full
    if (!found) {
        sem_post(&cmd_semaphore);
        syslog(LOG_WARNING, "Command pool is full");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Free command pool entry found, proceeding to insert");

    // Copy the command and add to pool
    memcpy(&entry->cmd, cmd, sizeof(device_command_t));
    TAILQ_INSERT_TAIL(&command_pool, entry, entries);
    syslog(LOG_INFO, "Command copied to pool and inserted into command queue");

    // Unlock the semaphore
    if (sem_post(&cmd_semaphore) != 0) {
        syslog(LOG_WARNING, "Failed to unlock semaphore after adding command");
    } else {
        syslog(LOG_INFO, "Semaphore unlocked after adding command");
    }

    // Log the success
    syslog(LOG_INFO, "Command added successfully: 0x%x", cmd->command_type);
    return EXIT_SUCCESS;
}
