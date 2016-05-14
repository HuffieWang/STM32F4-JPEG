/*********************** HNIT 3103 Application Team **************************
 * 文 件 名 ：main.c
 * 描    述 ：液晶显示    
 * 实验平台 ：STM32F407开发板
 * 库 版 本 ：ST1.4.0
 * 时    间 ：2016.3.19
 * 作    者 ：3103创新团队 王昱霏
 * 修改记录 ：无
******************************************************************************/
#include "stm32_sys.h"
#include "stm32_delay.h"
#include "stm32_usart.h"
#include "hnit_led.h" 
#include "hnit_key.h"
#include "hnit_lcd.h"
#include "hnit_jpeg.h"
#include "hnit_sram.h"

/*****************************************************************************
*	函 数 名: main
*	功    能: c程序入口
*   调用函数：无
*	形    参：无
*	返 回 值: 错误代码
******************************************************************************/	
int main(void)
{
    u32 t = 0;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);
    usart1_init(115200);		
    fsmc_lcd_init();
    fsmc_sram_init();
    led_gpio_config();
    key_gpio_config();
    
    lcd_clear(BLACK);   
    POINT_COLOR = BLACK;	
    BACK_COLOR = WHITE;
    
    jpeg_test();
    
    while(1)
    {
       
        LED0 = ~LED0;
        delay_ms(500);
        t++;
    }
}

/******************************* END OF FILE *********************************/
