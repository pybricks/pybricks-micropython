// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING_HUB_NETWORK

#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include "py/misc.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include <pybricks/messaging/messaging.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#if !MICROPY_PY_STRUCT
#error "messages are packed and unpacked with the struct module"
#endif

#if !PBDRV_CONFIG_BLUETOOTH_PEER
#error "PYBRICKS_PY_MESSAGING_HUB_NETWORK needs PBDRV_CONFIG_BLUETOOTH_PEER"
#endif

/** Number of characters in an address such as "00:16:53:12:34:56". */
#define ADDRESS_STR_SIZE (17)

/** Number of bricks that can be reached, which is the baseband ceiling. */
#define MAX_PEERS (PBDRV_CONFIG_BLUETOOTH_PEER_MAX_PEERS)

/**
 * How many times connect() pages a brick before it gives up. A program that
 * wants to keep waiting can call connect() again.
 */
#define CONNECT_ATTEMPTS (3)

/**
 * Each message is stored in an inbox behind its size and a sequence number.
 * The sequence number is what puts messages from different bricks back into
 * the order they arrived in.
 */
#define MESSAGE_SIZE_OFFSET (0)
#define MESSAGE_SEQUENCE_OFFSET (2)
#define MESSAGE_HEADER_SIZE (6)

/**
 * Smallest buffer that holds one message of any size, both as an inbox and
 * as the relay queue, which has the larger per-message overhead.
 */
#define MIN_INBOX_SIZE (PBDRV_BLUETOOTH_PEER_RELAY_MIN_SIZE)

_Static_assert(MIN_INBOX_SIZE >= MESSAGE_HEADER_SIZE + PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE,
    "an inbox must hold one largest message too");

/** Default inbox size per sending brick. Several messages of any size. */
#define DEFAULT_INBOX_SIZE (1024)

/**
 * Everything a program sends goes out behind one header byte.
 *
 * A zero header means that the rest of the message is the bytes the program
 * gave, with nothing added to them. Otherwise the low bits say how many
 * format characters follow, describing the objects packed in behind them,
 * and the high bit says that the program sent a tuple of them rather than one
 * object on its own.
 *
 * One object always has a format, so a header only reads as zero for bytes,
 * which leaves the empty tuple a header of its own.
 */
#define FORMAT_SIZE_MASK (0x7f)
#define FORMAT_TUPLE_FLAG (0x80)

/**
 * What one object looks like in a format.
 *
 * These are the characters that struct uses, except for the three things it
 * has no type for: text, which goes as the bytes of its UTF-8, and a bool and
 * None, which each go as one byte that struct never sees the meaning of. All
 * three are put back on the other side.
 */
#define FORMAT_INT ('i')
#define FORMAT_FLOAT ('f')
#define FORMAT_BOOL ('?')
#define FORMAT_NONE ('n')
#define FORMAT_BYTES ('s')
#define FORMAT_STR ('u')

/** Number of objects that one message can hold. */
#define MAX_MESSAGE_OBJECTS (32)

/** Longest format of one object, which is a 255 byte str or bytes. */
#define MAX_OBJECT_FORMAT_SIZE (4)

/** Room for the format of a whole message, its byte order and its 0 at the end. */
#define FORMAT_BUFFER_SIZE (MAX_MESSAGE_OBJECTS * MAX_OBJECT_FORMAT_SIZE + 2)

_Static_assert(MAX_MESSAGE_OBJECTS * MAX_OBJECT_FORMAT_SIZE + 1 <= PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE,
    "the format of the most objects must fit in a message, before its payload does not");

/**
 * How long send() keeps trying before giving up on a message.
 *
 * Only reached when the network has stopped taking data. Delivery is best
 * effort, so the message is then dropped rather than stalling the program.
 */
#define SEND_TIMEOUT_MS (1000)

/**
 * Messages from one brick, stored as a ring of size-prefixed payloads.
 *
 * There is one of these per brick we have heard from, rather than one shared
 * ring, so that a brick sending continuously cannot push another brick's
 * messages out before the program has read them.
 */
typedef struct {
    /** Address of the brick whose messages these are. */
    uint8_t address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    /** Whether a brick has claimed this slot. Slots are never given back. */
    bool in_use;
    /** Index of the oldest byte in the ring. */
    uint32_t tail;
    /** Number of bytes in use, including the size prefixes. */
    uint32_t used;
    /** Number of whole messages in the ring. */
    uint32_t count;
} pb_type_hub_network_inbox_t;

