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
        ACTION_TROT = 1,
        ACTION_WALK = 2,
        ACTION_PACE = 3,
        ACTION_BOUND = 4,
        ACTION_GALLOP = 5,
        ACTION_TURN = 6,
        ACTION_SIT = 7,
        ACTION_LAYDOWN = 8,
        ACTION_SHAKE = 9,
        ACTION_WIGGLE = 10,
        ACTION_JUMP = 11,
        ACTION_BOW = 12,
        ACTION_HOME = 13,
        ACTION_HANDSHAKE = 14,
        ACTION_HIGHFIVE = 15
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
                    case ACTION_TROT:
                        controller->dog_.Trot(params.steps, params.speed, params.direction);
                        break;
                    case ACTION_WALK:
                        controller->dog_.Walk(params.steps, params.speed, params.direction);
                        break;
                    case ACTION_PACE:
                        controller->dog_.Pace(params.steps, params.speed, params.direction);
                        break;
                    case ACTION_BOUND:
                        controller->dog_.Bound(params.steps, params.speed, params.direction);
                        break;
                    case ACTION_GALLOP:
                        controller->dog_.Gallop(params.steps, params.speed, params.direction);
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
                    case ACTION_WIGGLE:
                        controller->dog_.Wiggle(params.steps, params.speed);
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

        // Unified action tool for robot dog
        mcp_server.AddTool(
            "self.dog.action",
            "Execute robot dog action. action: action name; parameters based on action type: "
            "direction: 1=forward/left, -1=backward/right; "
            "steps: action cycles, 1-20; speed: action period in ms, 300-2000, lower=faster. "
            "Gaits: trot (diagonal gait, needs steps/speed/direction), "
            "walk (4-beat gait, needs steps/speed/direction), "
            "pace (lateral gait, needs steps/speed/direction), "
            "bound (front/rear together, needs steps/speed/direction), "
            "gallop (asymmetric running, needs steps/speed/direction), "
            "turn (rotate in place, needs steps/speed/direction). "
            "Behaviors: sit (sit down), laydown (lay flat), shake (body shake), "
            "wiggle (tail wiggle, needs steps/speed), jump (jump up), bow (play bow), "
            "handshake (shake paw, needs direction/steps/speed, direction: 1=left paw, -1=right paw), "
            "highfive (high-five gesture, needs direction/speed, direction: 1=left paw, -1=right paw, speed=hold_time in ms), "
            "home (return to rest position)",
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
                if (action == "trot") {
                    QueueAction(ACTION_TROT, steps, speed, direction);
                    return true;
                } else if (action == "walk") {
                    QueueAction(ACTION_WALK, steps, speed, direction);
                    return true;
                } else if (action == "pace") {
                    QueueAction(ACTION_PACE, steps, speed, direction);
                    return true;
                } else if (action == "bound") {
                    QueueAction(ACTION_BOUND, steps, speed, direction);
                    return true;
                } else if (action == "gallop") {
                    QueueAction(ACTION_GALLOP, steps, speed, direction);
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
                } else if (action == "wiggle") {
                    QueueAction(ACTION_WIGGLE, steps, speed, 0);
                    return true;
                } else if (action == "jump") {
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
                    return "Error: Invalid action name. Available actions: trot, walk, pace, "
                           "bound, gallop, turn, sit, laydown, shake, wiggle, jump, bow, handshake, highfive, home";
                }
            });

        mcp_server.AddTool("self.dog.stop", "Stop all actions and return to home position",
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
            "Calibrate individual servo position. Set trim parameters to adjust the dog's "
            "standing posture. Settings are saved permanently. "
            "servo_type: servo type (front_left/front_right/rear_left/rear_right); "
            "trim_value: trim value (-50 to 50 degrees)",
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

        mcp_server.AddTool("self.dog.get_trims", "Get current servo trim settings", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
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

        mcp_server.AddTool("self.dog.get_status", "Get robot dog status, returns moving or idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        mcp_server.AddTool("self.battery.get_level", "Get robot battery level and charging status",
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
