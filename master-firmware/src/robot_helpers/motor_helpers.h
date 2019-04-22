#ifndef MOTOR_HELPERS_H
#define MOTOR_HELPERS_H

#include "motor_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Moves motor with given speed and waits until a new index is detected */
float motor_wait_for_index(motor_driver_t* motor, float motor_speed);

/* Moves motor forward then backwards to get index from both sides and returns average index */
float motor_auto_index_sym(motor_driver_t* motor, int motor_dir, float motor_speed);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_HELPERS_H */