typedef struct _pb_type_hub_network_obj_t {
    mp_obj_base_t base;
    /** Size of one inbox ring in bytes, as given by the user. */
    uint32_t inbox_size;
    /** Number of messages stored so far, which stamps each of them. */
    uint32_t sequence;
    /** Awaitable of the ongoing connect() or send(). */
    pb_type_async_t *iter;
    /**
     * Message of an ongoing send, as it goes over the air, and its size.
     *
     * The message is built here rather than being an object the program
     * passed, both because a message is not the same as the objects it holds
     * and because the driver borrows it rather than copying it.
     */
    uint8_t tx_buffer[PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE];
    uint32_t tx_size;
    /** Destination of an ongoing send. */
    uint8_t tx_address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    /** Deadline of an ongoing send. */
    pbio_os_timer_t tx_timer;
    /** State of the driver send, and its result. */
    pbio_os_state_t tx_state;
    pbio_error_t tx_err;
    /** Address being connected to, and how often it has been paged. */
    uint8_t connect_address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    uint32_t connect_attempt;
    /**
     * Queue of messages passed on between other bricks, or NULL. Only a brick
     * that pages others has to relay, so it is allocated on connect().
     */
    uint8_t *relay_buffer;
    /** One inbox per brick we have heard from. */
    pb_type_hub_network_inbox_t inbox[MAX_PEERS];
    /**
     * Holds one message while it is turned back into objects.
     *
     * Allocating runs the Bluetooth driver (see MICROPY_GC_HOOK_LOOP), which
     * can append to an inbox and drop its oldest message, so a message must
     * be lifted out of the ring before anything is allocated for it.
     */
    uint8_t scratch[PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE];
    /** The struct functions that pack and unpack a message. */
    mp_obj_t pack_into;
    mp_obj_t unpack_from;
    /**
     * What struct is handed to pack a message into ::tx_buffer and to unpack
     * one out of ::scratch: a view of the buffer, and the format string.
     *
     * These are Python objects because struct takes nothing else, but they
     * live in this object and only their size changes from one message to the
     * next, so that a program that sends in a loop allocates nothing at all.
     */
    mp_obj_array_t tx_view;
    mp_obj_array_t rx_view;
    mp_obj_str_t tx_format_obj;
    mp_obj_str_t rx_format_obj;
    char tx_format[FORMAT_BUFFER_SIZE];
    char rx_format[FORMAT_BUFFER_SIZE];
    /** Arguments of the pack call: the format, the view, the offset, the objects. */
    mp_obj_t tx_args[MAX_MESSAGE_OBJECTS + 3];
    /** The inbox rings, ::inbox_size bytes each, in ::inbox order. */
    uint8_t buffer[];
} pb_type_hub_network_obj_t;

/**
 * The one radio object, so the driver callback can find it without a context
 * argument. Cleared by the finalizer, which is what stops the driver writing
 * into memory that MicroPython has collected.
 */
static pb_type_hub_network_obj_t *radio_instance;

// --------------------------------------------------------------------------
// Inboxes
// --------------------------------------------------------------------------

static uint8_t *inbox_ring(pb_type_hub_network_obj_t *self, uint32_t index) {
    return &self->buffer[index * self->inbox_size];
}

/**
 * Copies bytes out of an inbox ring, wrapping around at the end.
 *
 * @param [in]  self   The radio object.
 * @param [in]  index  Inbox index.
 * @param [in]  pos    Position in the ring to read from.
 * @param [out] dest   Where to put the bytes.
 * @param [in]  size   Number of bytes to copy.
 * @return             Position just past the bytes that were read.
 */
static uint32_t ring_read(pb_type_hub_network_obj_t *self, uint32_t index, uint32_t pos, uint8_t *dest, uint32_t size) {

    const uint8_t *ring = inbox_ring(self, index);

    for (uint32_t i = 0; i < size; i++) {
        dest[i] = ring[pos];
        pos = (pos + 1) % self->inbox_size;
    }

    return pos;
}

/**
 * Copies bytes into an inbox ring, wrapping around at the end.
 *
 * @param [in]  self   The radio object.
 * @param [in]  index  Inbox index.
 * @param [in]  pos    Position in the ring to write to.
 * @param [in]  src    Bytes to copy.
 * @param [in]  size   Number of bytes to copy.
 * @return             Position just past the bytes that were written.
 */
static uint32_t ring_write(pb_type_hub_network_obj_t *self, uint32_t index, uint32_t pos, const uint8_t *src, uint32_t size) {

    uint8_t *ring = inbox_ring(self, index);

    for (uint32_t i = 0; i < size; i++) {
        ring[pos] = src[i];
        pos = (pos + 1) % self->inbox_size;
    }

    return pos;
}

/**
 * Reads the header of the oldest message in an inbox that is known to hold one.
 */
static void inbox_peek(pb_type_hub_network_obj_t *self, uint32_t index, uint32_t *size, uint32_t *sequence) {

    uint8_t header[MESSAGE_HEADER_SIZE];
    ring_read(self, index, self->inbox[index].tail, header, sizeof(header));

    *size = pbio_get_uint16_le(&header[MESSAGE_SIZE_OFFSET]);
    *sequence = pbio_get_uint32_le(&header[MESSAGE_SEQUENCE_OFFSET]);
}

