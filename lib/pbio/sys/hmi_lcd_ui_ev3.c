// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

// Draws the user interface on the EV3 display.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_LCD_UI_EV3

#include <pbsys/main.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/display.h>
#include <pbdrv/hardware.h>

#include <pbio/busy_count.h>
#include <pbio/button.h>
#include <pbio/image.h>
#include <pbio/light_animation.h>
#include <pbio/version.h>

#include "pbio_version_hash.h"
#include "hmi_lcd_ui.h"
#include "pbio_image_media.h"

#define BLACK (3)
#define WHITE (0)

/**
 * Gets the label of a program slot, numbered from 1.
 *
 * @param [in]  slot: Slot index.
 * @return      Slot number and program name.
 */
static const char *pbsys_hmi_lcd_ui_ev3_get_slot_label(uint8_t slot) {
    static char label[20];
    snprintf(label, sizeof(label), "%c: %s", '1' + slot, pbsys_hmi_lcd_ui_get_program_name(slot));
    return label;
}

static uint8_t pbsys_hmi_lcd_ui_ev3_draw_overlay_box(uint8_t width, uint8_t height, bool divider) {
    pbio_image_t *display = pbdrv_display_get_image();

    // Centered horizontally, slightly more down.
    uint8_t x = (178 - width) / 2;
    uint8_t y = (128 - height) * 3 / 4;

    // White rectangle, black border and white margin.
    pbio_image_fill_rect(display, x, y, width, height, WHITE);
    for (uint8_t i = 1; i < 4; i++) {
        pbio_image_draw_rect(display, x - i, y - i, width + i * 2, height + i * 2, i == 1 ? BLACK: WHITE);
    }

    if (!divider) {
        return 0;
    }

    // Horizontal divider.
    uint8_t bar_y = y + height - 21;
    pbio_image_draw_hline(display, x, bar_y, width, BLACK);
    return bar_y;
}

static void pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(uint8_t separator_y) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__accept24_fill, 76, separator_y + 3, BLACK);
}

static void pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept_and_reject(uint8_t separator_y, bool accept_active) {
    pbio_image_t *display = pbdrv_display_get_image();
    const pbio_image_monochrome_t *accept = accept_active ? &pbio_image_media__accept24_fill : &pbio_image_media__accept24;
    const pbio_image_monochrome_t *reject = accept_active ? &pbio_image_media__reject24 : &pbio_image_media__reject24_fill;
    pbio_image_draw_image_transparent_from_monochrome(display, reject, 56, separator_y + 3, BLACK);
    pbio_image_draw_image_transparent_from_monochrome(display, accept, 96, separator_y + 3, BLACK);
}

/**
 * Draws the sweeping activity animation, centered on the given line.
 */
static void pbsys_hmi_lcd_ui_ev3_draw_activity_animation(int y) {
    static uint8_t counter;
    counter = counter > 60 ? 0 : counter + 1;
    pbio_image_draw_hline(pbdrv_display_get_image(), 178 / 2 - counter, y, counter * 2, BLACK);
}

/**
 * Draws the sweeping activity animation with a status text above it.
 */
static void pbsys_hmi_lcd_ui_ev3_draw_activity_status(const char *text) {
    pbsys_hmi_lcd_ui_ev3_draw_activity_animation(44);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, text, 0, 40);
}

static void pbsys_hmi_lcd_ui_ev3_draw_host_overlay(void) {

    const pbio_font_t *font = &pbio_font_liberationsans_regular_14;
    const char *name;

    switch (pbsys_hmi_lcd_ui_get_host_phase()) {

        case PBSYS_HMI_LCD_UI_HOST_PHASE_READY: {
            uint8_t separator_y = pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, true);
            name = pbdrv_bluetooth_classic_host_get_connected_name();
            pbsys_hmi_lcd_ui_draw_device_name(font, name ? name : "Computer", 44, 60, 150, false);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Connected", 0, 80);
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(separator_y);
            return;
        }

        case PBSYS_HMI_LCD_UI_HOST_PHASE_CONNECT: {
            // Drawn on every refresh, so the placeholder name gets replaced
            // as soon as the real one comes in.
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, false);
            name = pbdrv_bluetooth_classic_host_get_paired_name();
            pbsys_hmi_lcd_ui_draw_device_name(font, name ? name : "Computer", 44, 60, 150, false);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Saved! Now connect", 0, 82);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "in the Pybricks app", 0, 98);
            pbsys_hmi_lcd_ui_ev3_draw_activity_animation(110);
            return;
        }

        case PBSYS_HMI_LCD_UI_HOST_PHASE_PAIR:
        default: {
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, false);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Connect with the", 0, 48);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Bluetooth menu", 0, 66);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "on your computer", 0, 84);
            pbsys_hmi_lcd_ui_ev3_draw_activity_animation(104);
            return;
        }
    }
}

