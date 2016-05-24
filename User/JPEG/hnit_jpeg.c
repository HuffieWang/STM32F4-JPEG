/*********************** HNIT 3103 Application Team **************************
 * @file    hnit_jpeg.c
 * @brief   JPEG图像压缩与解压
 * @board   STM32F407开发板
 * @author  王昱霏
 * @date    2016.5.16
 * @history 无 
 *****************************************************************************/
#include "jpeg_data.h"
#include "hnit_jpeg.h"
#include "math.h"
#include "string.h"
#include "stm32_usart.h"
#include "hnit_lcd.h"
#include "hnit_led.h"

int8_t yuv[3][76800] __attribute__((at(0X68000000)));
uint16_t encode[7680] __attribute__((at(0X68000000+sizeof(yuv))));                 //存储压缩后的图像
uint16_t image[76800] __attribute__((at(0X68000000+sizeof(yuv)+sizeof(encode))));  //存储解压后的图像

#define JPEG_DEBUG
#define DEBUG_I 0
#define DEBUG_J 0

#ifdef JPEG_DEBUG
uint32_t debug;
void printf_array_f(float n[], uint16_t x, uint16_t y);
void printf_array_d(int8_t n[], uint16_t x, uint16_t y);
void printf_array_b(uint16_t n[], uint32_t start, uint32_t end);
#endif

/* -------------------------------------- JPEG部分 ------------------------------------------ */

//8*8 RBG565转YUV
void rgb565_to_yuv(uint16_t rgb[64], float y[64], uint8_t select)
{
    uint16_t i;
    float red, green, blue;
    
    for(i = 0; i < 64; i++)  
    {
        red = (float)((rgb[i]&0xF800)>>8);   
        green = (float)((rgb[i]&0x07E0)>>3);
        blue = (float)((rgb[i]&0x001F)<<3);
        switch(select)
        {
            case 0:
            {
                y[i] = 0.299f*red + 0.587f*green + 0.114f*blue - 128; 
            }break;
            case 1:
            {
                y[i] = -0.1687f*red - 0.3313f*green + 0.5f*blue;             
            }break;
            case 2:
            {
                y[i] = 0.5f*red - 0.418f*green - 0.0813f*blue; 
            }break;
        }            
               
    }
}

//320*240 YUV转RGB565
void yuv_to_rgb565(void)
{    
    uint32_t i;
    float red, green, blue;
    uint16_t r, g, b;
    for(i = 0; i < 76800; i++)
    {     
        red = (float)yuv[0][i]+1.402f*(float)yuv[2][i] +128;
        green = (float)yuv[0][i]-0.34414f*(float)yuv[1][i]-0.71414f*(float)yuv[2][i] +128;
        blue = (float)yuv[0][i]+1.772f*(float)yuv[1][i] +128;
        r = (uint16_t)red >> 3;
        g = (uint16_t)green >> 3;
        b = (uint16_t)blue >> 3;
        image[i] = (r<<11 | g<<6 | b);
    }
}


#define  C1 0.9808f
#define  C2 0.9239f
#define  C3 0.8315f
#define  C4 0.7071f
#define  C5 0.5556f
#define  C6 0.3827f
#define  C7 0.1951f

/**
  * @brief  8*8离散余弦变换的行变换
  * @param  blk 行的起始坐标地址
  * @retval 无
  */
