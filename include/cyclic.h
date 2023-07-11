#ifndef _CYCLIC_H
#define _CYCLIC_H

#include "globals.h"
#include "senddata.h"
#include "rcommand.h"
#include "bmesensor.h"
#include "display.h"
#include "sds01read.h"
#include "reset.h"

extern Ticker cyclicTimer;

void setCyclicIRQ(void);
void doHousekeeping(void);
uint32_t getFreeRAM();

#endif