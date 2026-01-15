// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// Bluetooth RFCOMM support using BlueKitchen BTStack.

#include <pbdrv/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbio/os.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <btstack_config.h>

#include "bluetooth.h"

#if MAX_NR_RFCOMM_CHANNELS > 0

#include <btstack.h>

#include <bluetooth_sdp.h>
#include <classic/rfcomm.h>
#include <classic/sdp_client.h>
#include <classic/sdp_server.h>
#include <classic/spp_server.h>
#include <gap.h>
#include <lwrb/lwrb.h>
#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>

#include <umm_malloc.h>

#include <pbdrv/bluetooth.h>

#include <pbio/int_math.h>



#define DEBUG 2

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

static int link_db_entry_size(void) {
    return sizeof(bd_addr_t) + sizeof(link_key_t) + sizeof(link_key_type_t);
}

static int link_db_overhead(void) {
    return 2;  // 1 byte for length, 1 byte for checksum.
}

static int link_db_max_entries(void) {
    const int result = (PBSYS_CONFIG_BLUETOOTH_CLASSIC_LINK_KEY_DB_SIZE - link_db_overhead()) / link_db_entry_size();
    if (result > 255) {
        return 255;  // Only 1 byte to store the count.
    }
    return result;
}

// Computes the checksum for the link db, which we use to verify data integrity.
static uint8_t link_db_compute_checksum(uint8_t *data) {
    uint16_t count = data[0];
    const int end_offset = link_db_overhead() + count * link_db_entry_size();
    uint8_t checksum = 0;
    for (int off = 0; off < end_offset; off++) {
        checksum ^= data[off];
    }
    return checksum;
}

static uint8_t link_db_read_checksum(uint8_t *data) {
    uint16_t count = data[0];
    const int end_offset = link_db_overhead() + count * link_db_entry_size();
    return data[end_offset];
}

// Stores the link db to the settings and requests that it be written to flash.
static void link_db_settings_save(void) {
    btstack_link_key_iterator_t it;
    if (!gap_link_key_iterator_init(&it)) {
        DEBUG_PRINT("Failed to initialize link key iterator\n");
        return;
    }
    uint8_t *base = pbsys_storage_settings_get_link_key_db();
    if (base == NULL) {
        DEBUG_PRINT("No storage for link key database in settings\n");
        gap_link_key_iterator_done(&it);
        return;
    }
    uint16_t off = 0;

    uint8_t count = 0;
    off += 1;  // The first entry starts at offset 1.

    bdaddr_t addr;
    link_key_t key;
    link_key_type_t type;
    while (count < link_db_max_entries() &&
           gap_link_key_iterator_get_next(&it, addr, key, &type)) {
        memcpy(base + off, addr, sizeof(bd_addr_t));
        off += sizeof(bd_addr_t);
        memcpy(base + off, key, sizeof(link_key_t));
        off += sizeof(link_key_t);
        base[off] = type;
        off += sizeof(link_key_type_t);
        count++;
    }
    gap_link_key_iterator_done(&it);
    base[0] = count;
    base[off] = link_db_compute_checksum(base);
    pbsys_storage_request_write();
    DEBUG_PRINT("Saved %u link keys to settings\n", count);
}

// Loads the link DB from the settings.
// Note that the link db cannot be loaded until after the storage settings
// are ready. This might happen after bluetooth initialization, so we first
// load them instead when we become connectable (we won't need any link key
// information while we're not connectable anyway, since all connection
// requests will be rejected). We become connectable 1. during listen() and
// 2. during connect().
static void link_db_settings_load_once(void) {
    static bool loaded = false;
    if (loaded || !pbsys_storage_settings_ready()) {
        return;
    }
    loaded = true;

    uint8_t *base = pbsys_storage_settings_get_link_key_db();
    if (base == NULL) {
        DEBUG_PRINT("No link key database in settings\n");
        return;
    }
    uint16_t count = base[0];
    if (count > link_db_max_entries()) {
        DEBUG_PRINT("Link key database has invalid entry count, ignoring: %u\n",
            count);
        return;
    }

    if (link_db_read_checksum(base) != link_db_compute_checksum(base)) {
        DEBUG_PRINT("Link key database has invalid checksum, ignoring.\n");
        return;
    }

    uint16_t off = 1;
    for (uint16_t i = 0; i < count; i++) {
        bdaddr_t addr;
        link_key_t key;
        link_key_type_t type;
        memcpy(addr, base + off, sizeof(bd_addr_t));
        off += sizeof(bd_addr_t);
        memcpy(key, base + off, sizeof(link_key_t));
        off += sizeof(link_key_t);
        type = base[off];
        off += sizeof(link_key_type_t);
        gap_store_link_key_for_bd_addr(addr, key, type);
    }
    DEBUG_PRINT("Loaded %u link keys from settings\n", count);
}

