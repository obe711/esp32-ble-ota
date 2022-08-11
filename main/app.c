#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "app.h"
#include "fwhw.h"

void ota_test_task(void *pvParameters)
{
  while (1)
  {
    ESP_LOGI("UPDATE SUCCESS", "FIRMWARE v: %s", FW_VER);
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void run_app(void)
{
  xTaskCreate(ota_test_task, "ota_test_task", 2048, NULL, 5, NULL);
}