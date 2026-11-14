/**
  ******************************************************************************
  * @file    main.c
  * @author  AI Assistant
  * @date    2026-01-27
  * @brief   HLW8110 智能功率计终极驱动 (48MHz 满血版)
  * @details
  * 1. 硬件架构: PY32F030 (HSI 24M + PLL x2 = 48MHz) + HLW8110
  * 2. 核心算法: 动态死区 + 强制同步自适应滤波器 (解决滞后与末位抖动)
  * 3. 输出格式: 智能 5 位有效数字 (自动四舍五入，自动调整小数点)
  * 4. 性能指标: 主频 48MHz (比默认8MHz快6倍)，采样率 5Hz (200ms)
  ******************************************************************************
  */

#include "py32f0xx.h"
#include <stdio.h>
#include <stdlib.h> // 提供 abs() 绝对值函数

/* =================================================================================
 * [1] 用户配置区 (User Config)
 * 警告: 修改此处参数需同步检查原理图
 * =================================================================================*/

/* 单片机系统主频 */
/* 注意: 本代码启用了 PLL 2倍频，故 24MHz * 2 = 48MHz */
#define F_CPU               48000000UL  

/* 硬件采样参数 */
/* 电压分压比: 例如 1.5M电阻 / 10K电阻 + 1 = 151倍 */
#define VOLT_DIVIDER        151.0f      
/* 电流采样电阻: 例如 5毫欧锰铜电阻 */
#define R_SHUNT_MR          5.0f        

/* 串口波特率配置 */
#define HLW_BAUD            9600        // 计量芯片固定波特率
#define PC_BAUD             115200      // 上位机通讯波特率

/* =================================================================================
 * [2] 全局变量定义 (Global Variables)
 * =================================================================================*/

/* 系统时间基准 (由 SysTick 中断每1ms累加) */
volatile uint32_t g_ms_ticks = 0;

/* 原始采集数据 (Raw Data) 
 * 说明: 直接从芯片读回来的值，包含噪声，未经过滤波
 */
int32_t g_v_raw = 0;
int32_t g_i_raw = 0;

/* 滤波后的稳定数据 (Filtered Data) 
 * 说明: 经过智能算法处理后的值，用于最终显示
 */
static int32_t g_v_flt = 0;
static int32_t g_i_flt = 0;

/* 系统状态标志 */
static uint8_t g_first_run = 1;         // 1: 首次运行 (用于初始化滤波器初值)

/* 强制同步计数器 (Sync Counters)
 * 作用: 记录数值在"死区"内滞留的次数，用于消除"死活差一点"的稳态误差
 */
static uint8_t g_v_sync_cnt = 0;
static uint8_t g_i_sync_cnt = 0;

/* =================================================================================
 * [3] 底层驱动层 (Low Level Drivers)
 * =================================================================================*/

/**
 * @brief  SysTick 中断服务函数
 * @note   硬件自动调用，频率 1kHz
 */
void SysTick_Handler(void) {
    g_ms_ticks++;
}

/**
 * @brief  系统时钟配置 (核心函数)
 * @note   配置流程: Flash延时 -> HSI 24M -> PLL x2 -> SysClk 48M
 */
