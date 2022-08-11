#include "gatt_svr.h"
#include "ble_notify.h"
#include "fwhw.h"

uint8_t gatt_svr_chr_ota_control_val;
uint8_t gatt_svr_chr_ota_data_val[512];
uint8_t gatt_svr_chr_app_data_val[512];

uint16_t ota_control_val_handle;
uint16_t ota_data_val_handle;
uint16_t app_data_val_handle;

const esp_partition_t *update_partition;
esp_ota_handle_t update_handle;
bool updating = false;
uint16_t num_pkgs_received = 0;
uint16_t packet_size = 0;

static const char *manuf_name = (char *)MANUF_NAME;
static const char *model_num = (char *)MODEL_NUM;
static const char *hardware_num = (char *)HARDWARE_NUM;
static const char *fw_ver = (char *)FW_VER;

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len);

static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg);

static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg);

static int gatt_svr_chr_app_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt,
                               void *arg);

static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg);

/* ---------------------------------------------------------------------------------- */

/**
 *
 *   ╔═══════════════════════════════════════════════════════════════════════════╗
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                                                                               ║
 * ║  GATT CONFIGURATION                                                           ║
 * ║                                                                               ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 *   ╚═══════════════════════════════════════════════════════════════════════════╝
 *
 */

/**
 * @brief GATT server table.
 */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {// Service: Device Information
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(GATT_DEVICE_INFO_UUID),
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 // Characteristic: Manufacturer Name
                 .uuid = BLE_UUID16_DECLARE(GATT_MANUFACTURER_NAME_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {
                 // Characteristic: Model Number
                 .uuid = BLE_UUID16_DECLARE(GATT_MODEL_NUMBER_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {
                 // Characteristic: Hardware Revision
                 .uuid = BLE_UUID16_DECLARE(GATT_HARDWARE_REVISION_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {
                 // Characteristic: Firmware Revision
                 .uuid = BLE_UUID16_DECLARE(GATT_FIRMWARE_REVISION_UUID),
                 .access_cb = gatt_svr_chr_access_device_info,
                 .flags = BLE_GATT_CHR_F_READ,
             },
             {
                 0,
             },
         }},
    {
        // Service: APP
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_app_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    // characteristic: App
                    .uuid = &gatt_svr_chr_app_uuid.u,
                    .access_cb = gatt_svr_chr_app_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                             BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &app_data_val_handle,
                },
                {
                    0,
                }},
    },

    {
        // service: OTA Service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_ota_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    // characteristic: OTA control
                    .uuid = &gatt_svr_chr_ota_control_uuid.u,
                    .access_cb = gatt_svr_chr_ota_control_cb,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                             BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &ota_control_val_handle,
                },
                {
                    // characteristic: OTA data
                    .uuid = &gatt_svr_chr_ota_data_uuid.u,
                    .access_cb = gatt_svr_chr_ota_data_cb,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &ota_data_val_handle,
                },
                {
                    0,
                }},
    },

    {
        0,
    },
};

/**
 * @brief BLE Device Info callback
 *
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt Context
 * @param arg Arguments
 * @return int
 */
