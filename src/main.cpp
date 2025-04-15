#include <Arduino.h>

#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "esp32/ulp.h"
#include "ulp_main.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static void init_ulp_program();
static void start_ulp_program();

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");
  Serial.printf("ESP-IDF: %d.%d.%d, ESP Arduino: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH, ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  // Serial.printf("CONFIG_ULP_COPROC_RESERVE_MEM: %d\n", CONFIG_ULP_COPROC_RESERVE_MEM);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause == ESP_SLEEP_WAKEUP_ULP)
  {
    printf("ULP wakeup\n");
  }
  if (cause == ESP_SLEEP_WAKEUP_TIMER)
  {
    printf("Timer wakeup\n");
  }

  if (cause != ESP_SLEEP_WAKEUP_ULP && cause != ESP_SLEEP_WAKEUP_TIMER)
  {
    printf("Not ULP/timer wakeup\n");
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /*
    // new esp-idf example code uses this block but can't access these functions here
    // https://github.com/espressif/esp-idf/blob/master/examples/system/ulp/ulp_fsm/ulp_adc/main/ulp_adc_example_main.c
    ulp_adc_cfg_t cfg = {
        .adc_n = 0,   // ADC1
        .channel = 6, // ch6
        .width = 0,   // default
        .atten = 3,   // ADC_ATTEN_DB_12
        .ulp_mode = ADC_ULP_MODE_FSM,
    };
    ESP_ERROR_CHECK(ulp_adc_init(&cfg));
    */

    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();

    /* Set ULP wake up period to 100ms */
    ulp_set_wakeup_period(0, 100 * 1000);
  }
  else
  {
    printf("Deep sleep wakeup\n");
    printf("ULP code version %d\n", ulp_cur_version & UINT16_MAX);
    printf("ULP start counter: %d\n", ulp_start_counter & UINT16_MAX);
    printf("ULP wake counter: %d\n", ulp_wakeup_counter & UINT16_MAX);
  }

  delay(100);
  ulp_wakeup_counter = 0;
  ulp_wakeup_counter_goal = 50;
  // ulp_do_adc = 0; // don't perform ADC
  ulp_do_adc = 1; // perform ADC, will get stuck on new SDK

  /* Start the program */
  esp_err_t err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
  ESP_ERROR_CHECK(err);

  // optional "timeout" timer. doesn't seem to affect the outcode but prevents the sleep from being forever and allows to see counters.
  // Serial.println("TEST: sleeping for 20 sec");
  // esp_sleep_enable_timer_wakeup((uint64_t)20 * 1000000L); // wake after specified time

  printf("Entering deep sleep\n\n");
  ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
  delay(100);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_deep_sleep_start();
}

void loop()
{
}
