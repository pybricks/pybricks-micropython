// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

// Draws the user interface on the NXT display.
//
// This is much smaller than the EV3 display, so instead of tabs with a list
// below, the menu shows only the tab icons with one label below. Entered tabs
// and overlays use the whole area below the status bar, without a box around.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_LCD_UI_NXT

#include <pbsys/main.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/display.h>

#include <pbio/busy_count.h>
#include <pbio/button.h>
#include <pbio/image.h>
#include <pbio/version.h>

#include "pbio_version_hash.h"
#include "hmi_lcd_ui.h"
#include "pbio_image_media.h"

#define BLACK (3)
#define WHITE (0)

/**
 * Tab names, shown below the tab icons.
 */
static const char *tab_names[] = {
    "Programs",
    "Apps",
    "Settings",
};

/**
 * Baseline of the label at the bottom of the display, with the normal font.
 */
#define PBSYS_HMI_LCD_UI_NXT_LABEL_Y (58)

/**
 * Baselines of the two label lines with the small font, centered at about
 * the same height as the normal font.
 */
#define PBSYS_HMI_LCD_UI_NXT_LABEL_Y1 (52)
#define PBSYS_HMI_LCD_UI_NXT_LABEL_Y2 (62)

/**
 * Draws a program name centered at the bottom of the display.
 *
 * If the name doesn't fit with the normal font, it uses the small font split
 * across two lines, cutting off what still doesn't fit.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_program_name(const char *name) {
    pbio_image_t *display = pbdrv_display_get_image();
    int max_width = display->width - 4;

    const pbio_font_t *font = &pbio_font_liberationsans_regular_14;
    pbio_image_rect_t rect;
    pbio_image_bbox_text(font, name, strlen(name), &rect);
    if (rect.width <= max_width) {
        pbsys_hmi_lcd_ui_draw_centered_text(font, name, 0, PBSYS_HMI_LCD_UI_NXT_LABEL_Y);
        return;
    }
    pbsys_hmi_lcd_ui_draw_device_name(&pbio_font_mono_8x5_8, name,
        PBSYS_HMI_LCD_UI_NXT_LABEL_Y1, PBSYS_HMI_LCD_UI_NXT_LABEL_Y2, max_width, false);
}

/**
 * Top of the area below the status bar, used by overlays.
 */
#define PBSYS_HMI_LCD_UI_NXT_PANEL_Y (11)

/**
 * Clears the area below the status bar, so overlays can draw on it.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_panel(void) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill_rect(display, 0, PBSYS_HMI_LCD_UI_NXT_PANEL_Y, display->width, display->height - PBSYS_HMI_LCD_UI_NXT_PANEL_Y, WHITE);
}

/**
 * Font for all overlay text. Fits 16 characters on one line.
 */
#define PBSYS_HMI_LCD_UI_NXT_OVERLAY_FONT (&pbio_font_mono_8x5_8)

/**
 * Baseline of the given text line in overlays, 10 pixels apart.
 */
#define PBSYS_HMI_LCD_UI_NXT_LINE_Y(line) (PBSYS_HMI_LCD_UI_NXT_PANEL_Y + 11 + (line) * 10)