// Returns true if the given address is already paired in our link database.
bool is_already_paired(bd_addr_t addr) {
    link_key_t key;
    link_key_type_t type;
    return gap_get_link_key_for_bd_addr(addr, key, &type);
}

#define RFCOMM_SOCKET_COUNT (MAX_NR_RFCOMM_CHANNELS > 0 ? MAX_NR_RFCOMM_CHANNELS - 1 : 0)
#if HAVE_UMM_MALLOC
#define RFCOMM_RX_BUFFER_SIZE (4 * 1024)
#define RFCOMM_TX_BUFFER_SIZE (2 * 1024)
#else
// When we don't have umm_malloc, statically allocate small buffers.
// This limits throughput but doesn't constraint the logical size of
// messages we can send.
#define RFCOMM_RX_BUFFER_SIZE (256)
#define RFCOMM_TX_BUFFER_SIZE (256)
#endif

typedef struct {
    #if HAVE_UMM_MALLOC
    uint8_t *tx_buffer_data; // tx_buffer from customer. We don't own this.
    uint8_t *rx_buffer_data;
    #else
    uint8_t tx_buffer_data[RFCOMM_TX_BUFFER_SIZE];
    uint8_t rx_buffer_data[RFCOMM_RX_BUFFER_SIZE];
    #endif
    pbio_os_timer_t tx_timer; // Timer for tracking timeouts on the current send.
    pbio_os_timer_t rx_timer; // Timer for tracking timeouts on the current receive.
    lwrb_t tx_buffer;         // Ring buffer to contain outgoing data.
    lwrb_t rx_buffer;         // Ring buffer to contain incoming data.

    int mtu; // MTU for this connection.

    // How many rfcomm credits are outstanding? When the connection is first started,
    // this is the rx buffer size divided by the MTU (the frame size). Each time we receive
    // a frame, this decreases by one. When frames are consumed by a reader, or if
    // the discrepancy between what we can hold and what is outstanding grows
    // too large, we grant more credits.
    int credits_outstanding;

    pbio_error_t err;         // The first encountered error.
    uint16_t cid;             // The local rfcomm connection handle.
    uint16_t server_channel;  // The remote rfcomm channel we're connected to.
    bool is_used;             // Is this socket descriptor in use?
    bool is_connected;        // Is this socket descriptor connected?
    bool is_cancelled;        // Has this socket been cancelled? Interrupts pending
                              // listen() or connect calls.
    bool is_using_sdp_system; // Is this socket currently using the SDP system?
} pbdrv_bluetooth_rfcomm_socket_t;

static pbdrv_bluetooth_rfcomm_socket_t pbdrv_bluetooth_rfcomm_sockets[RFCOMM_SOCKET_COUNT];

// The flow control mechanism in RFCOMM works on a credit system. Before a
// peer can send us a message, it needs a credit to do so. Each message sent
// consumes one credit.
//
// We allow a number of outstanding credits equal to the number of MTU-sized
// frames we have space for in the RX buffer. If the granted outstanding credits
// are less than this number, we grant the difference.
static void pbdrv_bluetooth_rfcomm_socket_grant_owed_credits(pbdrv_bluetooth_rfcomm_socket_t *socket) {
    const int desired_outstanding_credits =
        lwrb_get_free(&socket->rx_buffer) / socket->mtu;
    const int owed_credits =
        desired_outstanding_credits - socket->credits_outstanding;
    if (owed_credits > 0) {
        rfcomm_grant_credits(socket->cid, owed_credits);
        socket->credits_outstanding += owed_credits;
    }
}

