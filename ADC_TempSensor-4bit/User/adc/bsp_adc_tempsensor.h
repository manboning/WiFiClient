#ifndef __BSP_ADC_TMPSENSOR_H
#define __BSP_ADC_TMPSENSOR_H

/* 包含其他头文件 */
#include "py32f0xx_hal.h"
#include "usart/bsp_usart.h"
 
/* 宏定义 */
#define Vcc_Power     3.3l                                            /* VCC电源电压,根据实际情况修改  */
#define TScal1        (float)((HAL_ADC_TSCAL1) * 3.3 / Vcc_Power)     /* 30摄氏度校准值对应电压 */
#define TScal2        (float)((HAL_ADC_TSCAL2) * 3.3 / Vcc_Power)     /* 85摄氏度校准值对应电压 */
#define TStem1        30l                                             /* 30摄氏度 */
#define TStem2        85l                                             /* 85摄氏度 */
#define Temp_k        ((float)(TStem2-TStem1)/(float)(TScal2-TScal1)) /* 温度计算 */
    


void ADC_DMA_Init(void);
HAL_StatusTypeDef ADC_DMA_Start(void);
HAL_StatusTypeDef ADC_DMA_Sample(void);


#endif /* __BSP_ADC_TMPSENSOR_H */

