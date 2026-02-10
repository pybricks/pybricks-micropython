// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <string.h>
#include <stdio.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_EV3_UI

#include <pbsys/main.h>
#include <pbsys/status.h>

#include <pbdrv/display.h>

#include <pbio/light_animation.h>
#include <pbio/battery.h>
#include <pbio/button.h>
#include <pbio/image.h>
#include <pbio/version.h>
#include <pbio/int_math.h>

#include "hmi_ev3_ui.h"
#include "pbio_image_media.h"
#include "storage.h"

#define BLACK (3)
#define WHITE (0)

/**
 * Main menu tabs.
 */
typedef enum {
    PBSYS_HMI_EV3_UI_TAB_PROGRAMS = 0,
    PBSYS_HMI_EV3_UI_TAB_APPS = 1,
    PBSYS_HMI_EV3_UI_TAB_SETTINGS = 2,
    PBSYS_HMI_EV3_UI_TAB_NUM,
} pbsys_hmi_ev3_ui_tab_t;

/**
 * All possible overlays.
 */
typedef enum {
    /** No overlay. */
    PBSYS_HMI_EV3_UI_OVERLAY_NONE,
    /** Selected program unavailable. */
    PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM,
    /** Feature coming soon. */
    PBSYS_HMI_EV3_UI_OVERLAY_COMING_SOON,
    /** Shutdown. Yes highlighted. */
    PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES,
    /** Shutdown. No highlighted. */
    PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO,
} pbsys_hmi_ev3_ui_overlay_type_t;

typedef struct {
    /**
     * Currently active tab.
     */
    pbsys_hmi_ev3_ui_tab_t tab;
    /**
     * Selected item on tabs.
     */
    uint8_t selection[PBSYS_HMI_EV3_UI_TAB_NUM];
    /**
     * Currently active overlay.
     */
    pbsys_hmi_ev3_ui_overlay_type_t overlay;
    /**
     * Overlay information.
     */
} pbsys_hmi_ev3_ui_t;

/**
 * The UI state. Interactions modify this state and don't draw.
 *
 * On change, the whole UI is drawn.
 */
static pbsys_hmi_ev3_ui_t state;

/**
 * Available apps on app tab.
 */
static const char *apps[] = {
    " Motor Control",
    " IR Control",
};

/**
 * Available settings on settings tab.
 */