static void pbsys_hmi_lcd_ui_ev3_draw_gamepad_overlay(void) {

    const pbio_font_t *font = &pbio_font_liberationsans_regular_14;

    char buf[PBIO_BLUETOOTH_CLASSIC_NAME_SIZE + 4];

    switch (pbsys_hmi_lcd_ui_get_gamepad_phase()) {

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY: {
            uint8_t separator_y = pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, true);
            pbsys_hmi_lcd_ui_draw_device_name(font, pbsys_hmi_lcd_ui_get_gamepad_name(), 44, 60, 150, false);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Ready for use", 0, 80);
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(separator_y);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_CONNECT: {
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, false);
            pbsys_hmi_lcd_ui_ev3_draw_activity_status("Connecting...");
            pbsys_hmi_lcd_ui_draw_device_name(font, pbsys_hmi_lcd_ui_get_gamepad_name(), 64, 80, 150, false);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED: {
            uint8_t separator_y = pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, true);
            pbsys_hmi_lcd_ui_draw_centered_text(font, "Connection failed", 0, 55);
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(separator_y);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN:
        default: {
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box(160, 100, false);

            uint32_t selected;
            uint32_t num_results;
            pbio_bluetooth_inquiry_result_t *result = pbsys_hmi_lcd_ui_get_gamepad_scan_result(&selected, &num_results);

            // Show scanning action or number of results if any.
            if (!result) {
                pbsys_hmi_lcd_ui_ev3_draw_activity_status("Scanning: Gamepad");
                return;
            }
            snprintf(buf, sizeof(buf), "Showing %d of %d.", (int)selected + 1, (int)num_results);
            pbsys_hmi_lcd_ui_ev3_draw_activity_status(buf);

            // Selection arrows with the name in between.
            pbsys_hmi_lcd_ui_draw_centered_text(font, "<", -70, 72);
            pbsys_hmi_lcd_ui_draw_centered_text(font, ">", 70, 72);
            pbio_image_fill_rect(pbdrv_display_get_image(), 24, 52, 130, 33, BLACK);
            const char *name = result->name[0] ? result->name : "Unknown device";
            pbsys_hmi_lcd_ui_draw_device_name(font, name, 64, 80, 120, true);
            return;
        }
    }
}

/**
 * Width of the hub information overlay, matching the other large overlays.
 */
#define PBSYS_HMI_LCD_UI_EV3_INFO_WIDTH (160)

/**
 * Left edge of the text inside the hub information overlay.
 */
#define PBSYS_HMI_LCD_UI_EV3_INFO_X ((178 - PBSYS_HMI_LCD_UI_EV3_INFO_WIDTH) / 2 + 8)

/**
 * Draws the hub information overlay.
 *
 * The Bluetooth address is what this is mainly for: it is the one thing here
 * that the user has to copy into their program to reach this brick from
 * another one, so it gets the big monospace font. There is no bold font, so
 * the labels and the remaining details are set apart by being small instead.
 */