static int gatt_svr_chr_access_device_info(uint16_t conn_handle,
                                           uint16_t attr_handle,
                                           struct ble_gatt_access_ctxt *ctxt,
                                           void *arg)
{
  uint16_t uuid;
  int rc;

  uuid = ble_uuid_u16(ctxt->chr->uuid);

  if (uuid == GATT_FIRMWARE_REVISION_UUID)
  {
    rc = os_mbuf_append(ctxt->om, fw_ver, strlen(fw_ver));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  if (uuid == GATT_HARDWARE_REVISION_UUID)
  {
    rc = os_mbuf_append(ctxt->om, hardware_num, strlen(hardware_num));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  if (uuid == GATT_MODEL_NUMBER_UUID)
  {
    rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  if (uuid == GATT_MANUFACTURER_NAME_UUID)
  {
    rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  assert(0);
  return BLE_ATT_ERR_UNLIKELY;
} /**
   *
   *   ╔═══════════════════════════════════════════════════════════════════════════╗
   * ╔═══════════════════════════════════════════════════════════════════════════════╗
   * ║                                                                               ║
   * ║  GATT SERVICE FUNTIONS                                                        ║
   * ║                                                                               ║
   * ╚═══════════════════════════════════════════════════════════════════════════════╝
   *   ╚═══════════════════════════════════════════════════════════════════════════╝
   *
   */

/**
 * @brief Get client Write value from context
 *
 * @param om Buffer pointer
 * @param min_len Minimum length
 * @param max_len Maximum length
 * @param dst Out copy destination
 * @param len Out copy length
 * @return int
 */
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len)
{
  uint16_t om_len;
  int rc;

  om_len = OS_MBUF_PKTLEN(om);
  if (om_len < min_len || om_len > max_len)
  {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
  if (rc != 0)
  {
    return BLE_ATT_ERR_UNLIKELY;
  }

  return 0;
}

/**
 * @brief Update OTA service
 *
 * @param conn_handle Connection handle
 */
static void update_ota_control(uint16_t conn_handle)
{
  struct os_mbuf *om;
  esp_err_t err;
  ESP_LOGI(LOG_TAG_GATT_SVR, "update_ota_control: %d", gatt_svr_chr_ota_control_val);

  // check which value has been received
  switch (gatt_svr_chr_ota_control_val)
  {
  case SVR_CHR_OTA_CONTROL_REQUEST:
    // OTA request
    ESP_LOGI(LOG_TAG_GATT_SVR, "OTA has been requested via BLE.");
    // get the next free OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    // start the ota update
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES,
                        &update_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_begin failed (%s)",
               esp_err_to_name(err));
      esp_ota_abort(update_handle);
      gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_NAK;
    }
    else
    {
      gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_REQUEST_ACK;
      updating = true;

      // retrieve the packet size from OTA data
      packet_size =
          (gatt_svr_chr_ota_data_val[1] << 8) + gatt_svr_chr_ota_data_val[0];
      ESP_LOGI(LOG_TAG_GATT_SVR, "Packet size is: %d", packet_size);

      num_pkgs_received = 0;
    }

    // notify the client via BLE that the OTA has been acknowledged (or not)
    om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                               sizeof(gatt_svr_chr_ota_control_val));
    ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
    ESP_LOGI(LOG_TAG_GATT_SVR, "OTA request acknowledgement has been sent.");

    break;

  case SVR_CHR_OTA_CONTROL_DONE:

    updating = false;

    // end the OTA and start validation
    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
      if (err == ESP_ERR_OTA_VALIDATE_FAILED)
      {
        ESP_LOGE(LOG_TAG_GATT_SVR,
                 "Image validation failed, image is corrupted!");
      }
      else
      {
        ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_end failed (%s)!",
                 esp_err_to_name(err));
      }
    }
    else
    {
      // select the new partition for the next boot
      err = esp_ota_set_boot_partition(update_partition);
      if (err != ESP_OK)
      {
        ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_set_boot_partition failed (%s)!",
                 esp_err_to_name(err));
      }
    }

    // set the control value
    if (err != ESP_OK)
    {
      gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_NAK;
    }
    else
    {
      gatt_svr_chr_ota_control_val = SVR_CHR_OTA_CONTROL_DONE_ACK;
    }

    // notify the client via BLE that DONE has been acknowledged
    om = ble_hs_mbuf_from_flat(&gatt_svr_chr_ota_control_val,
                               sizeof(gatt_svr_chr_ota_control_val));
    ble_gattc_notify_custom(conn_handle, ota_control_val_handle, om);
    ESP_LOGI(LOG_TAG_GATT_SVR, "OTA DONE acknowledgement has been sent.");

    // restart the ESP to finish the OTA
    if (err == ESP_OK)
    {
      ESP_LOGI(LOG_TAG_GATT_SVR, "Preparing to restart!");
      vTaskDelay(pdMS_TO_TICKS(REBOOT_DEEP_SLEEP_TIMEOUT));
      esp_restart();
    }

    break;

  default:
    break;
  }
}

/**
 *
 *   ╔═══════════════════════════════════════════════════════════════════════════╗
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                                                                               ║
 * ║  OTA CALLBACKS                                                                ║
 * ║                                                                               ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 *   ╚═══════════════════════════════════════════════════════════════════════════╝
 *
 */

