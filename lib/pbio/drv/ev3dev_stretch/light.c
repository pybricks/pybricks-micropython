
#include <stdio.h>

#include <pbdrv/light.h>
#include <pbio/port.h>
#include <pbio/error.h>

#define NLEDS 4

FILE *f_brightness[NLEDS];

void _pbdrv_light_init(void) {

    const char trigger_paths[][51] = {
        "/sys/class/leds/led0:red:brick-status/trigger",
        "/sys/class/leds/led1:red:brick-status/trigger",
        "/sys/class/leds/led0:green:brick-status/trigger",
        "/sys/class/leds/led1:green:brick-status/trigger"
    };

    const char brightness_paths[][51] = {
        "/sys/class/leds/led0:red:brick-status/brightness",
        "/sys/class/leds/led1:red:brick-status/brightness",
        "/sys/class/leds/led0:green:brick-status/brightness",
        "/sys/class/leds/led1:green:brick-status/brightness"
    };

    for (int led = 0; led < NLEDS; led++) {
        FILE* f_trigger = fopen(trigger_paths[led], "w");
        fprintf(f_trigger, "none");
        fclose(f_trigger);
        f_brightness[led] = fopen(brightness_paths[led], "w");
    }
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_light_deinit(void) {
    for (int led = 0; led < NLEDS; led++) {
        fclose(f_brightness[led]);
    }
}
#endif

pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }
    for (int led = 0; led < NLEDS; led++) {
        fseek(f_brightness[led], 0, SEEK_SET);
        fprintf(f_brightness[led], "%d", led < 2 ? r : g);
        int err = fflush(f_brightness[led]);
        if (err == EOF) {
            return PBIO_ERROR_IO;
        }
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b) {

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (color) {
    case PBIO_LIGHT_COLOR_NONE:
        *r = 0;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_RED:
        *r = 255;
        *g = 0;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_ORANGE:
        *r = 255;
        *g = 255;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_YELLOW:
        *r = 30;
        *g = 255;
        *b = 0;
        break;
    case PBIO_LIGHT_COLOR_GREEN:
        *r = 0;
        *g = 255;
        *b = 0;
        break;
    default:
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}
