/* Driver for the NXT's I2C-based sensors.
 *
 * SoftMAC I2C driver.
 */

#include "base/at91sam7s256.h"

#define I2C_LOG FALSE

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/systick.h"
#include "base/util.h"
#include "base/drivers/aic.h"
#include "base/drivers/sensors.h"
#include "base/display.h"
#include "base/drivers/i2c.h"

/* The base clock frequency of the sensor I2C bus in Hz. */
#define I2C_BUS_SPEED 9600

/* A classic I2C exchange includes up to 4 partial transactions. */
#define I2C_MAX_TXN 4

/* Length of the I2C pauses (in 4th of BUS_SPEED) */
#define I2C_PAUSE_LEN 3


struct i2c_txn_info
{
  /* Bus control triggers, tells the state machine to issue start,
   * stop or restart bits at the begniing or at the end of the
   * transaction.
   */
  i2c_control pre_control, post_control;

  /* Sub transaction mode, tells whether data will be writen or
   * read to/from the bus.
   */
  i2c_txn_mode mode;

  /* Data to be sent or to buffer for reception. */
  U8 *data;
  U8 data_size;
  
  /* Sub transaction result. */
  i2c_txn_status result;
};

static volatile struct i2c_port {
  enum {
    I2C_OFF = 0, /* Port not initialized in I2C mode. */
    I2C_IDLE,    /* No transaction in progress. */
    I2C_CONFIG,  /* Nothing is happening, but the bus is locked by
                  * transaction configuration. */
    I2C_PAUSE,
    I2C_RECLOCK0,
    I2C_RECLOCK1,
    I2C_READ_ACK0,
    I2C_READ_ACK1,
    I2C_READ_ACK2,
    I2C_WRITE_ACK0,
    I2C_WRITE_ACK1,
    I2C_WRITE_ACK2,
    I2C_SEND_START_BIT0,
    I2C_SEND_START_BIT1,
    I2C_SCL_LOW,
    I2C_SAMPLE0,
    I2C_SAMPLE1,
    I2C_SAMPLE2,
    I2C_SEND_STOP_BIT0,
    I2C_SEND_STOP_BIT1,
  } bus_state;


  U8 device_addr;           /* The connected device address on the bus... */
  U8 addr[TXN_MODE_COUNT];  /* ... and the read/write ORed addresses. */
  
  /* Toggles the use of degraded I2C mode for LEGO Radar
   * compatibility (use STOP/START instead of RESTART, etc).
   */
  bool lego_compat; 

  enum {
    TXN_NONE = 0,
    TXN_WAITING,
    TXN_START,
    TXN_TRANSMIT_BYTE,
    TXN_WRITE_ACK,
    TXN_READ_ACK,
    TXN_STOP,
  } txn_state;

  /** I2C transactions and current transaction number. */
  struct i2c_txn_info txns[I2C_MAX_TXN];
  S8 current_txn;
  S8 n_txns;

  /* Data flow tracking values : currently processed bytes, the
   * currently transmitted byte, and the position of the bit currently
   * transmitted in this byte.
   */
  U8 processed;
  U8 current_byte;
  S8 current_pos;

  /* Pause mechanism for non-I2C compliant sensors (like the LEGO
   * Ultrasonic radar): number of interrupts to let pass, and bus
   * state to reach after the pause.
   */
  S8 p_ticks;
  U8 p_next;

} i2c_state[NXT_N_SENSORS] = {
  { I2C_OFF, 0, { 0 }, FALSE, TXN_NONE, {{ 0 }}, 0, 0, 0, 0, 0, 0, I2C_IDLE },
  { I2C_OFF, 0, { 0 }, FALSE, TXN_NONE, {{ 0 }}, 0, 0, 0, 0, 0, 0, I2C_IDLE },
  { I2C_OFF, 0, { 0 }, FALSE, TXN_NONE, {{ 0 }}, 0, 0, 0, 0, 0, 0, I2C_IDLE },
  { I2C_OFF, 0, { 0 }, FALSE, TXN_NONE, {{ 0 }}, 0, 0, 0, 0, 0, 0, I2C_IDLE },
};

