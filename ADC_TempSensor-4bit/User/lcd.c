#include "lcd.h"


unsigned int page = 0;
unsigned int autoplay = 0;



void delay_us(unsigned int _us_time)
{       
  unsigned char x=0;
  for(;_us_time>0;_us_time--)
  {
    x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;
	  x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;x++;
	  
  }
}

void delay_ms(unsigned int _ms_time)
  {
    unsigned int i,j;
    for(i=0;i<_ms_time;i++)
    {
    for(j=0;j<900;j++)
      {;}
    }
  }


void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_InitStructure.Pin = CLK_GPIO_PIN|SDA_GPIO_PIN|RES_GPIO_PIN|DC_GPIO_PIN;	 
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;//速度50MHz
	GPIO_InitStructure.Pull = GPIO_PULLUP;
 	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);	  //初始化GPIOB
	
	GPIO_InitStructure.Pin = CS_GPIO_PIN|BLK_GPIO_PIN;	 
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;//速度50MHz
	GPIO_InitStructure.Pull = GPIO_PULLUP;
 	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOA
	
 	HAL_GPIO_WritePin(GPIOA, CLK_GPIO_PIN|SDA_GPIO_PIN|RES_GPIO_PIN|DC_GPIO_PIN , GPIO_PIN_SET );
	HAL_GPIO_WritePin(GPIOB, CS_GPIO_PIN|BLK_GPIO_PIN , GPIO_PIN_SET);
}


/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(uint8_t dat) 
{	
	uint8_t i;
	LCD_CS_Clr();
	for(i=0;i<8;i++)
	{			  
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}	
  LCD_CS_Set();	
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(uint8_t dat)
{
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(uint16_t dat)
{
	LCD_Writ_Bus(dat>>8);
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(uint8_t dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(x2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(y2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2c);//储存器写
	}
	else
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+TFT_COLUMN_OFFSET);
		LCD_WR_DATA(y2+TFT_COLUMN_OFFSET);
		LCD_WR_REG(0x2c);//储存器写
	}
}

void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化GPIO
	
	LCD_RES_Clr();//复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(100);
	
	LCD_BLK_Set();//打开背光
  delay_ms(100);
	
	// ST7789 init
	LCD_WR_REG(0x11); 
//	delay_ms(120); 
	LCD_WR_REG(0x36); 
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x05);

	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33); 

	LCD_WR_REG(0xB7); 
	LCD_WR_DATA8(0x35);  

	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x35);

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x13);   

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20);  

	LCD_WR_REG(0xC6); 
	LCD_WR_DATA8(0x0F);    

	LCD_WR_REG(0xD0); 
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xD6); 
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x29);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x3E);
	LCD_WR_DATA8(0x38);
	LCD_WR_DATA8(0x12);
	LCD_WR_DATA8(0x12);
	LCD_WR_DATA8(0x28);
	LCD_WR_DATA8(0x30);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x28);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x3E);
	LCD_WR_DATA8(0x36);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x29);
	LCD_WR_DATA8(0x32);
	
// 	LCD_WR_REG(0x2A);
//	LCD_WR_DATA8(0x00);
//	LCD_WR_DATA8(0x22);
//	LCD_WR_DATA8(0x00);
//	LCD_WR_DATA8(0xCD);
//	LCD_WR_DATA8(0x2B);
//	LCD_WR_DATA8(0x00);
//	LCD_WR_DATA8(0x00);
//	LCD_WR_DATA8(0x01);
//	LCD_WR_DATA8(0x3F);
//	LCD_WR_REG(0x2C);
	LCD_WR_REG(0x21); 
  
  LCD_WR_REG(0x11);
  delay_ms(120);	
	LCD_WR_REG(0x29); 
} 




/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
								color       要填充的颜色
      返回值：  无
******************************************************************************/
void LCD_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color)
{          
	uint16_t i,j; 
	LCD_Address_Set(xsta,ysta,xend-1,yend-1);//设置显示范围
	for(i=ysta;i<yend;i++)
	{													   	 	
		for(j=xsta;j<xend;j++)
		{
			LCD_WR_DATA(color);
		}
	} 					  	    
}




/******************************************************************************
      函数说明：显示图片
      入口数据：x,y起点坐标               length 图片长度
                width  图片宽度
                pic[]  图片数组    
      返回值：  无
******************************************************************************/
void LCD_ShowPicture(uint16_t x,uint16_t y,uint16_t length,uint16_t width,const uint8_t pic[])
{
	uint16_t i,j;
	uint32_t k=0;
	LCD_Address_Set(x,y,x+length-1,y+width-1);
	for(i=0;i<length;i++)
	{
		for(j=0;j<width;j++)
		{
			LCD_WR_DATA8(pic[k*2]);
			LCD_WR_DATA8(pic[k*2+1]);
			k++;
		}
	}			
}