/**
 * Draws a centered line of overlay text.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_line(const char *text, uint8_t line) {
    pbsys_hmi_lcd_ui_draw_centered_text(PBSYS_HMI_LCD_UI_NXT_OVERLAY_FONT, text, 0, PBSYS_HMI_LCD_UI_NXT_LINE_Y(line));
}

/**
 * Draws a device name on the given line, continued on the next line if needed.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_device_name_lines(const char *name, uint8_t line, bool selected) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbsys_hmi_lcd_ui_draw_device_name(PBSYS_HMI_LCD_UI_NXT_OVERLAY_FONT, name,
        PBSYS_HMI_LCD_UI_NXT_LINE_Y(line), PBSYS_HMI_LCD_UI_NXT_LINE_Y(line + 1), display->width - 4, selected);
}

/**
 * Draws the sweeping activity animation, centered just below the given line.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_activity_animation(uint8_t line) {
    pbio_image_t *display = pbdrv_display_get_image();
    static uint8_t counter;
    counter = counter >= display->width / 2 - 4 ? 0 : counter + 1;
    pbio_image_draw_hline(display, display->width / 2 - counter, PBSYS_HMI_LCD_UI_NXT_LINE_Y(line) + 3, counter * 2, BLACK);
}

/**
 * Draws a status text with the sweeping activity animation below it.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_activity_status(const char *text, uint8_t line) {
    pbsys_hmi_lcd_ui_nxt_draw_line(text, line);
    pbsys_hmi_lcd_ui_nxt_draw_activity_animation(line);
}

static void pbsys_hmi_lcd_ui_nxt_draw_host_overlay(void) {

    const char *name;

    pbsys_hmi_lcd_ui_nxt_draw_panel();

    switch (pbsys_hmi_lcd_ui_get_host_phase()) {

        case PBSYS_HMI_LCD_UI_HOST_PHASE_READY: {
            name = pbdrv_bluetooth_classic_host_get_connected_name();
            pbsys_hmi_lcd_ui_nxt_draw_device_name_lines(name ? name : "Computer", 0, false);
            pbsys_hmi_lcd_ui_nxt_draw_line("Connected", 3);
            return;
        }

        case PBSYS_HMI_LCD_UI_HOST_PHASE_CONNECT: {
            // Drawn on every refresh, so the placeholder name gets replaced
            // as soon as the real one comes in.
            name = pbdrv_bluetooth_classic_host_get_paired_name();
            pbsys_hmi_lcd_ui_nxt_draw_device_name_lines(name ? name : "Computer", 0, false);
            pbsys_hmi_lcd_ui_nxt_draw_line("Saved! Connect", 2);
            pbsys_hmi_lcd_ui_nxt_draw_line("in Pybricks app", 3);
            pbsys_hmi_lcd_ui_nxt_draw_activity_animation(3);
            return;
        }

        case PBSYS_HMI_LCD_UI_HOST_PHASE_PAIR:
        default: {
            pbsys_hmi_lcd_ui_nxt_draw_line("Connect with the", 0);
            pbsys_hmi_lcd_ui_nxt_draw_line("Bluetooth menu", 1);
            pbsys_hmi_lcd_ui_nxt_draw_line("on your computer", 2);
            pbsys_hmi_lcd_ui_nxt_draw_activity_animation(3);
            return;
        }
    }
}

static void pbsys_hmi_lcd_ui_nxt_draw_gamepad_overlay(void) {

    char buf[PBIO_BLUETOOTH_CLASSIC_NAME_SIZE + 4];

    pbsys_hmi_lcd_ui_nxt_draw_panel();

    switch (pbsys_hmi_lcd_ui_get_gamepad_phase()) {

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY: {
            pbsys_hmi_lcd_ui_nxt_draw_device_name_lines(pbsys_hmi_lcd_ui_get_gamepad_name(), 0, false);
            pbsys_hmi_lcd_ui_nxt_draw_line("Ready for use", 3);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_CONNECT: {
            pbsys_hmi_lcd_ui_nxt_draw_activity_status("Connecting...", 0);
            pbsys_hmi_lcd_ui_nxt_draw_device_name_lines(pbsys_hmi_lcd_ui_get_gamepad_name(), 2, false);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED: {
            pbsys_hmi_lcd_ui_nxt_draw_line("Connection", 1);
            pbsys_hmi_lcd_ui_nxt_draw_line("failed", 2);
            return;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN:
        default: {
            uint32_t selected;
            uint32_t num_results;
            pbio_bluetooth_inquiry_result_t *result = pbsys_hmi_lcd_ui_get_gamepad_scan_result(&selected, &num_results);

            // Show scanning action or number of results if any.
            if (!result) {
                pbsys_hmi_lcd_ui_nxt_draw_activity_status("Scanning...", 1);
                return;
            }
            snprintf(buf, sizeof(buf), "Gamepad %d of %d", (int)selected + 1, (int)num_results);
            pbsys_hmi_lcd_ui_nxt_draw_activity_status(buf, 0);

            // Selection arrows with the name in between.
            pbio_image_t *display = pbdrv_display_get_image();
            const pbio_font_t *font = PBSYS_HMI_LCD_UI_NXT_OVERLAY_FONT;
            int arrow_y = (PBSYS_HMI_LCD_UI_NXT_LINE_Y(2) + PBSYS_HMI_LCD_UI_NXT_LINE_Y(3)) / 2;
            pbio_image_draw_text(display, font, 0, arrow_y, "<", 1, BLACK);
            pbio_image_draw_text(display, font, display->width - 5, arrow_y, ">", 1, BLACK);
            pbio_image_fill_rect(display, 7, PBSYS_HMI_LCD_UI_NXT_LINE_Y(2) - 9, display->width - 14, 23, BLACK);
            const char *name = result->name[0] ? result->name : "Unknown device";
            pbsys_hmi_lcd_ui_draw_device_name(font, name,
                PBSYS_HMI_LCD_UI_NXT_LINE_Y(2), PBSYS_HMI_LCD_UI_NXT_LINE_Y(3), display->width - 18, true);
            return;
        }
    }
}

/**
 * Draws a Bluetooth address with narrow colons, so it fits the width.
 *
 * Each character of the monospace font takes 6 pixels, which makes the full
 * address just too wide. The colons don't need a full character width: the
 * glyph is only drawn in its second and third column, so it gets 3 pixels
 * including one for spacing.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_compact_address(const char *address, int y) {
    pbio_image_t *display = pbdrv_display_get_image();
    const pbio_font_t *font = PBSYS_HMI_LCD_UI_NXT_OVERLAY_FONT;
    size_t len = strlen(address);

    // 12 digits with full width and 5 colons with 3 pixels each.
    int x = (display->width - (12 * 6 + 5 * 3)) / 2;
    for (size_t i = 0; i < len; i++) {
        if (address[i] == ':') {
            pbio_image_draw_text(display, font, x - 1, y, &address[i], 1, BLACK);
            x += 3;
        } else {
            pbio_image_draw_text(display, font, x, y, &address[i], 1, BLACK);
            x += 6;
        }
    }
}

/**
 * Draws the hub information overlay.
 *
 * The Bluetooth address is what this is mainly for: it is the one thing here
 * that the user has to copy into their program to reach this brick from
 * another one, so it comes first. The hardware version is omitted, since no
 * differences between brick versions are known.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_info_overlay(void) {

    pbsys_hmi_lcd_ui_nxt_draw_panel();

    char buf[40];

    pbsys_hmi_lcd_ui_nxt_draw_line("Bluetooth:", 0);

    uint8_t address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    if (pbdrv_bluetooth_peer_is_ready()) {
        pbdrv_bluetooth_peer_get_local_address(address);
        snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
            address[0], address[1], address[2], address[3], address[4], address[5]);
    } else {
        // Same shape, so the overlay does not jump when it becomes known.
        snprintf(buf, sizeof(buf), "--:--:--:--:--:--");
    }
    pbsys_hmi_lcd_ui_nxt_draw_compact_address(buf, PBSYS_HMI_LCD_UI_NXT_LINE_Y(1));

    // Plain version for a release, otherwise with the build and commit hash.
    if (PBIO_VERSION_LEVEL_HEX == 0xF) {
        snprintf(buf, sizeof(buf), "v%d.%d.%d",
            PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO);
    } else {
        snprintf(buf, sizeof(buf), "v%d.%d.%d%x%d-%.6s",
            PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO,
            PBIO_VERSION_LEVEL_HEX, PBIO_VERSION_SERIAL, PBIO_VERSION_HASH);
    }
    pbsys_hmi_lcd_ui_nxt_draw_line("Firmware:", 3);
    pbsys_hmi_lcd_ui_nxt_draw_line(buf, 4);
}

/**
 * Draws the overlay.
 *
 * @param  [in]  overlay The overlay to draw.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_overlay(pbsys_hmi_lcd_ui_overlay_type_t overlay) {

    // Nothing to do.
    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_NONE) {
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_HOST) {
        pbsys_hmi_lcd_ui_nxt_draw_host_overlay();
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_GAMEPAD) {
        pbsys_hmi_lcd_ui_nxt_draw_gamepad_overlay();
        return;
    }

    if (overlay == PBSYS_HMI_LCD_UI_OVERLAY_INFO) {
        pbsys_hmi_lcd_ui_nxt_draw_info_overlay();
        return;
    }

    pbio_image_t *display = pbdrv_display_get_image();
    pbsys_hmi_lcd_ui_nxt_draw_panel();

    // Add content specific to the overlay type.
    switch (overlay) {
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES:
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO: {
            bool yes = overlay == PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES;
            pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__off20, 40, 14, BLACK);
            pbio_image_draw_image_transparent_from_monochrome(display, yes ? &pbio_image_media__reject24 : &pbio_image_media__reject24_fill, 20, 38, BLACK);
            pbio_image_draw_image_transparent_from_monochrome(display, yes ? &pbio_image_media__accept24_fill : &pbio_image_media__accept24, 56, 38, BLACK);
            break;
        }
        case PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM:
        case PBSYS_HMI_LCD_UI_OVERLAY_COMING_SOON: {
            const char *text = overlay == PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM ? "No program!" : "Coming soon!";
            pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, text, 0, 30);
            pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__accept24_fill, 38, 38, BLACK);
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

    if (ui->in_tab) {
        // Entries of the current tab, one per row, inverting the selection.
        for (uint32_t s = 0; s < pbsys_hmi_lcd_ui_get_tab_num_entries(ui->tab); s++) {
            uint32_t row_y = 13 + s * 12;
            bool selected = s == ui->selection[ui->tab];
            if (selected) {
                pbio_image_fill_rect(display, 0, row_y, display->width, 11, BLACK);
            }
            const char *text = pbsys_hmi_lcd_ui_get_tab_entry_text(ui->tab, s);
            pbio_image_draw_text(display, &pbio_font_mono_8x5_8, 3, row_y + 9, text, strlen(text), selected ? WHITE : BLACK);
        }
    } else {
        // Three tab icons with a box around the selected one.
        const int icon_y = 18;
        for (uint8_t t = 0; t < PBSYS_HMI_LCD_UI_TAB_NUM; t++) {
            int center_x = 18 + t * 32;
            if (t == ui->tab) {
                pbio_image_draw_rounded_rect(display, center_x - 14, icon_y - 4, 29, 25, 2, BLACK);
            }
        }

        // Play icon.
        int x = 18;
        pbio_image_draw_circle(display, x, icon_y + 8, 8, BLACK);
        pbio_image_draw_vline(display, x - 2, icon_y + 4, 9, BLACK);
        pbio_image_draw_vline(display, x - 1, icon_y + 5, 7, BLACK);
        pbio_image_draw_vline(display, x, icon_y + 5, 7, BLACK);
        pbio_image_draw_vline(display, x + 1, icon_y + 6, 5, BLACK);
        pbio_image_draw_vline(display, x + 2, icon_y + 6, 5, BLACK);
        pbio_image_draw_vline(display, x + 3, icon_y + 7, 3, BLACK);
        pbio_image_draw_hline(display, x + 4, icon_y + 8, 2, BLACK);

        // Apps icon.
        uint8_t width = 8;
        uint8_t shift = width + 1;
        x = 50 - 8;
        pbio_image_draw_rounded_rect(display, x, icon_y, width, width, 1, BLACK);
        pbio_image_fill_rounded_rect(display, x + shift, icon_y, width, width, 1, BLACK);
        pbio_image_fill_rounded_rect(display, x, icon_y + shift, width, width, 1, BLACK);
        pbio_image_draw_rounded_rect(display, x + shift, icon_y + shift, width, width, 1, BLACK);

        // Settings icon.
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__wrench17, 82 - 8, icon_y, BLACK);

        // Program name or tab name below the icons. Empty without a program,
        // since starting it says so anyway.
        if (ui->tab == PBSYS_HMI_LCD_UI_TAB_PROGRAMS) {
            pbsys_hmi_lcd_ui_nxt_draw_program_name(pbsys_hmi_lcd_ui_get_program_name(ui->selection[PBSYS_HMI_LCD_UI_TAB_PROGRAMS]));
        } else {
            pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, tab_names[ui->tab], 0, PBSYS_HMI_LCD_UI_NXT_LABEL_Y);
        }
    }

    pbsys_hmi_lcd_ui_draw_status_bar();

    // Draw the overlay on top of everything else.
    pbsys_hmi_lcd_ui_nxt_draw_overlay(ui->overlay);

    // Draw everything to the screen.
    pbdrv_display_update();
}

/**
 * Width of the Pybricks logo on the program and credits screens.
 */