void SystemClock_Config_48M(void) {
    
    /* 1. Flash Latency 设置 (非常关键!) */
    /* PY32 手册规定: 当 HCLK > 24MHz 时，必须设置 Latency = 1 (等待1个周期)
     * 否则 CPU 读取指令的速度会超过 Flash 的响应速度，导致死机。
     */
    FLASH->ACR |= FLASH_ACR_LATENCY; 

    /* 2. 配置 HSI (内部高速时钟) 为 24MHz */
    /* 步骤: 读取工厂校准值 -> 清除旧配置 -> 写入新配置 */
    
    // 读取 24MHz 校准值 (地址: 0x1FFF 0F10，掩码 0x1FFF)
    uint32_t hsi_trim = (*(volatile uint32_t *)0x1FFF0F10) & 0x1FFF;
    
    // 操作 ICSCR 寄存器
    uint32_t icscr = RCC->ICSCR;
    icscr &= 0xFFFF0000;      // 清除低16位 (HSI_FS 和 HSI_TRIM)
    icscr |= (0x4 << 13);     // 设置 HSI_FS = 100 (代表 24MHz)
    icscr |= hsi_trim;        // 填入校准值
    RCC->ICSCR = icscr;

    // 确保 HSI 不分频 (HSIDIV = 0)
    RCC->CR &= ~RCC_CR_HSIDIV;
    
    // 等待 HSI 就绪
    while ((RCC->CR & RCC_CR_HSIRDY) == 0);

    /* 3. 配置 PLL (锁相环) */
    /* PY32F030 的 PLL 默认源为 HSI，默认倍频因子为 2 */
    /* 逻辑: 24MHz (HSI) * 2 = 48MHz (PLL输出) */
    
    RCC->CR |= RCC_CR_PLLON;  // 开启 PLL

    // 等待 PLL 锁定 (PLLRDY = 1)
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    /* 4. 切换系统时钟源到 PLL */
    /* 修改 CFGR 寄存器的 SW[2:0] 位 */
    /* 000=HSI, 001=HSE, 010=PLL, 011=LSI */
    
    uint32_t cfgr = RCC->CFGR;
    cfgr &= ~(0x07); // 清除 SW 位
    cfgr |= 0x02;    // 设置 SW = 010 (PLL)
    RCC->CFGR = cfgr;

    /* 5. 等待切换完成 */
    /* 读取 SWS[5:3] 状态位，确认是否已切换到 010 */
    while ((RCC->CFGR & (0x07 << 3)) != (0x02 << 3));

    /* 6. 更新全局变量，通知 HAL 库或后续代码主频已变 */
    SystemCoreClock = F_CPU;
}

/**
 * @brief  外设初始化总入口
 */
void Init_System(void) {
    /* 1. 提升主频到 48MHz (最优先执行) */
    SystemClock_Config_48M();

    /* 2. 配置 SysTick (基于 48MHz 计算重装载值) */
    SysTick_Config(F_CPU / 1000); 

    /* 3. 开启外设时钟 */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN; // GPIO
    RCC->APBENR2 |= RCC_APBENR2_USART1EN; // HLW串口
    RCC->APBENR1 |= RCC_APBENR1_USART2EN; // 打印串口

    /* 4. USART1 初始化 (连接 HLW8110) */
    /* 模式: 8E1 (8位 + 偶校验) */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4) | (3U << 6))) | ((2U << 4) | (2U << 6));
    GPIOA->AFR[0] |= ((1U << 8) | (1U << 12)); 
    
    // 波特率计算: 48000000 / 9600 = 5000
    USART1->BRR = F_CPU / HLW_BAUD;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_M | USART_CR1_PCE;

    /* 5. USART2 初始化 (连接电脑) */
    /* 模式: 8N1 (8位 + 无校验) */
    GPIOB->MODER = (GPIOB->MODER & ~(3U << 12)) | (2U << 12);
    GPIOB->AFR[0] = (GPIOB->AFR[0] & ~(0xFU << 24)) | (4U << 24); 
    
    // 波特率计算: 48000000 / 115200 ≈ 416
    USART2->BRR = F_CPU / PC_BAUD; 
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;

    /* 6. 初始化看门狗 (2秒超时) */
    IWDG->KR = 0x5555; IWDG->PR = 4; IWDG->RLR = 1000; IWDG->KR = 0xAAAA;
}

/**
 * @brief  串口发送字符串 (阻塞模式)
 */
void PC_Print(char *str) {
    while(*str) {
        while(!(USART2->SR & USART_SR_TXE)); // 等待发送缓冲空
        USART2->DR = *str++;
    }
}

/* =================================================================================
 * [4] 核心算法层 (Algorithms)
 * =================================================================================*/

/**
 * @brief  智能自适应滤波器 (Smart Adaptive Filter)
 * @details 采用"三级变速"策略，解决数值抖动和跟随滞后问题
 * * @param  prev:      指向历史滤波值的指针 (输入/输出)
 * @param  now:       当前采集的原始值 (输入)
 * @param  sync_cnt:  指向同步计数器的指针 (输入/输出)
 * @return int32_t:   计算后的新值
 */
