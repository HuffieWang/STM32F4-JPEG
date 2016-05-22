/*********************** HNIT 3103 Application Team **************************
 * @file    hnit_key.c
 * @brief   独立按键配置与操作         
 * @board   STM32F407开发板
 * @author  王昱霏
 * @date    2016.3.19
 * @history 无 
 *****************************************************************************/
#include "hnit_key.h"

/**
  * @brief  按键GPIO初始化
  * @param  无
  * @retval 无
  */
void key_gpio_config(void)
{    	 
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);  //使能GPIOE时钟

    // GPIOE3, E4初始化设置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;         //普通输入模式
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;    //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //上拉
    GPIO_Init(GPIOE, &GPIO_InitStructure);                //初始化	
}

/******************************* END OF FILE *********************************/
