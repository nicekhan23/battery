/**
 * @file test.c
 * @brief Comprehensive unit tests for serial communication module
 *
 * This file contains unit tests for the serial communication module using the cmocka framework.
 * The tests cover initialization, deinitialization, and command handling with various valid and
 * invalid inputs.
 *
 * Created on: May 11, 2025
 * @author Zhanibekuly Darkhan
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <termios.h>
#include <stdio.h>
#include "serial.h"

/**
 * @defgroup initialization_tests Initialization Tests
 * @brief Tests for the initialization and deinitialization functionality
 * @{
 */

/**
 * @brief Test initialization with a valid port
 *
 * This test verifies that the module can be initialized with a valid port name.
 *
 * @param state Test state (unused)
 */
static void test_init_valid_port(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test initialization with a port name that exceeds the maximum length
 *
 * This test verifies that initialization fails when the port name is too long.
 *
 * @param state Test state (unused)
 */
static void test_init_long_port(void **state) {
    (void)state;
    assert_int_equal(init("/dev/port_name_exceeding_thirty_chars_123", B9600), EXIT_FAILURE);
}

/**
 * @brief Test initialization with a NULL port name
 *
 * This test verifies that initialization fails when the port name is NULL.
 *
 * @param state Test state (unused)
 */
static void test_init_null_port(void **state) {
    (void)state;
    assert_int_equal(init(NULL, B9600), EXIT_FAILURE);
}

/**
 * @brief Test double initialization
 *
 * This test verifies that initialization fails when called twice without deinitialization.
 *
 * @param state Test state (unused)
 */
static void test_init_double_initialization(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    assert_int_equal(init("/dev/null", B9600), EXIT_FAILURE);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test initialization with various communication speeds
 *
 * This test verifies that the module can be initialized with different baud rates.
 *
 * @param state Test state (unused)
 */
static void test_init_various_speeds(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_SUCCESS);

    assert_int_equal(init("/dev/null", B115200), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/** @} */ /* End of initialization_tests group */

/**
 * @defgroup deinitialization_tests Deinitialization Tests
 * @brief Tests for the deinitialization functionality
 * @{
 */

/**
 * @brief Test deinitialization without initialization
 *
 * This test verifies that deinitialization fails when called without initialization.
 *
 * @param state Test state (unused)
 */
static void test_deinit_without_init(void **state) {
    (void)state;
    assert_int_equal(deinit(), EXIT_FAILURE);
}

/**
 * @brief Test double deinitialization
 *
 * This test verifies that deinitialization fails when called twice.
 *
 * @param state Test state (unused)
 */
static void test_deinit_double_deinit(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_FAILURE);
}

/** @} */ /* End of deinitialization_tests group */

/**
 * @defgroup cmd_set_params_tests SET_PARAMS Command Tests
 * @brief Tests for SET_PARAMS command functionality
 * @{
 */

/**
 * @brief Test adding a valid SET_PARAMS command
 *
 * This test verifies that a valid SET_PARAMS command can be added to the pool.
 *
 * @param state Test state (unused)
 */
static void test_add_valid_set_params_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    device_command_t cmd = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 10, .max_level = 90, .max_time = 60 }
    };
    assert_int_equal(add(&cmd), EXIT_SUCCESS);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding SET_PARAMS commands with boundary values
 *
 * This test verifies that SET_PARAMS commands with minimum and maximum valid values
 * can be added to the pool.
 *
 * @param state Test state (unused)
 */
static void test_add_boundary_set_params_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test minimum valid values
    device_command_t cmd1 = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 0, .max_level = 0, .max_time = 1 }
    };
    assert_int_equal(add(&cmd1), EXIT_SUCCESS);

    // Test maximum valid values
    device_command_t cmd2 = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 100, .max_level = 100, .max_time = 240 }
    };
    assert_int_equal(add(&cmd2), EXIT_SUCCESS);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding a SET_PARAMS command with an invalid min_level
 *
 * This test verifies that a SET_PARAMS command with min_level > 100 is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_min_level_params_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    device_command_t cmd = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 101, .max_level = 90, .max_time = 60 }
    };
    assert_int_equal(add(&cmd), EXIT_FAILURE);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding a SET_PARAMS command with an invalid max_level
 *
 * This test verifies that a SET_PARAMS command with max_level > 100 is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_max_level_params_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    device_command_t cmd = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 10, .max_level = 101, .max_time = 60 }
    };
    assert_int_equal(add(&cmd), EXIT_FAILURE);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding SET_PARAMS commands with invalid max_time values
 *
 * This test verifies that SET_PARAMS commands with max_time=0 or max_time>240 are rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_max_time_params_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test with max_time = 0 (invalid)
    device_command_t cmd1 = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 10, .max_level = 90, .max_time = 0 }
    };
    assert_int_equal(add(&cmd1), EXIT_FAILURE);

    // Test with max_time > 240 (invalid)
    device_command_t cmd2 = {
        .command_type = CMD_SET_PARAMS,
        .data.set_params = { .min_level = 10, .max_level = 90, .max_time = 241 }
    };
    assert_int_equal(add(&cmd2), EXIT_FAILURE);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/** @} */ /* End of cmd_set_params_tests group */

