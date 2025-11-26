#pragma once

#include <driver/adc.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define TAG "PowerManager"

// Battery calibration: ADC values at 0% and 100% battery
// TODO: Calibrate these values based on your actual hardware
#define BATTERY_ADC_MIN 1500   // ADC value when battery is at 0%
#define BATTERY_ADC_MAX 3500   // ADC value when battery is at 100%
#define BATTERY_SAMPLES 10     // Number of samples for moving average

class PowerManager {
private:
    gpio_num_t charging_pin_;
    adc_unit_t adc_unit_;
    adc_channel_t adc_channel_;
    esp_timer_handle_t timer_handle_ = nullptr;
    int battery_level_ = 100;
    bool is_charging_ = false;
    uint32_t adc_samples_[BATTERY_SAMPLES] = {0};
    int sample_index_ = 0;

    // Timer callback for periodic battery monitoring
    static void BatteryMonitorCallback(void* arg) {
        PowerManager* self = static_cast<PowerManager*>(arg);
        self->UpdateBatteryLevel();
    }

    // Read ADC value and update battery level with moving average
    void UpdateBatteryLevel() {
        // Read ADC value
        adc_oneshot_unit_handle_t adc_handle;
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = adc_unit_,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, adc_channel_, &config));

        int adc_reading = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, adc_channel_, &adc_reading));
        ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));

        // Store sample for moving average
        adc_samples_[sample_index_] = adc_reading;
        sample_index_ = (sample_index_ + 1) % BATTERY_SAMPLES;

        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < BATTERY_SAMPLES; i++) {
            sum += adc_samples_[i];
        }
        uint32_t avg_adc = sum / BATTERY_SAMPLES;

        // Convert ADC value to battery percentage (0-100)
        if (avg_adc <= BATTERY_ADC_MIN) {
            battery_level_ = 0;
        } else if (avg_adc >= BATTERY_ADC_MAX) {
            battery_level_ = 100;
        } else {
            battery_level_ = ((avg_adc - BATTERY_ADC_MIN) * 100) / 
                            (BATTERY_ADC_MAX - BATTERY_ADC_MIN);
        }

        // Check charging status (GPIO pin)
        is_charging_ = (gpio_get_level(charging_pin_) == 1);

        ESP_LOGI(TAG, "Battery: %d%% | ADC: %lu (avg) | Charging: %s", 
                 battery_level_, avg_adc, is_charging_ ? "YES" : "NO");
    }

public:
    PowerManager(gpio_num_t charging_pin, adc_unit_t adc_unit, adc_channel_t adc_channel)
        : charging_pin_(charging_pin), adc_unit_(adc_unit), adc_channel_(adc_channel) {
        
        // Initialize charging detection GPIO (input mode)
        gpio_config_t gpio_cfg = {
            .pin_bit_mask = (1ULL << charging_pin_),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&gpio_cfg));

        ESP_LOGI(TAG, "PowerManager initialized with charging pin=%d, ADC unit=%d, channel=%d",
                 charging_pin_, adc_unit_, adc_channel_);

        // Create periodic timer to monitor battery (every 1 second)
        esp_timer_create_args_t timer_args = {
            .callback = BatteryMonitorCallback,
            .arg = this,
            .name = "battery_monitor",
            .skip_unhandled_events = false,
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle_));
        ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle_, 1000000));  // 1 second

        // Initial battery level read
        UpdateBatteryLevel();
    }

    ~PowerManager() {
        if (timer_handle_ != nullptr) {
            esp_timer_stop(timer_handle_);
            esp_timer_delete(timer_handle_);
        }
    }

    int GetBatteryLevel() const {
        return battery_level_;
    }

    bool IsCharging() const {
        return is_charging_;
    }
};
