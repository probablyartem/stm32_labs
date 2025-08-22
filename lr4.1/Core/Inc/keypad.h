#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>



void Keypad_Init(void);

void Keypad_Update(void);

uint8_t Keypad_GetLastPressed(void);

#endif /* KEYPAD_H */
