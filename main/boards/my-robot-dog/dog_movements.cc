#include "dog_movements.h"

#include <algorithm>

#include "freertos/idf_additions.h"
#include "oscillator.h"
#include "esp_timer.h"

static const char* TAG = "DogMovements";

unsigned long millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

RobotDog::RobotDog() {
    is_dog_resting_ = false;
    // Initialize all servo pins to -1 (unconnected)
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        servo_pins_[i] = -1;
        servo_trim_[i] = 0;
    }
}

RobotDog::~RobotDog() {
    DetachServos();
}

void RobotDog::Init(int front_left, int front_right, int rear_left, int rear_right) {
    servo_pins_[FRONT_LEFT] = front_left;
    servo_pins_[FRONT_RIGHT] = front_right;
    servo_pins_[REAR_LEFT] = rear_left;
    servo_pins_[REAR_RIGHT] = rear_right;

    AttachServos();
    is_dog_resting_ = false;
}

///////////////////////////////////////////////////////////////////
//-- ATTACH & DETACH FUNCTIONS ----------------------------------//
///////////////////////////////////////////////////////////////////
void RobotDog::AttachServos() {
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].Attach(servo_pins_[i]);
        }
    }
}

void RobotDog::DetachServos() {
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].Detach();
        }
    }
}

///////////////////////////////////////////////////////////////////
//-- OSCILLATORS TRIMS ------------------------------------------//
///////////////////////////////////////////////////////////////////
void RobotDog::SetTrims(int front_left, int front_right, int rear_left, int rear_right) {
    servo_trim_[FRONT_LEFT] = front_left;
    servo_trim_[FRONT_RIGHT] = front_right;
    servo_trim_[REAR_LEFT] = rear_left;
    servo_trim_[REAR_RIGHT] = rear_right;

    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].SetTrim(servo_trim_[i]);
        }
    }
}