static void pbdrv_bluetooth_rfcomm_socket_reset(pbdrv_bluetooth_rfcomm_socket_t *socket) {
    #if HAVE_UMM_MALLOC
    if (socket->rx_buffer_data) {
        umm_free(socket->rx_buffer_data);
        socket->rx_buffer_data = NULL;
    }
    if (socket->tx_buffer_data) {
        umm_free(socket->tx_buffer_data);
        socket->tx_buffer_data = NULL;
    }
    #endif
    if (lwrb_is_ready(&socket->rx_buffer)) {
        lwrb_free(&socket->rx_buffer);
    }
    if (lwrb_is_ready(&socket->tx_buffer)) {
        lwrb_free(&socket->tx_buffer);
    }
    // A non-zero duration on these timers is used as a flag to indicate the
    // presence of a deadline on a connection attempt.
    socket->rx_timer.duration = 0;
    socket->tx_timer.duration = 0;
    socket->is_used = false;
    socket->is_connected = false;
    socket->is_cancelled = false;
    socket->is_using_sdp_system = false;
    socket->cid = (uint16_t)-1;
    socket->mtu = 0;
    socket->credits_outstanding = 0;
    socket->err = PBIO_SUCCESS;
}

static pbdrv_bluetooth_rfcomm_socket_t *pbdrv_bluetooth_rfcomm_socket_alloc(void) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (!pbdrv_bluetooth_rfcomm_sockets[i].is_used) {
            pbdrv_bluetooth_rfcomm_socket_t *sock = &pbdrv_bluetooth_rfcomm_sockets[i];
            pbdrv_bluetooth_rfcomm_socket_reset(sock);
            sock->is_used = true;
            #if HAVE_UMM_MALLOC
            sock->rx_buffer_data = umm_malloc(RFCOMM_RX_BUFFER_SIZE);
            sock->tx_buffer_data = umm_malloc(RFCOMM_TX_BUFFER_SIZE);
            if (!sock->rx_buffer_data || !sock->tx_buffer_data) {
                DEBUG_PRINT("Failed to allocate RFCOMM RX or TX buffer.\n");
                sock->is_used = false;
                return NULL;
            }
            #endif
            lwrb_init(&sock->rx_buffer, sock->rx_buffer_data, RFCOMM_RX_BUFFER_SIZE);
            lwrb_init(&sock->tx_buffer, sock->tx_buffer_data, RFCOMM_TX_BUFFER_SIZE);
            return sock;
        }
    }
    DEBUG_PRINT("[btc] Alloc failed; all sockets in use.\n");
    return NULL;
}

static pbdrv_bluetooth_rfcomm_socket_t *pbdrv_bluetooth_rfcomm_socket_find_by_cid(uint16_t cid) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (pbdrv_bluetooth_rfcomm_sockets[i].is_used &&
            pbdrv_bluetooth_rfcomm_sockets[i].cid == cid) {
            return &pbdrv_bluetooth_rfcomm_sockets[i];
        }
    }
    return NULL;
}

static pbdrv_bluetooth_rfcomm_socket_t *pbdrv_bluetooth_rfcomm_socket_find_by_conn(const pbdrv_bluetooth_rfcomm_conn_t *c) {
    if (c->conn_id < 0 || c->conn_id >= RFCOMM_SOCKET_COUNT) {
        return NULL;
    }
    return &pbdrv_bluetooth_rfcomm_sockets[c->conn_id];
}

static int pbdrv_bluetooth_rfcomm_socket_id(pbdrv_bluetooth_rfcomm_socket_t *socket) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (&pbdrv_bluetooth_rfcomm_sockets[i] == socket) {
            return i;
        }
    }
    return -1;
}

static pbdrv_bluetooth_rfcomm_socket_t *pending_listen_socket;

