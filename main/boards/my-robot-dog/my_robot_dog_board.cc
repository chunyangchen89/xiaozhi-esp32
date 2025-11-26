#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <wifi_station.h>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/oled_display.h"
#include "system_reset.h"
#include "wifi_board.h"
#include "power_manager.h"

#define TAG "MyRobotDogBoard"

extern void InitializeDogController();

class MyRobotDogBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Button boot_button_;
    PowerManager* power_manager_ = nullptr;

    void InitializeI2c() {
        // Initialize I2C peripheral for OLED display
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = DISPLAY_I2C_SDA_PIN,
            .scl_io_num = DISPLAY_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
        ESP_LOGI(TAG, "I2C bus initialized on SDA=%d, SCL=%d", DISPLAY_I2C_SDA_PIN, DISPLAY_I2C_SCL_PIN);
    }

    void InitializeOledDisplay() {
        // SSD1306/SSD1315 I2C config
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = DISPLAY_I2C_ADDRESS,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
            .scl_speed_hz = 400 * 1000,  // 400kHz I2C speed
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(i2c_bus_, &io_config, &panel_io_));
        ESP_LOGI(TAG, "Panel IO initialized");

        // Install SSD1306 driver (compatible with SSD1315)
        ESP_LOGI(TAG, "Installing SSD1306 driver (compatible with SSD1315)");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = -1;  // No hardware reset pin
        panel_config.bits_per_pixel = 1;   // Monochrome display

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
        ESP_LOGI(TAG, "SSD1306 driver installed");

        // Reset and initialize the display
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        if (esp_lcd_panel_init(panel_) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize display");
            return;
        }

        // Turn display on
        ESP_LOGI(TAG, "Turning display on");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        // Create OledDisplay instance
        display_ = new OledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                    DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        ESP_LOGI(TAG, "OLED display initialized: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting &&
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

    void InitializeDogController() {
        ESP_LOGI(TAG, "Initializing Robot Dog MCP controller");
        ::InitializeDogController();
    }

public:
    MyRobotDogBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2c();
        InitializeOledDisplay();
        InitializeButtons();
        InitializeDogController();
        
        // Initialize power manager
        power_manager_ = new PowerManager(POWER_CHARGE_DETECT_PIN, POWER_ADC_UNIT, POWER_ADC_CHANNEL);
    }

    ~MyRobotDogBoard() {
        if (power_manager_ != nullptr) {
            delete power_manager_;
        }
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override { return display_; }

    virtual Backlight* GetBacklight() override {
        // OLED displays don't have backlights
        static NoBacklight backlight;
        return &backlight;
    }

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        if (power_manager_ != nullptr) {
            level = power_manager_->GetBatteryLevel();
            charging = power_manager_->IsCharging();
            discharging = !charging;
            return true;
        }
        // Fallback if power manager not initialized
        level = 100;
        charging = false;
        discharging = false;
        return false;
    }
};

DECLARE_BOARD(MyRobotDogBoard);
