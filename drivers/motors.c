/* Driver for the NXT's motors.
 *
 * This driver provides a high level interface to the NXT's motor
 * ports. It takes care of controlling the motors (using the AVR
 * driver), of counting the tachymeter pulses sent by rotating motors
 * to get a rotation count, and provides APIs for starting/stopping
 * motors, or running them for a certain time or rotation angle only.
 */

#include "base/at91sam7s256.h"

#include "base/mytypes.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/drivers/avr.h"
#include "base/drivers/motors.h"

/* The following are easier mnemonics for the pins used by the
 * tachymeter. Each motor has a tach-pulse pin whose value flips at
 * regular rotation intervals, and a direction pin whose value depends
 * on the direction of rotation.
 */
#define MOTOR_A_TACH AT91C_PIO_PA15
#define MOTOR_A_DIR AT91C_PIO_PA1
#define MOTOR_B_TACH AT91C_PIO_PA26
#define MOTOR_B_DIR AT91C_PIO_PA9
#define MOTOR_C_TACH AT91C_PIO_PA0
#define MOTOR_C_DIR AT91C_PIO_PA8

/* We often need to manipulate all tach pins and/or all dir pins
 * together. Lets's simplify doing that.
 */
#define MOTORS_TACH (MOTOR_A_TACH | MOTOR_B_TACH | MOTOR_C_TACH)
#define MOTORS_DIR (MOTOR_A_DIR | MOTOR_B_DIR | MOTOR_C_DIR)
#define MOTORS_ALL (MOTORS_TACH | MOTORS_DIR)


/* Definitions of the state of each motor. */
static volatile struct {
  /* The pins that this motor's tachymeter uses. */
  struct {
    U32 tach;
    U32 dir;
  } pins;

  /* The mode this motor is currently in. */
  enum {
    MOTOR_STOP = 0,      /* No rotation. */
    MOTOR_CONFIGURING,   /* Reconfiguration in progress. */
    MOTOR_ON_CONTINUOUS, /* Rotate until stopped. */
    MOTOR_ON_ANGLE,      /* Rotate a given angle. */
    MOTOR_ON_TIME,       /* Rotate a given time. */
  } mode;

  /* If in one of the automatic modes (angle/time rotation), defines
   * whether the motor will brake (TRUE) or coast (FALSE) when
   * stopped.
   */
  bool brake;

  /* The current tachymeter count. */
  U32 current_count;

  /* The target is a mode-dependent value.
   *  - In stop and continuous mode, it is not used.
   *  - In angle mode, holds the target tachymeter count.
   *  - In time mode, holds the remaining number of milliseconds to
   *    run.
   */
  U32 target;
} motors_state[NXT_N_MOTORS] = {
  { { MOTOR_A_TACH, MOTOR_A_DIR }, MOTOR_STOP, TRUE, 0, 0 },
  { { MOTOR_B_TACH, MOTOR_B_DIR }, MOTOR_STOP, TRUE, 0, 0 },
  { { MOTOR_C_TACH, MOTOR_C_DIR }, MOTOR_STOP, TRUE, 0, 0 },
};


/* Tachymeter interrupt handler, triggered by a change of value of a
 * tachymeter pin.
 */
void motors_isr() {
  int i;
  U32 changes;
  U32 pins;
  U32 time;

  /* Acknowledge the interrupt and grab the state of the pins. */
  changes = *AT91C_PIOA_ISR;
  pins = *AT91C_PIOA_PDSR;

  /* Grab the time, as we're going to use it to check for timed
   * rotation end.
   */
  time = systick_get_ms();

  /* Check each motor's tachymeter. */
  for (i=0; i<NXT_N_MOTORS; i++) {
    if (changes & motors_state[i].pins.tach) {
      U32 tach = pins & motors_state[i].pins.tach;
      U32 dir = pins & motors_state[i].pins.dir;

      /* If the tachymeter pin value is the opposite the direction pin
       * value, then the motor is rotating 'forwards' (positive speed
       * value given to start it). Otherwise, it's rotating
       * 'backwards', and we should decrement the tachymeter count
       * instead of incrementing it.
       */
      if ((tach && !dir) || (!tach && dir))
        motors_state[i].current_count++;
      else
        motors_state[i].current_count--;

      /* If we are in angle rotation mode, check to see if we've
       * reached the target tachymeter value. If so, shut down the
       * motor.
       */
      if ((motors_state[i].mode == MOTOR_ON_ANGLE &&
           motors_state[i].current_count == motors_state[i].target) ||
          (motors_state[i].mode == MOTOR_ON_TIME &&
           time >= motors_state[i].target))
        motors_stop(i, motors_state[i].brake);
    }
  }
}


/* Initialize the tachymeters, ready to drive the motors with the
 * high-level API.
 */
