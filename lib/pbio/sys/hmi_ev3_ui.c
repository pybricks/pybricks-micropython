// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <string.h>
#include <stdio.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_HMI_EV3_UI

#include <pbsys/main.h>
#include "storage.h"

#include <pbdrv/display.h>

#include <pbio/button.h>
#include <pbio/image.h>

#include "hmi_ev3_ui.h"
#include "pbio_image_media.h"

#define BLACK (3)
#define WHITE (0)

typedef enum {
    PBSYS_HMI_EV3_UI_TAB_PROGRAMS = 0,
    PBSYS_HMI_EV3_UI_TAB_APPS = 1,
    PBSYS_HMI_EV3_UI_TAB_SETTINGS = 2,
    PBSYS_HMI_EV3_UI_TAB_NUM,
} pbsys_hmi_ev3_ui_tab_t;

typedef enum {
    PBSYS_HMI_EV3_UI_OVERLAY_NONE,
    PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM,
    PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES,
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
    /** Overlay icon */
    pbio_image_monochrome_t *overlay_icon;
} pbsys_hmi_ev3_ui_t;

static pbsys_hmi_ev3_ui_t state;

static const char *apps[] = {
    " Port View",
    " Motor Control",
    " IR Control",
};

void pbsys_hmi_ev3_ui_initialize(void) {
    state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
}

static uint8_t pbsys_hmi_ev3_ui_get_tab_num_entries(void) {
    switch (state.tab) {
        case PBSYS_HMI_EV3_UI_TAB_PROGRAMS:
            return PBSYS_CONFIG_HMI_NUM_SLOTS;
        case PBSYS_HMI_EV3_UI_TAB_APPS:
            return PBIO_ARRAY_SIZE(apps);
        default:
            return 0;
    }
}

static const char *pbsys_hmi_ev3_ui_get_program_name_at_slot(uint8_t slot) {
    // Revisit: This is inefficient. Store program name and meta data at system
    // level instead of in program shutdown.
    pbsys_main_program_t program;
    program.id = slot;
    pbsys_storage_get_program_data(&program);
    pbio_error_t err = pbsys_main_program_validate(&program);
    const char *result = err == PBIO_SUCCESS ? program.name : "";
    static char name[20];
    snprintf(name, sizeof(name), "%c: %s", '1' + slot, result);
    return name;
}

static const char *pbsys_hmi_ev3_ui_get_tab_entry_text(uint8_t entry) {
    switch (state.tab) {
        case PBSYS_HMI_EV3_UI_TAB_PROGRAMS: {
            return pbsys_hmi_ev3_ui_get_program_name_at_slot(entry);
        }
        case PBSYS_HMI_EV3_UI_TAB_APPS:
            return apps[entry];
        default:
            return "";
    }
}

static void pbsys_hmi_ev3_ui_increment_entry_on_current_tab(bool increment) {
    uint8_t *selection = &state.selection[state.tab];

    if (increment && *selection + 1 < pbsys_hmi_ev3_ui_get_tab_num_entries()) {
        (*selection)++;
    } else if (!increment && *selection > 0) {
        (*selection)--;
    }
}

pbsys_hmi_ev3_ui_action_t pbsys_hmi_ev3_ui_handle_button(pbio_button_flags_t button, uint8_t *payload) {
    *payload = 0;

    switch (state.overlay) {
        case PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM:
            if (button == PBIO_BUTTON_CENTER || button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NONE;
            }
            return PBSYS_HMI_EV3_UI_ACTION_NONE;
        case PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES:
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
            if (button == PBIO_BUTTON_LEFT_UP) {
                state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO;
                return PBSYS_HMI_EV3_UI_ACTION_NONE;
            }
            break;
    }

    // Left/right selects tab.
    if ((button == PBIO_BUTTON_LEFT) && state.tab > 0) {
        state.tab--;
        return PBSYS_HMI_EV3_UI_ACTION_NONE;
    }
    if ((button == PBIO_BUTTON_RIGHT) && state.tab < 1) {
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
            // Program selection not yet implemented.
            *payload = 255;
            return PBSYS_HMI_EV3_UI_ACTION_RUN_PROGRAM;
        }
        // Settings not yet implemented.
    }
    return PBSYS_HMI_EV3_UI_ACTION_NONE;
}