#define LOGO_WIDTH (70)

/**
 * Number of supporter names per credits page, one per line below the header.
 */
#define NUM_NAMES_PER_PAGE (5)

/**
 * Clears the screen and draws a credits page header with a line below.
 */
static void pbsys_hmi_lcd_ui_nxt_draw_credits_header(const char *text) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);
    pbio_image_draw_text(display, &pbio_font_mono_8x5_8, 2, 8, text, strlen(text), BLACK);
    pbio_image_draw_hline(display, 0, 10, display->width, BLACK);
}

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

        pbsys_hmi_lcd_ui_nxt_draw_credits_header("Our supporters:");

        // Populate page with list of names to fit. Last page may contain
        // some empty entries.
        for (size_t entry = 0; entry < NUM_NAMES_PER_PAGE; entry++) {
            const char *name = pbsys_hmi_lcd_ui_get_credits_name(page * NUM_NAMES_PER_PAGE + entry);
            if (!name) {
                continue;
            }
            pbio_image_draw_text(display, &pbio_font_mono_8x5_8, 2, 21 + entry * 10, name, strlen(name), BLACK);
        }

        // Show the page.
        pbdrv_display_update();
        PBIO_OS_AWAIT_MS(state, &timer, 1000);
        PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER);
    }

    // Display creator info.
    pbsys_hmi_lcd_ui_nxt_draw_credits_header("Made by:");
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_mono_8x5_8, "Laurens Valk", 0, 24);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_mono_8x5_8, "David Lechner", 0, 36);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_mono_8x5_8, "Pybricks", 0, 50);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_mono_8x5_8, "Community", 0, 60);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 1500);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_button_get_pressed() & PBIO_BUTTON_CENTER);

    // Display animated Pybricks thanks message.
    pbio_image_fill(display, WHITE);
    pbsys_hmi_lcd_ui_draw_pybricks_logo((display->width - LOGO_WIDTH) / 2, 4, LOGO_WIDTH, false);
    pbsys_hmi_lcd_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, "Thank you!", 0, 59);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 500);
    pbsys_hmi_lcd_ui_draw_pybricks_logo((display->width - LOGO_WIDTH) / 2, 4, LOGO_WIDTH, true);
    pbdrv_display_update();
    PBIO_OS_AWAIT_MS(state, &timer, 400);
    pbsys_hmi_lcd_ui_draw_pybricks_logo((display->width - LOGO_WIDTH) / 2, 4, LOGO_WIDTH, false);
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
 * Shows the Pybricks logo and program name while a program runs.
 *
 * This is a still image, since NXT has no lights and therefore no animation
 * timer.
 */
void pbsys_hmi_lcd_ui_run_animation_start(void) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);
    pbsys_hmi_lcd_ui_draw_pybricks_logo((display->width - LOGO_WIDTH) / 2, 3, LOGO_WIDTH, false);

    // Draw current program name below the logo.
    pbio_pybricks_user_program_id_t id;
    pbsys_main_program_start_request_type_t type;
    pbsys_main_program_get_info(&id, &type);
    if (id < PBSYS_CONFIG_STORAGE_NUM_SLOTS) {
        pbsys_hmi_lcd_ui_nxt_draw_program_name(pbsys_hmi_lcd_ui_get_program_name(id));
    }

    pbdrv_display_update();
}

void pbsys_hmi_lcd_ui_run_animation_stop(void) {
    // Nothing to stop, since this is a still image.
}

#endif // PBSYS_CONFIG_HMI_LCD_UI_NXT