static const char *settings[] = {
    " Version",
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
void pbsys_hmi_ev3_ui_initialize(void) {
    state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
}

/**
 * Gets the number of entries on the given tab.
 *
 * @param [in]  tab Tab index.
 * @return          Number of entries.
 */
static uint8_t pbsys_hmi_ev3_ui_get_tab_num_entries(pbsys_hmi_ev3_ui_tab_t tab) {
    switch (tab) {
        case PBSYS_HMI_EV3_UI_TAB_PROGRAMS:
            return PBSYS_CONFIG_HMI_NUM_SLOTS;
        case PBSYS_HMI_EV3_UI_TAB_APPS:
            return PBIO_ARRAY_SIZE(apps);
        case PBSYS_HMI_EV3_UI_TAB_SETTINGS:
            return PBIO_ARRAY_SIZE(settings);
        default:
            return 0;
    }
}

/**
 * Gets the string representation of a program at a given slot.
 *
 * @param [in]  slot: Slot index.
 * @return      Name or emptry string if unavailable.
 */
static const char *pbsys_hmi_ev3_ui_get_program_name_at_slot(uint8_t slot) {
    // Revisit: Get program name from meta data at system level.
    pbsys_main_program_t program;
    program.id = slot;
    pbsys_storage_get_program_data(&program);
    pbio_error_t err = pbsys_main_program_validate(&program);
    const char *result = err == PBIO_SUCCESS ? program.name : "";
    static char name[20];
    snprintf(name, sizeof(name), "%c: %s", '1' + slot, result);
    return name;
}

/**
 * Gets the text of the given entry on the given tab.
 *
 * @param [in]  tab    Tab index.
 * @param [in]  entry  Entry index.
 * @return             Entry text.
 */
static const char *pbsys_hmi_ev3_ui_get_tab_entry_text(pbsys_hmi_ev3_ui_tab_t tab, uint8_t entry) {
    switch (tab) {
        case PBSYS_HMI_EV3_UI_TAB_PROGRAMS: {
            return pbsys_hmi_ev3_ui_get_program_name_at_slot(entry);
        }
        case PBSYS_HMI_EV3_UI_TAB_APPS:
            return apps[entry];
        case PBSYS_HMI_EV3_UI_TAB_SETTINGS:
            if (entry == 0) {
                static char version[36];
                if (PBIO_VERSION_LEVEL_HEX == 0xF) {
                    snprintf(version, sizeof(version), " Version                  %d.%d.%d",
                        PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO);
                } else {
                    snprintf(version, sizeof(version), " Version %d.%d.%d%x%d-%.6s",
                        PBIO_VERSION_MAJOR, PBIO_VERSION_MINOR, PBIO_VERSION_MICRO,
                        PBIO_VERSION_LEVEL_HEX,
                        PBIO_VERSION_SERIAL, pbsys_main_get_application_version_hash());
                }
                return version;
            }
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
static void pbsys_hmi_ev3_ui_increment_entry_on_current_tab(bool increment) {
    uint8_t *selection = &state.selection[state.tab];

    if (increment && *selection + 1 < pbsys_hmi_ev3_ui_get_tab_num_entries(state.tab)) {
        (*selection)++;
    } else if (!increment && *selection > 0) {
        (*selection)--;
    }
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
pbsys_hmi_ev3_ui_action_t pbsys_hmi_ev3_ui_handle_button(pbio_button_flags_t button, uint8_t *payload) {
    *payload = 0;

    // Given the current state of the overlay and the button that is pressed,
    // update the state and return an action if applicable.
    switch (state.overlay) {
        case PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM:
        // fallthrough
        case PBSYS_HMI_EV3_UI_OVERLAY_COMING_SOON:
            // Coming soon is open. Close and do nothing.
            if (button == PBIO_BUTTON_CENTER || button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
            }
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        case PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES:
            // Shutdown yes is highlighted. Accept and shutdown or cancel and do nothing.
            if (button == PBIO_BUTTON_CENTER) {
                return PBSYS_HMI_EV3_UI_ACTION_SHUTDOWN;
            }
            if (button == PBIO_BUTTON_LEFT) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO;
            }
            if (button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
            }
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        case PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO:
            // Shutdown no is highlighted. Scroll left or or cancel and do nothing.
            if (button == PBIO_BUTTON_CENTER || button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
            }
            if (button == PBIO_BUTTON_RIGHT) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES;
            }
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        case PBSYS_HMI_EV3_UI_OVERLAY_NONE:
        // fallthrough
        default:
            // Currently no overlay. Open shutdown menu on cancel button.
            if (button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO;
                return PBSYS_HMI_EV3_UI_ACTION_NONE;
            }
            break;
    }

    // Left/right selects tab.
    if (button == PBIO_BUTTON_LEFT && state.tab > 0) {
        state.tab--;
        return PBSYS_HMI_EV3_UI_ACTION_NONE;
    }
    if (button == PBIO_BUTTON_RIGHT && state.tab < PBSYS_HMI_EV3_UI_TAB_NUM - 1) {
        state.tab++;
        return PBSYS_HMI_EV3_UI_ACTION_NONE;
    }

    // Up down selects entry on a tab.
    if (button == PBIO_BUTTON_DOWN || button == PBIO_BUTTON_UP) {
        pbsys_hmi_ev3_ui_increment_entry_on_current_tab(button == PBIO_BUTTON_DOWN);
        if (state.tab == PBSYS_HMI_EV3_UI_TAB_PROGRAMS) {
            *payload = state.selection[PBSYS_HMI_EV3_UI_TAB_PROGRAMS];
            return PBSYS_HMI_EV3_UI_ACTION_SET_SLOT;
        }
    }

    // Make selection.
    if (button == PBIO_BUTTON_CENTER) {
        if (state.tab == PBSYS_HMI_EV3_UI_TAB_PROGRAMS) {
            *payload = state.selection[PBSYS_HMI_EV3_UI_TAB_PROGRAMS];
            return PBSYS_HMI_EV3_UI_ACTION_RUN_PROGRAM;
        } else if (state.tab == PBSYS_HMI_EV3_UI_TAB_APPS) {
            // Apps not yet implemented.
            state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_COMING_SOON;
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        } else {
            // Settings don't currently do anything, just displays info.
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        }
    }
    return PBSYS_HMI_EV3_UI_ACTION_NONE;
}

/**
 * Updates the UI state based on an error raised in the HMI.
 *
 * @param  err  The error to handle.
 */
void pbsys_hmi_ev3_ui_handle_error(pbio_error_t err) {
    state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM;
}

// -----------------------------------------------------------------------------
// Draw the UI from the state without modifying it.
// -----------------------------------------------------------------------------

/**
 * Draw text relative to the horizontal center.
 *
 * @param [in] font      Font to use for drawing.
 * @param [in] x         X coordinate of the baseline relative to if centered text.
 * @param [in] y         Y coordinate of the baseline.
 * @param [in] text      Text string.
 */
static void pbsys_hmi_ev3_ui_draw_centered_text(const pbio_font_t *font, const char *text, int x_offset, int y) {
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_rect_t rect;
    pbio_image_bbox_text(font, text, strlen(text), &rect);
    int x = (display->width - rect.width) / 2 + x_offset;
    pbio_image_draw_text(display, &pbio_font_terminus_normal_16, x, y, text, strlen(text), BLACK);
}

/**
 * Draws the overlay.
 *
 * @param  [in]  overlay The overlay to draw.
 */
static void pbsys_hmi_ev3_ui_draw_overlay(pbsys_hmi_ev3_ui_overlay_type_t overlay) {

    // Nothing to do.
    if (overlay == PBSYS_HMI_EV3_UI_OVERLAY_NONE) {
        return;
    }

    // Currently all notifications fit on a single size.
    pbio_image_t *display = pbdrv_display_get_image();
    uint8_t width = 120;
    uint8_t height = 60;

    // Centered horizontally, slightly more down.
    uint8_t x = (178 - width) / 2;
    uint8_t y = (128 - height) * 3 / 4;

    // White rectangle, black border and white margin.
    pbio_image_fill_rect(display, x, y, width, height, WHITE);
    for (uint8_t i = 1; i < 4; i++) {
        pbio_image_draw_rect(display, x - i, y - i, width + i * 2, height + i * 2, i == 1 ? BLACK: WHITE);
    }

    // Horizontal divider.
    uint8_t bar_y = y + height - 21;
    pbio_image_draw_hline(display, x, bar_y, width, BLACK);

    // Add content specific to the overlay type.
    switch (overlay) {
        case PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES:
        case PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO:
            pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media_off20, 79, bar_y - 32, BLACK);
            bool accept_active = overlay == PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES;
            const pbio_image_monochrome_t *accept = accept_active ? &pbio_image_media_accept24_fill : &pbio_image_media_accept24;
            const pbio_image_monochrome_t *reject = accept_active ? &pbio_image_media_reject24 : &pbio_image_media_reject24_fill;
            pbio_image_draw_image_transparent_from_monochrome(display, reject, 56, bar_y + 3, BLACK);
            pbio_image_draw_image_transparent_from_monochrome(display, accept, 96, bar_y + 3, BLACK);
            break;
        case PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM:
        case PBSYS_HMI_EV3_UI_OVERLAY_COMING_SOON:
            pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media_accept24_fill, 76, bar_y + 3, BLACK);
            const char *text = overlay == PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM ? "No program!" : "Coming soon!";
            pbsys_hmi_ev3_ui_draw_centered_text(&pbio_font_liberationsans_regular_14, text, 0, bar_y - 16);
        default:
            break;
    }
}