/* Forward declarations. */
static void i2c_isr();
static void i2c_log(const char *s);
static void i2c_log_uint(U32 val);

/** Initializes the I2C SoftMAC driver, configures the TC (Timer Counter)
 * and set the interrupt handler.
 */
void nx_i2c_init() {
  nx_interrupts_disable();

  /* We need power for both the PIO controller and the first TC (Timer
   * Channel) controller.
   */
  *AT91C_PMC_PCER = (1 << AT91C_ID_PIOA) | (1 << AT91C_ID_TC0);

  /* Configure a timer to trigger interrupts for managing the I2C
   * ports.
   *
   * Disable the counter before reconfiguring it, and mask all
   * interrupt sources. Then wait for the clock to acknowledge the
   * shutdown in its status register. Reading the SR has the
   * side-effect of clearing any pending state in there.
   */
  *AT91C_TC0_CCR = AT91C_TC_CLKDIS;
  *AT91C_TC0_IDR = ~0;
  while (*AT91C_TC0_SR & AT91C_TC_CLKSTA);

  /* Configure the timer to count at a rate of MCLK/2 (24MHz), and to
   * reset on RC compare. This means the clock will be repeatedly
   * counting at 24MHz from 0 to the value in the RC register.
   */
  *AT91C_TC0_CMR = AT91C_TC_CPCTRG;
  *AT91C_TC0_RC = (NXT_CLOCK_FREQ/2)/(4*I2C_BUS_SPEED);

  /* Enable the timer. */
  *AT91C_TC0_CCR = AT91C_TC_CLKEN;

  /* Allow the timer to trigger interrupts and register our interrupt
   * handler.
   */
  *AT91C_TC0_IER = AT91C_TC_CPCS;
  nx_aic_install_isr(AT91C_ID_TC0, AIC_PRIO_SOFTMAC, AIC_TRIG_EDGE, i2c_isr);

  /* Softare trigger, to get the counter going. */
  *AT91C_TC0_CCR = AT91C_TC_SWTRG;

  nx_interrupts_enable();
}

static void i2c_log(const char *s)
{
  if (I2C_LOG)
    nx_display_string(s);
}

static void i2c_log_uint(U32 val)
{
  if (I2C_LOG)
    nx_display_uint(val);
}

/** Register a remote device (by its address) on the given sensor. */
void nx_i2c_register(U8 sensor, U8 address, bool lego_compat) {
  volatile struct i2c_port *p;

  if (sensor >= NXT_N_SENSORS || address <= 0)
    return;

  /* First, make sure the sensor port is configured for multi-driver
   * I2C devices.
   */
  nx_sensors_i2c_enable(sensor);

  p = &i2c_state[sensor];

  p->bus_state = I2C_IDLE;
  p->txn_state = TXN_NONE;
  p->device_addr = address;
  p->lego_compat = lego_compat;
  
  p->addr[TXN_MODE_WRITE] = (address << 1) | TXN_MODE_WRITE;
  p->addr[TXN_MODE_READ] = (address << 1) | TXN_MODE_READ;

  /* Reset the bus transaction structures and parameters. */
  //memset((U8 *)i2c_state[sensor].txns, 0,
  //  I2C_MAX_TXN*sizeof(struct i2c_txn_info));
  i2c_state[sensor].current_txn = 0;
  i2c_state[sensor].n_txns = 0;
}

/** Add a I2C sub transaction.
 *
 * Adds a sub transaction for the given sensor. The given data (along with its
 * size) will be writen or read from the bus. Additionnal pre control and
 * post control can be performed (see enum i2c_control).
 */
