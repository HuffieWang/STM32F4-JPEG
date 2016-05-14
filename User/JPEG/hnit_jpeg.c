/*********************** HNIT 3103 Application Team **************************
 * 文 件 名 ：hnit_jpeg.c
 * 描    述 ：JPEG图像压缩        
 * 实验平台 ：STM32F407开发板
 * 库 版 本 ：ST1.4.0
 * 时    间 ：2016.xx
 * 作    者 ：3103创新团队
 * 修改记录 ：无
******************************************************************************/
#include "jpeg_data.h"
#include "hnit_jpeg.h"
#include "math.h"
#include "string.h"
#include "stm32_usart.h"
#include "hnit_lcd.h"
#include "hnit_key.h"

#define M_PI  3.1415926f

uint32_t size = 0;
uint16_t encode[76800] __attribute__((at(0X68000000)));
uint16_t image[76800] __attribute__((at(0X68000000+sizeof(encode))));;

float i_yuv_y[64] = {0};
uint32_t i_size = 0;

void rgb_to_yuv(uint16_t rgb[3840], uint16_t yuv[3840])
{
    uint16_t i, temp;
    float red, green, blue;
    
    for(i = 0; i < 3840; i++)
    {
        red = (float)((rgb[i]&0xF800)>>8);   
        green = (float)((rgb[i]&0x07E0)>>3);
        blue = (float)((rgb[i]&0x001F)<<3);
                         
        temp = (uint16_t)(0.299f*red + 0.587f*green + 0.114f*blue); 
        temp >>= 3;
        yuv[i] = (temp<<11 | temp<<6 | temp);
    }
}

//图像格式转换 仅取亮度
void rgb565_to_yuv(uint16_t rgb[64], float y[64])
{
    uint16_t i;
    float red, green, blue;
    
    for(i = 0; i < 64; i++)  
    {
        red = (float)((rgb[i]&0xF800)>>8);   
        green = (float)((rgb[i]&0x07E0)>>3);
        blue = (float)((rgb[i]&0x001F)<<3);
                         
        y[i] = 0.299f*red + 0.587f*green + 0.114f*blue -128;        
    }
}

//由亮度生成灰度图
void yuv_to_rgb565(float y[64], uint16_t rgb[64])
{
    uint16_t i, temp;
    for(i = 0; i < 64; i++)
    {        
        temp = (uint16_t)(y[i] + 128);
        temp >>= 3;
        rgb[i] = (temp<<11 | temp<<6 | temp);
    }
}

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
}

//二值数组显示
void printf_array_b(uint16_t n[], uint16_t x)
{
    uint16_t i, j, temp;
    for(i = 0; i <= x; i++)
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
}

static float alpha(int n)
{
    if (n == 0) 
        return 1.0f / (float)sqrt(8);
    else        
        return 1.0f / 2.0f;
}

                           
//离散余弦变换
void jpeg_dct2(float data[64])
{
    int u, v;
    int x, y, i;
    float buf[64];
    float temp;

    for(u = 0; u < 8; u++)
    {
        for(v = 0; v < 8; v++)
        {
            temp = 0;
            for(x = 0; x < 8; x++)
            {
                for(y=0; y < 8; y++)
                {
                    temp += data[y*8+x]
                          * (float)cos((2.0f * x + 1.0f) / 16.0f * u * M_PI)
                          * (float)cos((2.0f * y + 1.0f) / 16.0f * v * M_PI);
                }
            }
            buf[v*8+u] = alpha(u) * alpha(v) * temp;
        }
    }

    for(i = 0; i < 64; i++)
    {
        data[i] = buf[i];
    }
}

//离散余弦逆变换
void jpeg_inv_dct2(float data[64])
{
    int u, v;
    int x, y, i;
    float buf[64];
    float temp;

    for (x = 0; x < 8; x++)
    {
        for (y = 0; y < 8; y++)
        {
            temp = 0;
            for (u = 0; u < 8; u++)
            {
                for (v = 0; v < 8; v++)
                {
                    temp += alpha(u) * alpha(v) * data[v*8+u]
                          * (float)cos((2.0f * x + 1.0f) / 16.0f * u * M_PI)
                          * (float)cos((2.0f * y + 1.0f) / 16.0f * v * M_PI);                
                }
            }
            buf[y*8+x] = temp;
        }
    }
    for(i = 0; i < 64; i++)
    {
        data[i] = buf[i];
    }
}

//量化
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

//游程编码
void jpeg_rle(int8_t in[64], int8_t out[30])
{
    uint16_t i, j;
    int8_t m = 0, n, count = 0, group = 0;

    for(i = 0; i < 30; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(in[i*8+j] == 0)
            {
                count++;
                if(count>15)
                {
                    out[m*3] = 15;
                    out[m*3+1] = 0;
                    out[m*3+2] = 0;                    
                }
                count = 0;
                m++;
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
            if(m == 10)
            {
                out[27] = 0;
                out[28] = 0;
                out[29] = 0;
                return;
            }
        }
    }
}
void jpeg_inv_rle(int8_t in[30], int8_t out[64])
{
    uint16_t i, n = 0, count;
    
    for(i = 0; i < 30; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 10; i++)
    {
        count = in[i*3];
        while(count--)
        {
            out[n++] = 0;
        }
        out[n++] = in[i*3+2];
    }
}

