#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static adc_oneshot_unit_handle_t adc;
static void init_adc(adc_channel_t channel)
{
    adc_oneshot_unit_init_cfg_t unit = {};
    unit.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit, &adc));
    adc_oneshot_chan_cfg_t cfg = {};
    cfg.atten = ADC_ATTEN_DB_12;
    cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, channel, &cfg));
}
static int read_adc(adc_channel_t channel)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc, channel, &raw));
    return raw;
}


extern "C" void app_main()
{
    init_adc(ADC_CHANNEL_3);
    adc_cali_handle_t calibration = nullptr;
    adc_cali_curve_fitting_config_t cfg = {};
    cfg.unit_id = ADC_UNIT_1;
    cfg.chan = ADC_CHANNEL_3;
    cfg.atten = ADC_ATTEN_DB_12;
    cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &calibration));
    const float FULL_SCALE_MV = 3100.0f; 
    printf("raw,manual_mv,calibrated_mv,difference_percent\n");
    TickType_t wake = xTaskGetTickCount();
    while (true) {
        int raw = read_adc(ADC_CHANNEL_3), mv;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration, raw, &mv));
        float manual = raw * FULL_SCALE_MV / 4095.0f;
        if (mv > 0)
            printf("%d,%.2f,%d,%.2f\n", raw, manual, mv,
                   100.0f * (manual - mv) / mv);
        else
            printf("%d,%.2f,%d,NA\n", raw, manual, mv);
        xTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
    }
}