static void user_rfcomm_event_handler(uint8_t *packet, uint16_t size) {
    (void)size;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch (event_type) {
        case RFCOMM_EVENT_CHANNEL_OPENED: {
            uint16_t cid = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
            pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_cid(cid);
            if (!sock) {
                uint8_t bdaddr[6];
                rfcomm_event_channel_opened_get_bd_addr(packet, bdaddr);
                DEBUG_PRINT("Unknown cid (%u) associated with address %s\n", cid,
                    bd_addr_to_str(bdaddr));
                rfcomm_disconnect(cid);
                break;
            }
            int status = rfcomm_event_channel_opened_get_status(packet);
            if (status != 0) {
                DEBUG_PRINT("RFCOMM channel open failed with status: %d", status);
                sock->err = PBIO_ERROR_FAILED;
                break;
            }
            DEBUG_PRINT("RFCOMM channel opened: cid=%u.\n", cid);
            sock->mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
            if (sock->mtu == 0) {
                rfcomm_disconnect(cid);
                DEBUG_PRINT("RFCOMM channel opened with invalid MTU==0, dropping "
                    "connection.\n");
                sock->err = PBIO_ERROR_FAILED;
            }
            sock->is_connected = true;
            sock->credits_outstanding = 0;
            pbdrv_bluetooth_rfcomm_socket_grant_owed_credits(sock);
            break;
        }

        case RFCOMM_EVENT_INCOMING_CONNECTION: {
            uint16_t cid = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);

            if (!pending_listen_socket) {
                // Someone tried to connect while we weren't listening.
                rfcomm_decline_connection(cid);
                break;
            }
            pbdrv_bluetooth_rfcomm_socket_t *sock = pending_listen_socket;
            pending_listen_socket = NULL;

            rfcomm_accept_connection(cid);
            sock->cid = cid;
            break;
        }

        case RFCOMM_EVENT_CAN_SEND_NOW: {
            uint16_t cid = rfcomm_event_can_send_now_get_rfcomm_cid(packet);
            pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_cid(cid);
            if (!sock) {
                DEBUG_PRINT("Unknown cid (%u) for CAN_SEND_NOW event, dropping connection.\n", cid);
                rfcomm_disconnect(cid);
                break;
            }

            if (lwrb_get_full(&sock->tx_buffer) == 0) {
                break;
            }

            uint8_t *data = lwrb_get_linear_block_read_address(&sock->tx_buffer);
            const int desired_send =
                lwrb_get_linear_block_read_length(&sock->tx_buffer);
            int write_len = pbio_int_math_min(sock->mtu, desired_send);
            lwrb_skip(&sock->tx_buffer, write_len);
            int err = rfcomm_send(sock->cid, data, write_len);
            if (err) {
                DEBUG_PRINT("Failed to send RFCOMM data: %d\n", err);
                sock->err = PBIO_ERROR_FAILED;
                rfcomm_disconnect(sock->cid);
                break;
            }

            DEBUG_PRINT(
                "[btc:rfcomm_send] Sent %d/%d bytes to RFCOMM channel.\n",
                write_len, desired_send);

            if (lwrb_get_full(&sock->tx_buffer) > 0) {
                // More to send.
                rfcomm_request_can_send_now_event(sock->cid);
            }
            // Threads may be waiting for the notification that there's room in
            // the send buffer.
            pbio_os_request_poll();

            break;
        }

        case RFCOMM_EVENT_CHANNEL_CLOSED: {
            uint16_t cid = rfcomm_event_channel_closed_get_rfcomm_cid(packet);
            pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_cid(cid);
            if (!sock) {
                break;
            }

            DEBUG_PRINT("RFCOMM_EVENT_CHANNEL_CLOSED by remote for cid=%u.\n", cid);

            // Note: we do not reset the socket, since the user is expected
            // to call pbdrv_bluetooth_rfcomm_close() or disconnect_all() first.
            if (sock->is_connected) {
                DEBUG_PRINT("RFCOMM channel closed: cid=%u.\n", cid);
                sock->err = PBIO_ERROR_IO;
                sock->is_connected = false;
            }
            break;
        }

        default:
            DEBUG_PRINT("Received unknown RFCOMM event: %u\n", event_type);
            break;
    }
}

static void user_rfcomm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET: {
            user_rfcomm_event_handler(packet, size);
            break;
        }
        case RFCOMM_DATA_PACKET: {
            pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_cid(channel);

            if (!sock) {
                DEBUG_PRINT("Received RFCOMM data for unknown channel: 0x%04x\n", channel);
                break;
            }

            if (size > lwrb_get_free(&sock->rx_buffer)) {
                DEBUG_PRINT("Received RFCOMM data that exceeds buffer capacity: %u\n", size);
                sock->err = PBIO_ERROR_FAILED;
            }
            DEBUG_PRINT("[btc:rfcomm_recv] %d bytes\n", size);
            lwrb_write(&sock->rx_buffer, packet, size);

            // Each packet we receive consumed a credit on the remote side.
            --sock->credits_outstanding;
            pbdrv_bluetooth_rfcomm_socket_grant_owed_credits(sock);
            // Threads may be waiting for the notification that there's data in
            // the receive buffer.
            pbio_os_request_poll();
            break;
        }
    }
}

static bool hci_handle_to_bd_addr(uint16_t handle, bd_addr_t addr) {
    hci_connection_t *conn = hci_connection_for_handle(handle);
    if (!conn) {
        return false;
    }
    memcpy(addr, conn->address, sizeof(bd_addr_t));
    return true;
}