//霍夫曼编码
void jpeg_huffman(int8_t in[30], uint32_t *c, uint16_t *out)
{
    uint16_t i;
    uint16_t zeros, length, huffman;
    uint32_t move, count = (*c)%16;
    uint16_t num1, mask = 0;
    for(i = 0; i < 10; i++)
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
//TODO调试用            printf("move:%d  %d  %d\n",move, zeros, length);
            count = count -16;             
            *out = *out << (move-count) | (huffman >> count);                             
            *(++out) = 0;                
            mask = 0xFFFF;
            mask = ~(mask <<= count);
            *out = *out | (huffman & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << move | huffman; 
        }  
        
        if(length == 0)
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
            mask = ~(mask <<= count);
            *out = *out | (num1 & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << length | num1; 
        }        
    }   
}

//霍夫曼解码
void jpeg_inv_huffman(uint16_t *in, uint32_t *c, uint32_t *s, int8_t out[30])
{
    uint16_t n, m = 0, k = 0;
    uint16_t zeros, length, huffman;
    uint16_t temp = 0;
    uint32_t count = *c, sum = *s;
    
    while(sum <= count)
    {      
        temp <<= 1;
        if((*in & 0x8000) == 0x8000)
        {              
            temp |= 0x0001;
        }
        k++;
        
        *in <<= 1;
        sum++;
        if(sum%16 == 0)
            in++; 
        
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
                in++;
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
                    in++;
            }                               
            
            out[3*m+2] *= (int8_t)huffman;               
            m++;
            temp = 0;           
        }
    }
}    

#define IDEBUG  39
#define JDEBUG  29

void jpeg_encode(uint32_t *num)
{   
    int8_t dct[64] = {0}, rle[30] = {0}; 
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
  
            rgb565_to_yuv(temp, bright);
            if(i == IDEBUG && j == JDEBUG)
            {  
                printf("\n 颜色格式变换结果：\n");
                printf_array_f(bright, 8, 8);            
            }
            
            jpeg_dct2(bright); 
            if(i == IDEBUG && j == JDEBUG)
            {  
//                printf("\n DCT变换结果：\n");
//                printf_array_f(bright, 8, 8);
            }
            jpeg_quantify(bright, dct); 
            if(i == IDEBUG && j == JDEBUG)
            {  
//                printf("\n 量化结果：\n");             
//                printf_array_d(dct, 8, 8);
            }
            jpeg_zigzag(dct); 
            if(i == IDEBUG && j == JDEBUG)
            {            
//                printf("\n Zigzag排序结果：\n");
//                printf_array_d(dct, 8, 8);
            }
            
            jpeg_rle(dct, rle);       
            if(i == IDEBUG && j == JDEBUG)
            {
//                printf("\n 游程编码结果：%d\n", j);
//                printf_array_d(rle, 10, 3);                
            }
          
            jpeg_huffman(rle, num, encode+*num/16);            
            if(i == IDEBUG && j == JDEBUG)
            {
//                printf("\n 霍夫曼编码结果：\n");    
//                printf_array_b(encode, size/16); 
                return;  
            }
            printf(" %d    %d %d\n", *num, i ,j);
        }
        
    }
    encode[size/16] <<= (16-size%16);
}

void jpeg_decode(uint32_t *num)
{
    uint32_t i = 0, j = 0, m, n;
    uint32_t i_num = 0;
    int8_t i_rle[30] = {0};
    int8_t i_dct[64] = {0}; 
    float temp[64] = {0};
    uint16_t rec[64] = {0};
    
    //while(i_num/16 < *num/16)        
    //TODO 停不下来？
    while(j <= IDEBUG*30+JDEBUG)
    {
        //printf("%d\n", j);
        
        jpeg_inv_huffman(encode+i_num/16, num, &i_num, i_rle);       
                     
        jpeg_inv_rle(i_rle, i_dct);
        
        jpeg_inv_zigzag(i_dct);
        
        jpeg_inv_quantify(i_dct, temp);
        
        jpeg_inv_dct2(temp);        
        if(j == IDEBUG*30+JDEBUG)
        {
            printf("\n DCT逆变换结果：\n");
            printf_array_f(temp, 8, 8);                
        }       
        
        yuv_to_rgb565(temp, rec);
        
        
        for(m = 0; m < 8; m++)
        {
            for(n = 0; n < 8; n++)
            {
                image[((j/30)*8+m)*240+(j%30)*8+n] = rec[m*8+n];
            }
        }
        
//        if(j == IDEBUG*30+JDEBUG)
//        {
//            printf("\n 颜色格式转换结果：\n");
//            printf_array_u(rec, 8, 8);                
//        }          
               
        for(i = 0; i < 30;i++)
            i_rle[i] = 0;
        j++;       
    } 
}

void lcd_show_image2(uint16_t m[76800])
{
    uint32_t i;
    lcd_scan_dir(3);
    lcd_set_cursor(0, 0);  // 设置光标位置                   
    lcd_write_ram_prepare();   // 开始写入GRAM
    for(i = 0; i < 76800; i++)
    {
        LCD->LCD_RAM = m[i];
    }
}

void jpeg_test(void)
{
    memset(encode, 0, 153600);
         
    memset(image, 0, 153600);
     
    jpeg_encode(&size);   
    
    printf("\n %d \n", size);
    
    jpeg_decode(&size); 
    
    lcd_show_image2(image);
}


/******************************* END OF FILE *********************************/