int32_t Smart_Filter(int32_t *prev, int32_t now, uint8_t *sync_cnt) {
    int32_t diff = now - *prev;
    if (diff < 0) diff = -diff; // 取绝对值

    /* --- 算法阈值参数 (单位: 1u) --- */
    /* 1. 激进跟随阈值 (0.05V): 超过此值，视为有效变化，权重 75% 快速跟随 */
    const int32_t FAST_ZONE = 50000;  
    /* 2. 死区锁定阈值 (0.003V): 小于此值，视为ADC底噪，数值锁定不动 */
    const int32_t DEAD_ZONE = 3000;   
    /* 3. 强制同步超时 (5次=1秒): 长期存在微小偏差时，强制消除误差 */
    const uint8_t TIMEOUT   = 5;      

    /* --- 逻辑分支 --- */
    
    // [模式A: 激进跟随] 差异巨大，快速响应
    if (diff > FAST_ZONE) {
        *prev = now;   // 直接更新
        *sync_cnt = 0; // 重置计数器
    }
    // [模式B: 死区锁定] 差异极小，尝试忽略噪声
    else if (diff <= DEAD_ZONE) {
        (*sync_cnt)++;
        // 如果连续 1秒 (5次) 都有偏差，说明是真实漂移而非随机噪声
        if (*sync_cnt >= TIMEOUT) {
            *prev = (*prev * 3 + now) / 4; // 软着陆更新
            if (diff <= 2) *prev = now;    // 极小误差直接对齐
            *sync_cnt = TIMEOUT - 1;       // 保持修正状态
        }
        // else: 保持 *prev 不变，视觉上数值非常稳
    }
    // [模式C: 常规平滑] 差异中等，执行加权平滑
    else {
        *prev = (*prev * 3 + now) / 4;
        *sync_cnt = 0;
    }
    return *prev;
}

/**
 * @brief  智能格式化输出
 * @note   将定点数转换为 5位有效数字 的字符串
 * @example 123450000 -> "123.45"
 */
void Print_Auto5(char* buf, int32_t val_u) {
    char sign = (val_u >= 0) ? ' ' : '-'; 
    uint32_t abs_v = (val_u >= 0) ? (uint32_t)val_u : (uint32_t)(-val_u);

    // 根据数值量级，自动选择小数点位置，并进行四舍五入
    if (abs_v >= 100000000) {      // >= 100.00
        uint32_t r = (abs_v + 5000) / 10000; 
        sprintf(buf, "%c%3u.%02u", sign, (unsigned int)(r/100), (unsigned int)(r%100));
    }
    else if (abs_v >= 10000000) {  // >= 10.000
        uint32_t r = (abs_v + 500) / 1000;
        sprintf(buf, "%c%2u.%03u", sign, (unsigned int)(r/1000), (unsigned int)(r%1000));
    }
    else if (abs_v >= 1000000) {   // >= 1.0000
        uint32_t r = (abs_v + 50) / 100;
        sprintf(buf, "%c%1u.%04u", sign, (unsigned int)(r/10000), (unsigned int)(r%10000));
    }
    else {                        // < 1.0000
        uint32_t r = (abs_v + 50) / 100;
        sprintf(buf, "%c0.%04u", sign, (unsigned int)(r%10000));
    }
}

/* =================================================================================
 * [5] 通讯协议层 (Communication Protocol)
 * =================================================================================*/

/**
 * @brief  解析 HLW8110 数据包
 * @param  reg: 寄存器地址
 * @param  out: 输出物理量指针
 * @param  is_V: 1=电压系数, 0=电流系数
 */