static i2c_txn_err i2c_add_txn(U8 sensor, i2c_txn_mode mode,
			       U8 *data, int size,
			       i2c_control pre_control, i2c_control post_control)
{
  volatile struct i2c_txn_info *t;
  
  if (sensor >= NXT_N_SENSORS)
    return I2C_ERR_UNKNOWN_SENSOR;
  
  if (i2c_state[sensor].n_txns == I2C_MAX_TXN)
    return I2C_ERR_TXN_FULL;

  /* Retrieve the sub transaction i2c_txn_info structure. */
  t = &(i2c_state[sensor].txns[i2c_state[sensor].n_txns]);

  /* Set the transaction parameters. */
  t->pre_control = pre_control;
  t->post_control = post_control;
  t->mode = mode;
  t->data = data;
  t->data_size = size;
  
  i2c_state[sensor].n_txns++;
  return I2C_ERR_OK;
}

/** Triggers the configured I2C transactions.
 *
 * Returns I2C_ERR_NOT_READY if the bus is busy.
 */
static i2c_txn_err i2c_trigger(U8 sensor) {
  i2c_state[sensor].txn_state = TXN_WAITING;
  i2c_state[sensor].bus_state = I2C_IDLE;
  
  return I2C_ERR_OK;
}

/** Start a new I2C transaction.
 *
 * If the I2C bus is available, a new transaction (consisting in 2 to 4
 * sub transactions) will be performed.
 *
 * For a write transaction, two sub transactions will be performed. The data
 * and data_size parameters must be provided, initialized and containing the
 * data to be sent on the bus (in most cases, the internal address in the
 * remote device and the value to put in this address).
 *
 * For a read transaction, four sub transactions will be performed. In
 * addition to the data and data_size parameters, the recv_buf and recv_size
 * parameters must be initialized. The recv_buf must be able to contain the
 * recv_size bytes that will be read from the bus.
 *
 * Returns an i2c_txn_err error code.
 */
i2c_txn_err nx_i2c_start_transaction(U8 sensor, i2c_txn_mode mode,
				     U8 *data, U8 data_size,
				     U8 *recv_buf, U8 recv_size)
{
  volatile struct i2c_txn_info *t;
  
  if (sensor >= NXT_N_SENSORS)
    return I2C_ERR_UNKNOWN_SENSOR;

  if (nx_i2c_busy(sensor))
    return I2C_ERR_NOT_READY;

  /* In any case, data must be initialized, and with a known data size. */
  if (!data || !data_size)
    return I2C_ERR_DATA;
  
  if (mode == TXN_MODE_READ && (!recv_buf || !recv_size))
    return I2C_ERR_DATA;

  i2c_state[sensor].bus_state = I2C_CONFIG;

  t = i2c_state[sensor].txns;
  i2c_state[sensor].current_txn = 0;
  i2c_state[sensor].n_txns = 0;
  // memset((U8 *)t, 0, I2C_MAX_TXN*sizeof(struct i2c_txn_info));

  /* In any case, write the device address and the given data
   * (can be an internal address, or an internal address followed
   * by a value, etc). This second sub transaction is ended by a
   * STOP if the device works in degraded mode or if this transaction
   * is only a write transaction.
   */
  i2c_add_txn(sensor, TXN_MODE_WRITE,
              &(i2c_state[sensor].addr[TXN_MODE_WRITE]), 1,
              I2C_CONTROL_START, I2C_CONTROL_NONE);
  i2c_add_txn(sensor, TXN_MODE_WRITE, data, data_size, I2C_CONTROL_NONE,
              i2c_state[sensor].lego_compat || mode == TXN_MODE_WRITE
                ? I2C_CONTROL_STOP
                : I2C_CONTROL_NONE);

  /* If this is a read transaction, write the device address on the bus
   * and switch to read mode, storing the received bytes in the provided
   * buffer ; ending with a STOP.
   */
  if (mode == TXN_MODE_READ) {
    i2c_add_txn(sensor, TXN_MODE_WRITE,
                &(i2c_state[sensor].addr[TXN_MODE_READ]), 1,
                I2C_CONTROL_RESTART, I2C_CONTROL_NONE);
    i2c_add_txn(sensor, TXN_MODE_READ, recv_buf, recv_size, I2C_CONTROL_NONE,
                I2C_CONTROL_STOP);
  }

  i2c_trigger(sensor);
  return I2C_ERR_OK;
}