static void dct2_row(float *blk)
{ 
    float S07,S16,S25,S34,S0734,S1625;
    float D07,D16,D25,D34,D0734,D1625;

    S07 = blk[0] + blk[7];
    S16 = blk[1] + blk[6];
    S25 = blk[2] + blk[5];
    S34 = blk[3] + blk[4];
    S0734 = S07 + S34;
    S1625 = S16 + S25;

    D07 = blk[0] - blk[7]; 
    D16 = blk[1] - blk[6];
    D25 = blk[2] - blk[5];
    D34 = blk[3] - blk[4];
    D0734 = S07 - S34;
    D1625 = S16 - S25;

    blk[0] = 0.5f*(C4 * (S0734+S1625));
    blk[1] = 0.5f*(C1*D07 + C3*D16 + C5*D25 + C7*D34);
    blk[2] = 0.5f*(C2*D0734 + C6*D1625);
    blk[3] = 0.5f*(C3*D07 - C7*D16 - C1*D25 - C5*D34);
    blk[4] = 0.5f*(C4 * (S0734-S1625));
    blk[5] = 0.5f*(C5*D07 - C1*D16 + C7*D25 + C3*D34);
    blk[6] = 0.5f*(C6*D0734 - C2*D1625);
    blk[7] = 0.5f*(C7*D07 - C5*D16 + C3*D25 - C1*D34);
}

/**
  * @brief  8*8离散余弦变换的列变换
  * @param  blk 列的起始坐标地址
  * @retval 无
  */
static void dct2_col(float *blk)
{
    float S07,S16,S25,S34,S0734,S1625;
    float D07,D16,D25,D34,D0734,D1625;

    S07 = blk[0*8] + blk[7*8];
    S16 = blk[1*8] + blk[6*8];
    S25 = blk[2*8] + blk[5*8];
    S34 = blk[3*8] + blk[4*8];
    S0734 = S07 + S34;
    S1625 = S16 + S25;

    D07 = blk[0*8] - blk[7*8]; 
    D16 = blk[1*8] - blk[6*8];
    D25 = blk[2*8] - blk[5*8];
    D34 = blk[3*8] - blk[4*8];
    D0734 = S07 - S34;
    D1625 = S16 - S25;

    blk[0*8] = 0.5f*(C4 * (S0734+S1625));
    blk[1*8] = 0.5f*(C1*D07 + C3*D16 + C5*D25 + C7*D34);
    blk[2*8] = 0.5f*(C2*D0734 + C6*D1625);
    blk[3*8] = 0.5f*(C3*D07 - C7*D16 - C1*D25 - C5*D34);
    blk[4*8] = 0.5f*(C4 * (S0734-S1625));
    blk[5*8] = 0.5f*(C5*D07 - C1*D16 + C7*D25 + C3*D34);
    blk[6*8] = 0.5f*(C6*D0734 - C2*D1625);
    blk[7*8] = 0.5f*(C7*D07 - C5*D16 + C3*D25 - C1*D34);
}

/**
  * @brief  8*8快速离散余弦变换
  * @param  block 8*8数据块的起始地址
  * @retval 无
  */
void jpeg_dct2(float *block)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        dct2_row(block + 8*i);
    }
    for (i = 0; i < 8; i++)
    {
        dct2_col(block + i);
    }
}

/**
  * @brief  8*8离散余弦逆变换的行变换
  * @param  blk 行的起始坐标地址
  * @retval 无
  */
