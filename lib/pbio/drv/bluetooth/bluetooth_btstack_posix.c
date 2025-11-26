#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX

#include "btstack_config.h"

#include "bluetooth_btstack.h"
#include "bluetooth_btstack_posix.h"

#include <errno.h>
#include <getopt.h>
#include <libusb.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btstack.h"
#include "ble/le_device_db_tlv.h"
#include "btstack_chipset_realtek.h"
#include "btstack_tlv_posix.h"
#include "classic/btstack_link_key_db_tlv.h"
#include "hci.h"
#include "hci_transport_usb.h"
#include "hci_dump_posix_stdout.h"

// This application only supports TP-Link UB500.
#define USB_MANUFACTURER_ID (BLUETOOTH_COMPANY_ID_REALTEK_SEMICONDUCTOR_CORPORATION)
#define USB_VENDOR_ID  (0x2357)
#define USB_PRODUCT_ID (0x0604)

#define TLV_DB_PATH_PREFIX "/tmp/btstack_"
#define TLV_DB_PATH_POSTFIX ".tlv"
static char tlv_db_path[100];
static const btstack_tlv_t *tlv_impl;
static btstack_tlv_posix_t tlv_context;
static bd_addr_t local_addr;

static const uint8_t read_static_address_command_complete_prefix[] = { 0x0e, 0x1b, 0x01, 0x09, 0xfc };
static bd_addr_t static_address;
static int using_static_address;

// We should not get here since we filtered device earlier, but this
// asserts that BTstack has discovered the same device from the port ID.
static void assert_vendor_and_product_id(uint16_t vendor_id, uint16_t product_id) {
    if (vendor_id != USB_VENDOR_ID || product_id != USB_PRODUCT_ID) {
        printf("Unexpected USB device: vendor ID 0x%04x, product ID 0x%04x\n", vendor_id, product_id);
        exit(1);
    }
}

// As above, but for Bluetooth manufacturer ID from HCI local version info.
static void assert_manufacturer_id(uint16_t manufacturer_id) {
    if (manufacturer_id != USB_MANUFACTURER_ID) {
        printf("Unexpected Bluetooth manufacturer ID: 0x%04x\n", manufacturer_id);
        exit(1);
    }
}

// Set by BTstack's packet handler when we get USB info and local version info.
static uint16_t usb_product_id;
static uint16_t usb_vendor_id;

void pbdrv_bluetooth_btstack_set_chipset(pbdrv_bluetooth_btstack_local_version_info_t *device_info) {

    assert_manufacturer_id(device_info->manufacturer);
    assert_vendor_and_product_id(usb_vendor_id, usb_product_id);

    btstack_chipset_realtek_set_lmp_subversion(device_info->lmp_pal_subversion);
    btstack_chipset_realtek_set_product_id(usb_product_id);

    btstack_chipset_realtek_set_firmware_file_path("/lib/firmware/rtl_bt/rtl8761bu_fw.bin");
    btstack_chipset_realtek_set_config_file_path("/lib/firmware/rtl_bt/rtl8761bu_config.bin");

    hci_set_chipset(btstack_chipset_realtek_instance());
}

static void noop_voidstararg(const void *) {
}

static int noop_returnint(void) {
    return 0;
}

static const btstack_control_t noop_btstack_control = {
    .init = noop_voidstararg,
    .on = noop_returnint,
    .off = noop_returnint,
    .sleep = noop_returnint,
    .wake = noop_returnint,
    .register_for_power_notifications = NULL,
};

const btstack_control_t *pbdrv_bluetooth_btstack_posix_control_instance(void) {
    return &noop_btstack_control;
}

const hci_transport_t *pbdrv_bluetooth_btstack_posix_transport_instance(void) {
    return hci_transport_usb_instance();
}

const void *pbdrv_bluetooth_btstack_posix_transport_config(void) {
    return NULL;
}

/**
 * Attempts to find the specified UB500 device among connected USB devices.
 *
 * BTstack has several ways to specify which USB device to use, but none are
 * suitable for our use case: we need to be able to specify multiple identical
 * devices on the same system.
 *
 * This function uses libusb to find the correct device based on an index
 * specified in the UB500_INDEX environment variable, and then passes the port
 * numbers to BTstack.
 *
 * @return ::PBIO_SUCCESS on success, or an ::ERROR_NO_DEV error if the device
 *         could not be found, so it can continue without Bluetooth.
 */
