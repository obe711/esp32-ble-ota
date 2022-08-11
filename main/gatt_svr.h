#pragma once

#include "esp_ota_ops.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define LOG_TAG_GATT_SVR "gatt_svr"
#define REBOOT_DEEP_SLEEP_TIMEOUT 500
#define GATT_DEVICE_INFO_UUID 0x180A
#define GATT_MANUFACTURER_NAME_UUID 0x2A29
#define GATT_MODEL_NUMBER_UUID 0x2A24
#define GATT_HARDWARE_REVISION_UUID 0x2A27
#define GATT_FIRMWARE_REVISION_UUID 0x2A26

typedef enum
{
  SVR_CHR_OTA_CONTROL_NOP,
  SVR_CHR_OTA_CONTROL_REQUEST,
  SVR_CHR_OTA_CONTROL_REQUEST_ACK,
  SVR_CHR_OTA_CONTROL_REQUEST_NAK,
  SVR_CHR_OTA_CONTROL_DONE,
  SVR_CHR_OTA_CONTROL_DONE_ACK,
  SVR_CHR_OTA_CONTROL_DONE_NAK,
} svr_chr_ota_control_val_t;

// *****************************************************************************
// **  OTA
// *****************************************************************************
// service: OTA Service
// d6f1d96d-594c-4c53-b1c6-244a1dfde6d8
static const ble_uuid128_t gatt_svr_svc_ota_uuid =
    BLE_UUID128_INIT(0xd8, 0xe6, 0xfd, 0x1d, 0x4a, 0x24, 0xc6, 0xb1, 0x53, 0x4c,
                     0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6);

// characteristic: OTA Control (handle: 29)
// 7ad671aa-21c0-46a4-b722-270e3ae3d830
static const ble_uuid128_t gatt_svr_chr_ota_control_uuid =
    BLE_UUID128_INIT(0x30, 0xd8, 0xe3, 0x3a, 0x0e, 0x27, 0x22, 0xb7, 0xa4, 0x46,
                     0xc0, 0x21, 0xaa, 0x71, 0xd6, 0x7a);

// characteristic: OTA Data
// 23408888-1f40-4cd8-9b89-ca8d45f8a5b0
static const ble_uuid128_t gatt_svr_chr_ota_data_uuid =
    BLE_UUID128_INIT(0xb0, 0xa5, 0xf8, 0x45, 0x8d, 0xca, 0x89, 0x9b, 0xd8, 0x4c,
                     0x40, 0x1f, 0x88, 0x88, 0x40, 0x23);

// *****************************************************************************
// **  App
// *****************************************************************************
// service: App
// 20e28dfb-e639-4d07-9dfb-6c4c3164331c
static const ble_uuid128_t gatt_svr_svc_app_uuid =
    BLE_UUID128_INIT(0x1c, 0x33, 0x64, 0x31, 0x4c, 0x6c, 0xfb, 0x9d, 0x07, 0x4d,
                     0x39, 0xe6, 0xfb, 0x8d, 0xe2, 0x20);
// characteristic: App (handle: 21)
// 4c0d85ca-b73e-46af-adaf-ab8f7f150c4c
static const ble_uuid128_t gatt_svr_chr_app_uuid =
    BLE_UUID128_INIT(0x4c, 0x0c, 0x15, 0x7f, 0x8f, 0xab, 0xaf, 0xad, 0xaf, 0x46,
                     0x3e, 0xb7, 0xca, 0x85, 0x0d, 0x4c);

void gatt_svr_init();