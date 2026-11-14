#include "gfx_driver.h"
#include "lcd.h"// 【关键】引用你原本可用的驱动
#include "font_assets.h"// 引用字库
/* ==================================================================
 * 1. 接口适配 (解决 Undefined symbol 报错)
 * ================================================================== *//**
 * @brief 适配 LCD_SetWindow -> LCD_Address_Set
 */
 void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
    LCD_Address_Set(x1, y1, x2, y2);
}

/**
 * @brief 适配 LCD_WriteData16 -> LCD_WR_DATA
 */
void LCD_WriteData16(uint16_t data){
    LCD_WR_DATA(data);
}

/* ==================================================================
 * 2. 基础绘图实现 (调用 lcd.c)
 * ================================================================== */
void GFX_DrawPixel(uint16_t x, uint16_t y, uint16_t color){
    LCD_DrawPoint(x, y, color);
}

void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){
    /* 你的驱动是 (xStart, yStart, xEnd, yEnd) */
    LCD_Fill(x, y, x + w - 1, y + h - 1, color);
}

void GFX_DrawHLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color){
    LCD_Fill(x, y, x + len - 1, y, color);
}

void GFX_DrawVLine(uint16_t x, uint16_t y, uint16_t len, uint16_t color){
    LCD_Fill(x, y, x, y + len - 1, color);
}

/* 圆角矩形 (软件算法实现) */
void GFX_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color){
    GFX_DrawHLine(x + r, y, w - 2 * r, color);
    GFX_DrawHLine(x + r, y + h - 1, w - 2 * r, color);
    GFX_DrawVLine(x, y + r, h - 2 * r, color);
    GFX_DrawVLine(x + w - 1, y + r, h - 2 * r, color);

    /* 简单的圆角模拟 (画点) */int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r;
    int16_t xx = 0, yy = r;

    while (xx < yy) {
        if (f >= 0) { yy--; ddF_y += 2; f += ddF_y; }
        xx++; ddF_x += 2; f += ddF_x;
        
        LCD_DrawPoint(x + w - r + xx - 1, y + h - r + yy - 1, color);
        LCD_DrawPoint(x + w - r + yy - 1, y + h - r + xx - 1, color);
        LCD_DrawPoint(x + r - xx, y + h - r + yy - 1, color);
        LCD_DrawPoint(x + r - yy, y + h - r + xx - 1, color);
        LCD_DrawPoint(x + r - xx, y + r - yy, color);
        LCD_DrawPoint(x + r - yy, y + r - xx, color);
        LCD_DrawPoint(x + w - r + xx - 1, y + r - yy, color);
        LCD_DrawPoint(x + w - r + yy - 1, y + r - xx, color);
    }
}

///* 绘制单色位图 (如 U/V, 箭头等) */
//void GFX_DrawMonoBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bmp, uint16_t color){
//    uint16_t i, j;
//    uint16_t byteWidth = (w + 7) / 8;
//    
//    for (j = 0; j < h; j++) {
//        for (i = 0; i < w; i++) {
//            if (bmp[j * byteWidth + i / 8] & (0x80 >> (i % 8))) {
//                LCD_DrawPoint(x + i, y + j, color);
//            }
//        }
//    }
//}