///////////////////////////////////////////////////////////////////
//-- BASIC MOTION FUNCTIONS -------------------------------------//
///////////////////////////////////////////////////////////////////
void RobotDog::MoveServos(int time, int servo_target[]) {
    if (GetRestState() == true) {
        SetRestState(false);
    }

    final_time_ = millis() + time;
    if (time > 10) {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            if (servo_pins_[i] != -1) {
                increment_[i] = (servo_target[i] - servo_[i].GetPosition()) / (time / 10.0);
            }
        }

        for (int iteration = 1; millis() < final_time_; iteration++) {
            partial_time_ = millis() + 10;
            for (int i = 0; i < DOG_SERVO_COUNT; i++) {
                if (servo_pins_[i] != -1) {
                    servo_[i].SetPosition(servo_[i].GetPosition() + increment_[i]);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    } else {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            if (servo_pins_[i] != -1) {
                servo_[i].SetPosition(servo_target[i]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(time));
    }

    // Final adjustment to the target
    bool f = true;
    int adjustment_count = 0;
    while (f && adjustment_count < 10) {
        f = false;
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            if (servo_pins_[i] != -1 && servo_target[i] != servo_[i].GetPosition()) {
                f = true;
                break;
            }
        }
        if (f) {
            for (int i = 0; i < DOG_SERVO_COUNT; i++) {
                if (servo_pins_[i] != -1) {
                    servo_[i].SetPosition(servo_target[i]);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            adjustment_count++;
        }
    };
}

void RobotDog::MoveSingle(int position, int servo_number) {
    if (position > 180)
        position = 90;
    if (position < 0)
        position = 90;

    if (GetRestState() == true) {
        SetRestState(false);
    }

    if (servo_number >= 0 && servo_number < DOG_SERVO_COUNT && servo_pins_[servo_number] != -1) {
        servo_[servo_number].SetPosition(position);
    }
}

void RobotDog::OscillateServos(int amplitude[DOG_SERVO_COUNT], int offset[DOG_SERVO_COUNT],
                               int period, double phase_diff[DOG_SERVO_COUNT], float cycle) {
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].SetO(offset[i]);
            servo_[i].SetA(amplitude[i]);
            servo_[i].SetT(period);
            servo_[i].SetPh(phase_diff[i]);
        }
    }

    double ref = millis();
    double end_time = period * cycle + ref;

    while (millis() < end_time) {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            if (servo_pins_[i] != -1) {
                servo_[i].Refresh();
            }
        }
        vTaskDelay(5);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

void RobotDog::Execute(int amplitude[DOG_SERVO_COUNT], int offset[DOG_SERVO_COUNT],
                       int period, double phase_diff[DOG_SERVO_COUNT], float steps) {
    if (GetRestState() == true) {
        SetRestState(false);
    }

    int cycles = (int)steps;

    //-- Execute complete cycles
    if (cycles >= 1)
        for (int i = 0; i < cycles; i++)
            OscillateServos(amplitude, offset, period, phase_diff, 1);

    //-- Execute the final not complete cycle
    OscillateServos(amplitude, offset, period, phase_diff, (float)steps - cycles);
    vTaskDelay(pdMS_TO_TICKS(10));
}

///////////////////////////////////////////////////////////////////
//-- HOME = Robot dog at rest position --------------------------//
///////////////////////////////////////////////////////////////////
void RobotDog::Home() {
    // Enhanced home position with proper stance
    // Front legs slightly forward for stability, rear legs centered
    int home_pos[DOG_SERVO_COUNT] = {85, 95, 90, 90};  // Slight forward stance
    MoveServos(800, home_pos);
    is_dog_resting_ = true;
    vTaskDelay(pdMS_TO_TICKS(200));
}

bool RobotDog::GetRestState() {
    return is_dog_resting_;
}

void RobotDog::SetRestState(bool state) {
    is_dog_resting_ = state;
}

///////////////////////////////////////////////////////////////////
//-- QUADRUPED GAITS --------------------------------------------//
///////////////////////////////////////////////////////////////////

//---------------------------------------------------------
//-- Trot gait: Diagonal legs move together
//--  FL+RR in phase, FR+RL in phase, 180° out of phase
//--  Parameters:
//--    steps: Number of trot cycles
//--    period: Time for one complete cycle (ms)
//--    dir: FORWARD or BACKWARD
//---------------------------------------------------------
void RobotDog::Trot(float steps, int period, int dir) {
    // Amplitude: How much each leg lifts
    int A[DOG_SERVO_COUNT] = {25, 25, 25, 25};

    // Offset: Slight lean for stability
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};

    // Phase differences:
    // FL(0°) + RR(0°) = diagonal pair 1
    // FR(180°) + RL(180°) = diagonal pair 2
    double phase_diff[DOG_SERVO_COUNT] = {
        0,                    // Front Left: 0°
        DEG2RAD(180),        // Front Right: 180° (opposite to FL)
        DEG2RAD(180),        // Rear Left: 180° (opposite to FL)
        0                     // Rear Right: 0° (same as FL)
    };

    // For backward motion, reverse phase
    if (dir == BACKWARD) {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            phase_diff[i] = -phase_diff[i];
        }
    }

    Execute(A, O, period, phase_diff, steps);
}

//---------------------------------------------------------
//-- Walk gait: 4-beat gait (one leg at a time)
//--  Sequence: FL → RR → FR → RL
//--  Parameters:
//--    steps: Number of walk cycles
//--    period: Time for one complete cycle (ms)
//--    dir: FORWARD or BACKWARD
//---------------------------------------------------------
void RobotDog::Walk(float steps, int period, int dir) {
    int A[DOG_SERVO_COUNT] = {20, 20, 20, 20};
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};

    // 4-beat gait: each leg 90° apart
    double phase_diff[DOG_SERVO_COUNT] = {
        0,                    // Front Left: 0°
        DEG2RAD(180),        // Front Right: 180°
        DEG2RAD(270),        // Rear Left: 270°
        DEG2RAD(90)          // Rear Right: 90°
    };

    if (dir == BACKWARD) {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            phase_diff[i] = -phase_diff[i];
        }
    }

    Execute(A, O, period, phase_diff, steps);
}

//---------------------------------------------------------
//-- Pace gait: Lateral pairs (same side together)
//--  FL+RL in phase, FR+RR in phase
//--  Like a camel walk
//---------------------------------------------------------
void RobotDog::Pace(float steps, int period, int dir) {
    int A[DOG_SERVO_COUNT] = {25, 25, 25, 25};
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};

    // Left legs together, right legs together
    double phase_diff[DOG_SERVO_COUNT] = {
        0,                    // Front Left: 0°
        DEG2RAD(180),        // Front Right: 180°
        0,                    // Rear Left: 0° (same as FL)
        DEG2RAD(180)         // Rear Right: 180° (same as FR)
    };

    if (dir == BACKWARD) {
        for (int i = 0; i < DOG_SERVO_COUNT; i++) {
            phase_diff[i] = -phase_diff[i];
        }
    }

    Execute(A, O, period, phase_diff, steps);
}

