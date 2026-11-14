#include "ui_layout.h"
#include "gfx_driver.h"
#include "icons.h"
#include <stdio.h>
#include <string.h>
#include <math.h> // 需要 fabsf

/* 参数配置 */
#define FONT_BIG_W    24
#define FONT_BIG_H    40
#define FONT_SMALL_W  14
#define FONT_SMALL_H  24

/* 外部函数声明 */
extern void LCD_DrawChar_DMA(uint16_t x, uint16_t y, char c, uint8_t is_large);

/* 显存缓存 */
static char cache_v[12] = "           "; 
static char cache_i[12] = "           ";
static char cache_p[12] = "           ";
static char cache_ah[12] = "           ";
static char cache_wh[12] = "           ";
static int8_t cache_dir = -99;

/**
 * @brief 脏字符刷新
 */
static void UI_Refresh_Line_DMA(uint16_t x, uint16_t y, char* new_str, char* cache_str, uint8_t is_large) {
    uint8_t font_w, font_h; 
    int i; 

    font_w = is_large ? FONT_BIG_W : FONT_SMALL_W;
    font_h = is_large ? FONT_BIG_H : FONT_SMALL_H;
    
    for (i = 0; i < 10; i++) { 
        if (new_str[i] == '\0') {
            if (cache_str[i] != '\0' && cache_str[i] != ' ') {
                 GFX_FillRect(x + (i * font_w), y, font_w, font_h, 0x0000);
                 cache_str[i] = '\0';
            }
            break; 
        }

        if (new_str[i] != cache_str[i]) {
            LCD_DrawChar_DMA(x + (i * font_w), y, new_str[i], is_large);
            cache_str[i] = new_str[i]; 
        }
    }
}

/**
 * @brief 箭头刷新 (用于指示方向)
 */
static void UI_Refresh_Arrow(int8_t dir) {
    uint16_t x = 260; 
    uint16_t y = 50;  
    
    GFX_FillRect(x+2, y+2, 36, 36, 0x0000); // 清除旧箭头

    if (dir == 1) {
        GFX_DrawMonoBitmap(x+8, y+12, 24, 15, Bitmap_Arrow_Right, 0xFFFF);
    } else if (dir == -1) {
        GFX_DrawMonoBitmap(x+8, y+12, 24, 15, Bitmap_Arrow_Left, 0xFFFF);
    }
    // dir=0 不画
}

/**
 * @brief 浮点格式化 (强制绝对值)
 */
static void Format_Float(char* buf, float val, uint8_t is_large) {
    float abs_val = fabsf(val); // 【关键】取绝对值，防止生成负号

    if (is_large) {
        if (abs_val < 10.0f) sprintf(buf, "%06.3f", abs_val);
        else if (abs_val < 100.0f) sprintf(buf, "%06.3f", abs_val);
        else sprintf(buf, "%06.2f", abs_val);
    } else {
        if (abs_val < 10.0f) sprintf(buf, "%06.4f", abs_val);
        else if (abs_val < 100.0f) sprintf(buf, "%06.3f", abs_val);
        else sprintf(buf, "%06.2f", abs_val);
    }
}

/**
 * @brief 主刷新函数
 */
void UI_Update_Values(float v, float i, float p, float ah, float wh, int8_t dir) {
    char buf[16];

    // 1. 电压
    sprintf(buf, "%06.3f", fabsf(v));
    UI_Refresh_Line_DMA(100, 5, buf, cache_v, 1);

    // 2. 电流 (绝对值)
    Format_Float(buf, i, 1);
    UI_Refresh_Line_DMA(100, 50, buf, cache_i, 1);
    
    // 箭头 (指示方向)
    if (dir != cache_dir) {
        UI_Refresh_Arrow(dir);
        cache_dir = dir;
    }

    // 3. 功率
    Format_Float(buf, p, 1);
    UI_Refresh_Line_DMA(100, 95, buf, cache_p, 1);

    // 4. AH
    Format_Float(buf, ah, 0);
    UI_Refresh_Line_DMA(64, 143, buf, cache_ah, 0); 

    // 5. WH
    Format_Float(buf, wh, 0);
    UI_Refresh_Line_DMA(224, 143, buf, cache_wh, 0); 
}


