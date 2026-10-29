// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

// User interface state for hubs with a display and buttons, and the drawing
// helpers shared by each platform's drawing of it.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_LCD_UI_EV3

#include <pbsys/host.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/storage.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>
#include <pbdrv/display.h>

#include <pbio/battery.h>
#include <pbio/button.h>
#include <pbio/image.h>
#include <pbio/int_math.h>
#include <pbio/util.h>

#include "hmi_lcd_ui.h"
#include "pbio_image_media.h"
#include "storage.h"

#define BLACK (3)
#define WHITE (0)

/**
 * The UI state. Interactions modify this state and don't draw.
 *
 * On change, the whole UI is drawn.
 */
static pbsys_hmi_lcd_ui_t state;

/**
 * Available apps on app tab.
 */
static const struct {
    const char *name;
    pbio_pybricks_user_program_id_t id;
} apps[] = {
    #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_EV3_APPS
    { "Port View", PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_PORT_VIEW },
    { "Motor Button Control", PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_MOTOR_BUTTON_CONTROL },
    { "Motor IR Control", PBIO_PYBRICKS_USER_PROGRAM_ID_EV3_MOTOR_IR_CONTROL },
    #endif
};

/**
 * Available settings on settings tab.
 */
static const char *settings[] = {
    "Hub information",
    "Add new computer",
    "Add new gamepad",
};

// -----------------------------------------------------------------------------
// Modify the state based on user actions, without drawing it.
// -----------------------------------------------------------------------------

/**
 * Resets the initial state when the UI opens (when a program ends).
 *
 * This essentially just closes overlays if they were open when a program
 * started. Tabs and entry selection are unchanged, so you can easily start
 * again.
 */
void pbsys_hmi_lcd_ui_initialize(void) {
    state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
}

/**
 * Gets the UI state for drawing it.
 *
 * @return          The UI state.
 */
const pbsys_hmi_lcd_ui_t *pbsys_hmi_lcd_ui_get_state(void) {
    return &state;
}

/**
 * Gets the number of entries on the given tab.
 *
 * @param [in]  tab Tab index.
 * @return          Number of entries.
 */
uint8_t pbsys_hmi_lcd_ui_get_tab_num_entries(pbsys_hmi_lcd_ui_tab_t tab) {
    switch (tab) {
        case PBSYS_HMI_LCD_UI_TAB_PROGRAMS:
            return PBSYS_CONFIG_HMI_NUM_SLOTS;
        case PBSYS_HMI_LCD_UI_TAB_APPS:
            return PBIO_ARRAY_SIZE(apps);
        case PBSYS_HMI_LCD_UI_TAB_SETTINGS:
            return PBIO_ARRAY_SIZE(settings);
        default:
            return 0;
    }
}

/**
 * Gets the name of the program at a given slot.
 *
 * @param [in]  slot: Slot index.
 * @return      Name or empty string if unavailable.
 */
const char *pbsys_hmi_lcd_ui_get_program_name(uint8_t slot) {
    // Revisit: Get program name from meta data at system level.
    pbsys_main_program_t program;
    program.id = slot;
    pbsys_storage_get_program_data(&program);
    pbio_error_t err = pbsys_main_program_validate(&program);
    return err == PBIO_SUCCESS ? program.name : "";
}

/**
 * Gets the text of the given entry on the given tab.
 *
 * @param [in]  tab    Tab index.
 * @param [in]  entry  Entry index.
 * @return             Entry text.
 */
const char *pbsys_hmi_lcd_ui_get_tab_entry_text(pbsys_hmi_lcd_ui_tab_t tab, uint8_t entry) {
    switch (tab) {
        case PBSYS_HMI_LCD_UI_TAB_PROGRAMS:
            return pbsys_hmi_lcd_ui_get_program_name(entry);
        case PBSYS_HMI_LCD_UI_TAB_APPS:
            return apps[entry].name;
        case PBSYS_HMI_LCD_UI_TAB_SETTINGS:
            return settings[entry];
        default:
            return "";
    }
}