void pbsys_hmi_ev3_ui_handle_error(pbio_error_t err) {
    state.overlay = PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM;
}

static void pbsys_hmi_ev3_ui_draw_overlay(pbsys_hmi_ev3_ui_overlay_type_t overlay) {

    if (overlay == PBSYS_HMI_EV3_UI_OVERLAY_NONE) {
        return;
    }

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

    // Reject and accept buttons.
    bool accept_only = overlay <= PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM;
    bool accept_active = accept_only || overlay == PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES;
    const pbio_image_monochrome_t *accept = accept_active ? &pbio_image_media_accept24_fill : &pbio_image_media_accept24;
    const pbio_image_monochrome_t *reject = accept_active ? &pbio_image_media_reject24 : &pbio_image_media_reject24_fill;
    if (accept_only) {
        pbio_image_draw_image_transparent_from_monochrome(display, accept, 76, bar_y + 3, BLACK);
    } else {
        pbio_image_draw_image_transparent_from_monochrome(display, reject, 56, bar_y + 3, BLACK);
        pbio_image_draw_image_transparent_from_monochrome(display, accept, 96, bar_y + 3, BLACK);
    }

    // State specific content.
    if (overlay == PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_YES || overlay == PBSYS_HMI_EV3_UI_OVERLAY_SHUTDOWN_NO) {
        pbio_image_draw_image_transparent_from_monochrome(display, &pbio_image_media_off20, 79, bar_y - 32, BLACK);
    } else if (overlay == PBSYS_HMI_EV3_UI_OVERLAY_NO_PROGRAM) {
        const char *no_program = "No program!";
        pbio_image_draw_text(display, &pbio_font_liberationsans_regular_14, x + 24, bar_y - 16, no_program, strlen(no_program), BLACK);
    }
}

void pbsys_hmi_ev3_ui_draw(void) {

    pbio_image_t *display = pbdrv_display_get_image();
    pbio_image_fill(display, WHITE);


    uint32_t slot_y = state.selection[state.tab] * 20;
    pbio_image_fill_rect(display, 6, 41 + slot_y, 166, 14, BLACK);

    for (uint32_t s = 0; s < pbsys_hmi_ev3_ui_get_tab_num_entries(); s++) {
        for (uint32_t i = 0; i < 56; i++) {
            pbio_image_draw_pixel(display, 6 + 3 * i, 56 + s * 20, BLACK);
        }

        const char *text = pbsys_hmi_ev3_ui_get_tab_entry_text(s);
        uint8_t color = s == state.selection[state.tab] ? WHITE : BLACK;
        pbio_image_draw_text(display, &pbio_font_liberationsans_regular_14, 8, 52 + s * 20, text, strlen(text), color);
        pbio_image_fill_rect(display, 172, 41, 6, 20 * 4, WHITE);
    }

    pbio_image_draw_hline(display, 0, 10, 178, BLACK);
    pbio_image_draw_rect(display, 1, 38, 176, 89, BLACK);
    pbio_image_draw_rounded_rect(display, 0, 37, 178, 91, 1, BLACK);

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

    // Play icon
    pbio_image_draw_circle(display, 26, 24, 8, BLACK);
    pbio_image_draw_vline(display, 23 + 1, 20, 9, BLACK);
    pbio_image_draw_vline(display, 24 + 1, 21, 7, BLACK);
    pbio_image_draw_vline(display, 25 + 1, 21, 7, BLACK);
    pbio_image_draw_vline(display, 26 + 1, 22, 5, BLACK);
    pbio_image_draw_vline(display, 27 + 1, 22, 5, BLACK);
    pbio_image_draw_vline(display, 28 + 1, 23, 3, BLACK);
    pbio_image_draw_hline(display, 29 + 1, 24, 2, BLACK);

    // Apps icon
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

    // Overlay
    pbsys_hmi_ev3_ui_draw_overlay(state.overlay);

    pbdrv_display_update();
}

#endif // PBSYS_CONFIG_HMI_EV3_UI
