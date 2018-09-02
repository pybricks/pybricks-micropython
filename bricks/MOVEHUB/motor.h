#ifndef _PYBRICKS_MOVEHUB_MOTOR_H_
#define _PYBRICKS_MOVEHUB_MOTOR_H_

typedef enum {
    MOTOR_PORT_A,
    MOTOR_PORT_B,
    MOTOR_PORT_C,
    MOTOR_PORT_D,
} motor_port_t;

typedef enum {
    MOTOR_STOP_COAST,
    MOTOR_STOP_BRAKE,
} motor_stop_t;

void motor_init(void);
int motor_get_position(motor_port_t port, int *pos);
int motor_run(motor_port_t port, int duty_cycle);
int motor_stop(motor_port_t port, motor_stop_t stop_action);
void motor_deinit(void);

#endif /* _PYBRICKS_MOVEHUB_MOTOR_H_ */