static void idct2_row(float *blk)
{ 
    float tmp[16];

    //first step
    tmp[0] = blk[0]*C4 + blk[2]*C2;
    tmp[1] = blk[4]*C4 + blk[6]*C6;
    tmp[2] = blk[0]*C4 + blk[2]*C6;
    tmp[3] = -blk[4]*C4 - blk[6]*C2;
    tmp[4] = blk[0]*C4 - blk[2]*C6;
    tmp[5] = -blk[4]*C4 + blk[6]*C2;
    tmp[6] = blk[0]*C4 - blk[2]*C2;
    tmp[7] = blk[4]*C4 - blk[6]*C6;

    tmp[8] = blk[1]*C7 - blk[3]*C5;
    tmp[9] = blk[5]*C3 - blk[7]*C1;
    tmp[10] = blk[1]*C5 - blk[3]*C1;
    tmp[11] = blk[5]*C7 + blk[7]*C3;
    tmp[12] = blk[1]*C3 - blk[3]*C7;
    tmp[13] = -blk[5]*C1 - blk[7]*C5;
    tmp[14] = blk[1]*C1 + blk[3]*C3;
    tmp[15] = blk[5]*C5 + blk[7]*C7;

    //second step
    tmp[0] = 0.5f *  (tmp[0]+tmp[1]);
    tmp[1] = 0.5f * (tmp[2]+tmp[3]);
    tmp[2] = 0.5f * (tmp[4]+tmp[5]);
    tmp[3] = 0.5f * (tmp[6]+tmp[7]);
    tmp[4] = 0.5f * (tmp[8]+tmp[9]);
    tmp[5] = 0.5f * (tmp[10]+tmp[11]);
    tmp[6] = 0.5f * (tmp[12]+tmp[13]);
    tmp[7] = 0.5f * (tmp[14]+tmp[15]);

    //third step
    blk[0] = tmp[0] + tmp[7];
    blk[1] = tmp[1] + tmp[6];
    blk[2] = tmp[2] + tmp[5];
    blk[3] = tmp[3] + tmp[4];
    blk[4] = tmp[3] - tmp[4];
    blk[5] = tmp[2] - tmp[5];
    blk[6] = tmp[1] - tmp[6];
    blk[7] = tmp[0] - tmp[7];
}

/**
  * @brief  8*8离散余弦逆变换的列变换
  * @param  blk 列的起始坐标地址
  * @retval 无
  */
static void idct2_col(float *blk)
{
    float tmp[16];

    //first step
    tmp[0] = blk[0*8]*C4 + blk[2*8]*C2;
    tmp[1] = blk[4*8]*C4 + blk[6*8]*C6;
    tmp[2] = blk[0*8]*C4 + blk[2*8]*C6;
    tmp[3] = -blk[4*8]*C4 - blk[6*8]*C2;
    tmp[4] = blk[0*8]*C4 - blk[2*8]*C6;
    tmp[5] = -blk[4*8]*C4 + blk[6*8]*C2;
    tmp[6] = blk[0*8]*C4 - blk[2*8]*C2;
    tmp[7] = blk[4*8]*C4 - blk[6*8]*C6;

    tmp[8] = blk[1*8]*C7 - blk[3*8]*C5;
    tmp[9] = blk[5*8]*C3 - blk[7*8]*C1;
    tmp[10] = blk[1*8]*C5 - blk[3*8]*C1;
    tmp[11] = blk[5*8]*C7 + blk[7*8]*C3;
    tmp[12] = blk[1*8]*C3 - blk[3*8]*C7;
    tmp[13] = -blk[5*8]*C1 - blk[7*8]*C5;
    tmp[14] = blk[1*8]*C1 + blk[3*8]*C3;
    tmp[15] = blk[5*8]*C5 + blk[7*8]*C7;

    //second step
    tmp[0] = 0.5f*(tmp[0] + tmp[1]);
    tmp[1] = 0.5f*(tmp[2] + tmp[3]);
    tmp[2] = 0.5f*(tmp[4] + tmp[5]);
    tmp[3] = 0.5f*(tmp[6] + tmp[7]);
    tmp[4] = 0.5f*(tmp[8] + tmp[9]);
    tmp[5] = 0.5f*(tmp[10] + tmp[11]);
    tmp[6] = 0.5f*(tmp[12] + tmp[13]);
    tmp[7] = 0.5f*(tmp[14] + tmp[15]);

    //third step
    blk[0*8] = tmp[0] + tmp[7];
    blk[1*8] = tmp[1] + tmp[6];
    blk[2*8] = tmp[2] + tmp[5];
    blk[3*8] = tmp[3] + tmp[4];
    blk[4*8] = tmp[3] - tmp[4];
    blk[5*8] = tmp[2] - tmp[5];
    blk[6*8] = tmp[1] - tmp[6];
    blk[7*8] = tmp[0] - tmp[7];
}

/**
  * @brief  8*8快速离散余弦逆变换
  * @param  block 8*8数据块的起始地址
  * @retval 无
  */