/**
 * Drops the oldest message in an inbox that is known to hold one.
 */
static void inbox_discard(pb_type_hub_network_obj_t *self, uint32_t index) {
    pb_type_hub_network_inbox_t *inbox = &self->inbox[index];
    uint32_t size, sequence;
    inbox_peek(self, index, &size, &sequence);
    uint32_t total = MESSAGE_HEADER_SIZE + size;
    inbox->tail = (inbox->tail + total) % self->inbox_size;
    inbox->used -= total;
    inbox->count--;
}

/**
 * Copies the oldest message out of an inbox and removes it.
 *
 * @param [in]  self   The radio object.
 * @param [in]  index  Inbox index, which must not be empty.
 * @param [out] dest   Buffer of ::PBDRV_BLUETOOTH_PEER_MAX_MESSAGE_SIZE bytes.
 * @return             Size of the message.
 */
static uint32_t inbox_take(pb_type_hub_network_obj_t *self, uint32_t index, uint8_t *dest) {

    pb_type_hub_network_inbox_t *inbox = &self->inbox[index];
    uint32_t size, sequence;
    inbox_peek(self, index, &size, &sequence);

    ring_read(self, index, (inbox->tail + MESSAGE_HEADER_SIZE) % self->inbox_size, dest, size);

    inbox_discard(self, index);
    return size;
}

/**
 * Adds a message to an inbox, dropping the oldest ones to make room.
 *
 * Dropping the oldest rather than refusing the newest keeps the freshest
 * value available, which is what a program polling slower than a sender
 * sends actually wants.
 */
static void inbox_put(pb_type_hub_network_obj_t *self, uint32_t index, const uint8_t *data, uint32_t size) {

    pb_type_hub_network_inbox_t *inbox = &self->inbox[index];
    uint32_t total = MESSAGE_HEADER_SIZE + size;

    if (total > self->inbox_size) {
        // Cannot happen: the smallest allowed inbox holds one largest message.
        return;
    }

    while (inbox->used + total > self->inbox_size) {
        inbox_discard(self, index);
    }

    uint8_t header[MESSAGE_HEADER_SIZE];
    pbio_set_uint16_le(&header[MESSAGE_SIZE_OFFSET], size);
    pbio_set_uint32_le(&header[MESSAGE_SEQUENCE_OFFSET], self->sequence++);

    uint32_t pos = ring_write(self, index, (inbox->tail + inbox->used) % self->inbox_size, header, sizeof(header));
    ring_write(self, index, pos, data, size);

    inbox->used += total;
    inbox->count++;
}

/**
 * Finds the inbox of a brick, claiming a free slot for a brick that has not
 * been heard from before.
 *
 * @param [in]  self     The radio object.
 * @param [in]  address  Address of the sending brick.
 * @param [out] index    Index of the inbox, if found.
 * @return               False if all slots belong to other bricks.
 */
static bool inbox_lookup(pb_type_hub_network_obj_t *self, const uint8_t *address, uint32_t *index) {

    for (uint32_t i = 0; i < MAX_PEERS; i++) {
        if (self->inbox[i].in_use &&
            memcmp(self->inbox[i].address, address, PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE) == 0) {
            *index = i;
            return true;
        }
    }

    for (uint32_t i = 0; i < MAX_PEERS; i++) {
        if (!self->inbox[i].in_use) {
            memcpy(self->inbox[i].address, address, PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE);
            self->inbox[i].in_use = true;
            *index = i;
            return true;
        }
    }

    return false;
}

/**
 * Handles one message from the Bluetooth driver.
 *
 * This runs in the driver context, so it may not allocate. Everything it
 * needs was allocated when the user created the radio object.
 */
static void handle_peer_message(const uint8_t *src_address, const uint8_t *data, uint32_t size) {

    pb_type_hub_network_obj_t *self = radio_instance;
    uint32_t index;

    if (!self || !inbox_lookup(self, src_address, &index)) {
        return;
    }

    inbox_put(self, index, data, size);
}

// --------------------------------------------------------------------------
// Addresses
// --------------------------------------------------------------------------

static mp_obj_t address_to_obj(const uint8_t *address) {
    static const char digits[] = "0123456789ABCDEF";
    char str[ADDRESS_STR_SIZE];

    for (uint32_t i = 0; i < PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE; i++) {
        str[i * 3] = digits[address[i] >> 4];
        str[i * 3 + 1] = digits[address[i] & 0xf];
        if (i < PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE - 1) {
            str[i * 3 + 2] = ':';
        }
    }

    return mp_obj_new_str(str, sizeof(str));
}

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/**
 * Parses an address such as "00:16:53:12:34:56", in either case.
 *
 * @param [in]  obj      The Python string.
 * @param [out] address  Buffer of ::PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE bytes.
 * @throws ValueError    If it is not a Bluetooth address.
 */
