#ifndef __DOG_MOVEMENTS_H__
#define __DOG_MOVEMENTS_H__

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oscillator.h"

//-- Constants
#define FORWARD 1
#define BACKWARD -1
#define LEFT 1
#define RIGHT -1
#define SMALL 5
#define MEDIUM 15
#define BIG 30

// -- Servo delta limit default. degree / sec
#define SERVO_LIMIT_DEFAULT 240

// -- Servo indexes for 4-legged robot dog
#define FRONT_LEFT 0
#define FRONT_RIGHT 1
#define REAR_LEFT 2
#define REAR_RIGHT 3
#define DOG_SERVO_COUNT 4

class RobotDog {
public:
    RobotDog();
    ~RobotDog();

    //-- Robot dog initialization
    void Init(int front_left, int front_right, int rear_left, int rear_right);

    //-- Attach & detach functions
    void AttachServos();
    void DetachServos();

    //-- Oscillator Trims (calibration)
    void SetTrims(int front_left, int front_right, int rear_left, int rear_right);

    //-- Basic motion functions
    void MoveServos(int time, int servo_target[]);
    void MoveSingle(int position, int servo_number);
    void OscillateServos(int amplitude[DOG_SERVO_COUNT], int offset[DOG_SERVO_COUNT],
                         int period, double phase_diff[DOG_SERVO_COUNT], float cycle);

    //-- HOME = Rest position (standing)
    void Home();
    bool GetRestState();
    void SetRestState(bool state);

    //-- Quadruped gaits
    void Trot(float steps = 4, int period = 600, int dir = FORWARD);      // Diagonal gait
    void Walk(float steps = 4, int period = 800, int dir = FORWARD);      // 4-beat gait
    void Pace(float steps = 4, int period = 700, int dir = FORWARD);      // Lateral gait
    void Bound(float steps = 2, int period = 500, int dir = FORWARD);     // Front/rear together
    void Gallop(float steps = 2, int period = 400, int dir = FORWARD);    // Asymmetric gait

    void Turn(float steps = 4, int period = 800, int dir = LEFT);         // Turn in place
    void Sit();                                                            // Sit down
    void LayDown();                                                        // Lay down flat
    void Shake();                                                          // Shake body
    void Wiggle(float steps = 3, int period = 500);                       // Wiggle tail motion
    void Jump(float steps = 1, int period = 500);                         // Jump up
    void Bow();                                                            // Play bow gesture
    void HandShake(int leg = LEFT, float steps = 5, int period = 400);   // Shake hand/paw
    void HighFive(int leg = LEFT, int hold_time = 1500);                  // High five gesture

    // -- Servo limiter
    void EnableServoLimit(int speed_limit_degree_per_sec = SERVO_LIMIT_DEFAULT);
    void DisableServoLimit();

private:
    Oscillator servo_[DOG_SERVO_COUNT];

    int servo_pins_[DOG_SERVO_COUNT];
    int servo_trim_[DOG_SERVO_COUNT];

    unsigned long final_time_;
    unsigned long partial_time_;
    float increment_[DOG_SERVO_COUNT];

    bool is_dog_resting_;

    void Execute(int amplitude[DOG_SERVO_COUNT], int offset[DOG_SERVO_COUNT],
                 int period, double phase_diff[DOG_SERVO_COUNT], float steps);
};

#endif  // __DOG_MOVEMENTS_H__