static void pbsys_hmi_lcd_ui_ev3_draw_info_overlay(void) {

    pbio_image_t *display = pbdrv_display_get_image();

    uint8_t separator_y = pbsys_hmi_lcd_ui_ev3_draw_overlay_box(PBSYS_HMI_LCD_UI_EV3_INFO_WIDTH, 100, true);

    const pbio_font_t *label_font = &pbio_font_mono_8x5_8;
    const pbio_font_t *value_font = &pbio_font_terminus_normal_16;
    const int x = PBSYS_HMI_LCD_UI_EV3_INFO_X;

    char buf[40];

    const char *heading = "Bluetooth address:";
    pbio_image_draw_text(display, label_font, x, 39, heading, strlen(heading), BLACK);

    uint8_t address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    if (pbdrv_bluetooth_peer_is_ready()) {
        pbdrv_bluetooth_peer_get_local_address(address);
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
            address[0], address[1], address[2], address[3], address[4], address[5]);
    } else {
        // Same shape, so the overlay does not jump when it becomes known.
        snprintf(buf, sizeof(buf), "--:--:--:--:--:--");
    }
    pbio_image_draw_text(display, value_font, x, 57, buf, strlen(buf), BLACK);

    // Plain version for a release, otherwise with the build and commit hash.
    if (PBIO_VERSION_LEVEL_HEX == 0xF) {
        snprintf(buf, sizeof(buf), "Firmware: %d.%d.%d",
            PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO);
    } else {
        snprintf(buf, sizeof(buf), "Firmware: %d.%d.%d%x%d-%.6s",
            PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO,
            PBIO_VERSION_LEVEL_HEX, PBIO_VERSION_SERIAL, PBIO_VERSION_HASH);
    }
    pbio_image_draw_text(display, label_font, x, 75, buf, strlen(buf), BLACK);

    const char *hardware = pbdrv_hardware_get_version();
    snprintf(buf, sizeof(buf), "Hardware: %s", hardware ? hardware : "Unknown");
    pbio_image_draw_text(display, label_font, x, 89, buf, strlen(buf), BLACK);

    pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(separator_y);
}

/**
 * Draws the overlay.
 *
 * @param  [in]  overlay The overlay to draw.
 */
static void pbsys_hmi_lcd_ui_ev3_draw_overlay(pbsys_hmi_lcd_ui_overlay_type_t overlay) {

    // Nothing to do.
    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_NONE) {
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_HOST) {
        pbsys_hmi_lcd_ui_ev3_draw_host_overlay();
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_GAMEPAD) {
        pbsys_hmi_lcd_ui_ev3_draw_gamepad_overlay();
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_INFO) {
        pbsys_hmi_lcd_ui_ev3_draw_info_overlay();
        return;
    }

    // All following basic notifications fit on a single size.
    uint8_t separator_y = pbsys_hmi_lcd_ui_ev3_draw_overlay_box(120, 60, true);

    // Add content specific to the overlay type.
    switch (overlay) {
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES:
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO:
            pbio_image_draw_image_transparent_from_monochrome(pbdrv_display_get_image(), &pbio_image_media__off20, 79, separator_y - 32, BLACK);
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept_and_reject(separator_y, overlay == PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES);
            break;
        case PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM:
        case PBSYS_HMI_LCD_UI_OVERLAY_COMING_SOON: {
            const char *text = overlay == PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM ? "No program!" : "Coming soon!";
            pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, text, 0, separator_y - 16);
            pbsys_hmi_lcd_ui_ev3_draw_overlay_box_draw_accept(separator_y);
            break;
        }
        default:
            break;
    }
}