void pbdrv_bluetooth_btstack_handle_classic_security_packet(uint8_t *packet, uint16_t size) {
    (void)size;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            bd_addr_t requester_addr;
            hci_event_user_confirmation_request_get_bd_addr(packet, requester_addr);
            DEBUG_PRINT("SSP User Confirmation Request. Auto-accepting...\n");
            gap_ssp_confirmation_response(requester_addr);
            break;
        }
        case HCI_EVENT_AUTHENTICATION_COMPLETE: {
            uint8_t auth_status =
                hci_event_authentication_complete_get_status(packet);
            if (auth_status == ERROR_CODE_AUTHENTICATION_FAILURE ||
                auth_status == ERROR_CODE_PIN_OR_KEY_MISSING) {
                DEBUG_PRINT(
                    "AUTH FAIL: Link key rejected/missing (Status 0x%02x).\n",
                    auth_status);
                uint16_t handle =
                    hci_event_authentication_complete_get_connection_handle(packet);
                bd_addr_t addr;
                if (!hci_handle_to_bd_addr(handle, addr)) {
                    DEBUG_PRINT("AUTH FAIL: Unknown address for handle 0x%04x\n",
                        handle);
                    break;
                }
                gap_drop_link_key_for_bd_addr(addr);
                link_db_settings_save();
            }
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            uint8_t reason = hci_event_disconnection_complete_get_reason(packet);
            uint16_t handle =
                hci_event_disconnection_complete_get_connection_handle(packet);
            bd_addr_t addr;
            if (reason == ERROR_CODE_AUTHENTICATION_FAILURE) {
                if (!hci_handle_to_bd_addr(handle, addr)) {
                    DEBUG_PRINT("DISCONNECTED: Unknown address for handle 0x%04x\n",
                        handle);
                    break;
                }
                DEBUG_PRINT("DISCONNECTED: Authentication failure.\n");
                gap_drop_link_key_for_bd_addr(addr);
                link_db_settings_save();
            } else {
                DEBUG_PRINT("DISCONNECTED: Reason 0x%02x\n", reason);
            }
            break;
        }
        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            // BTStack has already updated the link key database, so we just need to save it.
            DEBUG_PRINT("Link key updated, saving to settings.\n");
            link_db_settings_save();
            break;
        }
    }
}

// Is some active rfcomm_connect call using the SDP system?
static bool sdp_system_in_use = false;
// Is there an ongoing SDP query? Even if nobody is using the SDP system (e.g. a
// query was started then abandoned), we can't issue a new query until we get
// the SDP_EVENT_QUERY_COMPLETE event.
static bool sdp_query_pending = false;
// Memory location for the result of the current SDP query.
static uint16_t sdp_query_rfcomm_channel;

static void bluetooth_btstack_classic_sdp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;

    if (packet_type != HCI_EVENT_PACKET) {
        DEBUG_PRINT("SDP packet handler unexpected packet type %u\n",
            packet_type);
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
        case SDP_EVENT_QUERY_RFCOMM_SERVICE: {
            if (!sdp_query_pending) {
                DEBUG_PRINT("Received unexpected SDP query result.\n");
                return;
            }
            if (sdp_query_rfcomm_channel != 1) {
                // Note: we prefer to return channel 1 over any other channel, since this is the default
                // spp profile channel. The main purpose of this SDP query is to find channels *other* than
                // one, since the default channel may not be served especially in the case of Windows bluetooth
                // com ports.
                //
                // One limitation of our implementation here is that we don't provide the user any way to select
                // between multiple RFCOMM channels. Perhaps in the future we will allow users to manually specify
                // the channel if they know their server will be listening on a channel other than 1. All EV3s
                // will listen on channel 1.
                sdp_query_rfcomm_channel =
                    sdp_event_query_rfcomm_service_get_rfcomm_channel(packet);
            }
            DEBUG_PRINT("Found RFCOMM channel: %u\n", sdp_query_rfcomm_channel);
            break;
        }
        case SDP_EVENT_QUERY_COMPLETE: {
            DEBUG_PRINT("SDP query complete.\n");
            sdp_query_pending = false;
            pbio_os_request_poll();
            break;
        }
        default: {
            DEBUG_PRINT("Received ignored SDP event: %u\n",
                hci_event_packet_get_type(packet));
            break;
        }
    }
}

