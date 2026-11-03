#ifndef LCD_COMMANDS_H
#define LCD_COMMANDS_H

#include <stdio.h>

typedef struct {
    uint8_t opcode;
    size_t argc;
    size_t retc;
} lcd_command_t;

extern const lcd_command_t LCD_COM_SWRESET;
extern const lcd_command_t LCD_COM_RDDCOLMOD;
extern const lcd_command_t LCD_COM_SLPOUT;
extern const lcd_command_t LCD_COM_PTLON;
extern const lcd_command_t LCD_COM_DINVOFF;
extern const lcd_command_t LCD_COM_DINVON;
extern const lcd_command_t LCD_COM_DISPOFF;
extern const lcd_command_t LCD_COM_DISPON;
extern const lcd_command_t LCD_COM_CASET;
extern const lcd_command_t LCD_COM_PASET;
extern const lcd_command_t LCD_COM_RAMWR;
extern const lcd_command_t LCD_COM_RAMRD;
extern const lcd_command_t LCD_COM_PLTAR;
extern const lcd_command_t LCD_COM_MADCTL;
extern const lcd_command_t LCD_COM_PIXSET;

#endif