static void address_from_obj(mp_obj_t obj, uint8_t *address) {

    size_t len;
    const char *str = mp_obj_str_get_data(obj, &len);

    if (len != ADDRESS_STR_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("expected an address like '00:16:53:12:34:56'"));
    }

    for (uint32_t i = 0; i < PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE; i++) {
        int high = hex_digit(str[i * 3]);
        int low = hex_digit(str[i * 3 + 1]);
        bool separator = i == PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE - 1 || str[i * 3 + 2] == ':';

        if (high < 0 || low < 0 || !separator) {
            mp_raise_ValueError(MP_ERROR_TEXT("expected an address like '00:16:53:12:34:56'"));
        }

        address[i] = high << 4 | low;
    }
}

// --------------------------------------------------------------------------
// Objects on the wire
// --------------------------------------------------------------------------

/**
 * Writes a size of at most three digits.
 *
 * @param [out] format  Where to write it.
 * @param [in]  value   The size, at most 255.
 * @return              Number of characters written.
 */
static uint32_t format_write_size(uint8_t *format, uint32_t value) {

    uint32_t size = 0;

    if (value >= 100) {
        format[size++] = '0' + value / 100;
    }
    if (value >= 10) {
        format[size++] = '0' + value / 10 % 10;
    }
    format[size++] = '0' + value % 10;

    return size;
}

/**
 * Writes the format of one object that a program is sending.
 *
 * @param [in]     obj      The object.
 * @param [out]    format   Gets at most ::MAX_OBJECT_FORMAT_SIZE characters.
 * @param [in,out] payload  Grows by the number of bytes the object packs into.
 * @return                  Number of characters written.
 * @throws TypeError        If it is not an object that can be sent.
 */
static uint32_t format_object(mp_obj_t obj, uint8_t *format, uint32_t *payload) {

    // A byte that says nothing, since a program that leaves one thing out of
    // a message should not have to leave out the message.
    if (obj == mp_const_none) {
        *payload += 1;
        format[0] = FORMAT_NONE;
        return 1;
    }

    // Before the int below, since a bool is not an int here but does pass for
    // one, and it is worth keeping it a bool all the way across.
    if (mp_obj_is_bool(obj)) {
        *payload += 1;
        format[0] = FORMAT_BOOL;
        return 1;
    }

    if (mp_obj_is_int(obj)) {
        // An int is 32 bits on a hub, so this is only about a computer running
        // the same program, where one can be bigger than what is sent.
        mp_int_t value = mp_obj_get_int(obj);
        if ((int32_t)value != value) {
            mp_raise_msg(&mp_type_OverflowError, MP_ERROR_TEXT("int is too big to send"));
        }
        *payload += 4;
        format[0] = FORMAT_INT;
        return 1;
    }

    #if MICROPY_PY_BUILTINS_FLOAT
    if (mp_obj_is_float(obj)) {
        *payload += 4;
        format[0] = FORMAT_FLOAT;
        return 1;
    }
    #endif

    // Anything else that has bytes goes as those bytes. A str is one of them,
    // and the only difference is that it is put back together as a str.
    mp_buffer_info_t info;
    if (mp_get_buffer(obj, &info, MP_BUFFER_READ)) {
        if (info.len > UINT8_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("message is too big to send"));
        }
        uint32_t size = format_write_size(format, info.len);
        format[size++] = mp_obj_is_str(obj) ? FORMAT_STR : FORMAT_BYTES;
        *payload += info.len;
        return size;
    }

    mp_raise_TypeError(MP_ERROR_TEXT("cannot send this object"));
}

/**
 * Turns the format of a message into the format that struct understands.
 *
 * This is also where a message is checked, so that everything after it can be
 * left to struct.
 *
 * @param [in]  format  Format characters from the message.
 * @param [in]  size    Number of format characters.
 * @param [out] dest    Gets @p size + 2 characters, including the 0 at the end.
 * @throws ValueError   If it is not a format that a brick sends.
 */
static void format_to_struct(const uint8_t *format, uint32_t size, char *dest) {

    bool counted = false;

    // Everything goes in the same byte order and never with padding, which is
    // what makes each character after this one exactly one object.
    dest[0] = '<';

    for (uint32_t i = 0; i < size; i++) {

        char c = format[i];
        bool digit = c >= '0' && c <= '9';

        switch (c) {
            case FORMAT_STR:
                // struct has no text type, so it gets the bytes of the text.
                c = FORMAT_BYTES;
                break;
            case FORMAT_BYTES:
                break;
            case FORMAT_BOOL:
            case FORMAT_NONE:
                // struct has no type for either, so it gets the byte and this
                // is the only place that knows what it stands for.
                c = 'B';
                MP_FALLTHROUGH
            case FORMAT_INT:
            case FORMAT_FLOAT:
                // A size in front of one of these would make it stand for
                // several objects, which is not something a brick sends.
                if (counted) {
                    mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
                }
                break;
            default:
                if (!digit) {
                    mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
                }
                break;
        }

        counted = digit;
        dest[1 + i] = c;
    }

    // A size with nothing behind it.
    if (counted) {
        mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
    }

    dest[1 + size] = '\0';
}