void jpeg_inv_dct2(float *block)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        idct2_row(block + 8*i);
    }
    for (i = 0; i < 8; i++)
    {
        idct2_col(block + i);
    }
}

/**
  * @brief  将8*8数据块按照量化表进行量化
  * @param  in  待量化的数据
  * @param  out 保存量化后的结果
  * @retval 无
  */
void jpeg_quantify(float in[64], int8_t out[64])
{
    uint16_t i, j;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            out[i*8+j] = (int8_t)(in[i*8+j] / bright_code[i][j]);
        }
    }
}

/**
  * @brief  将8*8数据块按照量化表进行反量化
  * @param  in  待反量化的数据
  * @param  out 保存反量化后的结果
  * @retval 无
  */
void jpeg_inv_quantify(int8_t in[64], float  out[64])
{
    uint16_t i, j;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            out[i*8+j] = (float)in[i*8+j] * bright_code[i][j];
        }
    }
}

/**
  * @brief  将8*8数据块进行Z形排序
  * @param  data  待排序的数据, 排序后的结果也保存于它
  * @retval 无
  */
void jpeg_zigzag(int8_t data[64])
{
    uint16_t i, j;
    int8_t temp[64];
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            temp[zigzag_code[i][j]] = data[i*8+j]; 
        }
    }
    for(i = 0; i < 64; i++)
    {
        data[i] = temp[i];
    }    
}

/**
  * @brief  将8*8数据块进行Z形反排序
  * @param  data  待反排序的数据, 排序后的结果也保存于它
  * @retval 无
  */
void jpeg_inv_zigzag(int8_t data[64])
{
    uint16_t i, j;
    int8_t temp[64];
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            temp[i*8+j] = data[zigzag_code[i][j]];  
        }
    }
    for(i = 0; i < 64; i++)
    {
        data[i] = temp[i];
    }      
}

/**
  * @brief  将8*8数据块进行行程编码
  * @param  in  待编码的数据
  * @param  out 存储编码后的数据
  * @retval 无
  * @attention 本函数最多行成20组行程编码
  */
void jpeg_rle(int8_t in[64], int8_t out[60])
{
    uint16_t i, j;
    int8_t m = 0, n, count = 0, group = 0;

    for(i = 0; i < 60; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 8 && m < 19; i++)
    {
        for(j = 0; j < 8 && m < 19; j++)
        {
            if(in[i*8+j] == 0)
            {
                count++;
                if(count>15)
                {
                    out[m*3] = 15;
                    out[m*3+1] = 0;
                    out[m*3+2] = 0; 
                    count = 0;
                    m++;
                }               
            }
            else
            {
                n = in[i*8+j];
                while(n != 0)
                {
                    n = n / 2;
                    group++;
                }
                out[m*3] = count;
                out[m*3+1] = group;
                out[m*3+2] = in[i*8+j];
                count = 0;
                group = 0;
                m++;
            }            
        }
    }
  
    while(out[m*3+2] == 0)
    {
        m--;
    }
    m++;
    out[m*3] = 0;
    out[m*3+1] = 0;
    out[m*3+2] = 0;
}

/**
  * @brief  将行程码翻译为8*8数据块
  * @param  in  待译码的数据
  * @param  out 存储译码后的数据
  * @retval 无
  * @attention 本函数最多对20组行程码进行译码
  */
void jpeg_inv_rle(int8_t in[60], int8_t out[64])
{
    uint16_t i, n = 0, count;

    for(i = 0; i < 64; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 20; i++)
    {
        count = in[i*3];
        while(count--)
        {
            out[n++] = 0;
        }
        out[n++] = in[i*3+2];
    }
}

/**
  * @brief  将行程码进行霍夫曼编码，霍夫曼表已固定，位于hnit_jpeg.h
  * @param  in  待编码的数据
  * @param  c   已编码的数据流大小，单位bit
  * @param  out 存储编码后的数据
  * @retval 无
  * @attention 本函数一次最多对20组行程码进行编码
  */
void jpeg_huffman(int8_t in[60], uint32_t *c, uint16_t *out)
{
    uint16_t i;
    uint16_t zeros, length, huffman;
    uint32_t move, count = (*c)%16;
    uint16_t num1, mask = 0;
    for(i = 0; i < 20; i++)
    {
        zeros = in[i*3];
        length = in[i*3+1];
        
        if(in[i*3+2] < 0)
        {
            num1 = (uint16_t)(-in[i*3+2]);
            mask = 0x0001;
            mask = ~(mask << (length-1));
            num1 = num1 & mask;
        }
        else
        {
            num1 = (uint16_t)in[i*3+2];
        }
            
        huffman = huffman_ac_code[zeros*10+length][1];
        move = huffman_ac_code[zeros*10+length][0];
            
        count += move;          
        if(count > 15)
        {
            count -= 16;             
            *out = *out << (move-count) | (huffman >> count);                             
            *(++out) = 0;                
            mask = 0xFFFF;
            mask = ~(mask << count);
            *out = *out | (huffman & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << move | huffman; 
        }  
        
        if(length == 0 && zeros != 15)
        {
            *c/=16;
            *c*=16;
            *c += count;
            return;
        }
       
        count += length;
        if(count > 15)
        {
            count = count -16;             
            *out = *out << (length-count) | (num1 >> count);                             
            *(++out) = 0;                
            mask = 0xFFFF;
            mask = ~(mask << count);
            *out = *out | (num1 & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << length | num1; 
        }        
    }   
}

/**
  * @brief  将已知数据流长度的霍夫曼编码译码为行程码，霍夫曼表已固定，位于hnit_jpeg.h
  * @param  in  霍夫曼数据流
  * @param  c   霍夫曼数据流长度，单位bit
  * @param  s   已译码完成的数据流长度，单位bit
  * @param  out 存储译码后的数据
  * @retval 无
  * @attention 本函数一次最多译码为20组行程码
  */
void jpeg_inv_huffman(uint16_t *in, uint32_t *c, uint32_t *s, int8_t out[60])
{
    uint16_t n = 0, m = 0, k = 0;
    uint16_t zeros, length, huffman;
    uint16_t temp = 0;
    uint32_t count = *c, sum = *s;
    
    for(n = 0; n < 60; n++)
    {
        out[n] = 0;
    }   
    
    while(sum < count)
    {      
        temp <<= 1;
        if((*in & 0x8000) == 0x8000)
        {              
            temp |= 0x0001;
        }              
        *in <<= 1;
        sum++;
        if(sum%16 == 0)
        {
            in++; 
        }
        k++; 
        
        //TODO 查询太慢，需建立专用查询表
        for(n = 0; n < 161; n++)
        {
            if(k == huffman_ac_code[n][0] && temp == huffman_ac_code[n][1])
            {
                k = 0;
                break;
            }
        }

        if(n == 0)
        {
            *c = count;
            *s = sum;
            return;
        }      
        if(n != 161)
        {           
            zeros = n / 10;
            out[3*m] = zeros;
            length = n % 10; 
            out[3*m+1] = length;
                                     
            if(length == 0)
            {
                huffman = 0;
            }
            else
            {
                if((*in & 0x8000) == 0x8000)
                {
                    out[3*m+2] = 1;
                }
                else
                {
                    out[3*m+2] = -1;
                }
                *in <<= 1;
                sum++;
                if(sum%16 == 0)
                {
                    in++;
                }            
                huffman = 0x0001;
                
                length--;
                while(length--)
                {
                    huffman <<= 1;
                    if((*in & 0x8000) == 0x8000)
                    {
                        huffman |= 0x0001;
                    }
                    *in <<= 1;
                    sum++;
                    if(sum%16 == 0)
                    {
                        in++;
                    }
                }                  
            }                
                                         
            out[3*m+2] *= (int8_t)huffman;               
            m++;
            temp = 0;           
        }
    }
}    

/**
  * @brief  320*240的灰度图像的JPEG编码
  * @param  num  存储已编码的数据流大小，单位bit
  * @param  select 0/1/2分别对应Y/U/V层，用于选择本次编码的图层
  * @public test_image  待编码的图像数据
  * @public encode  保存编码的数据
  * @retval 无
  */
void jpeg_encode(uint32_t *num, uint8_t select)
{   
    int8_t dct[64] = {0}, rle[60] = {0}; 
    uint16_t i, j, m, n, temp[64];
    float bright[64];
   
    for(i = 0; i < 40; i++)
    {
        for(j = 0; j < 30; j++)
        {                   
            for(m = 0; m < 8; m++)
            {
                for(n = 0; n < 8; n++)
                {
                    temp[m*8+n] = test_image[(i*8+m)*240+j*8+n];
                }
            }
             
            rgb565_to_yuv(temp, bright, select);
            
            #ifdef JPEG_DEBUG
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("原始数据：\n");
                printf_array_f(bright, 8, 8);
            }
            #endif
            
            jpeg_dct2(bright);
            #ifdef JPEG_DEBUG
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("DCT变换结果：\n");
                printf_array_f(bright, 8, 8);
            }
            #endif
            
            jpeg_quantify(bright, dct);
            #ifdef JPEG_DEBUG            
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("量化结果：\n");
                printf_array_d(dct, 8, 8);
            }
            #endif
            
            jpeg_zigzag(dct);
            #ifdef JPEG_DEBUG
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("Zigzag排序结果：\n");
                printf_array_d(dct, 8, 8);
            }     
            #endif
            
            jpeg_rle(dct, rle);
            #ifdef JPEG_DEBUG
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("行程编码结果：\n");
                printf_array_d(rle, 20, 3);
            } 
            debug = *num;
            #endif
                      
            jpeg_huffman(rle, num, encode+*num/16);
            #ifdef JPEG_DEBUG
            if(i == DEBUG_I && j == DEBUG_J)
            {
                printf("霍夫曼编码结果：\n");
                printf_array_b(encode, debug, *num);
            }   
            #endif
        }              
    }
    encode[*num/16] <<= (16-*num%16);
}

