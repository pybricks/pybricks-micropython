/** Activity dump provider.
 *
 * This subsystem provides a dumping facility to the NXT baseplate or
 * application kernels. The bytes are stored in memory, and can be sent
 * via USB (or Bluetooth in future versions) to a remote computer for
 * analysis.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/memmap.h"
#include "base/util.h"
#include "base/drivers/usb.h"

/** Dumping area start pointer. */
static U8 *_dump_ptr = NULL;

/** Current dump position. */
static U8 *_dump_cur = NULL;

/** Dump size. */
static U32 _dump_size = 0;

/** Initialize the dump.
 *
 * Sets the start pointer and current pointer to the beginning of the
 * given region (or to the beginning of userspace memory if given NULL).
 */
void nx_dump_init(U8 *ptr) {
  if (ptr == NULL)
    ptr = NX_USERSPACE_START;

  _dump_ptr = ptr;
  _dump_cur = ptr;
  _dump_size = 0;
}

bool nx_dump_data(U8 *data, U32 size) {
  if (_dump_ptr == NULL || _dump_cur == NULL)
    return FALSE;

  if ((_dump_size + size) >= NX_USERSPACE_SIZE)
    return FALSE;

  memcpy(_dump_cur, data, size);

  _dump_cur += size;
  _dump_size += size;
  return TRUE;
}

bool nx_dump_string(const char *str) {
  return nx_dump_data((U8 *)str, strlen(str));
}

bool nx_dump_u8(U8 val) {
  U8 temp = val / 100;
  if (temp > 0) {
    temp += 48;
    if (!nx_dump_data(&temp, 1))
      return FALSE;
  }

  temp = (val - val/100) / 10;
  if (temp > 0) {
    temp += 48;
    if (!nx_dump_data(&temp, 1))
      return FALSE;
  }

  temp = val % 10 + 48;
  return nx_dump_data(&temp, 1);
}

/** Send the dump via USB.
 *
 * First sends the dump size (a 4 bytes U32), then sends the data.
 */
void nx_dump_send_usb(void) {
  nx_usb_write((U8 *)&_dump_size, 4);
  nx_usb_write(_dump_ptr, _dump_size);
}