/**
 * Builds the message for what a program passed to send() in ::tx_buffer.
 *
 * @param [in] self     The radio object.
 * @param [in] data_in  What the program is sending.
 * @throws TypeError    If it holds an object that cannot be sent.
 * @throws ValueError   If it does not fit in one message.
 */
static void message_encode(pb_type_hub_network_obj_t *self, mp_obj_t data_in) {

    // Not a message until it is whole, which is what a send that is still
    // being taken down goes by if this one does not get that far.
    self->tx_size = 0;

    // Something bytes-like goes out as it is, behind a zero header, so that a
    // program that builds its own messages pays nothing for any of this. A
    // str is bytes-like too, but it is sent as an object so that the program
    // on the other side gets a str back instead of bytes.
    mp_buffer_info_t info;
    if (!mp_obj_is_str(data_in) && mp_get_buffer(data_in, &info, MP_BUFFER_READ)) {
        if (info.len + 1 > sizeof(self->tx_buffer)) {
            mp_raise_ValueError(MP_ERROR_TEXT("message is too big to send"));
        }
        self->tx_buffer[0] = 0;
        memcpy(&self->tx_buffer[1], info.buf, info.len);
        self->tx_size = info.len + 1;
        return;
    }

    // One object is sent as itself rather than as a tuple holding it, so that
    // a program gets back what it sent either way.
    size_t n_objects = 1;
    mp_obj_t *objects = &data_in;
    bool single = !mp_obj_is_type(data_in, &mp_type_tuple);

    if (!single) {
        mp_obj_tuple_get(data_in, &n_objects, &objects);
    }

    if (n_objects > MAX_MESSAGE_OBJECTS) {
        mp_raise_ValueError(MP_ERROR_TEXT("message has too many objects"));
    }

    uint32_t format_size = 0;
    uint32_t payload_size = 0;

    for (size_t i = 0; i < n_objects; i++) {
        format_size += format_object(objects[i], &self->tx_buffer[1 + format_size], &payload_size);
        // None is the one object that struct cannot pack, so it packs the
        // zero that stands for it instead.
        self->tx_args[3 + i] = objects[i] == mp_const_none ? MP_OBJ_NEW_SMALL_INT(0) : objects[i];
    }

    if (format_size > FORMAT_SIZE_MASK || 1 + format_size + payload_size > sizeof(self->tx_buffer)) {
        mp_raise_ValueError(MP_ERROR_TEXT("message is too big to send"));
    }

    self->tx_buffer[0] = format_size | (single ? 0 : FORMAT_TUPLE_FLAG);
    self->tx_size = 1 + format_size + payload_size;

    format_to_struct(&self->tx_buffer[1], format_size, self->tx_format);
    self->tx_format_obj.len = 1 + format_size;

    // Packs straight into the message, behind the format it just wrote.
    self->tx_args[0] = MP_OBJ_FROM_PTR(&self->tx_format_obj);
    self->tx_args[1] = MP_OBJ_FROM_PTR(&self->tx_view);
    self->tx_args[2] = MP_OBJ_NEW_SMALL_INT(1 + format_size);
    mp_call_function_n_kw(self->pack_into, 3 + n_objects, 0, self->tx_args);

    // Let go of the objects, which the message no longer needs.
    for (size_t i = 0; i < n_objects; i++) {
        self->tx_args[3 + i] = MP_OBJ_NULL;
    }
}

/**
 * Turns one received message back into what the program that sent it passed
 * to send().
 *
 * @param [in] self   The radio object, with the message in ::scratch.
 * @param [in] size   Size of the message.
 * @return            The objects, or bytes for a message that is bytes.
 * @throws ValueError If the message does not match its own format.
 */