/**
 * @brief OTA Control characteristic callback
 *
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt Context
 * @param arg Arguments
 * @return int
 */
static int gatt_svr_chr_ota_control_cb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg)
{
  int rc;
  uint8_t length = sizeof(gatt_svr_chr_ota_control_val);

  // LOG RAW DATA
  ESP_LOGI(LOG_TAG_GATT_SVR, "update_ota_control: %d", ctxt->op);

  switch (ctxt->op)
  {
  case BLE_GATT_ACCESS_OP_READ_CHR:
    // a client is reading the current value of ota control
    rc = os_mbuf_append(ctxt->om, &gatt_svr_chr_ota_control_val, length);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    break;

  case BLE_GATT_ACCESS_OP_WRITE_CHR:
    // a client is writing a value to ota control
    rc = gatt_svr_chr_write(ctxt->om, 1, length,
                            &gatt_svr_chr_ota_control_val, NULL);
    // update the OTA state with the new value
    update_ota_control(conn_handle);
    return rc;
    break;

  default:
    break;
  }

  // this shouldn't happen
  assert(0);
  return BLE_ATT_ERR_UNLIKELY;
}

/**
 * @brief OTA Data characteristic callback
 *
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt Context
 * @param arg Arguments
 * @return int
 */
static int gatt_svr_chr_ota_data_cb(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt,
                                    void *arg)
{
  int rc;
  esp_err_t err;

  // store the received data into gatt_svr_chr_ota_data_val
  rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_ota_data_val),
                          gatt_svr_chr_ota_data_val, NULL);

  // LOG RAW DATA
  // ESP_LOG_BUFFER_HEX_LEVEL(LOG_TAG_GATT_SVR, gatt_svr_chr_ota_data_val, sizeof gatt_svr_chr_ota_data_val, ESP_LOG_INFO);

  // write the received packet to the partition
  if (updating)
  {
    err = esp_ota_write(update_handle, (const void *)gatt_svr_chr_ota_data_val,
                        packet_size);
    if (err != ESP_OK)
    {
      ESP_LOGE(LOG_TAG_GATT_SVR, "esp_ota_write failed (%s)!",
               esp_err_to_name(err));
    }

    num_pkgs_received++;
    ESP_LOGI(LOG_TAG_GATT_SVR, "Received packet %d", num_pkgs_received);
  }

  return rc;
}

/**
 *
 *   ╔═══════════════════════════════════════════════════════════════════════════╗
 * ╔═══════════════════════════════════════════════════════════════════════════════╗
 * ║                                                                               ║
 * ║  APP CALLBACKS                                                                ║
 * ║                                                                               ║
 * ╚═══════════════════════════════════════════════════════════════════════════════╝
 *   ╚═══════════════════════════════════════════════════════════════════════════╝
 *
 */

/**
 * @brief App characteristic callback
 *
 * @param conn_handle Connection handle
 * @param attr_handle Attribute handle
 * @param ctxt Context
 * @param arg Arguments
 * @return int
 */
static int gatt_svr_chr_app_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt,
                               void *arg)
{
  int rc = gatt_svr_chr_write(ctxt->om, 1, sizeof(gatt_svr_chr_app_data_val),
                              gatt_svr_chr_app_data_val, NULL);

  if (rc == 0)
  {
    esp_log_buffer_hex(LOG_TAG_GATT_SVR, gatt_svr_chr_app_data_val, 9);
    uint8_t cmd = (uint8_t)gatt_svr_chr_app_data_val[0];
    switch (cmd)
    {
    case 0xA0:
      ESP_LOGI(LOG_TAG_GATT_SVR, "Command A0");

      break;
    case 0xB0:
      ESP_LOGI(LOG_TAG_GATT_SVR, "Command B0");
      break;
    default:
      ESP_LOGI("Command not found", "V1");
      break;
    }
  }

  return rc;
}

void gatt_svr_init()
{
  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_gatts_count_cfg(gatt_svr_svcs);
  ble_gatts_add_svcs(gatt_svr_svcs);
}