void pbsys_hmi_lcd_ui_draw(void) {

    const pbsys_hmi_lcd_ui_t *ui = pbsys_hmi_lcd_ui_get_state();

    // Start with empty screen.
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);

    // Black box for selection highlight.
    pbio_image_fill_rect(display, 6, 41 + ui->selection[ui->tab] * 20, 166, 14, BLACK);

    // Draw entries with dotted underlines, inverting color for selection.
    for (uint32_t s = 0; s < pbsys_hmi_lcd_ui_get_tab_num_entries(ui->tab); s++) {
        for (uint32_t i = 0; i < 56; i++) {
            pbio_image_draw_pixel(display, 6 + 3 * i, 56 + s * 20, BLACK);
        }
        // Program slots are numbered, other entries are indented by a space.
        char text[32];
        if (ui->tab == PBSYS_HMI_LCD_UI_TAB_PROGRAMS) {
            snprintf(text, sizeof(text), "%s", pbsys_hmi_lcd_ui_ev3_get_slot_label(s));
        } else {
            snprintf(text, sizeof(text), " %s", pbsys_hmi_lcd_ui_get_tab_entry_text(ui->tab, s));
        }
        uint8_t color = s == ui->selection[ui->tab] ? WHITE : BLACK;
        pbio_image_draw_text(display, &pbio_font_liberationsans_regular_14, 8,  52 + s * 20, text, strlen(text), color);
        pbio_image_fill_rect(display, 172, 41, 6, 20 * 4, WHITE);
    }

    // Draw box around tab entries.
    pbio_image_draw_rect(display, 1, 38, 176, 89, BLACK);
    pbio_image_draw_rounded_rect(display, 0, 37, 178, 91, 1, BLACK);

    // Draw the tab selector rounded squares, using thick line for selected.
    for (uint8_t t = 0; t < 4; t++) {
        uint32_t tab_x = t * 42;
        if (t == ui->tab) {
            pbio_image_draw_rounded_rect(display, tab_x + 9, 13, 34, 26, 2, BLACK);
            pbio_image_draw_rect(display, tab_x + 10, 14, 32, 24, BLACK);
            pbio_image_draw_pixel(display, tab_x + 11, 15, BLACK);
            pbio_image_draw_pixel(display, tab_x + 40, 15, BLACK);
            pbio_image_draw_hline(display, tab_x + 11, 37, 30, WHITE);
            pbio_image_draw_hline(display, tab_x + 10, 38, 32, WHITE);
        } else {
            pbio_image_draw_rounded_rect(display, tab_x + 10, 14, 32, 21, 2, BLACK);
        }
    }

    // Play icon.
    pbio_image_draw_circle(display, 26, 24, 8, BLACK);
    pbio_image_draw_vline(display, 23 + 1, 20, 9, BLACK);
    pbio_image_draw_vline(display, 24 + 1, 21, 7, BLACK);
    pbio_image_draw_vline(display, 25 + 1, 21, 7, BLACK);
    pbio_image_draw_vline(display, 26 + 1, 22, 5, BLACK);
    pbio_image_draw_vline(display, 27 + 1, 22, 5, BLACK);
    pbio_image_draw_vline(display, 28 + 1, 23, 3, BLACK);
    pbio_image_draw_hline(display, 29 + 1, 24, 2, BLACK);

    // Apps icon.
    uint8_t width = 8;
    uint8_t shift = width + 1;
    uint8_t x = 59;
    uint8_t y = 16;
    pbio_image_draw_rounded_rect(display, x, y, width, width, 1, BLACK);
    pbio_image_fill_rounded_rect(display, x + shift, y, width, width, 1, 1);
    pbio_image_draw_rounded_rect(display, x + shift, y, width, width, 1, BLACK);
    pbio_image_fill_rounded_rect(display, x, y + shift, width, width, 1, 1);
    pbio_image_draw_rounded_rect(display, x, y + shift, width, width, 1, BLACK);
    pbio_image_draw_rounded_rect(display, x + shift, y + shift, width, width, 1, BLACK);

    // Settings icon.
    pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__wrench17, 101, 16, BLACK);

    pbsys_hmi_lcd_ui_draw_status_bar();

    // Draw the overlay on top of everything else.
    pbsys_hmi_lcd_ui_ev3_draw_overlay(ui->overlay);

    // Draw everything to the screen.
    pbdrv_display_update();
}

#define ANIMATION_REPEAT_MS (4000)
#define ANIMATION_REFRESH_MS (100)
#define ANIMATION_BLINK_MS (300)
#define ANIMATION_BLINK_AT_MS (2000)
#define ANIMATION_UP_MS (800 / 2)
#define ANIMATION_DY (12)

/**
 * Elapsed time in animation.
 */
static uint32_t elapsed;

/**
 * Draws animation frame for program running animation.
 *
 * @param [in] animation    The animation instance.
 */
static uint32_t pbsys_hmi_lcd_ui_ev3_run_animation_next(pbio_light_animation_t *animation) {
    elapsed += ANIMATION_REFRESH_MS;

    uint32_t phase = elapsed % ANIMATION_REPEAT_MS;
    uint8_t y = ANIMATION_DY;

    // Jump up and back down.
    if (phase < ANIMATION_UP_MS * 2) {
        y -= phase < ANIMATION_UP_MS ?
            phase * ANIMATION_DY / ANIMATION_UP_MS :
            ANIMATION_DY - (phase - ANIMATION_UP_MS) * ANIMATION_DY / ANIMATION_UP_MS;
    }

    // Blink briefly.
    bool blink = phase >= ANIMATION_BLINK_AT_MS && phase <= (ANIMATION_BLINK_AT_MS + ANIMATION_BLINK_MS);

    // Erase last logo and draw in updated state.
    pbio_image_fill_rect(pbdrv_display_get_image(), 0, 0, 178, 100, WHITE);
    pbsys_hmi_lcd_ui_draw_pybricks_logo(12, y, 154, blink);

    // Render to screen.
    pbdrv_display_update();
    return ANIMATION_REFRESH_MS;
}

