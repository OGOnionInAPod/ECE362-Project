#include "commands.h"

const lcd_command_t LCD_COM_SWRESET = {
    .opcode = 0x01,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_RDDCOLMOD = {
    .opcode = 0x0C,
    .argc = 0,
    .retc = 2
};
const lcd_command_t LCD_COM_SLPOUT = {
    .opcode = 0x11,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_PTLON = {
    .opcode = 0x12,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_DINVOFF = {
    .opcode = 0x20,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_DINVON = {
    .opcode = 0x21,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_DISPOFF = {
    .opcode = 0x28,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_DISPON = {
    .opcode = 0x29,
    .argc = 0,
    .retc = 0
};
const lcd_command_t LCD_COM_CASET = {
    .opcode = 0x2a,
    .argc = 4,
    .retc = 0
};
const lcd_command_t LCD_COM_PASET = {
    .opcode = 0x2b,
    .argc = 4,
    .retc = 0
};
const lcd_command_t LCD_COM_RAMWR = {
    .opcode = 0x2c,
    .argc = 0, // This is a dummy. This command has variable argc
    .retc = 0
};
const lcd_command_t LCD_COM_RAMRD = {
    .opcode = 0x2e,
    .argc = 0,
    .retc = 0 // This is a dummy. This command has variable argc
};
const lcd_command_t LCD_COM_PLTAR = {
    .opcode = 0x30,
    .argc = 4,
    .retc = 0
};
const lcd_command_t LCD_COM_MADCTL = {
    .opcode = 0x36,
    .argc = 1,
    .retc = 0
};
const lcd_command_t LCD_COM_PIXSET = {
    .opcode = 0x3a,
    .argc = 1,
    .retc = 0
};