void pbdrv_bluetooth_classic_init(void) {
    l2cap_init();
    rfcomm_init();
    sdp_init();

    gap_ssp_set_enable(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_auto_accept(true);
    gap_set_class_of_device(0x000804);  // Toy : Robot

    hci_set_link_key_db(btstack_link_key_db_memory_instance());

    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    sdp_client_init();
    rfcomm_set_required_security_level(LEVEL_2);

    static uint8_t spp_service_buffer[150];
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));

    spp_create_sdp_record(spp_service_buffer, 0x10001, 1, "Pybricks RFCOMM Service");
    sdp_register_service(spp_service_buffer);

    // All EV3s listen only on channel 1 (the default SPP channel).
    rfcomm_register_service_with_initial_credits(&user_rfcomm_packet_handler, 1, 1024, 0);
}

// Returns whether this socket is in a state where we should abandon
// our connection attempt, either listening or connecting.
static bool should_abandon_connection(pbdrv_bluetooth_rfcomm_socket_t *sock) {
    if (!sock) {
        return false;
    }
    if (sock->rx_timer.duration > 0 &&
        pbio_os_timer_is_expired(&sock->rx_timer)) {
        sock->err = PBIO_ERROR_TIMEDOUT;
        return true;
    }
    if (sock->is_cancelled) {
        sock->err = PBIO_ERROR_CANCELED;
        return true;
    }
    if (sock->err != PBIO_SUCCESS) {
        return true;
    }
    return false;
}

pbio_error_t pbdrv_bluetooth_rfcomm_connect(pbio_os_state_t *state, bdaddr_t bdaddr, int32_t timeout, pbdrv_bluetooth_rfcomm_conn_t *conn) {
    // Each time we resume this function, we need to load the socket pointer.
    // On the first time through, initialize the conn_id to some non-matching
    // value to prevent finding a socket owned by someone else.
    if (*state == 0) {
        conn->conn_id = -1;
    }
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (should_abandon_connection(sock)) {
        goto cleanup;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // The link db is loaded lazily since the storage settings system may not be
    // ready when the bluetooth system is initialized, but it will be ready by
    // the time we try to make any rfcomm connections.
    link_db_settings_load_once();

    sock = pbdrv_bluetooth_rfcomm_socket_alloc();
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_connect] No more sockets.\n");
        // In this one case we need to return directly, because the cleanup
        // handler would try to access sock.
        return PBIO_ERROR_RESOURCE_EXHAUSTED;
    }
    conn->conn_id = pbdrv_bluetooth_rfcomm_socket_id(sock);

    if (timeout > 0) {
        // The rx_timer is used to track connection timeouts both for
        // rfcomm_connect and ..._listen.
        pbio_os_timer_set(&sock->rx_timer, timeout);
    }

    // Wait until the Bluetooth controller is up.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);

    // Wait until any other pending SDP query is done.
    PBIO_OS_AWAIT_UNTIL(state, !sdp_query_pending && !sdp_system_in_use);

    // TODO: allow manually specifying the channel if the user knows it already.
    DEBUG_PRINT("[btc:rfcomm_connect] Starting SDP query...\n");
    sdp_system_in_use = true;
    sdp_query_pending = true;
    sock->is_using_sdp_system = true;

    // Note: valid server channels from 1-30, so this should never be returned
    // by a real SDP response.
#define SERVER_CHANNEL_UNSET ((uint16_t)-1)

    sdp_query_rfcomm_channel = sock->server_channel = SERVER_CHANNEL_UNSET;
    uint8_t sdp_err = sdp_client_query_rfcomm_channel_and_name_for_service_class_uuid(
        bluetooth_btstack_classic_sdp_packet_handler, (uint8_t *)bdaddr, BLUETOOTH_SERVICE_CLASS_SERIAL_PORT);
    if (sdp_err != 0) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to start SDP query: %d\n", sdp_err);
        // Since we won't get any SDP query events following this, we must
        // manually mark the query as no longer pending.
        sdp_query_pending = false;
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    PBIO_OS_AWAIT_UNTIL(state, !sdp_query_pending);
    sock->server_channel = sdp_query_rfcomm_channel;

    // Allow other SDP queries to go ahead.
    sdp_system_in_use = sock->is_using_sdp_system = false;
    if (sock->server_channel == SERVER_CHANNEL_UNSET) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to find RFCOMM channel for device.\n");
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    DEBUG_PRINT("[btc:rfcomm_connect] Found RFCOMM channel %d for device.\n", sock->server_channel);

    // We establish the channel with no credits. Once we know the negotiated
    // MTU, we can calculate the number of credits we should grant.
    uint8_t rfcomm_err;
    if ((rfcomm_err = rfcomm_create_channel_with_initial_credits(&user_rfcomm_packet_handler, (uint8_t *)bdaddr, sock->server_channel, 0, &sock->cid)) != 0) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to create RFCOMM channel: %d\n", rfcomm_err);
        (void)rfcomm_err;
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    PBIO_OS_AWAIT_UNTIL(state, sock->is_connected);
    if (!sock->is_connected) {
        goto cleanup;
    }

    DEBUG_PRINT("[btc:rfcomm_connect] Connected (cid=%d remote=%s mtu=%d server_chan=%d)\n",
        sock->cid, bd_addr_to_str(bdaddr), sock->mtu, sock->server_channel);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);