void TFT_Page_bar()
{
	unsigned long color[8] = { 0xFFFF, 0x0000, 0x001F, 0XF81F, 0XFFE0, 0X07FF, 0xF800, 0x07E0 };
	
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围


			for(int j=0;j<8;j++)
			{
				for(int k=0;k<LCD_COL*40;k++)
				{
					LCD_WR_DATA(color[j]);
				}
			}
}


//void TFT_Page_bar2()
//{
//	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
//	
//	int height = 10;

//			for(int j=0;j<LCD_ROW;j=j+height)
//			{
//				for(int k=0;k<LCD_COL*height;k++)
//				{
//					LCD_WR_DATA8(gImage_gary[j+1]);
//					LCD_WR_DATA8(gImage_gary[j]);
//				}
//			}
//}


void TFT_Page_border(void)
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c, r;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围

	
	for(r=0;r<row;r++)
	{
		for(c=0;c<col;c++)
		{
			if(c==0 || r==0 || c == col-1 || r == row-1)
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
			//delay_ms(50);
		}
	}
}
	

void TFT_Page_border2(void)
{
	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
	
	int radio = 50;
	
	int a,b,c,d;
	for(int i = 0; i<radio; i++)
	{
		for(int j = 0; j<LCD_COL; j++)
		{
			a = 0; b = radio - i; c = LCD_COL - 1 - radio + i; d = LCD_COL - 1;
			if( (i==a && j<=b) || (i==c && j>=d) )
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
		}
	}
}



/******************************************************************************
      函数说明：在指定位置画点
      入口数据：x,y 画点坐标
                color 点的颜色
      返回值：  无
******************************************************************************/
void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{
	LCD_Address_Set(x,y,x,y);//设置光标位置 
	LCD_WR_DATA(color);
} 


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
                color   圆的颜色
      返回值：  无
******************************************************************************/
void Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r,uint16_t color, int place)
{
	int a,b;
	a=0;b=r;	  
	while(a<=b)
	{
		if(place == 1)
		{
		LCD_DrawPoint(x0-b,y0-a,color);             //3       1    
		LCD_DrawPoint(x0-a,y0-b,color);             //2         2    
		}
		if(place == 2)
		{
		LCD_DrawPoint(x0+a,y0-b,color);             //5	3
		LCD_DrawPoint(x0+b,y0-a,color);             //0           4
		}
		if(place == 3)
		{
		LCD_DrawPoint(x0+b,y0+a,color);             //4               5
		LCD_DrawPoint(x0+a,y0+b,color);             //6 	6
		}
		if(place == 4)
		{
		LCD_DrawPoint(x0-a,y0+b,color);             //1                7
		LCD_DrawPoint(x0-b,y0+a,color);             //7	8
		}
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}



void TFT_Page_net()
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c, r;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围
	
	for(c=0;c<col;c++)
	{
		for(r=0;r<row;r++)
		{
			if(r%2 == 0)
			{
				LCD_Writ_Bus(0xFF);
				LCD_Writ_Bus(0xFF);
			}else{
				LCD_Writ_Bus(0x00);
				LCD_Writ_Bus(0x00);
			}
		}
	}
}	


void TFT_Page_cell()
{
	int col = LCD_COL;
	int row = LCD_ROW;
	
	int c = 0;
	int r = 0;

	int c2 = 0;
	int r2 = 0;

	int c3 = 0;
	int r3 = 0;

	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);//设置显示范围

	for(c=0;c<col;c++)
	{
		for(r=0;r<row;r++)
		{
			c2 = (int)(c/40);
			r2 = (int)(r/43);
			c3 = c2 % 2;
			r3 = r2 % 2;

			if(c3 == r3){
				LCD_WR_DATA(0xFF);
			}else{
				LCD_WR_DATA(0x0000);
			}			
		}
	}
}
	
//void Picture_display()
//{
//	LCD_Address_Set(0,0,LCD_COL-1,LCD_ROW-1);

//	long i,j, a = 0;
//		
//		a = 0;
//		for(i=0;i<LCD_COL;i++)
//		{
//			for(j=0;j<LCD_ROW;j++)
//			{
//				LCD_Writ_Bus(gImage_color[a+1]);
//				LCD_Writ_Bus(gImage_color[a]);
//				a=a+2;
//			}
//		}
//	
//}

 