//---------------------------------------------------------
//-- Turn: Rotate in place
//--  Left turn: left legs backward, right legs forward
//---------------------------------------------------------
void RobotDog::Turn(float steps, int period, int dir) {
    int A[DOG_SERVO_COUNT] = {20, 20, 20, 20};
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};

    if (dir == LEFT) {
        // Left legs move backward, right legs move forward
        double phase_diff[DOG_SERVO_COUNT] = {
            DEG2RAD(180),    // Front Left: backward
            0,                // Front Right: forward
            DEG2RAD(180),    // Rear Left: backward
            0                 // Rear Right: forward
        };
        Execute(A, O, period, phase_diff, steps);
    } else {  // RIGHT
        // Right legs move backward, left legs move forward
        double phase_diff[DOG_SERVO_COUNT] = {
            0,                // Front Left: forward
            DEG2RAD(180),    // Front Right: backward
            0,                // Rear Left: forward
            DEG2RAD(180)     // Rear Right: backward
        };
        Execute(A, O, period, phase_diff, steps);
    }
}

///////////////////////////////////////////////////////////////////
//-- SPECIAL BEHAVIORS ------------------------------------------//
///////////////////////////////////////////////////////////////////

void RobotDog::Sit() {
    // Enhanced sit position: rear legs bent (120°), front legs slightly forward (100°)
    int sit_pos[DOG_SERVO_COUNT] = {100, 80, 120, 120};
    MoveServos(1000, sit_pos);
    is_dog_resting_ = true;
}

void RobotDog::LayDown() {
    // All legs to side position
    int lay_pos[DOG_SERVO_COUNT] = {120, 60, 120, 60};
    MoveServos(1500, lay_pos);
    is_dog_resting_ = true;
}

void RobotDog::Shake() {
    // Quick left-right body shake
    int A[DOG_SERVO_COUNT] = {15, 15, 15, 15};
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};
    double phase_diff[DOG_SERVO_COUNT] = {
        0,                // Front Left
        DEG2RAD(180),    // Front Right (opposite)
        0,                // Rear Left
        DEG2RAD(180)     // Rear Right (opposite)
    };
    Execute(A, O, 300, phase_diff, 5);
}

void RobotDog::Jump(float steps, int period) {
    // All legs push down then extend
    int crouch[DOG_SERVO_COUNT] = {110, 70, 110, 70};
    int extend[DOG_SERVO_COUNT] = {70, 110, 70, 110};
    int land[DOG_SERVO_COUNT] = {90, 90, 90, 90};

    MoveServos(200, crouch);   // Crouch
    vTaskDelay(pdMS_TO_TICKS(100));
    MoveServos(150, extend);   // Jump up
    vTaskDelay(pdMS_TO_TICKS(200));
    MoveServos(200, land);     // Land
}

void RobotDog::Bow() {
    // Play bow: front legs down, rear legs up
    int bow_pos[DOG_SERVO_COUNT] = {110, 70, 90, 90};
    MoveServos(800, bow_pos);
    vTaskDelay(pdMS_TO_TICKS(1000));
    Home();
}

