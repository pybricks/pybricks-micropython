
#include "pbid.h"
#include "mpconfigbrick.h"

pbio_iodev_type_id_t get_id_from_qstr(qstr q){
    switch(q) {
        #if defined(PYBRICKS_BRICK_MOVEHUB)
        case MP_QSTR_TrainMotor:
            return PBIO_IODEV_TYPE_ID_LPF2_TRAIN;
        case MP_QSTR_MoveHubMotor:
            return PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR;
        case MP_QSTR_InteractiveMotor:
            return PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR;
        #endif //PYBRICKS_BRICK_MOVEHUB
        #if defined(PYBRICKS_BRICK_EV3)
        case MP_QSTR_MediumMotor:
            return PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR;
        case MP_QSTR_LargeMotor:
            return PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;
        #endif //PYBRICKS_BRICK_EV3
        default:
            return PBIO_IODEV_TYPE_ID_NONE;
    }
}
