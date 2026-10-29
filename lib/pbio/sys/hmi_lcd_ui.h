// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// User interface for hubs with a display and buttons.
//
// The state and how buttons change it are shared by all such hubs, along
// with some drawing helpers. Drawing the state is done by each platform.

#ifndef _PBSYS_SYS_HMI_LCD_UI_H_
#define _PBSYS_SYS_HMI_LCD_UI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbsys/config.h>

#include <pbio/bluetooth.h>
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/image.h>
#include <pbio/os.h>

typedef enum {
    /**
     * No action required.
     */
    PBSYS_HMI_LCD_UI_ACTION_NONE,
    /**
     * No action required, but refresh the UI again soon. Can be used to
     * update Bluetooth scan result UIs for example.
     */
    PBSYS_HMI_LCD_UI_ACTION_REFRESH_SOON,
    /**
     * Sets active code slot (payload: slot id).
     */
    PBSYS_HMI_LCD_UI_ACTION_SET_SLOT,
    /**
     * Run a program (payload: program id).
     */
    PBSYS_HMI_LCD_UI_ACTION_RUN_PROGRAM,
    /**
     * Shut the hub down.
     */
    PBSYS_HMI_LCD_UI_ACTION_SHUTDOWN,
} pbsys_hmi_lcd_ui_action_t;

#if PBSYS_CONFIG_HMI_LCD_UI_EV3

/**
 * Button that closes overlays and opens the shutdown menu. This is the same
 * button that stops programs.
 */
#define PBSYS_HMI_LCD_UI_BUTTON_EXIT (PBSYS_CONFIG_HMI_STOP_BUTTON)

/**
 * Main menu tabs.
 */
typedef enum {
    PBSYS_HMI_LCD_UI_TAB_PROGRAMS = 0,
    PBSYS_HMI_LCD_UI_TAB_APPS = 1,
    PBSYS_HMI_LCD_UI_TAB_SETTINGS = 2,
    PBSYS_HMI_LCD_UI_TAB_NUM,
} pbsys_hmi_lcd_ui_tab_t;

/**
 * All possible overlays.
 */
typedef enum {
    /** No overlay. */
    PBSYS_HMI_LCD_UI_OVERLAY_NONE,
    /** Selected program unavailable. */
    PBSYS_HMI_LCD_UI_OVERLAY_NO_PROGRAM,
    /** Feature coming soon. */
    PBSYS_HMI_LCD_UI_OVERLAY_COMING_SOON,
    /** Hub information such as the Bluetooth address and version. */
    PBSYS_HMI_LCD_UI_OVERLAY_INFO,
    /** Shutdown. Yes highlighted. */
    PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_YES,
    /** Shutdown. No highlighted. */
    PBSYS_HMI_LCD_UI_OVERLAY_SHUTDOWN_NO,
    /** Adding a host computer over Bluetooth Classic. */
    PBSYS_HMI_LCD_UI_OVERLAY_HOST,
    /** Adding a Bluetooth Classic gamepad. */
    PBSYS_HMI_LCD_UI_OVERLAY_GAMEPAD,
} pbsys_hmi_lcd_ui_overlay_type_t;

/**
 * Host computer overlay phase.
 */
typedef enum {
    /** Waiting for the computer to pair. */
    PBSYS_HMI_LCD_UI_HOST_PHASE_PAIR,
    /** Paired, waiting for the app to connect. */
    PBSYS_HMI_LCD_UI_HOST_PHASE_CONNECT,
    /** Connected. */
    PBSYS_HMI_LCD_UI_HOST_PHASE_READY,
} pbsys_hmi_lcd_ui_host_phase_t;

/**
 * Gamepad overlay phase.
 */
typedef enum {
    /** Gamepad is paired and connected. */
    PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_READY,
    /** Scanning for gamepads and browsing the results. */
    PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_SCAN,
    /** Connecting to the selected gamepad. */
    PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_CONNECT,
    /** Connection attempt timed out. */
    PBSYS_HMI_LCD_UI_GAMEPAD_PHASE_FAILED,
} pbsys_hmi_lcd_ui_gamepad_phase_t;

