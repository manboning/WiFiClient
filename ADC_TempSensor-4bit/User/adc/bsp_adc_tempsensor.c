/**
  ******************************************************************************
  * @file     bsp_adc_tempsensor.c
  * @author   embedfire
  * @version  V1.0
  * @date     2024
  * @brief    adc温度传感器接口
  ******************************************************************************
  * @attention
  *
  * 实验平台：野火 ebf_py32f030 PY32开发板 
  * 论坛      ：http://www.firebbs.cn
  * 官网      ：https://embedfire.com/
  * 淘宝      ：https://yehuosm.tmall.com/
  *
  ******************************************************************************
  */
#include "adc/bsp_adc_tempsensor.h"
#include "main.h"



#define DMA_SAMP_COUNT 3
ADC_HandleTypeDef hadcdma;
uint32_t adc_dma_value[DMA_SAMP_COUNT] = {0};

void ADC_DMA_Init(void)
{
    ADC_ChannelConfTypeDef adConfig = {0};
    
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET();
    __HAL_RCC_ADC_CLK_ENABLE();    //ADC????
 
    hadcdma.Instance = ADC1;
    if (HAL_ADCEx_Calibration_Start(&hadcdma) != HAL_OK)                // ADC ??
        APP_ErrorHandler();
    
    hadcdma.Instance                   = ADC1;
    hadcdma.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;      // ??ADC????PCLK,???
    hadcdma.Init.Resolution            = ADC_RESOLUTION_12B;            // ?????12bit
    hadcdma.Init.DataAlign             = ADC_DATAALIGN_RIGHT;           // ???
    hadcdma.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;    // ??????:0-12
    hadcdma.Init.LowPowerAutoWait      = ENABLE;                        // ????????
    hadcdma.Init.ContinuousConvMode    = ENABLE;                        // ????
    hadcdma.Init.DiscontinuousConvMode = DISABLE;                       // ??????
    hadcdma.Init.ExternalTrigConv      = ADC_SOFTWARE_START;            // ADC ?????
    hadcdma.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE; // ???????
    hadcdma.Init.DMAContinuousRequests = ENABLE;                        // DMA ????
    hadcdma.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;      // ??????,ADC_DR ????
    hadcdma.Init.SamplingTimeCommon    = ADC_SAMPLETIME_239CYCLES_5;    // ????
    
    if (HAL_ADC_Init(&hadcdma) != HAL_OK)                               // ADC???
        APP_ErrorHandler();
    
    adConfig.Channel = ADC_CHANNEL_0; /* ?? ADC ??0 */
    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
    
    adConfig.Channel = ADC_CHANNEL_1; /* ?? ADC ??1 */
    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
		
		adConfig.Channel = ADC_CHANNEL_2; /* ?? ADC ??1 */
    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
		
//		adConfig.Channel = ADC_CHANNEL_3; /* ?? ADC ??1 */
//    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
//    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
//		
//		adConfig.Channel = ADC_CHANNEL_4; /* ?? ADC ??1 */
//    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
//    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
// 
//    adConfig.Channel = ADC_CHANNEL_5; /* ?? ADC ??4 */
//    adConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
//    if (HAL_ADC_ConfigChannel(&hadcdma, &adConfig) != HAL_OK) APP_ErrorHandler();
    
    ADC_DMA_Start();
}
 
HAL_StatusTypeDef ADC_DMA_Start(void)
{
    for( uint8_t i = 0; i < DMA_SAMP_COUNT; i++) 
        adc_dma_value[i] = 0;
    
    if (HAL_ADC_Start_DMA(&hadcdma, &(adc_dma_value[0]), DMA_SAMP_COUNT) != HAL_OK)
        APP_ErrorHandler();
    
    return HAL_OK;
}
 
HAL_StatusTypeDef ADC_DMA_Sample(void )
{
    if(__HAL_DMA_GET_FLAG(DMA1->ISR, DMA_ISR_TCIF1))
    {
				
        __HAL_DMA_CLEAR_FLAG(DMA1->ISR, DMA_IFCR_CTCIF1); 
    }
    
    return HAL_OK;
}



