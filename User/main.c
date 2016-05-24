/*********************** HNIT 3103 Application Team **************************
 * @file    mainc
 * @brief   JPEG图像压缩与解压
 * @board   STM32F407开发板
 * @author  王昱霏
 * @date    2016.5.18
 * @history 无 
 *****************************************************************************/

#include "stm32_sys.h"
#include "stm32_delay.h"
#include "stm32_usart.h"
#include "hnit_led.h" 
#include "hnit_key.h"
#include "hnit_lcd.h"
#include "hnit_jpeg.h"
#include "hnit_sram.h"

/**
  * @brief  c程序入口
  * @param  无
  * @retval 错误代码
  */
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