pbio_error_t pbdrv_bluetooth_btstack_platform_init(void) {

    const char *env_index = getenv("UB500_INDEX");
    if (!env_index) {
        // Silently continue without Bluetooth if not specified.
        return PBIO_ERROR_NO_DEV;
    }

    printf("Looking for UB500 with index %s.\n", env_index);

    int target_index = atoi(env_index);

    libusb_context *ctx = NULL;
    libusb_device **list = NULL;

    if (libusb_init(&ctx) < 0) {
        return PBIO_ERROR_NO_DEV;
    }

    ssize_t count = libusb_get_device_list(ctx, &list);
    if (count < 0) {
        libusb_exit(ctx);
        return PBIO_ERROR_NO_DEV;
    }

    libusb_device *match = NULL;
    int match_count = 0;

    for (ssize_t i = 0; i < count; i++) {
        libusb_device *dev = list[i];
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        if (desc.idVendor == USB_VENDOR_ID && desc.idProduct == USB_PRODUCT_ID) {
            if (match_count == target_index) {
                match = dev;
                break;
            }
            match_count++;
        }
    }

    pbio_error_t err = PBIO_SUCCESS;

    if (!match) {
        err = PBIO_ERROR_NO_DEV;
        goto exit;
    }

    printf("USB device found.\n");

    uint8_t ports[16];
    int port_count = libusb_get_port_numbers(match, ports, sizeof(ports));

    if (port_count) {
        // Tell BTstack to use this port path.
        hci_transport_usb_set_path(port_count, ports);
        printf("Using port: ");
        for (int i = 0; i < port_count; i++) {
            printf("%u%s", ports[i], i == port_count - 1 ? "" : ".");
        }
        printf("\n");
    } else {
        err = PBIO_ERROR_NO_DEV;
    }

exit:
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);

    if (err != PBIO_SUCCESS) {
        printf("Could not find specified device or port.\n");
    }
    return err;
}

void pbdrv_bluetooth_btstack_platform_poll(void) {

    btstack_run_loop_base_poll_data_sources();

    int nfds = btstack_linked_list_count(&btstack_run_loop_base_data_sources);
    struct pollfd fds[nfds];

    btstack_linked_list_iterator_t it;
    int i;
    for (i = 0, btstack_linked_list_iterator_init(&it, &btstack_run_loop_base_data_sources);
         btstack_linked_list_iterator_has_next(&it); ++i) {
        // cache pointer to next data_source to allow data source to remove itself
        btstack_data_source_t *ds = (void *)btstack_linked_list_iterator_next(&it);

        // Identify data source FDs that are ready for reading or writing.
        struct pollfd *pfd = &fds[i];
        pfd->fd = ds->source.fd;
        pfd->events = 0;
        if (ds->flags & DATA_SOURCE_CALLBACK_READ) {
            pfd->events |= POLLIN;
        }
        if (ds->flags & DATA_SOURCE_CALLBACK_WRITE) {
            pfd->events |= POLLOUT;
        }

    }

    int err = poll(fds, nfds, 0);
    if (err < 0) {
        printf("btstack: poll() returned %d, ignoring\n", errno);
    } else if (err > 0) {
        // Some fd was ready.
        for (i = 0, btstack_linked_list_iterator_init(&it, &btstack_run_loop_base_data_sources);
             btstack_linked_list_iterator_has_next(&it); ++i) {
            btstack_data_source_t *ds = (void *)btstack_linked_list_iterator_next(&it);
            struct pollfd *pfd = &fds[i];
            if (pfd->revents & POLLIN) {
                ds->process(ds, DATA_SOURCE_CALLBACK_READ);
            } else if (pfd->revents & POLLOUT) {
                ds->process(ds, DATA_SOURCE_CALLBACK_WRITE);
            } else if (pfd->revents & POLLERR) {
                printf("btstack: poll() error on fd %d\n", pfd->fd);
            }
        }
    }
}

void pbdrv_bluetooth_btstack_platform_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_TRANSPORT_USB_INFO: {
            usb_vendor_id = hci_event_transport_usb_info_get_vendor_id(packet);
            usb_product_id = hci_event_transport_usb_info_get_product_id(packet);
            assert_vendor_and_product_id(usb_vendor_id, usb_product_id);
            break;
        }
        case HCI_EVENT_COMMAND_COMPLETE:
            if (memcmp(packet, read_static_address_command_complete_prefix, sizeof(read_static_address_command_complete_prefix)) == 0) {
                reverse_48(&packet[7], static_address);
                gap_random_address_set(static_address);
                using_static_address = 1;
            }
            break;
        case BTSTACK_EVENT_STATE:
            switch (btstack_event_state_get_state(packet)) {
                case HCI_STATE_WORKING:
                    gap_local_bd_addr(local_addr);
                    btstack_strcpy(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_PREFIX);
                    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), bd_addr_to_str_with_delimiter(local_addr, '-'));
                    btstack_strcat(tlv_db_path, sizeof(tlv_db_path), TLV_DB_PATH_POSTFIX);
                    printf("\n");
                    tlv_impl = btstack_tlv_posix_init_instance(&tlv_context, tlv_db_path);
                    btstack_tlv_set_instance(tlv_impl, &tlv_context);
                    #ifdef ENABLE_CLASSIC
                    hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(tlv_impl, &tlv_context));
                    #endif
                    #ifdef ENABLE_BLE
                    le_device_db_tlv_configure(tlv_impl, &tlv_context);
                    #endif
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                    break;
                case HCI_STATE_OFF:
                    btstack_tlv_posix_deinit(&tlv_context);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX
