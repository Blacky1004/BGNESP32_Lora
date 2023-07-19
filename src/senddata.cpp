// Basic Config
#include "senddata.h"

// void setSendIRQ(TimerHandle_t xTimer) {
//  xTaskNotify(irqHandlerTask, SENDCYCLE_IRQ, eSetBits);
//}

// void setSendIRQ(void) { setSendIRQ(NULL); }

void setSendIRQ(void) { xTaskNotify(irqHandlerTask, SENDCYCLE_IRQ, eSetBits); }

void SendPayload(uint8_t port) {
  ESP_LOGD(TAG, "sending Payload for Port %d", port);

  MessageBuffer_t SendBuffer; // contains MessageSize, MessagePort, Message[]

  SendBuffer.MessageSize = payload.getSize();
  SendBuffer.MessagePort = port;
  memcpy(SendBuffer.Message, payload.getBuffer(), SendBuffer.MessageSize);

// enqueue message in device's send queues
#if (HAS_LORA)
  lora_enqueuedata(&SendBuffer);
#endif
#ifdef HAS_SPI
  spi_enqueuedata(&SendBuffer);
#endif
#ifdef HAS_MQTT
  mqtt_enqueuedata(&SendBuffer);
#endif
} // SendPayload

void sendData() {
  uint8_t bitmask = cfg.payloadmask;
  uint8_t mask = 1;

#if (HAS_GPS)
  gpsStatus_t gps_status;
#endif
#if (HAS_SDS011)
  sdsStatus_t sds_status;
#endif

while (bitmask) {
    ESP_LOGD(TAG,"Verarbeite MASKE: %d", mask);
    switch (bitmask & mask) {

    case COUNT_DATA:
      payload.reset();     

      #if (HAS_SDS011)
            sds011_store(&sds_status);
            payload.addSDS(sds_status);
            SendPayload(SENSOR1PORT);
      #endif

      #ifdef HAS_DISPLAY
            //dp_plotCurve(count.pax, true);
      #endif
      
      break; // case COUNTDATA

      #if (HAS_BME)
          case MEMS_DATA:
            payload.reset();
            payload.addBME(bme_status);
            SendPayload(BMEPORT);
            break;
      #endif

      #if (HAS_GPS)
          case GPS_DATA:
            if (GPSPORT != COUNTERPORT) {
              // send GPS position only if we have a fix
              if (gps_hasfix()) {
                gps_storelocation(&gps_status);
                payload.reset();
                payload.addGPS(gps_status);
                SendPayload(GPSPORT);
              } else
                ESP_LOGD(TAG, "No valid GPS position");
            }
            break;
      #endif
    } // switch
    bitmask &= ~mask;
    mask <<= 1;
  } // while (bitmask)
} // sendData()

void flushQueues(void) {
  //rcmd_queuereset();
#if (HAS_LORA)
  lora_queuereset();
#endif
#ifdef HAS_SPI
  spi_queuereset();
#endif
#ifdef HAS_MQTT
  mqtt_queuereset();
#endif
}

bool allQueuesEmtpy(void) {
  uint32_t rc = 0;
#if (HAS_LORA)
  rc += lora_queuewaiting();
#endif
#ifdef HAS_SPI
  rc += spi_queuewaiting();
#endif
#ifdef HAS_MQTT
  rc += mqtt_queuewaiting();
#endif
  return (rc == 0) ? true : false;
}