void motors_init()
{
  interrupts_disable();

  /* Enable the PIO controller. */
  *AT91C_PMC_PCER = (1 << AT91C_ID_PIOA);

  /* Disable all PIO interrupts until we are ready to handle them. */
  *AT91C_PIOA_IDR = ~0;

  /* Configure all tachymeter pins:
   *  - Enable input glitch filtering
   *  - Disable pull-up (externally driven pins)
   *  - Assign pins to the PIO controller
   *  - Set pins to be inputs
   */
  *AT91C_PIOA_IFER = MOTORS_ALL;
  *AT91C_PIOA_PPUDR = MOTORS_ALL;
  *AT91C_PIOA_PER = MOTORS_ALL;
  *AT91C_PIOA_ODR = MOTORS_ALL;

  /* Register the tachymeter interrupt handler. */
  aic_install_isr(AT91C_ID_PIOA, AIC_PRIO_SOFTMAC,
                  AIC_TRIG_LEVEL, motors_isr);

  /* Trigger interrupts on changes to the state of the tachy pins. */
  *AT91C_PIOA_IER = MOTORS_TACH;

  interrupts_enable();
}


/* Immediately stop the given motor, either braking or coasting. */
void motors_stop(U8 motor, bool brake) {
  /* Cannot rotate imaginary motors. */
  if (motor > 2)
    return;

  motors_state[motor].mode = MOTOR_STOP;
  avr_set_motor(motor, 0, brake);
}


/* Start rotating the given motor continuously at the given speed. It
 * will continue to rotate until another motor command is issued to
 * it.
 */
void motors_rotate(U8 motor, S8 speed) {
  /* Cannot rotate imaginary motors. */
  if (motor > 2)
    return;

  /* Cap the given motor speed. */
  if (speed > 0 && speed > 100)
    speed = 100;
  else if (speed < 0 && speed < -100)
    speed = -100;

  /* Continuous mode has no target or brake parameter, just set the
   * mode and fire up the motor.
   */
  motors_state[motor].mode = MOTOR_ON_CONTINUOUS;
  avr_set_motor(motor, speed, FALSE);
}


/* Start rotating the motor at the given speed, and stop it (with the
 * given braking mode) after rotating the given number of degrees.
 */
void motors_rotate_angle(U8 motor, S8 speed, U32 angle, bool brake) {
  /* If we're not moving, we can never reach the target. Take a
   * shortcut.
   */
  if (speed == 0) {
    motors_stop(motor, brake);
    return;
  }

  /* Cannot rotate imaginary motors. */
  if (motor > 2)
    return;

  /* Cap the given motor speed. */
  if (speed > 0 && speed > 100)
    speed = 100;
  else if (speed < 0 && speed < -100)
    speed = -100;

  /* Set the motor to configuration mode. This way, if we are
   * overriding another intelligent mode, the tachymeter interrupt
   * handler will ignore the motor while we tweak its settings.
   */
  motors_state[motor].mode = MOTOR_CONFIGURING;

  /* Set the target tachymeter value based on the rotation
   * direction */
  if (speed < 0)
    motors_state[motor].target =
      motors_state[motor].current_count - angle;
  else
    motors_state[motor].target =
      motors_state[motor].current_count + angle;

  /* Remember the brake setting, change to angle target mode and fire
   * up the motor.
   */
  motors_state[motor].brake = brake;
  motors_state[motor].mode = MOTOR_ON_ANGLE;
  avr_set_motor(motor, speed, FALSE);
}


/* Start rotating the motor at the given speed, and stop it (with the
 * given braking mode) after given number of milliseconds.
 */
void motors_rotate_time(U8 motor, S8 speed, U32 ms, bool brake) {
  /* If we're not moving, we can never reach the target. Take a
   * shortcut.
   */
  if (speed == 0) {
    motors_stop(motor, brake);
    return;
  }

  /* Cannot rotate imaginary motors. */
  if (motor > 2)
    return;

  /* Cap the given motor speed. */
  if (speed > 0 && speed > 100)
    speed = 100;
  else if (speed < 0 && speed < -100)
    speed = -100;

  /* Set the motor to configuration mode. This way, if we are
   * overriding another intelligent mode, the tachymeter interrupt
   * handler will ignore the motor while we tweak its settings.
   */
  motors_state[motor].mode = MOTOR_CONFIGURING;

  /* Set the target system time. */
  motors_state[motor].target = systick_get_ms() + ms;

  /* Remember the brake setting, change to angle target mode and fire
   * up the motor.
   */
  motors_state[motor].brake = brake;
  motors_state[motor].mode = MOTOR_ON_TIME;
  avr_set_motor(motor, speed, FALSE);
}


/* Get the current value of the tachymeter counter for the given
 * motor. This value is of no real use outside of the tachymeter
 * driver, but is useful for displaying motor activity.
 */
U32 motors_get_tach_count(U8 motor) {
  /* Cannot query imaginary motors. */
  if (motor > 2)
    return 0;

  return motors_state[motor].current_count;
}
