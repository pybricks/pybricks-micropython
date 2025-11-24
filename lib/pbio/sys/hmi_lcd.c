// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Provides Human Machine Interface (HMI) between hub and user for systems
// with directional buttons and an LCD display.

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_LCD

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/display.h>

#include <pbio/button.h>
#include <pbio/os.h>
#include <pbsys/host.h>
#include <pbio/image.h>
#include <pbsys/light.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>

#include "hmi.h"
#include "storage.h"

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

// Scaling factors and functions to simplify drawing the logo.
static float _scale;
static uint32_t _offset_x;
static uint32_t _offset_y;

static uint32_t sx(uint32_t x) {
    return x * _scale + _offset_x + 0.5f;
}

static uint32_t sy(uint32_t y) {
    return y * _scale + _offset_y + 0.5f;
}

static uint32_t sr(uint32_t r) {
    return r * _scale + 0.5f;
}

/**
 * Draws the Pybricks logo on the screen.
 *
 * @param  x     [in] Horizontal offset from the left.
 * @param  y     [in] Vertical offset from the top.
 * @param  scale [in] Scale (1.0 is 154 x 84).
 */
static void draw_pybricks_logo(uint32_t x, uint32_t y, float scale) {
    _offset_x = x;
    _offset_y = y;
    _scale = scale;

    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, 0);

    uint8_t v = pbdrv_display_get_max_value();

    // Rounded rectangles making up the left and right side of the head.
    pbio_image_fill_rounded_rect(display, sx(0), sy(0), sr(42), sr(84), sr(11), v);
    pbio_image_fill_rounded_rect(display, sx(112), sy(0), sr(42), sr(84), sr(11), v);

    // Forehead, main fill, and jaw.
    pbio_image_fill_rect(display, sx(14), sy(0), sr(126), sr(14), v);
    pbio_image_fill_rect(display, sx(14), sy(14), sr(126), sr(56), 0);
    pbio_image_fill_rect(display, sx(28), sy(56), sr(98), sr(14), v);

    // Eyes.
    pbio_image_fill_circle(display, sx(49), sy(29), sr(10), v);
    pbio_image_fill_circle(display, sx(106), sy(29), sr(10), v);

    // Teeth.
    for (uint32_t i = 0; i < 6; i++) {
        pbio_image_fill_rect(display, sx(40 + 14 * i), sy(51), sr(4), sr(5), v);
    }

    pbdrv_display_update();
}

static void hmi_lcd_grid_show_pixel(uint8_t row, uint8_t col, bool on) {
    pbio_image_t *display = pbdrv_display_get_image();
    uint8_t value = on ? pbdrv_display_get_max_value(): 0;
    const uint32_t size = PBDRV_CONFIG_DISPLAY_NUM_ROWS / PBSYS_CONFIG_HMI_NUM_SLOTS;
    const uint32_t width = size * 4 / 5;
    const uint32_t offset = (PBDRV_CONFIG_DISPLAY_NUM_COLS - (PBSYS_CONFIG_HMI_NUM_SLOTS * size)) / 2;
    pbio_image_fill_rect(display, col * size + offset, row * size, width, width, value);
}

void pbsys_hmi_init(void) {
}

void pbsys_hmi_deinit(void) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, 0);
    pbdrv_display_update();
}

