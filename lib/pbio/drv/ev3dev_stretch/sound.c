// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>

#include <linux/input.h>
#include <linux/pci.h>

#include <pbio/error.h>

#include <pbdrv/sound.h>

struct _pbdrv_sound_dev_t {
    int beep_file;
};

static pbdrv_sound_dev_t sound_dev;

pbio_error_t pbdrv_sound_get(pbdrv_sound_dev_t **_dev) {

    pbdrv_sound_dev_t *dev = &sound_dev;

    dev->beep_file = open("/dev/input/by-path/platform-sound-event", O_RDWR, 0);

    if (dev->beep_file == -1) {
        return PBIO_ERROR_IO;
    }

    *_dev = dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_sound_beep_freq(pbdrv_sound_dev_t *dev, uint32_t freq) {

    struct input_event event;

    event.type = EV_SND;
    event.code = SND_TONE;
    event.value = freq;

    if (write(dev->beep_file, &event, sizeof(struct input_event)) == -1) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}