/**
 * Increments or decrements the entry on the current tab if possible.
 *
 * Modifies the state.
 *
 * @param  [in] increment   True to increment, false to decrement.
 */
static void pbsys_hmi_lcd_ui_increment_entry_on_current_tab(bool increment) {
    uint8_t *selection = &state.selection[state.tab];

    if (increment && *selection + 1 < pbsys_hmi_lcd_ui_get_tab_num_entries(state.tab)) {
        (*selection)++;
    } else if (!increment && *selection > 0) {
        (*selection)--;
    }
}

/**
 * Hub information overlay.
 *
 * Everything shown here is fixed at build time except the Bluetooth address,
 * which the controller only knows once it has finished starting up. So this
 * keeps refreshing until then, letting the address fill itself in rather than
 * leaving the user with a blank one if they get here quickly after boot.
 */
static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_info_button(pbio_button_flags_t button) {

    if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT || button == PBIO_BUTTON_CENTER) {
        state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
        return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }

    return pbdrv_bluetooth_peer_is_ready() ?
           PBSYS_HMI_LCD_UI_ACTION_NONE : PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
}

/**
 * Host computer overlay.
 *
 * The user pairs from their computer's own Bluetooth menu rather than from
 * the hub. That way the computer learns the hub name and shows its usual
 * pairing prompt, instead of an unnamed toy appearing out of nowhere. It is
 * also a one-off: afterwards the connection is always opened from a Pybricks
 * app, never from the computer's Bluetooth menu.
 *
 * So this walks the user through three steps, each drawn from live state:
 * pair from the computer, then connect from the app, then done. The hub makes
 * itself discoverable for the first step, which is what gets it into that
 * menu. The computer drops the link again right after pairing, since it has
 * no use for it yet, so the second step is also what tells the user that this
 * was expected and what to do next.
 *
 * Pairing needs nothing from the hub and can also happen on its own, such as
 * when a computer that still has the hub in its list reconnects after a
 * firmware update wiped the hub's key. If that happens while this is open,
 * moving on to the next step is still right.
 */
static struct {
    /** Pairing count when this was opened, to tell if one happened since. */
    uint32_t pair_count;
} host_ui;

pbsys_hmi_lcd_ui_host_phase_t pbsys_hmi_lcd_ui_get_host_phase(void) {
    if (pbdrv_bluetooth_classic_host_is_connected()) {
        return PBSYS_HMI_LCD_UI_HOST_PHASE_READY;
    }
    if (pbdrv_bluetooth_classic_host_get_pair_count() != host_ui.pair_count) {
        return PBSYS_HMI_LCD_UI_HOST_PHASE_CONNECT;
    }
    return PBSYS_HMI_LCD_UI_HOST_PHASE_PAIR;
}

static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_host_button(pbio_button_flags_t button) {

    if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT || button == PBIO_BUTTON_CENTER) {
        pbdrv_bluetooth_classic_host_set_discoverable(false);
        state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
        return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }

    // Discoverable only while waiting for pairing. Set on every refresh
    // rather than once, so that it follows the phase while this stays open.
    pbdrv_bluetooth_classic_host_set_discoverable(pbsys_hmi_lcd_ui_get_host_phase() == PBSYS_HMI_LCD_UI_HOST_PHASE_PAIR);
    return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
}

static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_host_open(void) {
    state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_HOST;
    host_ui.pair_count = pbdrv_bluetooth_classic_host_get_pair_count();
    return pbsys_hmi_lcd_ui_handle_host_button(0);
}

/**
 * Gamepad overlay.
 *
 * Gamepads have no user interface of their own, so the hub scans for them and
 * initiates pairing, which is the opposite of how a computer is added.
 */
static struct {
    pbsys_hmi_lcd_ui_gamepad_phase_t phase;
    /** Name of the gamepad being paired with or already in use. */
    char name[PBIO_BLUETOOTH_CLASSIC_NAME_SIZE];
    uint32_t selected_scan;
} gamepad_ui;

