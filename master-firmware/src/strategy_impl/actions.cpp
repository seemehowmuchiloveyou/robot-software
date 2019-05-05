#include <aversive/blocking_detection_manager/blocking_detection_manager.h>

#include <aversive/position_manager/position_manager.h>
#include <aversive/trajectory_manager/trajectory_manager_utils.h>
#include <aversive/obstacle_avoidance/obstacle_avoidance.h>

#include "robot_helpers/math_helpers.h"
#include "robot_helpers/trajectory_helpers.h"
#include "robot_helpers/beacon_helpers.h"
#include "robot_helpers/strategy_helpers.h"

#include "control_panel.h"
#include "protobuf/sensors.pb.h"
#include "config.h"

#include "strategy_impl/actions.h"

bool IndexArms::execute(RobotState& state)
{
    strat->log("Indexing arms!");

    // set index when user presses color button, so indexing is done manually

    strat->arm_manual_index(RIGHT);
    strat->wait_ms(500);
    strat->wait_for_user_input();

    strat->arm_manual_index(LEFT);
    strat->wait_ms(500);
    strat->wait_for_user_input();

    state.arms_are_indexed = true;
    return true;
}

bool RetractArms::execute(RobotState& state)
{
    strat->log("Retracting arms!");

    strat->gripper_set(RIGHT, GRIPPER_OFF);
    strat->manipulator_goto(RIGHT, MANIPULATOR_RETRACT);

    state.has_puck = false;
    state.arms_are_deployed = false;
    return true;
}

bool TakePuck::execute(RobotState& state)
{
    switch (pucks[puck_id].color) {
        case PuckColor_RED:
            strat->log("Taking red puck");
            break;
        case PuckColor_GREEN:
            strat->log("Taking green puck");
            break;
        case PuckColor_BLUE:
            strat->log("Taking blue puck");
            break;
        case PuckColor_RED_OR_GREEN:
            strat->log("Taking red/green puck");
            break;
    }

    float x, y, a;
    if (pucks[puck_id].orientation == PuckOrientiation_HORIZONTAL) {
        x = MIRROR_X(strat->color, pucks[puck_id].pos_x_mm - 170);
        y = pucks[puck_id].pos_y_mm + MIRROR(strat->color, 50);
        a = MIRROR_A(strat->color, 180);
    } else {
        x = MIRROR_X(strat->color, pucks[puck_id].pos_x_mm) - 50;
        y = pucks[puck_id].pos_y_mm - 260;
        a = MIRROR_A(strat->color, -90);
    }

    if (!strat->goto_xya(strat, x, y, a)) {
        return false;
    }

    state.arms_are_deployed = true;
    strat->gripper_set(RIGHT, GRIPPER_ACQUIRE);

    if (pucks[puck_id].orientation == PuckOrientiation_HORIZONTAL) {
        strat->manipulator_goto(RIGHT, MANIPULATOR_PICK_HORZ);
    } else {
        strat->manipulator_goto(RIGHT, MANIPULATOR_PICK_VERT);
    }
    strat->wait_ms(500);
    strat->manipulator_goto(RIGHT, MANIPULATOR_LIFT_HORZ);

    state.puck_available[puck_id] = false;

    if (!strat->puck_is_picked()) {
        strat->gripper_set(RIGHT, GRIPPER_OFF);
        return false;
    }

    state.has_puck = true;
    state.has_puck_color = pucks[puck_id].color;
    return true;
}

bool DepositPuck::execute(RobotState& state)
{
    strat->log("Depositing puck");

    float x = MIRROR_X(strat->color, areas[zone_id].pos_x_mm);
    float y = areas[zone_id].pos_y_mm - MIRROR(strat->color, 50);
    float a = MIRROR_A(strat->color, 0);

    if (!strat->goto_xya(strat, x, y, a)) {
        return false;
    }
    strat->gripper_set(RIGHT, GRIPPER_RELEASE);
    strat->wait_ms(100);

    strat->gripper_set(RIGHT, GRIPPER_OFF);

    pucks_in_area++;
    state.has_puck = false;
    state.classified_pucks[areas[zone_id].color]++;
    state.arms_are_deployed = true;
    return true;
}

bool LaunchAccelerator::execute(RobotState& state)
{
    strat->log("Pushing puck to launch accelerator");

    float x = (strat->color == VIOLET) ? 1695 : 1405;

    if (!strat->goto_xya(strat, x, 330, MIRROR_A(strat->color, 90))) {
        return false;
    }

    state.arms_are_deployed = true;
    strat->manipulator_goto(RIGHT, MANIPULATOR_DEPLOY_FULLY);

    strat->forward(strat, -30);
    strat->rotate(strat, MIRROR(strat->color, 20));
    strat->forward(strat, 40);

    state.accelerator_is_done = true;
    return true;
}

bool TakeGoldonium::execute(RobotState& state)
{
    strat->log("Taking goldenium");

    float x = (strat->color == VIOLET) ? 2275 : 825;

    if (!strat->goto_xya(strat, x, 400, MIRROR_A(strat->color, 90))) {
        return false;
    }

    state.arms_are_deployed = true;
    strat->manipulator_goto(RIGHT, MANIPULATOR_PICK_GOLDONIUM);

    if (!strat->goto_xya(strat, x, 330, MIRROR_A(strat->color, 90))) {
        return false;
    }

    strat->gripper_set(RIGHT, GRIPPER_ACQUIRE);
    strat->forward(strat, -27);
    strat->wait_ms(1500);

    if (!strat->puck_is_picked()) {
        strat->gripper_set(RIGHT, GRIPPER_OFF);
        strat->forward(strat, 80);
        return false;
    }

    strat->manipulator_goto(RIGHT, MANIPULATOR_LIFT_GOLDONIUM);
    strat->wait_ms(500);
    strat->gripper_set(RIGHT, GRIPPER_OFF);

    strat->forward(strat, 80);

    state.goldonium_in_house = false;
    state.has_goldonium = true;
    return true;
}