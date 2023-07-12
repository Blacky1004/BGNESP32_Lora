#ifndef _PAYLOAD_H_
#define _PAYLOAD_H_

// #include "sensor.h"
#include "sds01read.h"
#include "gpsread.h"


class PayloadConvert {
public:
  PayloadConvert(uint8_t size);
  ~PayloadConvert();

  void reset(void);
  uint8_t getSize(void);
  uint8_t *getBuffer(void);
  void addByte(uint8_t value);
  void addCount(uint16_t value, uint8_t sniffytpe);
  void addConfig(systemConfig_t value);
  void addStatus(uint16_t voltage, uint64_t uptime, float cputemp, uint32_t mem,
                 uint8_t reset0, uint32_t restarts);
  void addVoltage(uint16_t value);
  void addGPS(gpsStatus_t value);
  void addBME(bmeStatus_t value);
  void addButton(uint8_t value);
  void addSensor(uint8_t[]);
  void addTime(time_t value);
  void addSDS(sdsStatus_t value);

private:
  void addChars( char* string, int len);
private:
  uint8_t *buffer;
  uint8_t cursor;
};

extern PayloadConvert payload;

#endif // _PAYLOAD_H_