static mp_obj_t message_decode(pb_type_hub_network_obj_t *self, uint32_t size) {

    if (size < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
    }

    uint32_t header = self->scratch[0];
    uint32_t format_size = header & FORMAT_SIZE_MASK;

    if (!header) {
        return mp_obj_new_bytes(&self->scratch[1], size - 1);
    }

    if (1 + format_size > size) {
        mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
    }

    format_to_struct(&self->scratch[1], format_size, self->rx_format);
    self->rx_format_obj.len = 1 + format_size;

    // Only as far as the message goes, so that struct is the one that says a
    // message and its format do not match, rather than it quietly reading
    // whatever the message before it left behind.
    self->rx_view.len = size;

    mp_obj_t args[] = {
        MP_OBJ_FROM_PTR(&self->rx_format_obj),
        MP_OBJ_FROM_PTR(&self->rx_view),
        MP_OBJ_NEW_SMALL_INT(1 + format_size),
    };
    mp_obj_t result = mp_call_function_n_kw(self->unpack_from, MP_ARRAY_SIZE(args), 0, args);

    size_t n_objects;
    mp_obj_t *objects;
    mp_obj_tuple_get(result, &n_objects, &objects);

    // Put back the three things that struct has no type for. Each character
    // that is not a size is one object, which is what format_to_struct
    // already made sure of.
    for (uint32_t i = 0, object = 0; i < format_size && object < n_objects; i++) {
        uint8_t c = self->scratch[1 + i];
        if (c == FORMAT_STR) {
            size_t len;
            const char *str = mp_obj_str_get_data(objects[object], &len);
            objects[object] = mp_obj_new_str(str, len);
        } else if (c == FORMAT_BOOL) {
            objects[object] = mp_obj_new_bool(mp_obj_get_int(objects[object]));
        } else if (c == FORMAT_NONE) {
            objects[object] = mp_const_none;
        }
        object += !(c >= '0' && c <= '9');
    }

    if (header & FORMAT_TUPLE_FLAG) {
        return result;
    }

    if (n_objects != 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("received a bad message"));
    }

    return objects[0];
}

// --------------------------------------------------------------------------
// Methods
// --------------------------------------------------------------------------

static mp_obj_t pb_type_hub_network_address(mp_obj_t self_in) {
    uint8_t address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    pbdrv_bluetooth_peer_get_local_address(address);
    return address_to_obj(address);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_hub_network_address_obj, pb_type_hub_network_address);

/**
 * Pages one brick, retrying a few times before giving up.
 *
 * No local variables survive a yield in a protothread, so everything this
 * needs is kept in the object.
 */
static pbio_error_t pb_type_hub_network_connect_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pb_type_hub_network_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

    for (self->connect_attempt = 0; self->connect_attempt < CONNECT_ATTEMPTS; self->connect_attempt++) {

        // Returns straight away for a brick that is already connected, which
        // is what makes a second connect() cheap.
        if (pbdrv_bluetooth_peer_connect(self->connect_address) != PBIO_SUCCESS) {
            // Out of slots or Bluetooth is down, so retrying is pointless.
            break;
        }

        PBIO_OS_AWAIT_UNTIL(state,
            pbdrv_bluetooth_peer_connect_status(self->connect_address) != PBIO_ERROR_AGAIN);

        if (pbdrv_bluetooth_peer_is_connected(self->connect_address)) {
            return PBIO_SUCCESS;
        }
    }

    // Raised rather than returned as an error code, since the ones that come
    // closest are about motors and sensors not being plugged in.
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("could not connect to brick"));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static mp_obj_t pb_type_hub_network_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_hub_network_obj_t, self,
        PB_ARG_REQUIRED(address));

    uint8_t local_address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    pbdrv_bluetooth_peer_get_local_address(local_address);

    address_from_obj(address_in, self->connect_address);

    if (memcmp(self->connect_address, local_address, sizeof(local_address)) == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot connect to this brick itself"));
    }

    // Only a brick that pages others has to pass messages between them, so
    // this is not allocated until it does. It then stays for the life of the
    // radio, since the network outlives any one connect().
    if (!self->relay_buffer) {
        self->relay_buffer = m_new(uint8_t, self->inbox_size);
        pbdrv_bluetooth_peer_set_relay_buffer(self->relay_buffer, self->inbox_size);
    }

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .iter_once = pb_type_hub_network_connect_iterate_once,
    };

    return pb_type_async_wait_or_await(&config, &self->iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_hub_network_connect_obj, 1, pb_type_hub_network_connect);

static mp_obj_t pb_type_hub_network_is_connected(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD_SKIP_SELF(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(address));

    if (address_in == mp_const_none) {
        return mp_obj_new_bool(pbdrv_bluetooth_peer_get_count() > 0);
    }

    uint8_t address[PBDRV_BLUETOOTH_PEER_ADDRESS_SIZE];
    address_from_obj(address_in, address);
    return mp_obj_new_bool(pbdrv_bluetooth_peer_is_connected(address));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_hub_network_is_connected_obj, 1, pb_type_hub_network_is_connected);

/**
 * One turn of the driver send, mapping the outcomes a program cannot act on
 * to success.
 *
 * @return  ::PBIO_ERROR_AGAIN while sending, else the result.
 */