#define NUM_NAMES_PER_PAGE (7)

pbio_error_t pbsys_hmi_lcd_ui_closing_credits(pbio_os_state_t *state, void *context) {

    pbio_image_t *display = pbdrv_display_get_image();

    static pbio_os_timer_t timer;
    static size_t page;

    // Pressing cancel exits closing credits.
    if (pbdrv_button_get_pressed() & PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
        goto close;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // Display all supporter names across several pages.
    for (page = 0; page < (pbsys_hmi_lcd_ui_get_credits_num() + NUM_NAMES_PER_PAGE - 1) / NUM_NAMES_PER_PAGE; page++) {

        pbio_image_fill(display, WHITE);

        // Populate page with list of names to fit. Last page may contain
        // some empty entries.
        for (size_t entry = 0; entry < NUM_NAMES_PER_PAGE; entry++) {
            const char *name = pbsys_hmi_lcd_ui_get_credits_name(page * NUM_NAMES_PER_PAGE + entry);
            if (!name) {
                continue;
            }
            int name_y = 28 + entry * display->print_font->line_height;
            pbio_image_draw_text(display, &pbio_font_terminus_normal_16, 10, name_y, name, strlen(name), BLACK);
        }

        // Header with line.
        const char *thanks = "Thanks to our supporters!";
        pbio_image_draw_text(display, &pbio_font_mono_8x5_8, 10, 12, thanks, strlen(thanks), BLACK);
        pbio_image_draw_hline(display, 0, 14, 178, 3);

        // Show the page.
        pbdrv_display_update();
        PBIO_OS_AWAIT_MS(state, &timer, 1000);
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER);
    }

    // Display creator info.
    pbio_image_fill(display, WHITE);
    const char *thanks = "Made by:";
    pbio_image_draw_hline(display, 0, 14, 178, 3);
    pbio_image_draw_text(display, &pbio_font_mono_8x5_8, 10, 12, thanks, strlen(thanks), BLACK);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_terminus_normal_16, "Laurens Valk", 0, 40);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_terminus_normal_16, "David Lechner", 0, 70);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_terminus_normal_16, "Pybricks Community", 0, 100);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 1500);

    // Display animated Pybricks thanks message.
    pbio_image_fill(display, WHITE);
    pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__pybricks_join, 31, 6, BLACK);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 1500);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER);
    pbio_image_fill(display, WHITE);
    pbsys_hmi_lcd_ui_draw_pybricks_logo(12, 12, 154, false);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_terminus_normal_16, "Thanks for your help!", 0, 114);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 500);
    pbsys_hmi_lcd_ui_draw_pybricks_logo(12, 12, 154, true);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 400);
    pbsys_hmi_lcd_ui_draw_pybricks_logo(12, 12, 154, false);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 600);

close:

    // Prepare for shutdown and release lock.
    pbio_image_fill(display, WHITE);
    pbdrv_display_update();
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * User animation state.
 */
static pbio_light_animation_t animation;

/**
 * Starts the user program animation.
 */
void pbsys_hmi_lcd_ui_run_animation_start(void) {
    // Erase UI.
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);

    // Start animation after the blink.
    elapsed = ANIMATION_BLINK_AT_MS + ANIMATION_BLINK_MS + ANIMATION_REFRESH_MS;
    pbio_light_animation_init(&animation, pbsys_hmi_lcd_ui_ev3_run_animation_next);
    pbio_light_animation_start(&animation);

    // Draw current program name below animation.
    pbio_pybricks_user_program_id_t id;
    pbsys_main_program_start_request_type_t type;
    pbsys_main_program_get_info(&id, &type);
    if (id < PBSYS_CONFIG_HMI_NUM_SLOTS) {
        const char *name = pbsys_hmi_lcd_ui_ev3_get_slot_label(id);
        pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_terminus_normal_16, name, -10, 116);
    }
    // Not updating display here. First animation frame will do that.
}

void pbsys_hmi_lcd_ui_run_animation_stop(void) {
    pbio_light_animation_stop(&animation);
}

#endif // PBSYS_CONFIG_HMI_LCD_UI_EV3
