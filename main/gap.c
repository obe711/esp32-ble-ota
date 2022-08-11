#include "gap.h"
#include "ble_notify.h"
#include "fwhw.h"

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
  // fields.tx_pwr_lvl_is_present = 1;
  // fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  // include device name
  fields.name = (uint8_t *)device_name;
  fields.name_len = strlen(device_name);
  fields.name_is_complete = 1;

  // fields.uuids128 = (ble_uuid128_t[]){
  //     BLE_UUID128_INIT(0x1c, 0x33, 0x64, 0x31, 0x4c, 0x6c, 0xfb, 0x9d, 0x07, 0x4d,
  //                      0x39, 0xe6, 0xfb, 0x8d, 0xe2, 0x20)};
  // fields.num_uuids128 = 1;
  // fields.uuids128_is_complete = 1;

  fields.uuids16 = (ble_uuid16_t[]){
      BLE_UUID16_INIT(0x00ff)};
  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0)
  {
    ESP_LOGE(LOG_TAG_GAP, "Error setting advertisement data: rc=%d", rc);
    return;
  }

  // start advertising
  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
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
    set_notify_enabled(event->subscribe.attr_handle, event->subscribe.cur_notify);
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
  nimble_port_run();
  nimble_port_freertos_deinit();
}