static pbio_error_t pb_type_hub_network_send_now(pb_type_hub_network_obj_t *self) {

    // There is nothing to send when building the message did not finish, so
    // that what is left in the buffer is never sent as if it were one.
    if (!self->tx_size) {
        return PBIO_SUCCESS;
    }

    pbio_error_t err = pbdrv_bluetooth_peer_send(&self->tx_state, self->tx_address, self->tx_buffer, self->tx_size);

    // Sending to a brick that is not on the network does nothing. A message
    // to a brick that *is* connected can vanish just as silently, so raising
    // in only one of the two cases would be a distinction the program cannot
    // act on, and it would take down a network that is otherwise working.
    if (err == PBIO_ERROR_NO_DEV || err == PBIO_ERROR_INVALID_OP) {
        return PBIO_SUCCESS;
    }

    return err;
}

/**
 * Stops the driver reading the message buffer, so that the next send may
 * build its message there. Used when a send ends, however it ends.
 */
static mp_obj_t pb_type_hub_network_send_release(mp_obj_t parent_obj) {
    (void)parent_obj;
    pbdrv_bluetooth_peer_send_cancel();
    return mp_const_none;
}

static pbio_error_t pb_type_hub_network_send_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pb_type_hub_network_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

    // Always yield once. A loop that does nothing but send is the most
    // obvious program to write, and this is what keeps it from starving the
    // rest of the system.
    PBIO_OS_AWAIT_ONCE(state);

    self->tx_state = 0;
    pbio_os_timer_set(&self->tx_timer, SEND_TIMEOUT_MS);

    // Completes once the driver has handed the message to every destination,
    // which is what makes a program that only sends wait its turn behind the
    // traffic this brick is passing on for others.
    PBIO_OS_AWAIT_UNTIL(state,
        (self->tx_err = pb_type_hub_network_send_now(self)) != PBIO_ERROR_AGAIN ||
        pbio_os_timer_is_expired(&self->tx_timer));

    pb_type_hub_network_send_release(parent_obj);

    if (self->tx_err == PBIO_ERROR_AGAIN) {
        // Out of time, so the message is dropped like any other that does not
        // make it across.
        return PBIO_SUCCESS;
    }

    return self->tx_err;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static mp_obj_t pb_type_hub_network_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_hub_network_obj_t, self,
        PB_ARG_REQUIRED(data),
        PB_ARG_DEFAULT_NONE(address));

    if (address_in == mp_const_none) {
        memcpy(self->tx_address, pbdrv_bluetooth_peer_address_all, sizeof(self->tx_address));
    } else {
        address_from_obj(address_in, self->tx_address);
    }

    // A send that is being replaced has to let go of the driver first, or
    // this one would find it busy, and it is still reading the buffer that
    // the message below is built in.
    pbdrv_bluetooth_peer_send_cancel();

    message_encode(self, data_in);

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .iter_once = pb_type_hub_network_send_iterate_once,
        .close = pb_type_hub_network_send_release,
    };

    return pb_type_async_wait_or_await(&config, &self->iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_hub_network_send_obj, 1, pb_type_hub_network_send);

static mp_obj_t pb_type_hub_network_inbox(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_hub_network_obj_t, self,
        PB_ARG_DEFAULT_FALSE(latest));

    bool latest = mp_obj_is_true(latest_in);

    // Bounded by what is here now. Allocating below runs the Bluetooth
    // driver, which can add more, and those belong to the next read.
    uint32_t remaining[MAX_PEERS];

    for (uint32_t i = 0; i < MAX_PEERS; i++) {

        pb_type_hub_network_inbox_t *inbox = &self->inbox[i];

        if (latest) {
            while (inbox->count > 1) {
                inbox_discard(self, i);
            }
        }

        remaining[i] = inbox->count;
    }

    mp_obj_t messages = mp_obj_new_list(0, NULL);

    while (true) {

        // Oldest message across all the inboxes. Per-brick inboxes keep a busy
        // sender from pushing out a quiet one, but a program that acts on the
        // last message it gets still needs them in the order they arrived
        // rather than grouped by sender.
        uint32_t oldest = MAX_PEERS;
        uint32_t oldest_sequence = 0;

        for (uint32_t i = 0; i < MAX_PEERS; i++) {
            if (!remaining[i] || !self->inbox[i].count) {
                continue;
            }
            uint32_t size, sequence;
            inbox_peek(self, i, &size, &sequence);
            if (oldest == MAX_PEERS || (int32_t)(sequence - oldest_sequence) < 0) {
                oldest = i;
                oldest_sequence = sequence;
            }
        }

        if (oldest == MAX_PEERS) {
            break;
        }

        remaining[oldest]--;

        // Out of the ring before anything is allocated: from here on the
        // driver may evict messages, but not this one.
        uint32_t size = inbox_take(self, oldest, self->scratch);

        mp_obj_t item[] = {
            address_to_obj(self->inbox[oldest].address),
            message_decode(self, size),
        };
        mp_obj_list_append(messages, mp_obj_new_tuple(MP_ARRAY_SIZE(item), item));
    }

    // A tuple, so it is obvious that it is a snapshot and not a live view.
    size_t len;
    mp_obj_t *items;
    mp_obj_list_get(messages, &len, &items);
    return mp_obj_new_tuple(len, items);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_hub_network_inbox_obj, 1, pb_type_hub_network_inbox);

