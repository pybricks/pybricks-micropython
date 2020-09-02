// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

// ev3dev-stretch I/O port

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>
#include <libudev.h>
#include <uthash.h>

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

typedef struct {
    const char *name;
    struct udev_device *device;
    UT_hash_handle hh;
} ev3dev_port_t;

static ev3dev_port_t *ev3dev_ports;

PROCESS(pbdrv_ioport_ev3dev_stretch_process, "ev3dev-stretch I/O port");

static void add_port(struct udev_device *device) {
    ev3dev_port_t *new_port;

    new_port = malloc(sizeof(*new_port));
    new_port->name = udev_device_get_property_value(device, "LEGO_ADDRESS");
    new_port->device = udev_device_ref(device);
    HASH_ADD_STR(ev3dev_ports, name, new_port);
}

static ev3dev_port_t *find_port(const char *name) {
    ev3dev_port_t *found_port;

    HASH_FIND_STR(ev3dev_ports, name, found_port);
    return found_port;
}

static void remove_port(struct udev_device *device) {
    ev3dev_port_t *remove_port;
    const char *name;

    name = udev_device_get_property_value(device, "LEGO_ADDRESS");
    if (!name) {
        return;
    }

    remove_port = find_port(name);
    if (!remove_port) {
        return;
    }

    HASH_DEL(ev3dev_ports, remove_port);
    udev_device_unref(remove_port->device);
    free(remove_port);
}

pbio_error_t pbdrv_ioport_ev3dev_get_syspath(pbio_port_t port, const char **syspath) {
    const char *name;
    ev3dev_port_t *p;

    switch (port) {
        case PBIO_PORT_1:
            name = "ev3-ports:in1";
            break;
        case PBIO_PORT_2:
            name = "ev3-ports:in2";
            break;
        case PBIO_PORT_3:
            name = "ev3-ports:in3";
            break;
        case PBIO_PORT_4:
            name = "ev3-ports:in4";
            break;
        case PBIO_PORT_A:
            name = "ev3-ports:outA";
            break;
        case PBIO_PORT_B:
            name = "ev3-ports:outB";
            break;
        case PBIO_PORT_C:
            name = "ev3-ports:outC";
            break;
        case PBIO_PORT_D:
            name = "ev3-ports:outD";
            break;
        default:
            return PBIO_ERROR_INVALID_PORT;
    }

    p = find_port(name);
    if (!p) {
        return PBIO_ERROR_NO_DEV;
    }

    *syspath = udev_device_get_syspath(p->device);

    return PBIO_SUCCESS;
}

PROCESS_THREAD(pbdrv_ioport_ev3dev_stretch_process, ev, data) {
    static struct udev *udev;
    static struct udev_monitor *monitor;
    static struct etimer timer;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *list, *entry;
    int ret;

    PROCESS_BEGIN();

    udev = udev_new();
    if (!udev) {
        perror("Failed to get udev");
        PROCESS_EXIT();
    }

    monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (!monitor) {
        perror("Failed to get udev monitor");
        PROCESS_EXIT();
    }

    enumerate = udev_enumerate_new(udev);
    if (!enumerate) {
        perror("Failed to get udev enumerate");
        PROCESS_EXIT();
    }

    ret = udev_enumerate_add_match_subsystem(enumerate, "lego-port");
    if (ret < 0) {
        errno = -ret;
        perror("udev_enumerate_add_match_subsystem failed");
        PROCESS_EXIT();
    }

    udev_monitor_filter_add_match_subsystem_devtype(monitor, "lego-port", NULL);
    udev_monitor_enable_receiving(monitor);

    ret = udev_enumerate_scan_devices(enumerate);
    if (ret < 0) {
        errno = -ret;
        perror("udev_enumerate_scan_devices failed");
        PROCESS_EXIT();
    }

    list = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(entry, list) {
        const char *syspath;
        struct udev_device *device;

        syspath = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udev, syspath);
        if (!device) {
            continue;
        }

        add_port(device);
        udev_device_unref(device);
    }

    // poll for added/removed devices every 100 milliseconds
    etimer_set(&timer, clock_from_msec(100));

    while (true) {
        struct udev_device *device;
        const char *action;

        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        etimer_reset(&timer);

        device = udev_monitor_receive_device(monitor);
        if (!device) {
            continue;
        }

        action = udev_device_get_action(device);
        if (!action) {
            continue;
        }

        if (strcmp(action, "add") == 0) {
            add_port(device);
        }
        if (strcmp(action, "remove") == 0) {
            remove_port(device);
        }
        udev_device_unref(device);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_IOPORT_EV3DEV_STRETCH