void LCD_DrawChar_DMA(uint16_t x, uint16_t y, char c, uint8_t is_large){
    uint8_t idx;
    const uint8_t* pData;
    uint16_t w, h;
    uint32_t i, size;
    uint8_t val_packed, val_pixel;
    uint16_t color;

    // 1. 字符索引映射
		if (c >= '0' && c <= '9') idx = c - '0';
    else if (c == '.') idx = 10;
    else if (c == '-') idx = 11;
    else idx = 12; // 空格// 2. 获取数据源 (4-bit)
		if (is_large) {
        w = 24; h = 40;
        pData = Font40_Table[idx]; // 对应 Python 生成的变量名
    } else {
        w = 14; h = 24;
        pData = Font24_Table[idx];
    }

    // 3. 硬件开窗
    LCD_Address_Set(x, y, x + w - 1, y + h - 1);
    
    LCD_CS_Clr();
    LCD_DC_Set();

    size = w * h; // 总像素点数// 4. 解压与渲染循环// 数据格式：每个字节包含2个像素 [高4位=点1][低4位=点2]
		for (i = 0; i < size; i++) {
        
        // 步骤A: 取出 4-bit 灰度值
				if (i % 2 == 0) {
            val_packed = pData[i / 2];      // 读一个字节
            val_pixel = (val_packed >> 4);  // 取高4位
        } else {
            val_pixel = (val_packed & 0x0F); // 取低4位
        }

        // 步骤B: 调色 (灰度 0-15 -> RGB565)// 0=背景(黑), 15=字体(白), 中间值=灰色边缘
						if (val_pixel == 0) {
            // 纯黑背景 -> 直接发 0x0000
						LCD_WR_DATA8(0x00);
            LCD_WR_DATA8(0x00);
        } 
        else if (val_pixel == 15) {
            // 纯白字体 -> 直接发 0xFFFF
						LCD_WR_DATA8(0xFF);
            LCD_WR_DATA8(0xFF);
        } 
        else {
            // 灰色边缘 (抗锯齿的关键!)// 算法：R=G=B = val * 2  (0-30范围，凑合用)// 更精细算法: val*2 扩展到 5bit/6bit// 简单快速法：// R (5bit) = val * 2// G (6bit) = val * 4// B (5bit) = val * 2
            color = ((val_pixel * 2) << 11) | ((val_pixel * 4) << 5) | (val_pixel * 2);
						LCD_WR_DATA8(color >> 8);
            LCD_WR_DATA8(color & 0xFF);
        }
    }
    LCD_CS_Set();
}

/* 兼容函数：普通单色图标 (用于 U/V/A 等符号) *//* 注意：图标建议还是用 1-bit 或 Image2Lcd 生成的紧凑格式 */
void GFX_DrawMonoBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* bmp, uint16_t color){
    uint16_t i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            // 万能位图读取逻辑
						uint32_t bit_index = j * w + i; 
            if (bmp[bit_index / 8] & (0x80 >> (bit_index % 8))) {
                LCD_DrawPoint(x + i, y + j, color);
            }
        }
    }
}


///* ==================================================================
// * 3. 核心字符绘制 (解决 LCD_DrawChar_DMA 报错)
// * ================================================================== *//** * @brief 实现 display_refresh.c 需要的接口
// * 虽然名字叫 DMA，但这里我们用“批量写入”来模拟，速度也很快
// */
//void LCD_DrawChar_DMA(uint16_t x, uint16_t y, char c, uint8_t is_large){
//    uint8_t idx;
//    const uint8_t* pData;
//    uint16_t w, h;
//    uint32_t size, i;

//    /* 1. 字符映射 */if (c >= '0' && c <= '9') idx = c - '0';
//    else if (c == '.') idx = 10;
//    else if (c == '-') {
//        /* 负号直接画矩形，不查字库 */
//			if (is_large) LCD_Fill(x + 4, y + 18, x + 19, y + 21, 0xFFFF);
//        else          LCD_Fill(x + 2, y + 11, x + 11, y + 12, 0xFFFF);
//        return;
//    }
//    else idx = 11; // 默认空格/* 2. 获取字库数据 (font_assets.c) */
//		if (is_large) {
//        w = 24; h = 40;
//        pData = Font40_Table[idx];
//    } else {
//        w = 14; h = 24;
//        pData = Font24_Table[idx];
//    }

//    /* 3. 批量写入 (利用 lcd.c 的 Address_Set) */
//    LCD_Address_Set(x, y, x + w - 1, y + h - 1);
//    
//    size = w * h;
//    for (i = 0; i < size; i++) {
//        LCD_WR_DATA8(pData[i]); // 调用你的 lcd.c 写入函数
//    }
//}