void RobotDog::HandShake(int leg, float steps, int period) {
    // Lift one front leg and shake it up and down
    // leg: LEFT (default) or RIGHT to choose which front leg

    // First, shift weight to the opposite side for balance
    int weight_shift[DOG_SERVO_COUNT];
    int shake_servo;

    if (leg == LEFT) {
        // Shift weight to the right, lift left front leg
        weight_shift[FRONT_LEFT] = 90;   // Will be lifted
        weight_shift[FRONT_RIGHT] = 100; // Lean slightly right
        weight_shift[REAR_LEFT] = 95;    // Lean slightly right
        weight_shift[REAR_RIGHT] = 100;  // Support weight
        shake_servo = FRONT_LEFT;
    } else {  // RIGHT
        // Shift weight to the left, lift right front leg
        weight_shift[FRONT_LEFT] = 100;  // Lean slightly left
        weight_shift[FRONT_RIGHT] = 90;  // Will be lifted
        weight_shift[REAR_LEFT] = 100;   // Support weight
        weight_shift[REAR_RIGHT] = 95;   // Lean slightly left
        shake_servo = FRONT_RIGHT;
    }

    // Shift weight for balance
    MoveServos(500, weight_shift);
    vTaskDelay(pdMS_TO_TICKS(200));

    // Now shake the lifted leg using oscillator
    int A[DOG_SERVO_COUNT] = {0, 0, 0, 0};
    int O[DOG_SERVO_COUNT] = {0, 0, 0, 0};
    double phase_diff[DOG_SERVO_COUNT] = {0, 0, 0, 0};

    // Set amplitude only for the shaking leg
    A[shake_servo] = 25;  // 25 degree shake amplitude

    if (leg == LEFT) {
        O[FRONT_LEFT] = -20;   // Offset to keep leg lifted (70 degrees)
        O[FRONT_RIGHT] = 10;   // Keep supporting
        O[REAR_LEFT] = 5;      // Keep supporting
        O[REAR_RIGHT] = 10;    // Keep supporting
    } else {
        O[FRONT_LEFT] = 10;    // Keep supporting
        O[FRONT_RIGHT] = 20;   // Offset to keep leg lifted (110 degrees)
        O[REAR_LEFT] = 10;     // Keep supporting
        O[REAR_RIGHT] = 5;     // Keep supporting
    }

    // Execute the shake motion
    Execute(A, O, period, phase_diff, steps);

    // Return to home position
    vTaskDelay(pdMS_TO_TICKS(200));
}

void RobotDog::HighFive(int leg, int hold_time) {
    // Lift one front paw high and hold it (for high-five gesture)
    // leg: LEFT (default) or RIGHT to choose which front leg
    // hold_time: how long to hold paw up in milliseconds (default: 1500ms)

    // First, shift weight to the opposite side for balance
    int weight_shift[DOG_SERVO_COUNT];

    if (leg == LEFT) {
        // Shift weight to the right, prepare to lift left front leg
        weight_shift[FRONT_LEFT] = 90;   // Will be lifted
        weight_shift[FRONT_RIGHT] = 100; // Lean slightly right
        weight_shift[REAR_LEFT] = 95;    // Lean slightly right
        weight_shift[REAR_RIGHT] = 100;  // Support weight
    } else {  // RIGHT
        // Shift weight to the left, prepare to lift right front leg
        weight_shift[FRONT_LEFT] = 100;  // Lean slightly left
        weight_shift[FRONT_RIGHT] = 90;  // Will be lifted
        weight_shift[REAR_LEFT] = 100;   // Support weight
        weight_shift[REAR_RIGHT] = 95;   // Lean slightly left
    }

    // Shift weight for balance
    MoveServos(500, weight_shift);
    vTaskDelay(pdMS_TO_TICKS(200));

    // Lift paw HIGH (higher than handshake, ready for high-five)
    int high_five_pos[DOG_SERVO_COUNT];

    if (leg == LEFT) {
        high_five_pos[FRONT_LEFT] = 60;   // Lifted HIGH (60° = raised up for high-five)
        high_five_pos[FRONT_RIGHT] = 100; // Keep supporting
        high_five_pos[REAR_LEFT] = 95;    // Keep supporting
        high_five_pos[REAR_RIGHT] = 100;  // Keep supporting
    } else {  // RIGHT
        high_five_pos[FRONT_LEFT] = 100;  // Keep supporting
        high_five_pos[FRONT_RIGHT] = 120; // Lifted HIGH (120° = raised up for high-five)
        high_five_pos[REAR_LEFT] = 100;   // Keep supporting
        high_five_pos[REAR_RIGHT] = 95;   // Keep supporting
    }

    // Lift paw to high-five position
    MoveServos(600, high_five_pos);

    // HOLD the position (key difference from handshake - static hold, no shaking)
    // This allows user to slap the paw for a high-five, or just see the paw clearly
    vTaskDelay(pdMS_TO_TICKS(hold_time));

    // Brief pause before returning
    vTaskDelay(pdMS_TO_TICKS(200));

    // Note: Home() will be called automatically by the controller after this action
}

///////////////////////////////////////////////////////////////////
//-- SERVO LIMITER ----------------------------------------------//
///////////////////////////////////////////////////////////////////
void RobotDog::EnableServoLimit(int speed_limit_degree_per_sec) {
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].SetLimiter(speed_limit_degree_per_sec);
        }
    }
}

void RobotDog::DisableServoLimit() {
    for (int i = 0; i < DOG_SERVO_COUNT; i++) {
        if (servo_pins_[i] != -1) {
            servo_[i].DisableLimiter();
        }
    }
}