uint8_t HLW_Process(uint8_t reg, int32_t *out, uint8_t is_V) {
    static uint8_t buf[4], idx = 0;
    
    // 读取串口 FIFO
    while (USART1->SR & USART_SR_RXNE) {
        buf[idx++] = USART1->DR;
        
        // 收到完整 4 字节
        if (idx >= 4) {
            idx = 0;
            // 校验和计算
            uint8_t check = ~(0xA5 + reg + buf[0] + buf[1] + buf[2]);
            
            if (buf[3] == check) {
                // 拼合数据
                int32_t raw = (int32_t)((buf[0]<<16) | (buf[1]<<8) | buf[2]);
                // 补码处理
                if (raw & 0x800000) raw |= 0xFF000000;
                raw = -raw; 

                // 物理量换算
                if(is_V) *out = (int32_t)(((long long)raw * 14399928LL) / 1000000LL); 
                else     *out = (int32_t)(((long long)raw * 11921LL) / 10000LL);
                return 1;
            }
        }
    }
    return 0;
}

void HLW_SendCmd(uint8_t reg) {
    while(!(USART1->SR & USART_SR_TXE)); USART1->DR = 0xA5; 
    while(!(USART1->SR & USART_SR_TXE)); USART1->DR = reg;  
}

void HLW_Init_Cmd(void) {
    uint8_t cmds[][5] = {
        {0xA5,0xEA,0xE5,0x8B}, {0xA5,0x80,0x0A,0x04,0xCC},     
        {0xA5,0x81,0x02,0x30,0xA7}, {0xA5,0x93,0x01,0x21,0xA5}      
    };
    for(int i=0; i<4; i++) {
        for(int j=0; j<(i==0?4:5); j++) {
            while(!(USART1->SR & USART_SR_TXE)); USART1->DR = cmds[i][j];
        }
        for(volatile int d=0; d<30000; d++); 
    }
}

/* =================================================================================
 * [6] 主程序循环 (Main Loop)
 * =================================================================================*/
int main(void) {
    Init_System();  // 48MHz 系统初始化
    HLW_Init_Cmd(); // 计量芯片初始化
    
    uint32_t t_last = 0;    
    uint8_t  step = 0;      
    char str_buf[64];       

    while (1) {
        IWDG->KR = 0xAAAA; // 喂看门狗
        uint32_t t_now = g_ms_ticks; 

        /* 状态机调度: 5Hz (200ms) 采样周期 */
        switch (step) {
            case 0: // [T=0ms] 请求电压
                HLW_SendCmd(0x26); 
                t_last = t_now;    
                step = 1;          
                break;
            
            case 1: // 接收电压 (超时 45ms)
                if (HLW_Process(0x26, &g_v_raw, 1)) step = 2; 
                if (t_now - t_last > 45) step = 2; 
                break;

            case 2: // [T=60ms] 请求电流
                if (t_now - t_last >= 60) { 
                    HLW_SendCmd(0x24); 
                    t_last = t_now;    
                    step = 3; 
                } 
                break;

            case 3: // 接收电流 + 滤波计算 + 打印
                if (HLW_Process(0x24, &g_i_raw, 0)) {
                    
                    // 首次运行初始化
                    if (g_first_run) { 
                        g_v_flt = g_v_raw; g_i_flt = g_i_raw; g_first_run = 0; 
                    }

                    // 调用核心算法
                    Smart_Filter(&g_v_flt, g_v_raw, &g_v_sync_cnt);
                    Smart_Filter(&g_i_flt, g_i_raw, &g_i_sync_cnt);
                    
                    // 计算功率
                    int32_t p_flt = (int32_t)((long long)g_v_flt * g_i_flt / 1000000LL);

                    // 打印最终结果
                    PC_Print("V: "); Print_Auto5(str_buf, g_v_flt); PC_Print(str_buf);
                    PC_Print(" I: "); Print_Auto5(str_buf, g_i_flt); PC_Print(str_buf);
                    PC_Print(" P: "); Print_Auto5(str_buf, p_flt); PC_Print(str_buf);
                    PC_Print("\r\n");

                    step = 4; 
                }
                if (t_now - t_last > 45) step = 4; 
                break;

            case 4: // [T=200ms] 等待周期结束
                if (t_now - t_last >= 140) step = 0; 
                break;
        }
    }
}