void pbsys_hmi_ev3_ui_draw(void) {

    // Start with empty screen.
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);

    // Black box for selection highlight.
    pbio_image_fill_rect(display, 6, 41 + state.selection[state.tab] * 20, 166, 14, BLACK);

    // Draw entries with dotted underlines, inverting color for selection.
    for (uint32_t s = 0; s < pbsys_hmi_ev3_ui_get_tab_num_entries(state.tab); s++) {
        for (uint32_t i = 0; i < 56; i++) {
            pbio_image_draw_pixel(display, 6 + 3 * i, 56 + s * 20, BLACK);
        }
        const char *text = pbsys_hmi_ev3_ui_get_tab_entry_text(state.tab, s);
        uint8_t color = s == state.selection[state.tab] ? WHITE : BLACK;
        pbio_image_draw_text(display, &pbio_font_liberationsans_regular_14, 8, 52 + s * 20, text, strlen(text), color);
        pbio_image_fill_rect(display, 172, 41, 6, 20 * 4, WHITE);
    }

    // Draw box around tab entries.
    pbio_image_draw_rect(display, 1, 38, 176, 89, BLACK);
    pbio_image_draw_rounded_rect(display, 0, 37, 178, 91, 1, BLACK);

    // Draw the tab selector rounded squares, using thick line for selected.
    for (uint8_t t = 0; t < 4; t++) {
        uint32_t tab_x = t * 42;
        if (t == state.tab) {
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
    pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media_wrench17, 101, 16, BLACK);

    // Draw upper status bar.
    pbio_image_draw_hline(display, 0, 10, 178, BLACK);

    // Battery indicator: REVISIT: better percentage estimation at driver level.
    const int vmin = 5800;
    const int vmax = 8000;
    int voltage = pbio_int_math_bind(pbio_battery_get_average_voltage(), vmin, vmax);
    int bars = (voltage - vmin) * 11 / (vmax - vmin);
    pbio_image_draw_rect(display, 160, 1, 15, 8, BLACK);
    pbio_image_draw_vline(display, 174, 4, 2, WHITE);
    pbio_image_draw_vline(display, 175, 3, 4, BLACK);
    pbio_image_fill_rect(display, 162, 3, bars, 4, BLACK);

    // USB if connected.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USB_HOST_CONNECTED)) {
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media_usb_host, 130, 2, BLACK);
    }

    // Draw the overlay on top of everything else.
    pbsys_hmi_ev3_ui_draw_overlay(state.overlay);

    // Draw everything to the screen.
    pbdrv_display_update();
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
 */