typedef struct {
    /**
     * Currently active tab.
     */
    pbsys_hmi_lcd_ui_tab_t tab;
    /**
     * Selected item on tabs.
     */
    uint8_t selection[PBSYS_HMI_LCD_UI_TAB_NUM];
    /**
     * Currently active overlay.
     */
    pbsys_hmi_lcd_ui_overlay_type_t overlay;
} pbsys_hmi_lcd_ui_t;

// Shared: used by the HMI loop.

void pbsys_hmi_lcd_ui_initialize(void);

void pbsys_hmi_lcd_ui_handle_error(pbio_error_t err);

pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_button(pbio_button_flags_t button, uint8_t *payload);

// Shared: used by the platform to draw the state.

const pbsys_hmi_lcd_ui_t *pbsys_hmi_lcd_ui_get_state(void);

uint8_t pbsys_hmi_lcd_ui_get_tab_num_entries(pbsys_hmi_lcd_ui_tab_t tab);

const char *pbsys_hmi_lcd_ui_get_tab_entry_text(pbsys_hmi_lcd_ui_tab_t tab, uint8_t entry);

const char *pbsys_hmi_lcd_ui_get_program_name(uint8_t slot);

pbsys_hmi_lcd_ui_host_phase_t pbsys_hmi_lcd_ui_get_host_phase(void);

pbsys_hmi_lcd_ui_gamepad_phase_t pbsys_hmi_lcd_ui_get_gamepad_phase(void);

const char *pbsys_hmi_lcd_ui_get_gamepad_name(void);

pbio_bluetooth_inquiry_result_t *pbsys_hmi_lcd_ui_get_gamepad_scan_result(uint32_t *selected, uint32_t *num_results);

size_t pbsys_hmi_lcd_ui_get_credits_num(void);

const char *pbsys_hmi_lcd_ui_get_credits_name(size_t position);

void pbsys_hmi_lcd_ui_draw_centered_text(const pbio_font_t *font, const char *text, int x_offset, int y);

void pbsys_hmi_lcd_ui_draw_truncated_text(const pbio_font_t *font, const char *text, int x, int y, int max_width);

void pbsys_hmi_lcd_ui_draw_device_name(const pbio_font_t *font, const char *name, int y1, int y2, int max_width, bool selected);

void pbsys_hmi_lcd_ui_draw_pybricks_logo(uint32_t x, uint32_t y, uint32_t width, bool blink);

void pbsys_hmi_lcd_ui_draw_status_bar(void);

// Platform specific: implemented by hmi_lcd_ui_<platform>.c.

void pbsys_hmi_lcd_ui_draw(void);

void pbsys_hmi_lcd_ui_run_animation_start(void);

void pbsys_hmi_lcd_ui_run_animation_stop(void);

pbio_error_t pbsys_hmi_lcd_ui_closing_credits(pbio_os_state_t *state, void *context);

#else // PBSYS_CONFIG_HMI_LCD_UI_EV3

static inline void pbsys_hmi_lcd_ui_initialize(void) {
}

static inline void pbsys_hmi_lcd_ui_handle_error(pbio_error_t err) {
}

static inline pbsys_hmi_lcd_ui_action_t pbsys_hmi_lcd_ui_handle_button(pbio_button_flags_t button, uint8_t *payload) {
    return PBSYS_HMI_LCD_UI_ACTION_NONE;
}

static inline void pbsys_hmi_lcd_ui_draw(void) {
}

static inline void pbsys_hmi_lcd_ui_run_animation_start(void) {
}

static inline void pbsys_hmi_lcd_ui_run_animation_stop(void) {
}

static inline pbio_error_t pbsys_hmi_lcd_ui_closing_credits(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBSYS_CONFIG_HMI_LCD_UI_EV3

#endif // _PBSYS_SYS_HMI_LCD_UI_H_
