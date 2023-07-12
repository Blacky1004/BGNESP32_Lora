#include "globals.h"
#include "payload.h"

// initialize payload encoder
PayloadConvert payload(PAYLOAD_BUFFER_SIZE);

PayloadConvert::PayloadConvert(uint8_t size) {
  buffer = (uint8_t *)malloc(size);
  cursor = 0;
}

PayloadConvert::~PayloadConvert(void) { free(buffer); }

void PayloadConvert::reset(void) { cursor = 0; }

uint8_t PayloadConvert::getSize(void) { return cursor; }

uint8_t *PayloadConvert::getBuffer(void) { return buffer; }

/* ---------------- plain format without special encoding ---------- */
void PayloadConvert::addByte(uint8_t value) { buffer[cursor++] = (value); }

void PayloadConvert::addCount(uint16_t value, uint8_t snifftype) {
  buffer[cursor++] = highByte(value);
  buffer[cursor++] = lowByte(value);
}

void PayloadConvert::addVoltage(uint16_t value) {
  buffer[cursor++] = highByte(value);
  buffer[cursor++] = lowByte(value);
}

void PayloadConvert::addConfig(systemConfig_t value) {
  buffer[cursor++] = value.loradr;
  buffer[cursor++] = value.txpower;
  buffer[cursor++] = value.adrmode;
  buffer[cursor++] = value.screensaver;
  buffer[cursor++] = value.screenon;
  buffer[cursor++] = value.countermode;
    buffer[cursor++] = value.sendcycle;
  buffer[cursor++] = highByte(value.sleepcycle);
  buffer[cursor++] = lowByte(value.sleepcycle);
  buffer[cursor++] = value.payloadmask;
  buffer[cursor++] = 0; // reserved
  memcpy(buffer + cursor, value.version, 10);
  cursor += 10;
}

void PayloadConvert::addStatus(uint16_t voltage, uint64_t uptime, float cputemp,
                               uint32_t mem, uint8_t reset0,
                               uint32_t restarts) {
  buffer[cursor++] = highByte(voltage);
  buffer[cursor++] = lowByte(voltage);
  buffer[cursor++] = (byte)((uptime & 0xFF00000000000000) >> 56);
  buffer[cursor++] = (byte)((uptime & 0x00FF000000000000) >> 48);
  buffer[cursor++] = (byte)((uptime & 0x0000FF0000000000) >> 40);
  buffer[cursor++] = (byte)((uptime & 0x000000FF00000000) >> 32);
  buffer[cursor++] = (byte)((uptime & 0x00000000FF000000) >> 24);
  buffer[cursor++] = (byte)((uptime & 0x0000000000FF0000) >> 16);
  buffer[cursor++] = (byte)((uptime & 0x000000000000FF00) >> 8);
  buffer[cursor++] = (byte)((uptime & 0x00000000000000FF));
  buffer[cursor++] = (byte)(cputemp);
  buffer[cursor++] = (byte)((mem & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((mem & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((mem & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((mem & 0x000000FF));
  buffer[cursor++] = (byte)(reset0);
  buffer[cursor++] = (byte)((restarts & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((restarts & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((restarts & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((restarts & 0x000000FF));
}

void PayloadConvert::addGPS(gpsStatus_t value) {
#if (HAS_GPS)
  buffer[cursor++] = (byte)((value.latitude & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((value.latitude & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((value.latitude & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((value.latitude & 0x000000FF));
  buffer[cursor++] = (byte)((value.longitude & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((value.longitude & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((value.longitude & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((value.longitude & 0x000000FF));
#if (!PAYLOAD_OPENSENSEBOX)
  buffer[cursor++] = value.satellites;
  buffer[cursor++] = highByte(value.hdop);
  buffer[cursor++] = lowByte(value.hdop);
  buffer[cursor++] = highByte(value.altitude);
  buffer[cursor++] = lowByte(value.altitude);
#endif
#endif
}

void PayloadConvert::addSensor(uint8_t buf[]) {
#if (HAS_SENSORS)
  uint8_t length = buf[0];
  memcpy(buffer, buf + 1, length);
  cursor += length; // length of buffer
#endif
}

void PayloadConvert::addBME(bmeStatus_t value) {
#if (HAS_BME)
  int temperature = (int16_t)(value.temperature * 100); // float -> int
  byte humidity = (byte)(value.humidity *2);     // float -> int
  int pressure = (int)(value.pressure * 10);     // float -> int
  int iaq = (uint16_t)(value.iaq);               // float -> int
  buffer[cursor++] = highByte(temperature);
  buffer[cursor++] = lowByte(temperature);
  buffer[cursor++] = highByte(pressure);
  buffer[cursor++] = lowByte(pressure);
  buffer[cursor++] = humidity;
  buffer[cursor++] = highByte(iaq);
  buffer[cursor++] = lowByte(iaq);
#endif
}

void PayloadConvert::addSDS(sdsStatus_t sds) {
#if (HAS_SDS011)
    int pm10 = ((int)(sds.pm10 *100));
    int pm25 = ((int)(sds.pm25 * 100));
    buffer[cursor++] = highByte(pm10);
    buffer[cursor++] = lowByte(pm10);
    buffer[cursor++] = highByte(pm25);
    buffer[cursor++] = lowByte(pm25);
    ESP_LOGD(TAG, "Adde SDS für Payload. PM10= %d; PM25 = %d", pm10, pm25);
#endif // HAS_SDS011
}

void PayloadConvert::addButton(uint8_t value) {
#ifdef HAS_BUTTON
  buffer[cursor++] = value;
#endif
}

void PayloadConvert::addTime(time_t value) {
  uint32_t time = (uint32_t)value;
  buffer[cursor++] = (byte)((time & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((time & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((time & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((time & 0x000000FF));
}

void PayloadConvert::addChars(char *string, int len) {
  for (int i = 0; i < len; i++)
    addByte(string[i]);
}