/** Retrieve the transaction status for the given sensor.
 */
i2c_txn_status nx_i2c_get_txn_status(U8 sensor)
{
  volatile struct i2c_port *p;

  if (sensor >= NXT_N_SENSORS)
    return TXN_STAT_UNKNOWN;

  p = &i2c_state[sensor];

  /* If the current sub transaction was left in the FAILED state,
   * the whole transaction is failed.
   */
  if (p->txns[p->current_txn].result == TXN_STAT_FAILED)
    return TXN_STAT_FAILED;

  /* If the transaction is not failed, it's in progress until the
   * current_txn number reaches the number of sub transactions (minus 1
   * because indexes start at 0).
   */
  if (p->current_txn == p->n_txns - 1)
    return TXN_STAT_IN_PROGRESS;

  /* When everything's done, simply return the last sub transaction
   * result.
   */
  return p->txns[p->current_txn].result;
}

bool nx_i2c_busy(U8 sensor)
{
  if (sensor >= NXT_N_SENSORS)
    return FALSE;

  return i2c_state[sensor].bus_state > I2C_IDLE
    || i2c_state[sensor].current_txn < i2c_state[sensor].n_txns;
}

/** Sets the I2C bus state for the given sensor to the provided state.
 *
 * This function takes into account the lego_compat parameter of the given
 * I2C port and will include a pause before going to 'next_state' if the
 * port is configured for lego I2C radar compatibility.
 *
 * Note: only use this function to change the I2C bus state when a pause
 * is required in compat mode.
 */
static void i2c_set_bus_state(U8 sensor, U8 next_state) {
  volatile struct i2c_port *p;

  if (sensor >= NXT_N_SENSORS)
    return;

  p = &i2c_state[sensor];

  if (p->lego_compat) {
    p->bus_state = I2C_PAUSE;
    p->p_ticks = next_state == I2C_IDLE
      ? 10 * I2C_PAUSE_LEN
      : I2C_PAUSE_LEN;
    p->p_next = next_state;
    return;
  }

  p->bus_state = next_state;
}