pbsys_hmi_lcd_ui_gamepad_phase_t pbsys_hmi_lcd_ui_get_gamepad_phase(void) {
    return gamepad_ui.phase;
}

const char *pbsys_hmi_lcd_ui_get_gamepad_name(void) {
    return gamepad_ui.name;
}

/**
 * Gets the selected gamepad scan result.
 *
 * Results can change while scanning, so this also keeps the selection within
 * the available results.
 *
 * @param  [out] selected     Index of the selected result.
 * @param  [out] num_results  Number of results so far.
 * @return                    Selected result or NULL if there are none.
 */
pbio_bluetooth_inquiry_result_t *pbsys_hmi_lcd_ui_get_gamepad_scan_result(uint32_t *selected, uint32_t *num_results) {
    pbio_bluetooth_inquiry_result_t *results;
    if (pbdrv_bluetooth_inquiry_get_results(num_results, &results) != PBIO_SUCCESS) {
        *num_results = 0;
    }
    if (gamepad_ui.selected_scan >= *num_results) {
        gamepad_ui.selected_scan = 0;
    }
    *selected = gamepad_ui.selected_scan;
    return *num_results ? &results[gamepad_ui.selected_scan] : NULL;
}

static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_gamepad_button(pbio_button_flags_t button) {

    switch (gamepad_ui.phase) {

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY: {
            if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT || button == PBIO_BUTTON_CENTER) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
                return PBSYS_HMI_LCD_UI_ACTION_NONE;
            }
            return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN: {
            if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
                pbdrv_bluetooth_inquiry_stop();
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
                return PBSYS_HMI_LCD_UI_ACTION_NONE;
            }

            uint32_t num_results;
            pbio_bluetooth_inquiry_result_t *results;
            if (pbdrv_bluetooth_inquiry_get_results(&num_results, &results) != PBIO_SUCCESS) {
                // Not scanning yet or previous scan completed, so (re)start. This
                // gets called again soon, so no need to handle errors here.
                pbdrv_bluetooth_inquiry_start();
                return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
            }

            if (num_results == 0) {
                return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
            }

            if (button == PBIO_BUTTON_LEFT) {
                gamepad_ui.selected_scan = (gamepad_ui.selected_scan + num_results - 1) % num_results;
            }
            if (button == PBIO_BUTTON_RIGHT) {
                gamepad_ui.selected_scan = (gamepad_ui.selected_scan + 1) % num_results;
            }
            if (button == PBIO_BUTTON_CENTER && gamepad_ui.selected_scan < num_results) {
                // Start pairing with the selected gamepad. The driver
                // registers the bonding record and stores the negotiated
                // link key into it. This is refused if a gamepad connected
                // while we were scanning.
                pbio_bluetooth_inquiry_result_t *result = &results[gamepad_ui.selected_scan];
                pbdrv_bluetooth_inquiry_stop();
                snprintf(gamepad_ui.name, sizeof(gamepad_ui.name), "%s", result->name);
                gamepad_ui.phase = pbdrv_bluetooth_classic_hid_pair(result->bdaddr, result->name) == PBIO_SUCCESS ?
                    PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_CONNECT : PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED;
            }
            return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_CONNECT: {
            pbio_error_t err = pbdrv_bluetooth_classic_hid_pair_status();
            if (err == PBIO_SUCCESS) {
                gamepad_ui.phase = PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY;
                return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
            }
            if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT || err == PBIO_ERROR_CANCELED) {
                // Cancelled here or externally (e.g. a program started).
                pbdrv_bluetooth_classic_hid_pair_cancel();
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
                return PBSYS_HMI_LCD_UI_ACTION_NONE;
            }
            if (err != PBIO_ERROR_AGAIN) {
                gamepad_ui.phase = PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED;
            }
            return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
        }

        case PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED:
        default: {
            if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
                return PBSYS_HMI_LCD_UI_ACTION_NONE;
            }
            if (button == PBIO_BUTTON_CENTER) {
                // Acknowledged, so scan again.
                gamepad_ui.phase = PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN;
            }
            return PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON;
        }
    }
}