cleanup:
    if (sock->is_using_sdp_system) {
        sdp_system_in_use = false;
        sock->is_using_sdp_system = false;
    }
    pbio_error_t err = sock->err;
    pbdrv_bluetooth_rfcomm_socket_reset(sock);
    return err;
}

pbio_error_t pbdrv_bluetooth_rfcomm_listen(pbio_os_state_t *state, int32_t timeout, pbdrv_bluetooth_rfcomm_conn_t *conn) {
    if (*state == 0) {
        conn->conn_id = -1;
    }
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (should_abandon_connection(sock)) {
        goto cleanup;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // The link db is loaded lazily since the storage settings system may not be
    // ready when the bluetooth system is initialized, but it will be ready by
    // the time we try to make any rfcomm connections.
    link_db_settings_load_once();
    gap_discoverable_control(1);

    sock = pbdrv_bluetooth_rfcomm_socket_alloc();
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_listen] No more sockets.\n");
        return PBIO_ERROR_RESOURCE_EXHAUSTED;
    }
    conn->conn_id = pbdrv_bluetooth_rfcomm_socket_id(sock);

    if (timeout > 0) {
        // We use the rx timer to track listen timeouts, since we don't have
        // any other need for it until the connection is established.
        pbio_os_timer_set(&sock->rx_timer, timeout);
    }

    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);
    if (pending_listen_socket) {
        // Unlike with connect, where it's plausible for multiple async contexts
        // to be connecting to different devices, it's always going to be an
        // error to listen more than once at a time.
        DEBUG_PRINT("[btc:rfcomm_listen] Already listening.\n");
        sock->err = PBIO_ERROR_BUSY;
        goto cleanup;
    }

    // Wait until either we time out, there is an error, or the socket is
    // connected.
    pending_listen_socket = sock;
    DEBUG_PRINT("[btc:rfcomm_listen] Listening for incoming RFCOMM connections...\n");
    PBIO_OS_AWAIT_UNTIL(state, sock->is_connected);
    pending_listen_socket = NULL;

    if (sock->err != PBIO_SUCCESS) {
        DEBUG_PRINT("[btc:rfcomm_listen] Other error.\n");
        goto cleanup;
    }

    DEBUG_PRINT("[btc:rfcomm_listen] Connected\n");
    gap_discoverable_control(0);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);

cleanup:;
    gap_discoverable_control(0);
    pbio_error_t err = sock->err;
    pbdrv_bluetooth_rfcomm_socket_reset(sock);
    return err;
}

pbio_error_t pbdrv_bluetooth_rfcomm_close(pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_close] Invalid CID: %d\n", conn->conn_id);
        conn->conn_id = -1;
        return PBIO_ERROR_INVALID_OP;
    }
    rfcomm_disconnect(sock->cid);
    pbdrv_bluetooth_rfcomm_socket_reset(sock);
    conn->conn_id = -1;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_rfcomm_send(const pbdrv_bluetooth_rfcomm_conn_t *conn, const uint8_t *data, size_t length, size_t *bytes_sent) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (!sock || !sock->is_connected) {
        DEBUG_PRINT("[btc:rfcomm_send] Socket is not connected or does not exist.\n");
        return PBIO_ERROR_FAILED;
    }

    bool was_idle = lwrb_get_full(&sock->tx_buffer) == 0;
    *bytes_sent = lwrb_write(&sock->tx_buffer, data, length);
    if (*bytes_sent > 0) {
        DEBUG_PRINT("[btc:rfcomm_send] Queued %d/%d bytes to RFCOMM channel.\n",
            *bytes_sent, (int)length);
    }

    if (was_idle && *bytes_sent > 0) {
        // If we were idle before, we need to request a send event to kick
        // things off.
        rfcomm_request_can_send_now_event(sock->cid);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_rfcomm_recv(const pbdrv_bluetooth_rfcomm_conn_t *conn, uint8_t *buffer, size_t buffer_size, size_t *bytes_received) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);

    if (!sock || !sock->is_connected) {
        DEBUG_PRINT("[btc:rfcomm_recv] Socket is not connected or does not exist.\n");
        return PBIO_ERROR_FAILED;
    }

    *bytes_received = lwrb_read(&sock->rx_buffer, buffer, buffer_size);
    if (*bytes_received > 0) {
        // After reading data, we may have freed up enough space to grant some
        // credits back to our peer.
        DEBUG_PRINT("[btc:rfcomm_recv] Received %d bytes for requested read of "
            "%d bytes, granting credits.\n",
            *bytes_received, buffer_size);
        pbdrv_bluetooth_rfcomm_socket_grant_owed_credits(sock);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_rfcomm_in_waiting(const pbdrv_bluetooth_rfcomm_conn_t *conn, size_t *waiting) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);

    if (!sock || !sock->is_connected) {
        return PBIO_ERROR_FAILED;
    }

    *waiting = lwrb_get_full(&sock->rx_buffer);

    return PBIO_SUCCESS;
}