// --------------------------------------------------------------------------
// Lifetime
// --------------------------------------------------------------------------

static mp_obj_t pb_type_hub_network_close(mp_obj_t self_in) {
    // Stops the driver using memory that is about to be collected.
    pbdrv_bluetooth_peer_set_receive_callback(NULL);
    pbdrv_bluetooth_peer_set_relay_buffer(NULL, 0);
    pbdrv_bluetooth_peer_send_cancel();
    radio_instance = NULL;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_hub_network_close_obj, pb_type_hub_network_close);

static mp_obj_t pb_type_hub_network_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_INT(inbox_size, DEFAULT_INBOX_SIZE));

    // Waiting for the radio to come up is not something that can be done
    // halfway, and the inboxes must exist before the driver is pointed at
    // them, so this is all before any multitasking starts.
    pb_module_tools_assert_blocking();

    mp_int_t inbox_size = pb_obj_get_positive_int(inbox_size_in);
    if (inbox_size < MIN_INBOX_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("inbox_size is too small for one message"));
    }

    if (radio_instance) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("radio already exists"));
    }

    // Bluetooth comes up well after boot, so a program that creates the radio
    // at its very first instruction has to wait for it. Without this the
    // address would not be known yet and no brick could be reached.
    while (!pbdrv_bluetooth_peer_is_ready()) {
        if (!pbdrv_bluetooth_hci_is_enabled()) {
            pb_assert(PBIO_ERROR_NO_DEV);
        }
        mp_event_wait_indefinite();
    }

    // Use a finalizer so the driver callback is dropped if this is collected.
    pb_type_hub_network_obj_t *self = mp_obj_malloc_var_with_finaliser(
        pb_type_hub_network_obj_t, buffer, uint8_t, MAX_PEERS * inbox_size, type);

    self->inbox_size = inbox_size;
    self->sequence = 0;
    self->iter = NULL;
    self->tx_size = 0;
    self->relay_buffer = NULL;
    memset(self->inbox, 0, sizeof(self->inbox));

    // Looked up once, since every message goes through them.
    self->pack_into = pb_function_import_helper(MP_QSTR_struct, MP_QSTR_pack_into);
    self->unpack_from = pb_function_import_helper(MP_QSTR_struct, MP_QSTR_unpack_from);

    // What struct packs into and unpacks out of. Only the size of each of
    // these changes from one message to the next.
    mp_obj_memoryview_init(&self->tx_view, 'B' | MP_OBJ_ARRAY_TYPECODE_FLAG_RW, 0,
        sizeof(self->tx_buffer), self->tx_buffer);
    mp_obj_memoryview_init(&self->rx_view, 'B', 0, sizeof(self->scratch), self->scratch);

    self->tx_format_obj.base.type = &mp_type_str;
    self->tx_format_obj.hash = 0;
    self->tx_format_obj.data = (const byte *)self->tx_format;
    self->rx_format_obj.base.type = &mp_type_str;
    self->rx_format_obj.hash = 0;
    self->rx_format_obj.data = (const byte *)self->rx_format;

    // Only now can messages be stored, which is also what makes sure a
    // program never sees traffic from before it started.
    radio_instance = self;
    pbdrv_bluetooth_peer_set_receive_callback(handle_peer_message);

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t pb_type_hub_network_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),       MP_ROM_PTR(&pb_type_hub_network_close_obj)          },
    { MP_ROM_QSTR(MP_QSTR_address),       MP_ROM_PTR(&pb_type_hub_network_address_obj)        },
    { MP_ROM_QSTR(MP_QSTR_connect),       MP_ROM_PTR(&pb_type_hub_network_connect_obj)        },
    { MP_ROM_QSTR(MP_QSTR_is_connected),  MP_ROM_PTR(&pb_type_hub_network_is_connected_obj)   },
    { MP_ROM_QSTR(MP_QSTR_inbox), MP_ROM_PTR(&pb_type_hub_network_inbox_obj)  },
    { MP_ROM_QSTR(MP_QSTR_send),          MP_ROM_PTR(&pb_type_hub_network_send_obj)           },
};
static MP_DEFINE_CONST_DICT(pb_type_hub_network_locals_dict, pb_type_hub_network_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_hub_network,
    MP_QSTR_HubNetwork,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_hub_network_make_new,
    locals_dict, &pb_type_hub_network_locals_dict);

#endif // PYBRICKS_PY_MESSAGING_HUB_NETWORK