static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_gamepad_open(void) {
    state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_GAMEPAD;
    // Only one gamepad can be connected, so if there already is one, say so
    // instead of offering to pair another that would be rejected.
    const char *name = NULL;
    if (pbdrv_bluetooth_classic_hid_is_connected()) {
        gamepad_ui.phase = PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY;
        name = pbdrv_bluetooth_classic_hid_get_connected_name();
    } else {
        gamepad_ui.phase = PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN;
    }
    snprintf(gamepad_ui.name, sizeof(gamepad_ui.name), "%s", name ? name : "Unknown device");
    gamepad_ui.selected_scan = 0;

    return pbsys_hmi_lcd_ui_handle_gamepad_button(0);
}

/**
 * Activates the selected entry on the current tab.
 *
 * @param  [out] payload   Payload of the return action.
 * @return                 Action to perform based on user interaction.
 */
static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_activate_selection(uint8_t *payload) {
    uint8_t selection = state.selection[state.tab];

    switch (state.tab) {
        case PBSYS_HMI_LCD_UI_TAB_PROGRAMS:
            *payload = selection;
            return PBSYS_HMI_LCD_UI_ACTION_RUN_PROGRAM;
        case PBSYS_HMI_LCD_UI_TAB_APPS:
            *payload = apps[selection].id;
            return PBSYS_HMI_LCD_UI_ACTION_RUN_PROGRAM;
        case PBSYS_HMI_LCD_UI_TAB_SETTINGS:
            switch (selection) {
                case 0:
                    state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_INFO;
                    return pbsys_hmi_lcd_ui_handle_info_button(0);
                case 1:
                    return pbsys_hmi_lcd_ui_handle_host_open();
                case 2:
                    return pbsys_hmi_lcd_ui_handle_gamepad_open();
                default:
                    // Other settings have no info.
                    return PBSYS_HMI_LCD_UI_ACTION_NONE;
            }
        default:
            // Other tabs not enabled.
            return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }
}

/**
 * Handles one button press on the menu, when no overlay is open.
 *
 * Left and right select the tab. Up and down select the entry on it.
 *
 * @param  [in]  button    Button that was pressed.
 * @param  [out] payload   Payload of the return action.
 * @return                 Action to perform based on user interaction.
 */
static pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_menu_button(pbio_button_flags_t button, uint8_t *payload) {

    // Open shutdown menu on exit button.
    if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
        state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO;
        return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }

    // Left/right selects tab.
    if (button == PBIO_BUTTON_LEFT && state.tab > 0) {
        state.tab--;
        return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }
    if (button == PBIO_BUTTON_RIGHT && state.tab < PBSYS_HMI_LCD_UI_TAB_NUM - 1) {
        state.tab++;
        return PBSYS_HMI_LCD_UI_ACTION_NONE;
    }

    // Up down selects entry on a tab.
    if (button == PBIO_BUTTON_DOWN || button == PBIO_BUTTON_UP) {
        pbsys_hmi_lcd_ui_increment_entry_on_current_tab(button == PBIO_BUTTON_DOWN);
        if (state.tab == PBSYS_HMI_LCD_UI_TAB_PROGRAMS) {
            *payload = state.selection[PBSYS_HMI_LCD_UI_TAB_PROGRAMS];
            return PBSYS_HMI_LCD_UI_ACTION_SET_SLOT;
        }
    }

    // Make selection.
    if (button == PBIO_BUTTON_CENTER) {
        return pbsys_hmi_lcd_ui_activate_selection(payload);
    }
    return PBSYS_HMI_LCD_UI_ACTION_NONE;
}

/**
 * Handles one button press to updates the state.
 *
 * Buttons are tested for equality, so only handling input for one button. If
 * multiple buttons are pressed, the input is ignored.
 *
 *
 * @param  [in]  button    Button that was pressed.
 * @param  [out] payload   Payload of the return action.
 * @return                 Action to perform based on user interaction.
 */
pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_button(pbio_button_flags_t button, uint8_t *payload) {
    *payload = 0;

    // Given the current state of the overlay and the button that is pressed,
    // update the state and return an action if applicable.
    switch (state.overlay) {
        case PBSYS_HMI_LCD_UI_OVERLAY_INFO:
            return pbsys_hmi_lcd_ui_handle_info_button(button);
        case PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM:
        // fallthrough
        case PBSYS_HMI_LCD_UI_OVERLAY_COMING_SOON:
            // Coming soon is open. Close and do nothing.
            if (button == PBIO_BUTTON_CENTER || button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
            }
            return PBSYS_HMI_LCD_UI_ACTION_NONE;
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES:
            // Shutdown yes is highlighted. Accept and shutdown or cancel and do nothing.
            if (button == PBIO_BUTTON_CENTER) {
                return PBSYS_HMI_LCD_UI_ACTION_SHUTDOWN;
            }
            if (button == PBIO_BUTTON_LEFT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO;
            }
            if (button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
            }
            return PBSYS_HMI_LCD_UI_ACTION_NONE;
        case PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO:
            // Shutdown no is highlighted. Scroll left or or cancel and do nothing.
            if (button == PBIO_BUTTON_CENTER || button == PBSYS_HMI_LCD_UI_BUTTON_EXIT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NONE;
            }
            if (button == PBIO_BUTTON_RIGHT) {
                state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES;
            }
            return PBSYS_HMI_LCD_UI_ACTION_NONE;
        case PBSYS_HMI_LCD_UI_OVERLAY_HOST:
            return pbsys_hmi_lcd_ui_handle_host_button(button);
        case PBSYS_HMI_LCD_UI_OVERLAY_GAMEPAD:
            return pbsys_hmi_lcd_ui_handle_gamepad_button(button);
        case PBSYS_HMI_LCD_UI_OVERLAY_NONE:
        // fallthrough
        default:
            return pbsys_hmi_lcd_ui_handle_menu_button(button, payload);
    }
}

/**
 * Updates the UI state based on an error raised in the HMI.
 *
 * @param  err  The error to handle.
 */
void pbsys_hmi_lcd_ui_handle_error(pbio_error_t err) {
    state.overlay = PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM;
}

// -----------------------------------------------------------------------------
// Drawing helpers shared by all platforms. These don't modify the state.
// -----------------------------------------------------------------------------

/**
 * Draw text relative to the horizontal center.
 *
 * @param [in] font      Font to use for drawing.
 * @param [in] text      Text string.
 * @param [in] x_offset  Horizontal offset from the centered position.
 * @param [in] y         Y coordinate of the baseline.
 */
void pbsys_hmi_lcd_ui_draw_centered_text(const pbio_font_t *font, const char *text, int x_offset, int y) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_rect_t rect;
    pbio_image_bbox_text(font, text, strlen(text), &rect);
    int x = (display->width - rect.width) / 2 + x_offset;
    pbio_image_draw_text(display, font, x, y, text, strlen(text), BLACK);
}

/**
 * Draws text, cut off with an ellipsis if it does not fit the given width.
 *
 * Only used for the hub name, which is what the scratch buffer is sized for.
 *
 * @param [in] font       Font to use for drawing.
 * @param [in] text       Text string.
 * @param [in] x          X coordinate of the left of the text.
 * @param [in] y          Y coordinate of the baseline.
 * @param [in] max_width  Width available for the text.
 */
void pbsys_hmi_lcd_ui_draw_truncated_text(const pbio_font_t *font, const char *text, int x, int y, int max_width) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_rect_t rect;
    size_t len = strlen(text);

    pbio_image_bbox_text(font, text, len, &rect);
    if (rect.width <= max_width) {
        pbio_image_draw_text(display, font, x, y, text, len, BLACK);
        return;
    }

    // Drop characters until the text and the ellipsis together fit.
    char buf[PBSYS_HOST_HUB_NAME_SIZE + 3];
    do {
        snprintf(buf, sizeof(buf), "%.*s...", (int)--len, text);
        pbio_image_bbox_text(font, buf, strlen(buf), &rect);
    } while (len > 0 && rect.width > max_width);

    pbio_image_draw_text(display, font, x, y, buf, strlen(buf), BLACK);
}