bool pbdrv_bluetooth_rfcomm_is_writeable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (!sock || !sock->is_connected) {
        return false;
    }
    return lwrb_get_free(&sock->tx_buffer) > 0;
}

bool pbdrv_bluetooth_rfcomm_is_readable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        return false;
    }
    return lwrb_get_full(&sock->rx_buffer) > 0;
}

bool pbdrv_bluetooth_rfcomm_is_connected(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_rfcomm_socket_t *sock = pbdrv_bluetooth_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        return false;
    }
    return sock->is_connected;
}

void pbdrv_bluetooth_rfcomm_cancel_connection(void) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        pbdrv_bluetooth_rfcomm_socket_t *sock = &pbdrv_bluetooth_rfcomm_sockets[i];
        if (sock->is_used) {
            sock->is_cancelled = true;
        }
    }
}

void pbdrv_bluetooth_rfcomm_disconnect_all(void) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        pbdrv_bluetooth_rfcomm_socket_t *sock = &pbdrv_bluetooth_rfcomm_sockets[i];
        if (sock->is_used && sock->is_connected) {
            rfcomm_disconnect(sock->cid);
            pbdrv_bluetooth_rfcomm_socket_reset(sock);
        }
    }
    pending_listen_socket = NULL;
}

#else

pbio_error_t pbdrv_bluetooth_rfcomm_connect(pbio_os_state_t *state, bdaddr_t bdaddr, int32_t timeout, pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)state;
    (void)bdaddr;
    (void)timeout;
    (void)conn;

    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_rfcomm_listen(pbio_os_state_t *state, int32_t timeout, pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)state;
    (void)timeout;
    (void)conn;

    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_rfcomm_close(pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)conn;

    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_rfcomm_send(const pbdrv_bluetooth_rfcomm_conn_t *conn, const uint8_t *data, size_t length, size_t *bytes_sent) {
    (void)conn;
    (void)data;
    (void)length;

    if (bytes_sent) {
        *bytes_sent = 0;
    }

    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_rfcomm_recv(const pbdrv_bluetooth_rfcomm_conn_t *conn, uint8_t *buffer, size_t buffer_size, size_t *bytes_received) {
    (void)conn;
    (void)buffer;
    (void)buffer_size;

    if (bytes_received) {
        *bytes_received = 0;
    }

    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_bluetooth_rfcomm_in_waiting(const pbdrv_bluetooth_rfcomm_conn_t *conn, size_t *waiting) {
    (void)conn;

    if (waiting) {
        *waiting = 0;
    }

    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_bluetooth_rfcomm_is_writeable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)conn;

    return false;
}

bool pbdrv_bluetooth_rfcomm_is_readable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)conn;

    return false;
}

bool pbdrv_bluetooth_rfcomm_is_connected(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    (void)conn;

    return false;
}

void pbdrv_bluetooth_classic_init(void) {
}

void pbdrv_bluetooth_btstack_handle_classic_security_packet(uint8_t *packet, uint16_t size) {
    (void)packet;
    (void)size;
}

void pbdrv_bluetooth_rfcomm_cancel_connection(void) {
}

void pbdrv_bluetooth_rfcomm_disconnect_all(void) {
}

#endif  // MAX_NR_RFCOMM_CHANNELS > 0

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
