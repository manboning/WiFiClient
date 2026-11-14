#include "ui_layout.h"
#include "gfx_driver.h"// ÒýÓÃµÚÒ»²¿·ÖµÄÍ¼ÐÎÇý¶
#include "icons.h"// ÒýÓÃµÚÒ»²¿·ÖµÄ×ÊÔ´/* ÑÕÉ«¶¨Òå (RGB565) */
#define C_BG     0x0000 // ºÚÉ«±³¾° 
#define C_RED    0xF800 // ºìÉ«¿ò (U/V)
#define C_GREEN  0x07E0 // ÂÌÉ«¿ò (I/A)
#define C_PURPLE 0xF81F // ×ÏÉ«¿ò (P/W)
#define C_CYAN   0x07FF // ÇàÉ«¿ò (AH/WH)
#define C_GRAY   0x8410 // »ÒÉ« (×°ÊÎ¿ò/·Ö¸îÏß)
#define C_WHITE  0xFFFF // °×É« (ÎÄ×Ö/Í¼±ê)
/**
 * @brief »æÖÆ¾²Ì¬½çÃæ¿ò¼Ü
 * ²¼¾Ö¹æÔò£º
 * - ÆÁÄ»: 320 x 172
 * - ±ß¾à: Left=10, Top=5
 * - ÐÐ¸ß: ´ó×ÖÐÐ¼ä¾à45px (5->50->95)
 */
 void UI_Draw_Static_Layout(void){
    // 1. ÇåÆÁ
    GFX_FillRect(0, 0, 320, 172, C_BG);

    // ================= Line 1: µçÑ¹ (U/V) =================// Y = 5// ×ó²à±êÇ©¿ò: 80x40
    GFX_DrawRoundRect(10, 5, 80, 40, 6, C_RED);
    
    // ¾ÓÖÐ»æÖÆ U / V// ÄÚÈÝ×Ü¿í: U(22) + /(14) + V(26) = 62px// ¾ÓÖÐÆ«ÒÆ: (80 - 62) / 2 = 9px -> ÆðÊ¼X = 10 + 9 = 19
    GFX_DrawMonoBitmap(19,    6, 22, 38, Bitmap_U_38px, C_WHITE);
    GFX_DrawMonoBitmap(19+24, 6, 14, 38, Bitmap_Slash_38px, C_WHITE); // +2px¼ä¾à
    GFX_DrawMonoBitmap(19+40, 6, 26, 38, Bitmap_V_38px, C_WHITE);

    // ÓÒ²à×°ÊÎ¿Õ¿ò (¸´¿ÌÊµÅÄÍ¼Ð§¹û)// Î»ÖÃ: X=260, W=40, H=40
    GFX_DrawRoundRect(260, 5, 40, 40, 4, C_GRAY); 

    // ================= Line 2: µçÁ÷ (I/A) =================// Y = 50// ×ó²à±êÇ©¿ò
    GFX_DrawRoundRect(10, 50, 80, 40, 6, C_GREEN);
    
    // ÄÚÈÝ×Ü¿í: I(6) + /(14) + A(26) = 46px// ¾ÓÖÐÆ«ÒÆ: (80 - 46) / 2 = 17px -> ÆðÊ¼X = 10 + 17 = 27
    GFX_DrawMonoBitmap(27,    51, 6,  38, Bitmap_I_38px, C_WHITE);
    GFX_DrawMonoBitmap(27+8,  51, 14, 38, Bitmap_Slash_38px, C_WHITE);
    GFX_DrawMonoBitmap(27+24, 51, 26, 38, Bitmap_A_38px, C_WHITE);

    // ÓÒ²à¼ýÍ·¿ò (ÏÈ»­¸ö¿Õ¿ò£¬¼ýÍ·ÓÉ¶¯Ì¬Ë¢ÐÂº¯Êý»­)
    GFX_DrawRoundRect(260, 50, 40, 40, 4, C_GRAY);

    // ================= Line 3: ¹¦ÂÊ (P/W) =================// Y = 95// ×ó²à±êÇ©¿ò
    GFX_DrawRoundRect(10, 95, 80, 40, 6, C_PURPLE);
    
    // ÄÚÈÝ×Ü¿í: P(22) + /(14) + W(30) = 66px// ¾ÓÖÐÆ«ÒÆ: (80 - 66) / 2 = 7px -> ÆðÊ¼X = 10 + 7 = 17
    GFX_DrawMonoBitmap(17,    96, 22, 38, Bitmap_P_38px, C_WHITE);
    GFX_DrawMonoBitmap(17+24, 96, 14, 38, Bitmap_Slash_38px, C_WHITE);
    GFX_DrawMonoBitmap(17+40, 96, 30, 38, Bitmap_W_38px, C_WHITE);

    // ÓÒ²à×°ÊÎ¿Õ¿ò
    GFX_DrawRoundRect(260, 95, 40, 40, 4, C_GRAY);

    // ================= Line 4: ÄÜÁ¿ (AH/WH) =================// ·Ö¸îÏß Y=140
    GFX_DrawHLine(5, 140, 310, C_GRAY);

    // Y = 143// AH ±êÇ© (X=10, W=48, H=24)
    GFX_DrawRoundRect(10, 143, 48, 24, 4, C_CYAN);
    // ÄÚÈÝ¿í24, Æ«ÒÆ12 -> X=22
    GFX_DrawMonoBitmap(22, 144, 24, 22, Bitmap_AH_22px, C_WHITE);

    // WH ±êÇ© (X=170, W=48, H=24) -> È·±£ÓëAH×óÓÒ¶Ô³Æ
    GFX_DrawRoundRect(170, 143, 48, 24, 4, C_CYAN);
    // ÄÚÈÝ¿í24, Æ«ÒÆ12 -> X=182
    GFX_DrawMonoBitmap(182, 144, 24, 22, Bitmap_WH_22px, C_WHITE);
}
 