/**
 * Draws one centered line of a device name, optionally as a selection with
 * white text on a black box.
 */
static void pbsys_hmi_lcd_ui_draw_device_name_line(const pbio_font_t *font, const char *text, int y, bool selected) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_rect_t rect;
    size_t len = strlen(text);
    pbio_image_bbox_text(font, text, len, &rect);
    int x = (display->width - rect.width) / 2;
    pbio_image_draw_text(display, font, x, y, text, len, selected ? WHITE : BLACK);
}

/**
 * Draws a device name centered, split across two lines if needed.
 *
 * Prefers to split on a space. Text that still does not fit is cut off.
 */
void pbsys_hmi_lcd_ui_draw_device_name(const pbio_font_t *font, const char *name, int y1, int y2, int max_width, bool selected) {
    pbio_image_rect_t rect;
    size_t len = strlen(name);

    pbio_image_bbox_text(font, name, len, &rect);
    if (rect.width <= max_width) {
        // Single line, so center it vertically between the two lines.
        pbsys_hmi_lcd_ui_draw_device_name_line(font, name, (y1 + y2) / 2, selected);
        return;
    }

    // Find the longest prefix that fits on the first line.
    size_t fit = len;
    do {
        pbio_image_bbox_text(font, name, --fit, &rect);
    } while (fit > 1 && rect.width > max_width);

    // Prefer to split on a space, if there is one.
    size_t split = fit;
    while (split > 1 && name[split] != ' ') {
        split--;
    }
    if (name[split] != ' ') {
        split = fit;
    }

    char line[PBIO_BLUETOOTH_CLASSIC_NAME_SIZE];
    snprintf(line, sizeof(line), "%.*s", (int)split, name);
    pbsys_hmi_lcd_ui_draw_device_name_line(font, line, y1, selected);

    // Second line is cut off at whatever fits.
    const char *rest = name + split + (name[split] == ' ' ? 1 : 0);
    size_t rest_fit = strlen(rest);
    pbio_image_bbox_text(font, rest, rest_fit, &rect);
    while (rest_fit > 1 && rect.width > max_width) {
        pbio_image_bbox_text(font, rest, --rest_fit, &rect);
    }
    snprintf(line, sizeof(line), "%.*s", (int)rest_fit, rest);
    pbsys_hmi_lcd_ui_draw_device_name_line(font, line, y2, selected);
}

// Scaling factors and helper functions to simplify drawing the logo.
static uint32_t _scale_mul, _scale_div;
static uint32_t _offset_x, _offset_y;

static uint32_t sx(uint32_t x) {
    return (x * _scale_mul + _scale_div / 2) / _scale_div + _offset_x;
}
static uint32_t sy(uint32_t y) {
    return (y * _scale_mul + _scale_div / 2) / _scale_div + _offset_y;
}
static uint32_t sr(uint32_t r) {
    return (r * _scale_mul + _scale_div / 2) / _scale_div;
}

/**
 * Draws the Pybricks logo on the screen.
 *
 * @param  x     [in] Horizontal offset from the left.
 * @param  y     [in] Vertical offset from the top.
 * @param  width [in] Width (natural size is 154 x 84).
 * @param  blink [in] Whether to draw the eyes closed.
 */
