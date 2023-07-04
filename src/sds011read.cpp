#if (HAS_SDS011)
#include "sds01read.h"

SdsDustSensor sds(Serial1);

bool isSDS011Active = false;
static float pm10 = 0.0, pm25 = 0.0;

bool sds011_init() {
    Serial1.begin(9600, SERIAL_8N1, SDS_RX, SDS_TX);
    sds.begin();
    sds011_wakeup();
    ESP_LOGI(TAG, "SDS011: %s", sds.queryFirmwareVersion().toString().c_str());
    sds.setQueryReportingMode();

    return true;
}

void sds011_loop() {
    if(isSDS011Active) {
        PmResult pm = sds.queryPm();
        if(!pm.isOk()) {
            ESP_LOGE(TAG, "SDS011: Abfragefehler: %s", pm.statusToString().c_str());
            pm10 = pm25 = 0.0;
        } else {
            ESP_LOGI(TAG, "SDS011: %s", pm.toString().c_str());
            pm10 = pm.pm10;
            pm25 = pm.pm25;
        }
        ESP_LOGD(TAG, "SDS011: Gehe in SleepMode.");
        sds011_sleep();
    }
}

void sds011_store(sdsStatus_t *sds_store) {
    sds_store->pm10 = pm10;
    sds_store->pm25 = pm25;
}

void sds011_sleep(void) {
    WorkingStateResult state = sds.sleep();
    isSDS011Active = state.isWorking();
}

void sds011_wakeup() {
    WorkingStateResult state = sds.wakeup();
    isSDS011Active = state.isWorking();
    ESP_LOGD(TAG, "SDS011: %s", state.toString().c_str());
}

#endif