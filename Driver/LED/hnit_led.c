/*********************** HNIT 3103 Application Team **************************
 * @file    hnit_led.c
 * @brief   LED配置与操作         
 * @board   STM32F407开发板
 * @author  王昱霏
 * @date    2016.3.19
 * @history 无 
 *****************************************************************************/
#include "hnit_led.h" 
	 
/**
  * @brief  LED GPIO初始化
  * @param  无
  * @retval 无
  */
void led_gpio_config(void)
{    	 
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);  //使能GPIOF时钟

    /* GPIOF9,F10初始化设置 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //普通输出模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;    //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //上拉
    GPIO_Init(GPIOF, &GPIO_InitStructure);                //初始化
	
    GPIO_SetBits(GPIOF,GPIO_Pin_9 | GPIO_Pin_10);   //GPIOF9,F10设置高，灯灭
}

/******************************* END OF FILE *********************************/