/**
 * @defgroup cmd_on_off_tests ON/OFF Command Tests
 * @brief Tests for ON/OFF command functionality
 * @{
 */

/**
 * @brief Test adding valid ON_OFF commands
 *
 * This test verifies that valid ON and OFF commands can be added to the pool.
 *
 * @param state Test state (unused)
 */
static void test_add_valid_on_off_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test ON command
    device_command_t cmd_on = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 1, .channel = 3 }
    };
    assert_int_equal(add(&cmd_on), EXIT_SUCCESS);

    // Test OFF command
    device_command_t cmd_off = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 0, .channel = 5 }
    };
    assert_int_equal(add(&cmd_off), EXIT_SUCCESS);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding ON_OFF commands with boundary channel values
 *
 * This test verifies that ON_OFF commands with minimum and maximum valid channel values
 * can be added to the pool.
 *
 * @param state Test state (unused)
 */
static void test_add_boundary_on_off_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test with minimum channel
    device_command_t cmd1 = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 1, .channel = 0 }
    };
    assert_int_equal(add(&cmd1), EXIT_SUCCESS);

    // Test with maximum channel
    device_command_t cmd2 = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 0, .channel = 7 }
    };
    assert_int_equal(add(&cmd2), EXIT_SUCCESS);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding an ON_OFF command with an invalid on_off value
 *
 * This test verifies that an ON_OFF command with on_off not equal to 0 or 1 is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_on_off_value_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test with invalid on_off value (not 0 or 1)
    device_command_t cmd = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 2, .channel = 3 }
    };
    assert_int_equal(add(&cmd), EXIT_FAILURE);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding an ON_OFF command with an invalid channel value
 *
 * This test verifies that an ON_OFF command with channel > 7 is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_channel_value_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Test with invalid channel (> 7)
    device_command_t cmd = {
        .command_type = CMD_ON_OFF,
        .data.on_off = { .on_off = 1, .channel = 8 }
    };
    assert_int_equal(add(&cmd), EXIT_FAILURE);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/** @} */ /* End of cmd_on_off_tests group */

/**
 * @defgroup cmd_emergency_tests Emergency Command Tests
 * @brief Tests for Emergency command functionality
 * @{
 */

/**
 * @brief Test adding an EMERGENCY command
 *
 * This test verifies that an EMERGENCY command can be added to the pool.
 *
 * @param state Test state (unused)
 */
static void test_add_emergency_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    device_command_t cmd = {
        .command_type = CMD_EMERGENCY
    };
    assert_int_equal(add(&cmd), EXIT_SUCCESS);

    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/** @} */ /* End of cmd_emergency_tests group */

/**
 * @defgroup invalid_cmd_tests Invalid Command Tests
 * @brief Tests for invalid command handling
 * @{
 */

/**
 * @brief Test adding a command with an invalid command type
 *
 * This test verifies that a command with an unknown command type is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_invalid_command_type(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    device_command_t cmd = { .command_type = 0xFF };
    assert_int_equal(add(&cmd), EXIT_FAILURE);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/**
 * @brief Test adding a NULL command
 *
 * This test verifies that a NULL command pointer is rejected.
 *
 * @param state Test state (unused)
 */
static void test_add_null_command(void **state) {
    (void)state;
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);
    assert_int_equal(add(NULL), EXIT_FAILURE);
    assert_int_equal(deinit(), EXIT_SUCCESS);
}

/** @} */ /* End of invalid_cmd_tests group */

/**
 * @defgroup pool_tests Command Pool Tests
 * @brief Tests for command pool functionality
 * @{
 */

