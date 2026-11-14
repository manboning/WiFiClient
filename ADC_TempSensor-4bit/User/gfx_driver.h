#ifndef __GFX_DRIVER_H
#define __GFX_DRIVER_H



#include <stdint.h> /* 颜色定义 (RGB565) */



#define GFX_COLOR_BLACK   0x0000
#define GFX_COLOR_WHITE   0xFFFF
#define GFX_COLOR_RED     0xF800
#define GFX_COLOR_GREEN   0x07E0
#define GFX_COLOR_BLUE    0x001F
#define GFX_COLOR_CYAN    0x07FF
#define GFX_COLOR_MAGENTA 0xF81F
#define GFX_COLOR_YELLOW  0xFFE0
#define GFX_COLOR_GRAY    0x8410/* 基础函数 (需要对接您的 LCD 驱动) */
void GFX_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* 高级图形函数 */
void GFX_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void GFX_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void GFX_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
void GFX_DrawMonoBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bmp, uint16_t color);

void LCD_DrawChar_DMA(uint16_t x, uint16_t y, char c, uint8_t is_large);


#endif