void pbsys_hmi_lcd_ui_draw_pybricks_logo(uint32_t x, uint32_t y, uint32_t width, bool blink) {
    _offset_x = x;
    _offset_y = y;
    _scale_mul = width;
    _scale_div = 154;

    pbio_image_t *display = pbdrv_display_get_image();

    // Rounded rectangles making up the left and right side of the head.
    pbio_image_fill_rounded_rect(display, sx(0), sy(0), sr(42), sr(84), sr(11), BLACK);
    pbio_image_fill_rounded_rect(display, sx(112), sy(0), sr(42), sr(84), sr(11), BLACK);

    // Forehead, main fill, and jaw.
    pbio_image_fill_rect(display, sx(14), sy(0), sr(126), sr(14), BLACK);
    pbio_image_fill_rect(display, sx(14), sy(14), sr(126), sr(56), 0);
    pbio_image_fill_rect(display, sx(28), sy(56), sr(98), sr(14), BLACK);

    // Eyes.
    pbio_image_fill_circle(display, sx(49), sy(29), sr(10), blink ? WHITE : BLACK);
    pbio_image_fill_circle(display, sx(106), sy(29), sr(10), blink ? WHITE : BLACK);

    // Teeth.
    for (uint32_t i = 0; i < 6; i++) {
        pbio_image_fill_rect(display, sx(40 + 14 * i), sy(51), sr(4), sr(5), BLACK);
    }
}

/**
 * Draws the status bar along the top of the display, with the hub name on the
 * left and the battery and connections on the right.
 */
void pbsys_hmi_lcd_ui_draw_status_bar(void) {
    pbio_image_t *display = pbdrv_display_get_image();

    // Line below the status bar.
    pbio_image_draw_hline(display, 0, 10, display->width, BLACK);

    // Battery indicator: REVISIT: better percentage estimation at driver level.
    const int vmin = 5800;
    const int vmax = 8000;
    int voltage = pbio_int_math_bind(pbio_battery_get_average_voltage(), vmin, vmax);
    int bars = (voltage - vmin) * 11 / (vmax - vmin);
    int battery_x = display->width - 18;
    pbio_image_draw_rect(display, battery_x, 1, 15, 8, BLACK);
    pbio_image_draw_vline(display, battery_x + 14, 4, 2, WHITE);
    pbio_image_draw_vline(display, battery_x + 15, 3, 4, BLACK);
    pbio_image_fill_rect(display, battery_x + 2, 3, bars, 4, BLACK);

    // USB if connected.
    uint8_t right_align = battery_x;
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED)) {
        right_align -= pbio_image_media__usb_host.width + 2;
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__usb_host, right_align, 2, BLACK);
    }

    // Bluetooth Classic host computer and gamepad if connected.
    if (pbdrv_bluetooth_classic_host_is_connected()) {
        right_align -= pbio_image_media__bt_host.width + 2;
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__bt_host, right_align, 2, BLACK);
    }
    if (pbdrv_bluetooth_classic_hid_is_connected()) {
        right_align -= pbio_image_media__gamepad_small.width + 2;
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media__gamepad_small, right_align, 2, BLACK);
    }

    // Hub name on the left, in whatever space the icons leave over. This is
    // almost always on screen, so the hub information overlay does not repeat
    // it.
    pbsys_hmi_lcd_ui_draw_truncated_text(&pbio_font_mono_8x5_8, pbsys_host_get_hub_name(), 2, 9, right_align - 4);
}

// List of supporters created during build process.
extern const char *const hmi_lcd_ui_credits_names[];
extern const size_t hmi_lcd_ui_credits_size;

/**
 * Gets the number of supporters in the closing credits.
 */
size_t pbsys_hmi_lcd_ui_get_credits_num(void) {
    return hmi_lcd_ui_credits_size;
}

/**
 * Gets the name of a supporter in the closing credits.
 *
 * The first name is chosen randomly, keeping the order from there.
 *
 * @param [in]  position   Position of the name in order of appearance.
 * @return                 Name, or NULL if past the last name.
 */
const char *pbsys_hmi_lcd_ui_get_credits_name(size_t position) {

    // Choose first name randomly, keeping order.
    static size_t random = SIZE_MAX;
    if (random == SIZE_MAX) {
        random = pbdrv_clock_get_ms() % hmi_lcd_ui_credits_size;
    }

    if (position >= hmi_lcd_ui_credits_size) {
        return NULL;
    }

    return hmi_lcd_ui_credits_names[(random + position) % hmi_lcd_ui_credits_size];
}

#endif // PBSYS_CONFIG_HMI_LCD_UI_EV3