static pbio_error_t run_ui(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    PBIO_OS_ASYNC_BEGIN(state);

    // Centered above 5 code slot indicators.
    draw_pybricks_logo(27, 12, 0.8);

    for (;;) {

        DEBUG_PRINT("Start HMI loop\n");

        // Visually indicate current slot.
        uint8_t selected_slot = pbsys_status_get_selected_slot();
        for (uint8_t c = 0; c < PBSYS_CONFIG_HMI_NUM_SLOTS; c++) {
            hmi_lcd_grid_show_pixel(4, c, c == selected_slot);
        }

        pbsys_main_program_t program;
        program.id = selected_slot;
        pbsys_storage_get_program_data(&program);
        const char *name = pbsys_main_program_validate(&program) == PBIO_SUCCESS ?
            program.name : "------";

        pbio_image_t *display = pbdrv_display_get_image();
        pbio_image_fill_rect(display, 0, 82, 178, 14, 0);
        pbio_image_draw_text(display, &pbio_font_liberationsans_regular_14, 20, 92, name, strlen(name), 3);

        pbdrv_display_update();

        // Buttons could be pressed at the end of the user program, so wait for
        // a release and then a new press, or until we have to exit early.
        DEBUG_PRINT("Waiting for initial button release.\n");
        PBIO_OS_AWAIT_WHILE(state, ({
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }
            pbdrv_button_get_pressed();
        }));

        DEBUG_PRINT("Start waiting for input.\n");
        // Wait on a button, external program start, or connection change. Stop
        // waiting on timeout or shutdown.
        PBIO_OS_AWAIT_UNTIL(state, ({
            // Shutdown may be requested by a background process such as critical
            // battery or holding the power button.
            if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
                return PBIO_ERROR_CANCELED;
            }

            // Exit on timeout except while connected to host.
            if (pbsys_host_is_connected()) {
                pbio_os_timer_reset(timer);
            } else if (pbio_os_timer_is_expired(timer)) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // Wait for button press, external program start, or connection change.
            pbdrv_button_get_pressed() || pbsys_main_program_start_is_requested();
        }));

        // External progran request takes precedence over buttons.
        if (pbsys_main_program_start_is_requested()) {
            DEBUG_PRINT("Start program from Pybricks Code.\n");
            break;
        }

        // On right, increment slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_RIGHT) {
            pbsys_status_increment_selected_slot(true);
            continue;
        }
        // On left, decrement slot when possible, then start waiting on new inputs.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_LEFT) {
            pbsys_status_increment_selected_slot(false);
            continue;
        }

        // On center, attempt to start program.
        if (pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER) {
            pbio_error_t err = pbsys_main_program_request_start(pbsys_status_get_selected_slot(), PBSYS_MAIN_PROGRAM_START_REQUEST_TYPE_HUB_UI);
            if (err == PBIO_SUCCESS) {
                DEBUG_PRINT("Start program with button\n");
                break;
            } else {
                DEBUG_PRINT("Requested program not available.\n");
                // We can run an animation here to indicate that the program is not available.
            }
        }

        DEBUG_PRINT("No valid action selected, start over.\n");
    }

    // Wait for all buttons to be released so the user doesn't accidentally
    // push their robot off course.
    DEBUG_PRINT("Waiting for final button release.\n");
    PBIO_OS_AWAIT_WHILE(state, ({
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }
        pbdrv_button_get_pressed();
    }));

    // Start light or display animations.
    pbio_color_light_start_breathe_animation(pbsys_status_light_main, PBSYS_CONFIG_STATUS_LIGHT_STATE_ANIMATIONS_HUE);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Drives all processes while we wait for user input. This completes when a
 * user program request is made using the buttons or by a connected host.
 *
 * @return  Error code.
 *          ::PBIO_SUCCESS when a program is selected.
 *          ::PBIO_ERROR_CANCELED when selection was cancelled by shutdown request.
 *          ::PBIO_ERROR_TIMEDOUT when there was no user interaction for a long time.
 */
pbio_error_t pbsys_hmi_await_program_selection(void) {

    pbio_os_timer_t idle_timer;
    pbio_os_timer_set(&idle_timer, PBSYS_CONFIG_HMI_IDLE_TIMEOUT_MS);

    pbio_os_state_t state = 0;

    pbio_error_t err;
    while ((err = run_ui(&state, &idle_timer)) == PBIO_ERROR_AGAIN) {
        // run all processes and wait for next event.
        pbio_os_run_processes_and_wait_for_event();
    }
    DEBUG_PRINT("Finished program selection with status: %d\n", err);
    return err;
}

#endif // PBSYS_CONFIG_HMI_LCD