static void pbsys_hmi_ev3_ui_draw_pybricks_logo(uint32_t x, uint32_t y, uint32_t width, bool blink) {
    _offset_x = x;
    _offset_y = y;
    _scale_mul = width;
    _scale_div = 154;

    pbio_image_t *display = pbdrv_display_get_image();

    uint8_t v = pbdrv_display_get_max_value();

    // Rounded rectangles making up the left and right side of the head.
    pbio_image_fill_rounded_rect(display, sx(0), sy(0), sr(42), sr(84), sr(11), v);
    pbio_image_fill_rounded_rect(display, sx(112), sy(0), sr(42), sr(84), sr(11), v);

    // Forehead, main fill, and jaw.
    pbio_image_fill_rect(display, sx(14), sy(0), sr(126), sr(14), v);
    pbio_image_fill_rect(display, sx(14), sy(14), sr(126), sr(56), 0);
    pbio_image_fill_rect(display, sx(28), sy(56), sr(98), sr(14), v);

    // Eyes.
    if (!blink) {
        pbio_image_fill_circle(display, sx(49), sy(29), sr(10), v);
        pbio_image_fill_circle(display, sx(106), sy(29), sr(10), v);
    }

    // Teeth.
    for (uint32_t i = 0; i < 6; i++) {
        pbio_image_fill_rect(display, sx(40 + 14 * i), sy(51), sr(4), sr(5), v);
    }
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
static uint32_t pbsys_hmi_ev3_ui_run_animation_next(pbio_light_animation_t *animation) {
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
    pbsys_hmi_ev3_ui_draw_pybricks_logo(12, y, 154, blink);

    // Render to screen.
    pbdrv_display_update();
    return ANIMATION_REFRESH_MS;
}

/**
 * User animation state.
 */
static pbio_light_animation_t animation;

/**
 * Starts the user program animation.
 */
void pbsys_hmi_ev3_ui_run_animation_start(void) {
    // Erase UI.
    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);

    // Start animation after the blink.
    elapsed = ANIMATION_BLINK_AT_MS + ANIMATION_BLINK_MS + ANIMATION_REFRESH_MS;
    pbio_light_animation_init(&animation, pbsys_hmi_ev3_ui_run_animation_next);
    pbio_light_animation_start(&animation);

    // Draw current program name below animation.
    pbio_pybricks_user_program_id_t id;
    pbsys_main_program_start_request_type_t type;
    pbsys_main_program_get_info(&id, &type);
    if (id < PBSYS_CONFIG_HMI_NUM_SLOTS) {
        const char *name = pbsys_hmi_ev3_ui_get_program_name_at_slot(id);
        pbsys_hmi_ev3_ui_draw_centered_text(&pbio_font_terminus_normal_16, name, -10, 116);
    }
    // Not updating display here. First animation frame will do that.
}

#endif // PBSYS_CONFIG_HMI_EV3_UI