/**
* @brief Test filling the command pool to capacity
*
* This test verifies that commands can be added until the pool is full,
* and that additional commands are rejected once the pool is full.
*
* @param state Test state (unused)
*/
static void test_add_commands_to_fill_pool(void **state) {
    (void)state;

    // Clear any previous state
    assert_int_equal(init("/dev/null", B9600), EXIT_SUCCESS);

    // Create an emergency command for testing
    device_command_t cmd = {
        .command_type = CMD_EMERGENCY
    };

    int successful_adds = 0;

    // Try to add POOL_SIZE + 1 commands
    for (int i = 0; i < POOL_SIZE + 1; i++) {
        // If the pool is full, the add should fail
        if (i < POOL_SIZE) {
            printf("Adding command %d of %d to pool...\n", i+1, POOL_SIZE);
            int result = add(&cmd);
            if (result == EXIT_SUCCESS) {
                successful_adds++;
            } else {
                printf("Failed to add command %d (unexpected)\n", i+1);
                assert_int_equal(result, EXIT_SUCCESS);
            }
        } else {
            // This one should fail as the pool is full
            printf("Attempting to add command %d (should fail as pool is full)\n", i+1);
            assert_int_equal(add(&cmd), EXIT_FAILURE);
            printf("Command correctly rejected as pool is full\n");
        }
    }

    // Verify we added exactly POOL_SIZE commands successfully
    printf("Successfully added %d commands to the pool\n", successful_adds);
    assert_int_equal(successful_adds, POOL_SIZE);

    // Clean up
    assert_int_equal(deinit(), EXIT_SUCCESS);
    printf("Pool test completed successfully\n");
}

/**
 * @brief Test adding a command without initialization
 *
 * This test verifies that attempting to add a command without initializing
 * the module first fails.
 *
 * @param state Test state (unused)
 */
static void test_add_command_without_init(void **state) {
    (void)state;
    device_command_t cmd = {
        .command_type = CMD_EMERGENCY
    };
    assert_int_equal(add(&cmd), EXIT_FAILURE);
}

/** @} */ /* End of pool_tests group */

/**
 * @brief Main function for the test suite
 *
 * This function sets up and runs all the tests.
 *
 * @return Result of the test execution
 */
/**
* @brief Main function for the test suite
*
* This function sets up and runs all the tests with enhanced verbosity.
*
* @return Result of the test execution
*/
int main(void) {
    // Set cmocka to output all test information with maximum verbosity
    cmocka_set_message_output(CM_OUTPUT_STDOUT);

    printf("\n============= Starting Battery Charger Serial Communication Tests =============\n\n");

    const struct CMUnitTest tests[] = {
        /* Initialization Tests */
        cmocka_unit_test(test_init_valid_port),
        cmocka_unit_test(test_init_long_port),
        cmocka_unit_test(test_init_null_port),
        cmocka_unit_test(test_init_double_initialization),
        cmocka_unit_test(test_init_various_speeds),

        /* Deinitialization Tests */
        cmocka_unit_test(test_deinit_without_init),
        cmocka_unit_test(test_deinit_double_deinit),

        /* Set Params Command Tests */
        cmocka_unit_test(test_add_valid_set_params_command),
        cmocka_unit_test(test_add_boundary_set_params_command),
        cmocka_unit_test(test_add_invalid_min_level_params_command),
        cmocka_unit_test(test_add_invalid_max_level_params_command),
        cmocka_unit_test(test_add_invalid_max_time_params_command),

        /* On/Off Command Tests */
        cmocka_unit_test(test_add_valid_on_off_command),
        cmocka_unit_test(test_add_boundary_on_off_command),
        cmocka_unit_test(test_add_invalid_on_off_value_command),
        cmocka_unit_test(test_add_invalid_channel_value_command),

        /* Emergency Command Tests */
        cmocka_unit_test(test_add_emergency_command),

        /* Invalid Command Tests */
        cmocka_unit_test(test_add_invalid_command_type),
        cmocka_unit_test(test_add_null_command),

        /* Pool Tests */
        cmocka_unit_test(test_add_commands_to_fill_pool),
        cmocka_unit_test(test_add_command_without_init),
    };

    // Print each test as it's about to run (extra logging)
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        printf("Test %zu: %s\n", i + 1, tests[i].name);
    }

    // Run the tests with verbose output and print summary after
    int result = cmocka_run_group_tests_name("Battery Charger Serial Communication Tests", tests, NULL, NULL);

    printf("\n============= Tests Complete: Return Code = %d =============\n", result);

    return result;
}
