#pragma once

#include <stdint.h>
#include "esp_ota_ops.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

void set_notify_enabled(uint16_t handle, bool enabled);
void send_notify(uint16_t attr_handle, uint8_t *new_notify_data, uint8_t new_notify_data_len);