/**
  * @brief  320*240的灰度图像的JPEG解码
  * @param  num    已编码的数据流大小，单位bit
  * @param  select 0/1/2分别对应Y/U/V层，用于选择本次解码的图层
  * @public yuv    保存解码后的YUV数据
  * @public encode 霍夫曼编码数据流
  * @retval 无
  */
void jpeg_decode(uint32_t *num, uint8_t select)
{
    uint32_t i = 0, m, n;
    uint32_t i_num = 0;
    int8_t i_rle[60] = {0};
    int8_t i_dct[64] = {0}; 
    float temp[64] = {0};
   
    while(i_num/16 < *num/16)
    {  
        jpeg_inv_huffman(encode+i_num/16, num, &i_num, i_rle);        
        #ifdef JPEG_DEBUG
        if(i == DEBUG_I*30+DEBUG_J)
        {
            printf("霍夫曼解码结果：\n");
            printf_array_d(i_rle, 20, 3);
        }      
        #endif
        
        jpeg_inv_rle(i_rle, i_dct);
        #ifdef JPEG_DEBUG
        if(i == DEBUG_I*30+DEBUG_J)
        {
            printf("行程解码结果：\n");
            printf_array_d(i_dct, 8, 8);
        }      
        #endif
        
        jpeg_inv_zigzag(i_dct);
        #ifdef JPEG_DEBUG
        if(i == DEBUG_I*30+DEBUG_J)
        {
            printf("Zigzag逆排序结果：\n");
            printf_array_d(i_dct, 8, 8);
        }      
        #endif
        
        jpeg_inv_quantify(i_dct, temp);
        #ifdef JPEG_DEBUG
        if(i == DEBUG_I*30+DEBUG_J)
        {
            printf("反量化结果：\n");
            printf_array_f(temp, 8, 8);
        }      
        #endif
        
        jpeg_inv_dct2(temp); 
        #ifdef JPEG_DEBUG
        if(i == DEBUG_I*30+DEBUG_J)
        {
            printf("DCT逆变换结果：\n");
            printf_array_f(temp, 8, 8);
        }      
        #endif
                   
        for(m = 0; m < 8; m++)
        {
            for(n = 0; n < 8; n++)
            {
                yuv[select][((i/30)*8+m)*240+(i%30)*8+n] = (int8_t)temp[m*8+n];
            }
        }
        i++;
    } 
}

