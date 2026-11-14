#ifndef __UI_LAYOUT_H
#define __UI_LAYOUT_H
#include <stdint.h>/* 静态布局绘制 (开机调用一次) */
void UI_Draw_Static_Layout(void);

/* 动态数据刷新 (100ms 调用一次)
 * v: 电压 (V)
 * i: 电流 (A)
 * p: 功率 (W)
 * ah: 容量 (Ah)
 * wh: 能量 (Wh)
 * dir: 电流方向 (1=正向, -1=反向, 0=无)
 */
 void UI_Update_Values(float v, float i, float p, float ah, float wh, int8_t dir);

#endif

