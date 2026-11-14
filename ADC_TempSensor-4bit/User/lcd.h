#ifndef __LCD_H_
#define __LCD_H_


#include "main.h"
#include <stdint.h> 

/**********SPI引脚分配，连接TFT屏，更具实际情况修改*********/

#define TFT_COLUMN_OFFSET 34	//列偏移
#define TFT_LINE_OFFSET 0   //行偏移
#define USE_HORIZONTAL 2    //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_COL 172	
#define LCD_ROW 320
#else
#define LCD_COL 320
#define LCD_ROW 172
#endif




#define		CLK_GPIO_CLK_ENABLE		__HAL_RCC_GPIOA_CLK_ENABLE
#define		CLK_GPIO_PORT					GPIOA
#define		CLK_GPIO_PIN					GPIO_PIN_4

#define		SDA_GPIO_CLK_ENABLE		__HAL_RCC_GPIOA_CLK_ENABLE
#define		SDA_GPIO_PORT					GPIOA
#define		SDA_GPIO_PIN					GPIO_PIN_5

#define		RES_GPIO_CLK_ENABLE		__HAL_RCC_GPIOA_CLK_ENABLE
#define		RES_GPIO_PORT					GPIOA
#define		RES_GPIO_PIN					GPIO_PIN_6

#define		DC_GPIO_CLK_ENABLE		__HAL_RCC_GPIOA_CLK_ENABLE
#define		DC_GPIO_PORT					GPIOA
#define		DC_GPIO_PIN						GPIO_PIN_7

#define		CS_GPIO_CLK_ENABLE		__HAL_RCC_GPIOB_CLK_ENABLE
#define		CS_GPIO_PORT					GPIOB
#define		CS_GPIO_PIN						GPIO_PIN_0

#define		BLK_GPIO_CLK_ENABLE		__HAL_RCC_GPIOB_CLK_ENABLE
#define		BLK_GPIO_PORT					GPIOB
#define		BLK_GPIO_PIN					GPIO_PIN_1




/**********SPI引脚分配，连接TFT屏，更具实际情况修改*********/
#define LCD_SCLK_Clr() HAL_GPIO_WritePin(CLK_GPIO_PORT, CLK_GPIO_PIN , GPIO_PIN_RESET )//SCL=SCLK
#define LCD_SCLK_Set() HAL_GPIO_WritePin(CLK_GPIO_PORT, CLK_GPIO_PIN , GPIO_PIN_SET )

#define LCD_MOSI_Clr() HAL_GPIO_WritePin(SDA_GPIO_PORT, SDA_GPIO_PIN, GPIO_PIN_RESET )//SDA=MOSI
#define LCD_MOSI_Set() HAL_GPIO_WritePin(SDA_GPIO_PORT, SDA_GPIO_PIN, GPIO_PIN_SET )

#define LCD_RES_Clr()  HAL_GPIO_WritePin(RES_GPIO_PORT, RES_GPIO_PIN, GPIO_PIN_RESET )//RES
#define LCD_RES_Set()  HAL_GPIO_WritePin(RES_GPIO_PORT, RES_GPIO_PIN, GPIO_PIN_SET )

#define LCD_DC_Clr()   HAL_GPIO_WritePin(DC_GPIO_PORT, DC_GPIO_PIN , GPIO_PIN_RESET )//DC
#define LCD_DC_Set()   HAL_GPIO_WritePin(DC_GPIO_PORT, DC_GPIO_PIN , GPIO_PIN_SET )
 		     
#define LCD_CS_Clr()   HAL_GPIO_WritePin(CS_GPIO_PORT, CS_GPIO_PIN, GPIO_PIN_RESET)//CS
#define LCD_CS_Set()   HAL_GPIO_WritePin(CS_GPIO_PORT, CS_GPIO_PIN, GPIO_PIN_SET)

#define LCD_BLK_Clr()  HAL_GPIO_WritePin(BLK_GPIO_PORT, BLK_GPIO_PIN , GPIO_PIN_RESET)//BLK
#define LCD_BLK_Set()  HAL_GPIO_WritePin(BLK_GPIO_PORT, BLK_GPIO_PIN , GPIO_PIN_SET)


/*****************颜色数据**************************/
#define WHITE            0xFFFF
#define BLACK            0x0000   
#define BLUE             0x001F  
#define BRED             0XF81F
#define GRED             0XFFE0
#define GBLUE            0X07FF
#define RED              0xF800
#define GREEN            0x07E0



void delay_us(unsigned int _us_time);
void delay_ms(unsigned int _ms_time);

void LCD_Init(void);
void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);
void LCD_ShowPicture(uint16_t x,uint16_t y,uint16_t length,uint16_t width,const uint8_t pic[]);
void TFT_Page_border(void);
void TFT_Page_bar(void);
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color);
void Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color, int place);
void LCD_WR_DATA(uint16_t dat);
void LCD_WR_DATA8(uint8_t dat);
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);

#endif