/**
  * @brief  LCD显示320*240的彩色图像
  * @param  无
  * @retval 无
  */
void lcd_show_rgb565(uint16_t m[76800])
{
    uint32_t i;
    lcd_scan_dir(3);
    lcd_set_cursor(0, 0);          
    lcd_write_ram_prepare();   
    for(i = 0; i < 76800; i++)
    {
        LCD->LCD_RAM = m[i];
    }
}
/**
  * @brief  LCD显示320*240的彩色图像
  * @param  无
  * @retval 无
  * @attention 输入数据为const uint16_t
  */
void lcd_show_const_rgb565(const uint16_t m[76800])
{
    uint32_t i;
    lcd_scan_dir(3);
    lcd_set_cursor(0, 0);          
    lcd_write_ram_prepare();   
    for(i = 0; i < 76800; i++)
    {
        LCD->LCD_RAM = m[i];
    }
}

/**
  * @brief  JPEG编码、译码测试，先对320*240图像进行编码和译码，结果显示至液晶
  * @public test_image 原始图像数据
  * @public encode 霍夫曼数据流
  * @public yuv    解码后的YUV数据
  * @public image  YUV转换的RGB565数据
  * @param  无
  * @retval 无
  */
void jpeg_test(void)
{
    uint32_t size = 0;        //已压缩图像数据流大小，单位bit
    uint32_t size_all = 0;    //整张图像编码数据流长度，单位bit
    memset(encode, 0, 15360);        
    memset(image, 0, 153600);
   
    lcd_show_const_rgb565(test_image);

    //亮度层
    size = 0;
    jpeg_encode(&size, 0);                     
    jpeg_decode(&size, 0); 
    size_all += size;
    //色度层
    size = 0;    
    jpeg_encode(&size, 1);                     
    jpeg_decode(&size, 1);
    size_all += size;
    //饱和度层
    size = 0;
    jpeg_encode(&size, 2);                     
    jpeg_decode(&size, 2);     
    size_all += size;
    
    printf("\n JPEG数据流长度为 %d byte\n", size_all/8);
    yuv_to_rgb565();
    lcd_show_rgb565(image); 
}

/* ------------------------------------ 串口调试部分 ---------------------------------------- */

#ifdef JPEG_DEBUG
//浮点数组显示
void printf_array_f(float n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %f ", n[i*y+j]);
        }
        printf("\n");
    }
    printf("\n");
}

//整型数组显示
void printf_array_d(int8_t n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %d ", n[i*y+j]);
        }
        printf("\n");
    }
    printf("\n");
}

//无符号整型数组显示
void printf_array_u(uint16_t n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %d ", n[i*y+j]);
        }
        printf("\n");
    }
    printf("\n");
}

//二值数组显示
void printf_array_b(uint16_t n[], uint32_t start, uint32_t end)
{
    uint16_t i, j, temp;
    for(i = start/16; i <= end/16; i++)
    {
        temp = n[i];
        for(j = 0; j < 16; j++)
        {
            if(temp&0x8000)
            {
                printf(" %d", 1);
            }
            else
            {
                printf(" %d", 0);
            }
            temp<<=1;
        }
        printf("\n");
    }
    printf("\n");
}
#endif

/******************************* END OF FILE *********************************/
