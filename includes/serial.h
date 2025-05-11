/**
 * @file serial.h
 * @brief Serial communication interface for multi-channel battery charger
 *
 * This header file defines the interface for serial communication with a battery charger device.
 * It provides functions for initialization, deinitialization, and command handling.
 *
 * Created on: May 11, 2025
 * @author Zhanibekuly Darkhan
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <semaphore.h>

/** @brief Maximum size of the command pool */
#define POOL_SIZE 32

/** @brief Maximum length for port name string */
#define MAX_PORT_NAME 30

/**
 * @brief Command codes for battery charger
 * @{
 */
/** @brief Command code for setting battery charging parameters */
#define CMD_SET_PARAMS 0x63

/** @brief Command code for turning a channel on or off */
#define CMD_ON_OFF 0x64

/** @brief Command code for emergency operation */
#define CMD_EMERGENCY 0x65
/** @} */

/**
 * @brief Structure for setting battery charging parameters
 */
typedef struct {
    /** @brief Minimum battery level percentage (0-100) */
    uint8_t min_level;

    /** @brief Maximum battery level percentage (0-100) */
    uint8_t max_level;

    /** @brief Maximum charging time in minutes (1-240) */
    uint8_t max_time;
} cmd_set_params_t;

/**
 * @brief Structure for channel on/off control
 */
typedef struct {
    /** @brief On/off status (0=off, 1=on) */
    uint8_t on_off;

    /** @brief Channel number (0-7) */
    uint8_t channel;
} cmd_on_off_t;

/**
 * @brief Command structure for the device
 *
 * This structure represents a command to be sent to the battery charger device.
 * It contains a command type and associated parameters.
 */
typedef struct {
    /** @brief Command type (CMD_SET_PARAMS, CMD_ON_OFF, CMD_EMERGENCY) */
    uint8_t command_type;

    /** @brief Command-specific data */
    union {
        /** @brief Parameters for SET_PARAMS command */
        cmd_set_params_t set_params;

        /** @brief Parameters for ON_OFF command */
        cmd_on_off_t on_off;
    } data;
} device_command_t;

/**
 * @brief Structure for storing command entries in a queue
 *
 * This structure is used internally to maintain the command pool.
 */
struct cmd_entry {
    /** @brief The device command */
    device_command_t cmd;

    /** @brief Queue entry for the sys/queue.h TAILQ macros */
    TAILQ_ENTRY(cmd_entry) entries;
};

/**
 * @brief Initialize the serial communication
 *
 * This function initializes the serial communication with the battery charger device.
 * It opens the specified serial port, initializes the command pool, and prepares the module
 * for operation.
 *
 * @param port Serial port name (max 30 characters)
 * @param speed Communication speed (use B9600, B115200, etc. from termios.h)
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int init(const char *port, int speed);

/**
 * @brief Deinitialize the serial communication
 *
 * This function cleans up resources used by the serial communication module,
 * including closing the serial port, freeing memory, and destroying the semaphore.
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int deinit(void);

/**
 * @brief Add a command to the pool
 *
 * This function validates the command and adds it to the command pool if valid.
 * The command will be processed by the module's internal thread.
 * The function is thread-safe.
 *
 * @param cmd Pointer to the command structure to add
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure
 */
int add(const device_command_t *cmd);

#endif /* SERIAL_H_ */
