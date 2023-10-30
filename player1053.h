
#ifndef PLAYER_RECORDER_H
#define PLAYER_RECORDER_H

#include "vs10xx_uc.h"

int VSTestInitHardware(void);
int VSTestInitSoftware(void);
int VSTestHandleFile(const char *fileName, int record);

void WriteSci(uint8_t addr, uint16_t data);


uint16_t ReadSci(uint8_t addr);

void pin_set_high(int pin);
void pin_set_low(int pin);


int WriteSdi(const uint8_t *data, uint8_t bytes);

void SaveUIState(void);
void RestoreUIState(void);
int GetUICommand(void);

#endif
