#include "ble_notify.h"

#define OTA_ATTR_HANDLE 25
#define APP_ATTR_HANDLE 21

struct notify_check
{
  bool is_notify_enabled;
  uint16_t handle_notify;
};

enum
{
  APP_NOTIFY_HANDLE,
  NOTIFY_NUM_IDX
};

static struct notify_check is_notify_tab[NOTIFY_NUM_IDX] = {
    [APP_NOTIFY_HANDLE] = {
        .is_notify_enabled = false,
        .handle_notify = APP_ATTR_HANDLE,
    },
};

void set_notify_enabled(uint16_t handle, bool enabled)
{
  ESP_LOGI("NOTIFY", "Handle: %d", handle);
  switch (handle)
  {
  case APP_ATTR_HANDLE:
    ESP_LOGI("NOTIFY", "APP NOTIFY ENABLED: %d", enabled);
    break;
  case OTA_ATTR_HANDLE:
    ESP_LOGI("NOTIFY", "OTA NOTIFY ENABLED: %d", enabled);
    break;
  default:
    ESP_LOGI("NOTIFY", "handler not found: %d", handle);
    break;
  }
}

void send_notify(uint16_t attr_handle, uint8_t *new_notify_data, uint8_t new_notify_data_len)
{

  struct os_mbuf *om;
  om = ble_hs_mbuf_from_flat(new_notify_data,
                             new_notify_data_len);

  switch (attr_handle)
  {
  case APP_ATTR_HANDLE:
    if (is_notify_tab[APP_NOTIFY_HANDLE].is_notify_enabled)
    {
      esp_err_t err = ble_gattc_notify_custom(0, attr_handle, om);
      if (err != ESP_OK)
      {
        ESP_LOGE("NOTIFY", "Error sending notify: %d", err);
      }
      ESP_LOGI("NOTIFY", "Sent to APP");
    }
    break;
  default:
    ESP_LOGI("NOTIFY", "No active notify");
    break;
  }
}