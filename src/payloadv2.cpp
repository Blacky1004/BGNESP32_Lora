#include "globals.h"
#include "payloadv2.h"

PayloadConverter myPayload(PAYLOAD_BUFFER_SIZE);

//Konstruktor
PayloadConverter::PayloadConverter(uint8_t size) {
    buffer = (uint8_t *) malloc(size);
    cursor = 0;
}

//Destruktor
PayloadConverter::~PayloadConverter(void) { free(buffer);}

void PayloadConverter::reset(void) {cursor = 0;}

uint8_t PayloadConverter::getSize(void) {return cursor;}

uint8_t *PayloadConverter::getBuffer(void) {return buffer;}

//reine Plainausgabe
#if(PAYLOAD_ENCODER == 1)
void PayloadConverter::addByte(uint8_t value) { buffer[cursor++] = (value);}

void PayloadConverter::addConfig(systemConfig_t value) {
    buffer[cursor++] = value.adrmode;
    buffer[cursor++] = value.loradr;
    buffer[cursor++] = value.screenon;
    buffer[cursor++] = value.sendcycle;
    buffer[cursor++] = value.payloadmask;
    buffer[cursor++] = value.txpower;
    buffer[cursor++] = highByte(value.sleepcycle);
    buffer[cursor++] = lowByte(value.sleepcycle);
    buffer[cursor++] = 0; //reserviert
    memcpy(buffer + cursor, value.version, 10);
    cursor += 10;
}
void PayloadConverter::addStatus(uint64_t uptime, float cputemp, uint32_t mem, uint8_t reset0, uint32_t restarts) {
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
void PayloadConverter::addGPS(gpsStatus_t value) {
#if (HAS_GPS)
  buffer[cursor++] = (byte)((value.latitude & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((value.latitude & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((value.latitude & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((value.latitude & 0x000000FF));
  buffer[cursor++] = (byte)((value.longitude & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((value.longitude & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((value.longitude & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((value.longitude & 0x000000FF));
  buffer[cursor++] = value.satellites;
  buffer[cursor++] = highByte(value.hdop);
  buffer[cursor++] = lowByte(value.hdop);
  buffer[cursor++] = highByte(value.altitude);
  buffer[cursor++] = lowByte(value.altitude);
#endif
}
void PayloadConverter::addSensor(uint8_t buf[]) {
#if (HAS_SENSORS)
  uint8_t length = buf[0];
  memcpy(buffer, buf + 1, length);
  cursor += length; // length of buffer
#endif
}

void PayloadConverter::addBME(bmeStatus_t value) {
#if (HAS_BME)
  int16_t temperature = (int16_t)(value.temperature); // float -> int
  uint16_t humidity = (uint16_t)(value.humidity);     // float -> int
  uint16_t pressure = (uint16_t)(value.pressure);     // float -> int
  uint16_t iaq = (uint16_t)(value.iaq);               // float -> int
  buffer[cursor++] = highByte(temperature);
  buffer[cursor++] = lowByte(temperature);
  buffer[cursor++] = highByte(pressure);
  buffer[cursor++] = lowByte(pressure);
  buffer[cursor++] = highByte(humidity);
  buffer[cursor++] = lowByte(humidity);
  buffer[cursor++] = highByte(iaq);
  buffer[cursor++] = lowByte(iaq);
#endif
}

void PayloadConverter::addSDS(sdsStatus_t sds) {
#if (HAS_SDS011)
  char tempBuffer[10 + 1];
  sprintf(tempBuffer, ",%5.1f", sds.pm10);
  addChars(tempBuffer, strlen(tempBuffer));
  sprintf(tempBuffer, ",%5.1f", sds.pm25);
  addChars(tempBuffer, strlen(tempBuffer));
#endif // HAS_SDS011
}
void PayloadConverter::addTime(time_t value) {
  uint32_t time = (uint32_t)value;
  buffer[cursor++] = (byte)((time & 0xFF000000) >> 24);
  buffer[cursor++] = (byte)((time & 0x00FF0000) >> 16);
  buffer[cursor++] = (byte)((time & 0x0000FF00) >> 8);
  buffer[cursor++] = (byte)((time & 0x000000FF));
}
#elif (PAYLOAD_ENCODER == 2)

#elif (PAYLOAD_ENCODER == 3)

#endif
void PayloadConverter::addChars(char *string, int len) {
  for (int i = 0; i < len; i++)
    addByte(string[i]);
}
