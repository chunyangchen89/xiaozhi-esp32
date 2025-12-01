/*
    Robot Dog Controller - MCP Protocol Version
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstdlib>
#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "dog_movements.h"
#include "mcp_server.h"
#include "sdkconfig.h"
#include "settings.h"

#define TAG "DogController"

class DogController {
private:
    RobotDog dog_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool is_action_in_progress_ = false;

    struct DogActionParams {
        int action_type;
        int steps;
        int speed;
        int direction;
    };

    enum ActionType {
        ACTION_WALK = 1,     // Unified walking gait (replaces trot, pace, gallop, bound)
        ACTION_TURN = 2,
        ACTION_SIT = 3,
        ACTION_LAYDOWN = 4,
        ACTION_SHAKE = 5,
        ACTION_JUMP = 6,     // Replaces bound
        ACTION_BOW = 7,
        ACTION_HOME = 8,
        ACTION_HANDSHAKE = 9,
        ACTION_HIGHFIVE = 10
    };

    static void ActionTask(void* arg) {
        DogController* controller = static_cast<DogController*>(arg);
        DogActionParams params;
        controller->dog_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "Executing action: %d", params.action_type);
                controller->is_action_in_progress_ = true;

                switch (params.action_type) {
                    case ACTION_WALK:
                        // Use different gaits based on speed for variety
                        if (params.speed < 400) {
                            // Fast speed = trot-like movement
                            controller->dog_.Trot(params.steps, params.speed, params.direction);
                        } else if (params.speed < 700) {
                            // Medium speed = walk
                            controller->dog_.Walk(params.steps, params.speed, params.direction);
                        } else {
                            // Slow speed = pace-like movement
                            controller->dog_.Pace(params.steps, params.speed, params.direction);
                        }
                        break;
                    case ACTION_TURN:
                        controller->dog_.Turn(params.steps, params.speed, params.direction);
                        break;
                    case ACTION_SIT:
                        controller->dog_.Sit();
                        break;
                    case ACTION_LAYDOWN:
                        controller->dog_.LayDown();
                        break;
                    case ACTION_SHAKE:
                        controller->dog_.Shake();
                        break;
                                      case ACTION_JUMP:
                        controller->dog_.Jump();
                        break;
                    case ACTION_BOW:
                        controller->dog_.Bow();
                        break;
                    case ACTION_HOME:
                        controller->dog_.Home();
                        break;
                    case ACTION_HANDSHAKE:
                        controller->dog_.HandShake(params.direction, params.steps, params.speed);
                        break;
                    case ACTION_HIGHFIVE:
                        controller->dog_.HighFive(params.direction, params.speed);
                        break;
                }

                // Auto-return to home position after movement actions
                if (params.action_type != ACTION_SIT && params.action_type != ACTION_LAYDOWN &&
                    params.action_type != ACTION_HOME) {
                    controller->dog_.Home();
                }

                controller->is_action_in_progress_ = false;
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "dog_action", 1024 * 3, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction) {
        ESP_LOGI(TAG, "Action control: type=%d, steps=%d, speed=%d, direction=%d", action_type,
                 steps, speed, direction);

        DogActionParams params = {action_type, steps, speed, direction};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void LoadTrimsFromNVS() {
        Settings settings("dog_trims", false);

        int front_left = settings.GetInt("front_left", 0);
        int front_right = settings.GetInt("front_right", 0);
        int rear_left = settings.GetInt("rear_left", 0);
        int rear_right = settings.GetInt("rear_right", 0);

        ESP_LOGI(TAG, "Loading trims from NVS: FL=%d, FR=%d, RL=%d, RR=%d", front_left,
                 front_right, rear_left, rear_right);

        dog_.SetTrims(front_left, front_right, rear_left, rear_right);
    }

public:
    DogController() {
        dog_.Init(FRONT_LEFT_PIN, FRONT_RIGHT_PIN, REAR_LEFT_PIN, REAR_RIGHT_PIN);

        ESP_LOGI(TAG, "Robot Dog initialized with 4 servos");

        LoadTrimsFromNVS();

        action_queue_ = xQueueCreate(10, sizeof(DogActionParams));

        QueueAction(ACTION_HOME, 1, 500, 1);

        RegisterMcpTools();
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Registering MCP tools...");

        // Main action tool for robot dog with comprehensive movement behaviors
        mcp_server.AddTool(
            "self.dog.action",
            "Execute a specific robot dog action or movement. Use this for locomotion, behaviors, and gestures. "
            "Parameters: action (string): the specific action to perform; direction (int): movement direction - 1=forward/left turn, -1=backward/right turn; "
            "steps (int, 1-20): number of action cycles/repetitions; speed (int, 300-2000ms): timing period - lower=faster movement. "
            "\n\nLOCOMOTION GAITS:\n"
            "- walk: Primary walking gait with adaptive style based on speed parameter. Fast(300-400ms)=energetic trot-like diagonal movement for quick travel, Medium(400-700ms)=stable 4-beat walking for normal movement, Slow(700+ms)=careful pace-like lateral movement for precise navigation. Requires steps/speed/direction.\n"
            "- turn: Rotate the dog in place. Left turn (direction=1) rotates counterclockwise, right turn (direction=-1) rotates clockwise. Useful for repositioning and changing orientation. Requires steps/speed/direction.\n\n"
            "BEHAVIORS & GESTURES:\n"
            "- sit: Command the dog to sit down in a natural position with rear legs bent and front legs positioned for stability. No parameters needed.\n"
            "- laydown: Make the dog lie flat on the ground in a resting position. No parameters needed.\n"
            "- shake: Perform a full body shake gesture as if shaking off water. Brief rhythmic side-to-side movement. No parameters needed.\n"
            "- jump: Execute a vertical jumping motion with crouch, leap, and landing phases. Dynamic and energetic movement. No parameters needed.\n"
            "- bow: Perform a play bow gesture - front legs lowered while rear stays elevated. Common dog communication posture for inviting play. No parameters needed.\n"
            "- handshake: Offer a paw shake gesture. Use direction=1 for left paw, direction=-1 for right paw. The dog shifts weight appropriately and shakes the chosen paw rhythmically. Requires direction/steps/speed.\n"
            "- highfive: Hold a paw up in a high-five position. Use direction=1 for left paw, direction=-1 for right paw. The speed parameter determines how long to hold the position (in milliseconds). Perfect for interaction demonstrations. Requires direction/speed.\n"
            "- home: Return to the default neutral standing position with all servos centered. Used to reset position or as a stable resting stance. No parameters needed.",
            PropertyList({Property("action", kPropertyTypeString, "home"),
                          Property("steps", kPropertyTypeInteger, 4, 1, 20),
                          Property("speed", kPropertyTypeInteger, 600, 300, 2000),
                          Property("direction", kPropertyTypeInteger, 1, -1, 1)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string action = properties["action"].value<std::string>();
                int steps = properties["steps"].value<int>();
                int speed = properties["speed"].value<int>();
                int direction = properties["direction"].value<int>();

                // Gait actions
                if (action == "walk") {
                    QueueAction(ACTION_WALK, steps, speed, direction);
                    return true;
                } else if (action == "turn") {
                    QueueAction(ACTION_TURN, steps, speed, direction);
                    return true;
                }
                // Behavior actions
                else if (action == "sit") {
                    QueueAction(ACTION_SIT, 1, 0, 0);
                    return true;
                } else if (action == "laydown") {
                    QueueAction(ACTION_LAYDOWN, 1, 0, 0);
                    return true;
                } else if (action == "shake") {
                    QueueAction(ACTION_SHAKE, 1, 0, 0);
                    return true;
                }  else if (action == "jump") {
                    QueueAction(ACTION_JUMP, 1, 0, 0);
                    return true;
                } else if (action == "bow") {
                    QueueAction(ACTION_BOW, 1, 0, 0);
                    return true;
                } else if (action == "handshake") {
                    QueueAction(ACTION_HANDSHAKE, steps, speed, direction);
                    return true;
                } else if (action == "highfive") {
                    QueueAction(ACTION_HIGHFIVE, 1, speed, direction);
                    return true;
                } else if (action == "home") {
                    QueueAction(ACTION_HOME, 1, 500, 0);
                    return true;
                } else {
                    return "Error: Invalid action name. Available actions: walk, turn, sit, laydown, "
                           "shake, jump, bow, handshake, highfive, home";
                }
            });

        mcp_server.AddTool("self.dog.stop", "Emergency stop function that immediately halts all ongoing dog movements and resets to a safe home position. "
                           "Use this to stop any action in progress or when the dog needs to be safely positioned. "
                           "This will cancel any queued movements and return all servos to their neutral positions.",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               if (action_task_handle_ != nullptr) {
                                   vTaskDelete(action_task_handle_);
                                   action_task_handle_ = nullptr;
                               }
                               is_action_in_progress_ = false;
                               xQueueReset(action_queue_);

                               QueueAction(ACTION_HOME, 1, 500, 0);
                               return true;
                           });

        mcp_server.AddTool(
            "self.dog.set_trim",
            "Fine-tune and calibrate individual servo positions to achieve perfect standing posture and movement balance. "
            "Each servo can be adjusted by -50 to +50 degrees to compensate for mechanical variations, assembly differences, or wear. "
            "Settings are automatically saved to persistent memory and applied on startup. "
            "Use this when the dog leans, stands unevenly, or legs don't align properly. "
            "Parameters: servo_type (string): which leg servo to adjust - 'front_left', 'front_right', 'rear_left', 'rear_right'; "
            "trim_value (int, -50 to 50): degrees to offset servo position - positive values rotate clockwise, negative values rotate counter-clockwise.",
            PropertyList({Property("servo_type", kPropertyTypeString, "front_left"),
                          Property("trim_value", kPropertyTypeInteger, 0, -50, 50)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Setting servo trim: %s = %d degrees", servo_type.c_str(),
                         trim_value);

                // Get current trim values
                Settings settings("dog_trims", true);
                int front_left = settings.GetInt("front_left", 0);
                int front_right = settings.GetInt("front_right", 0);
                int rear_left = settings.GetInt("rear_left", 0);
                int rear_right = settings.GetInt("rear_right", 0);

                // Update specified servo trim
                if (servo_type == "front_left") {
                    front_left = trim_value;
                    settings.SetInt("front_left", front_left);
                } else if (servo_type == "front_right") {
                    front_right = trim_value;
                    settings.SetInt("front_right", front_right);
                } else if (servo_type == "rear_left") {
                    rear_left = trim_value;
                    settings.SetInt("rear_left", rear_left);
                } else if (servo_type == "rear_right") {
                    rear_right = trim_value;
                    settings.SetInt("rear_right", rear_right);
                } else {
                    return "Error: Invalid servo type, use: front_left, front_right, rear_left, "
                           "rear_right";
                }

                dog_.SetTrims(front_left, front_right, rear_left, rear_right);

                return "Servo " + servo_type + " trim set to " + std::to_string(trim_value) +
                       " degrees, permanently saved";
            });

        mcp_server.AddTool("self.dog.get_trims", "Retrieve the current calibration trim values for all four servos. "
                           "Returns a JSON object showing the current offset in degrees for each leg servo: "
                           "front_left, front_right, rear_left, rear_right. "
                           "Use this to check current calibration settings or when troubleshooting posture issues. "
                           "Values range from -50 to +50, where 0 means no trim adjustment is applied.",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("dog_trims", false);

                               int front_left = settings.GetInt("front_left", 0);
                               int front_right = settings.GetInt("front_right", 0);
                               int rear_left = settings.GetInt("rear_left", 0);
                               int rear_right = settings.GetInt("rear_right", 0);

                               std::string result =
                                   "{\"front_left\":" + std::to_string(front_left) +
                                   ",\"front_right\":" + std::to_string(front_right) +
                                   ",\"rear_left\":" + std::to_string(rear_left) +
                                   ",\"rear_right\":" + std::to_string(rear_right) + "}";

                               ESP_LOGI(TAG, "Get trim settings: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.dog.get_status", "Check the current operational state of the robot dog. "
                           "Returns 'moving' if an action is currently being executed, or 'idle' if the dog is stationary and ready for new commands. "
                           "Use this to check if previous commands have completed before issuing new movements, or for monitoring the dog's activity in automated sequences.",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        mcp_server.AddTool("self.battery.get_level", "Monitor the power status of the robot dog. "
                           "Returns a JSON object containing battery level (percentage) and charging status. "
                           "Use this to check battery charge, determine when recharging is needed, or monitor power consumption during movement sequences. "
                           "Helps prevent unexpected power loss during operation by allowing proactive battery management.",
                           PropertyList(), [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status = "{\"level\":" + std::to_string(level) +
                                                    ",\"charging\":" + (charging ? "true" : "false") +
                                                    "}";
                               return status;
                           });

        ESP_LOGI(TAG, "MCP tools registration completed");
    }

    ~DogController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static DogController* g_dog_controller = nullptr;

void InitializeDogController() {
    if (g_dog_controller == nullptr) {
        g_dog_controller = new DogController();
        ESP_LOGI(TAG, "Dog controller initialized and MCP tools registered");
    }
}