/** Interrupt handler. */
static void i2c_isr()
{
  volatile struct i2c_port *p;
  volatile struct i2c_txn_info *t;
  U32 dummy;
  U32 lines = *AT91C_PIOA_PDSR;
  U32 codr = 0;
  U32 sodr = 0;
  short sensor;

  /* Read the TC0 status register to ack the TC0 timer and allow this
   * interrupt handler to be called again.
   */
  dummy = *AT91C_TC0_SR;

  for (sensor=0; sensor<NXT_N_SENSORS; sensor++) {
    volatile sensor_pins pins = nx_sensors_get_pins(sensor);
    p = &i2c_state[sensor];
    t = &(p->txns[p->current_txn]);

    switch (p->bus_state)
      {
      case I2C_OFF:
      case I2C_CONFIG:
        /* Port is OFF or in txn configuration mode, do nothing. */
        break;

      case I2C_RECLOCK0:
        /* First step of reclocking: pull SCL low. */
        codr |= pins.scl;
        p->bus_state = I2C_RECLOCK1;
        break;

      case I2C_RECLOCK1:
        /* Second and last step of reclocking: set SCL high again, and
         * retry transaction.
         */
        sodr |= pins.scl;
        p->bus_state = I2C_SEND_START_BIT0;
        break;

      case I2C_READ_ACK0:
        /* Issue a clock pulse by releasing SCL. */
        sodr |= pins.scl;
        p->bus_state = I2C_READ_ACK1;
        break;

      case I2C_READ_ACK1:
        /* Wait for SCL to go up and let it stabilize. */
        if (lines & pins.scl) {
          p->bus_state = I2C_READ_ACK2;
        }
        break;

      case I2C_READ_ACK2:
        if (lines & pins.sda) {
          /* SDA is still high, this is a ACK fault. Setting
           * transaction status to TXN_STAT_FAILED and sending stop
           * bit.
           */
          i2c_log(" no-ack!\n");

          t->result = TXN_STAT_FAILED;
          
          /* Always issue a STOP bit after a ACK fault. */
          p->bus_state = I2C_SEND_STOP_BIT0;
          p->txn_state = TXN_STOP;
          
          /* Bypass remaining sub transactions. */
          p->current_txn = p->n_txns;
        } else {
          if (p->processed < t->data_size) {
            p->txn_state = TXN_TRANSMIT_BYTE;
          } else {
            t->result = TXN_STAT_SUCCESS;
            p->current_txn++;

            if (t->post_control == I2C_CONTROL_STOP) {
              p->bus_state = I2C_SCL_LOW;
              p->txn_state = TXN_STOP;
            } else {
              p->bus_state = I2C_IDLE;
              p->txn_state = TXN_WAITING;
            }
          }

          /* Pull SCL low to complete the clock pulse. SDA should be
           * released by the slave after that.
           */
          codr |= pins.scl;

          i2c_log(" r-ack.\n");
        }

        break;

      case I2C_WRITE_ACK0:
        /* Release SCL to do a clock pulse. */
        sodr |= pins.scl;
        p->bus_state = I2C_WRITE_ACK1;
        break;

      case I2C_WRITE_ACK1:
        /* Pull SCL low again to complete the clock pulse. */
        codr |= pins.scl;
        p->bus_state = I2C_WRITE_ACK2;
        break;

      case I2C_WRITE_ACK2:
        /* Release SDA for the slave to regain control of it. */
        sodr |= pins.sda;
        p->bus_state = I2C_SCL_LOW;
        p->txn_state = TXN_TRANSMIT_BYTE;

        i2c_log(" w-ack.\n");
        break;

      case I2C_IDLE:
        /* If current_txn < n_txns, we have work to do. */
        if (p->txn_state == TXN_WAITING && p->current_txn < p->n_txns) {
          if (t->pre_control == I2C_CONTROL_NONE) {
            p->txn_state = TXN_TRANSMIT_BYTE;
            p->bus_state = I2C_SCL_LOW;
          } else {
            /* Before issuing a START bit, set both pins high, just to be
             * sure, and proceed to SEND_START_BIT.
             */
            sodr |= pins.sda | pins.scl;
            
            if (t->pre_control == I2C_CONTROL_RESTART && p->lego_compat) {
              /* In LEGO compatibility mode, issue a reclock before the
               * restart (which is just a new START bit).
               */
              p->bus_state = I2C_RECLOCK0;
            } else {
              p->bus_state = I2C_SEND_START_BIT0;
            }
            
            p->txn_state = TXN_START;
          }
          
          /* Prepare the first bit to be sent. */
          p->processed = 0;
          p->current_byte = t->data[p->processed];
          p->current_pos = 7;
        }
        
        if (p->current_txn == p->n_txns) {
          p->txn_state = TXN_NONE;
        }

        break;

      case I2C_PAUSE:
        p->p_ticks--;
        if (p->p_ticks == 0) {
          p->bus_state = p->p_next;
        }
        break;

      case I2C_SEND_START_BIT0:
        if (lines & pins.sda) {
          /* Pull SDA low. */
          codr |= pins.sda;

          i2c_set_bus_state(sensor, I2C_SEND_START_BIT1);
        } else {
          /* Something is holding SDA low. Reclock until we get our data
           * line back.
           */
          p->bus_state = I2C_RECLOCK0;
        }

        break;

      case I2C_SEND_START_BIT1:
        /* Pull SCL low. */
        codr |= pins.scl;

        i2c_set_bus_state(sensor, I2C_SCL_LOW);
        p->txn_state = TXN_TRANSMIT_BYTE;

        break;

      case I2C_SCL_LOW:
        /* SCL is low. */

        switch (p->txn_state) {
        case TXN_TRANSMIT_BYTE:
          /* In write mode, it's time to set SDA to the bit
           * value we want. In read mode, let the remote device set
           * SDA.
           */
          if (t->mode == TXN_MODE_WRITE) {
            if ((p->current_byte & (1 << p->current_pos))) {
              sodr |= pins.sda;
              i2c_log_uint(1);
            } else {
              codr |= pins.sda;
              i2c_log_uint(0);
            }

            p->current_pos--;
          }

          p->bus_state = I2C_SAMPLE0;
          break;

        case TXN_WRITE_ACK:
          if (lines & pins.sda) {
            /* SDA is high: the slave has released SDA. Pull it low
             * and reclock.
             */
            codr |= pins.sda;
            p->bus_state = I2C_WRITE_ACK0;
          }

          /* Stay in the same state until the slave release SDA. */
          break;

        case TXN_READ_ACK:
          /* Release SDA and pull SCL low to prepare for the clock
           * pulse.
           */
          sodr |= pins.sda;
          codr |= pins.scl;
          p->bus_state = I2C_READ_ACK0;
          break;

        case TXN_STOP:
          /* Pull SDA low, to be able to release it up after SCL went
           * up.
           */
          codr |= pins.sda;
          i2c_set_bus_state(sensor, I2C_SEND_STOP_BIT0);
          break;

        default:
          break;
        }

        break;

      case I2C_SAMPLE0:
        /* Start sampling, rising SCL. */
        sodr |= pins.scl;
        p->bus_state = I2C_SAMPLE1;
        break;

      case I2C_SAMPLE1:
        /* End sampling.  In write mode, let the remote device read
         * the bit set in I2C_SCL_LOW. In read mode, retrieve SDA
         * value and store it.
         */
        if (t->mode == TXN_MODE_READ) {
          U8 value = (lines & pins.sda) ? 1 : 0;
          t->data[p->processed] |= (value << p->current_pos);
          p->current_pos--;
          i2c_log_uint(value);
        }

        p->bus_state = I2C_SAMPLE2;
        break;

      case I2C_SAMPLE2:
        /* Finally, pull SCL low. */
        codr |= pins.scl;

        if (p->current_pos < 0) {
          p->processed++;
          p->current_pos = 7;

          if (t->mode == TXN_MODE_WRITE) {
            /* In write mode, update the current_byte being
             * processed so it can be send next until we reach
             * data_size. Now, we expect a ACK from the slave.
             */
            if (p->processed < t->data_size) {
              p->current_byte = t->data[p->processed];
            }
            
            p->txn_state = TXN_READ_ACK;
          } else {
            /* In read mode, we need to give ACK to the slave so it can
             * continue transmission.
             */
            if (p->processed < t->data_size) {
              p->txn_state = TXN_WRITE_ACK;
            } else {
              if (t->post_control == I2C_CONTROL_STOP) {
                p->txn_state = TXN_STOP;
              } else {
                p->bus_state = I2C_IDLE;
                p->txn_state = TXN_WAITING;
              }
            
              t->result = TXN_STAT_SUCCESS;
              p->current_txn++;
            }
          }
        }

        p->bus_state = I2C_SCL_LOW;
        break;

      case I2C_SEND_STOP_BIT0:
        /* First, rise SCL. */
        sodr |= pins.scl;

        i2c_set_bus_state(sensor, I2C_SEND_STOP_BIT1);
        break;

      case I2C_SEND_STOP_BIT1:
        /* Finally, release SDA. */
        sodr |= pins.sda;

        i2c_set_bus_state(sensor, I2C_IDLE);
        p->txn_state = TXN_WAITING;
        break;
      }

    /** Update CODR and SODR to reflect changes for this sensor's
     * pins. */
    if (codr)
      *AT91C_PIOA_CODR = codr;
    if (sodr)
      *AT91C_PIOA_SODR = sodr;
  }
}
