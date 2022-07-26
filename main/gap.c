#include "gap.h"
#include "gatt_svr.h"

uint8_t addr_type;

int gap_event_handler(struct ble_gap_event *event, void *arg);

void advertise()
{
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  int rc;

  memset(&fields, 0, sizeof(fields));

  // flags: discoverability + BLE only
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  // include power levels
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  // include device name
  fields.name = (uint8_t *)device_name;
  fields.name_len = strlen(device_name);
  fields.name_is_complete = 1;

  fields.uuids16 = (ble_uuid16_t[]){
      BLE_UUID16_INIT(0xff02)

      // BLE_UUID128_INIT(0x30, 0xd8, 0xe3, 0x3a, 0x0e, 0x27, 0x22, 0xb7, 0xa4, 0x46,
      //  0xc0, 0x21, 0xaa, 0x71, 0xd6, 0x7a);

      // BLE_UUID16_INIT(0x1811),
      // gatt_svr_svc_ota_uuid
      // BLE breaks when not passed an & before the gatt_svr thing

      // RECEIVED AS 'Unknown Service' and 'D600'
      // BLE_UUID128_INIT(0xd8e6fd1d4a24c6b1534c4c596dd9f1d6),

      // Fails to build.
      // BLE_UUID128_INIT(d8e6fd1d4a24c6b1534c4c596dd9f1d6),

      // RECEIVED AS 'Unknown Service' and 'D800'
      // BLE_UUID128_INIT(0xd8, 0xe6, 0xfd, 0x1d, 0x4a, 0x24, 0xc6, 0xb1, 0x53, 0x4c,
      //  0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6),

      // RECEIVED AS 'Unknown Service' and 'E600'
      // BLE_UUID128_INIT(0xd8e6, 0xfd, 0x1d, 0x4a, 024, 0xc6, 0xb1, 0x53, 0x4c,
      //  0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6),

      // RECIEVED AS 'Unknown Service' and 'F1D6'
      // BLE_UUID16_INIT(0xd8e6fd1d4a24c6b1534c4c596dd9f1d6),
      // BLE_UUID16_INIT(0xd6f1d96d594c4c53b1c6244a1dfde6d8),

      // Fails to build.
      // BLE_UUID16_INIT(0xd8, 0xe6, 0xfd, 0x1d, 0x4a, 0x24, 0xc6, 0xb1, 0x53, 0x4c, 0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6),

      // RECEIVED AS 'Unknown Service' and 'E600'
      // BLE_UUID128_INIT(0xd8e6, 0xfd, 0x1d, 0x4a, 024, 0xc6, 0xb1, 0x53, 0x4c,
      //  0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6),

      // // BLE_UUID16_INIT(gatt_svr_svc_ota_uuid),
      // BLE_UUID128_INIT(&gatt_svr_svc_ota_uuid),
      // BLE_UUID128_INIT(0xd8, 0xe6, 0xfd, 0x1d, 0x4a, 024, 0xc6, 0xb1, 0x53, 0x4c,
      //                  0x4c, 0x59, 0x6d, 0xd9, 0xf1, 0xd6),
      // BLE_UUID128_INIT(0xd6, 0xf1, 0xd9, 0x6d, 0x59, 0x4c, 0x4c, 0x53, 0xb1, 0xc6, 0x24, 0x4a, 0x1d, 0xfd, 0xe6, 0xd8),
      // BLE_UUID128_INIT(0x19, 0x8c, 0x0f, 0xb6, 0x0d, 0x11, 0x11, 0xed, 0x86, 0x1d, 0x02, 0x42, 0xac, 0x12, 0x00, 0x02),
  };

  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0)
  {
    ESP_LOGE(LOG_TAG_GAP, "Error setting advertisement data: rc=%d", rc);
    return;
  }

  // fields.uuids16 = (ble_uuid16_t[]) {
  //     BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)
  // };
  // fields.num_uuids16 = 1;
  // fields.uuids16_is_complete = 1;

  // start advertising
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  //    rc = ble_gap_adv_start(BLE_GAP_DISC_MODE_GEN, BLE_GAP_CONN_MODE_UND,
  //                       NULL, 0, NULL, bleprph_on_connect, NULL);
  rc = ble_gap_adv_start(addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                         gap_event_handler, NULL);
  if (rc != 0)
  {
    ESP_LOGE(LOG_TAG_GAP, "Error enabling advertisement data: rc=%d", rc);
    return;
  }
}

void reset_cb(int reason)
{
  ESP_LOGE(LOG_TAG_GAP, "BLE reset: reason = %d", reason);
}

void sync_cb(void)
{
  // determine best adress type
  ble_hs_id_infer_auto(0, &addr_type);

  // start avertising
  advertise();
}

int gap_event_handler(struct ble_gap_event *event, void *arg)
{
  switch (event->type)
  {
  case BLE_GAP_EVENT_CONNECT:
    // A new connection was established or a connection attempt failed
    ESP_LOGI(LOG_TAG_GAP, "GAP: Connection %s: status=%d",
             event->connect.status == 0 ? "established" : "failed",
             event->connect.status);
    break;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(LOG_TAG_GAP, "GAP: Disconnect: reason=%d\n",
             event->disconnect.reason);

    // Connection terminated; resume advertising
    advertise();
    break;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(LOG_TAG_GAP, "GAP: adv complete");
    advertise();
    break;

  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(LOG_TAG_GAP, "GAP: Subscribe: conn_handle=%d",
             event->connect.conn_handle);
    break;

  case BLE_GAP_EVENT_MTU:
    ESP_LOGI(LOG_TAG_GAP, "GAP: MTU update: conn_handle=%d, mtu=%d",
             event->mtu.conn_handle, event->mtu.value);
    break;
  }

  return 0;
}

void host_task(void *param)
{
  // returns only when nimble_port_stop() is executed
  nimble_port_run();
  nimble_port_freertos_deinit();
}
