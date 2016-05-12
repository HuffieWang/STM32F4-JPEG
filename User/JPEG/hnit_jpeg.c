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

#define M_PI  3.1415926f

float yuv_y[64] = {0}, yuv_u[64] = {0}, yuv_v[64] = {0};
int8_t dct[64] = {0}; 
int8_t rle[30] = {0};  
uint16_t ans[80] = {0}, ans_m = 0, ans_n = 0;

int8_t i_rle[30] = {0};
int8_t i_dct[64] = {0};
float i_yuv_y[64] = {0};

void rgb565_to_yuv(uint16_t rgb[8][8], 
                   float y[64], float u[64], float v[64])
{
    uint16_t i, j;
    float red, green, blue;
    
    for(i = 0; i < 8; i++)  
    {
        for(j = 0; j < 8; j++)
        {
            red = (float)((rgb[i][j]&0xF800)>>8);   
            green = (float)((rgb[i][j]&0x07E0)>>3);
            blue = (float)((rgb[i][j]&0x001F)<<3);
                             
            y[i*8+j] = 0.299f*red + 0.587f*green + 0.114f*blue -128;
            u[i*8+j] = -0.1687f*red - 0.3313f*green + 0.5f*blue;
            v[i*8+j] = 0.5f*red - 0.418f*green - 0.0813f*blue;             
        }
    }
}

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
void fdct2_8x8(float data[64])
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
void idct2_8x8(float data[64])
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
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(in[i*8+j] == 0)
            {
                count++;
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
                if(m == 10)
                    return;
            }
        }
    }
}
void jpeg_inv_rle(int8_t in[30], int8_t out[64])
{
    uint16_t i, n = 0, count;
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
void jpeg_huffman(int8_t in[30],uint16_t *c, uint16_t *b, uint16_t *out)
{
    uint16_t i;
    uint16_t zeros, length, huffman;
    uint16_t move, count = *b;
    uint16_t num, mask = 0;
    for(i = 0; i < 10; i++)
    {
        zeros = in[i*3];
        length = in[i*3+1];
        
        if(in[i*3+2] < 0)
        {
            num = (uint16_t)(-in[i*3+2]);
            mask = 0x0001;
            mask = ~(mask << (length-1));
            num = num & mask;
        }
        else
        {
            num = (uint16_t)in[i*3+2];
        }

        if(in[i*3+2] != 0)
        {
            huffman = huffman_ac_code[zeros*10+length][1];
            move = huffman_ac_code[zeros*10+length][0];
            
            count += move;          
            if(count > 15)
            {
                count = count -16;             
                *out = *out << (move-count) | (huffman >> count);                             
                *(++out) = 0;                
                mask = 0xFFFF;
                mask = ~(mask <<= count);
                *out = *out | (huffman & mask);
                (*c)++;
            }
            else
            {
                *out = *out << move | huffman; 
            }  
            
            count += length;
            if(count > 15)
            {
                count = count -16;             
                *out = *out << (length-count) | (num >> count);                             
                *(++out) = 0;                
                mask = 0xFFFF;
                mask = ~(mask <<= count);
                *out = *out | (num & mask);
                (*c)++;
            }
            else
            {
                *out = *out << length | num; 
            }  
        }
    } 
    
    //TODO简单测试用，待完善
    *out <<= 3;
    
    *b = count;
}

//霍夫曼解码
void jpeg_inv_huffman(uint16_t *in, uint16_t *c, int8_t out[30])
{
    uint16_t i, j, k = 0, n, m = 0;
    uint16_t zeros, length, huffman;
    uint16_t temp = 0;
       
    for(i = 0; i < *c+1; i++)
    {
        for(j = 0; j < 16; j++)
        {
            k++;
            temp <<= 1;
            if((*in & 0x8000) == 0x8000)
            {              
                temp |= 0x0001;
            }
            
            *in <<= 1;
            for(n = 0; n < 161; n++)
            {
                if(k == huffman_ac_code[n][0] && temp == huffman_ac_code[n][1])
                    break;
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
                huffman = 0x0001;
                length--;
                j++;
                while(length--)
                {
                    huffman <<= 1;
                    if((*in & 0x8000) == 0x8000)
                    {
                        huffman |= 0x0001;
                    }
                    *in <<= 1;
                    j++;
                }                               
                
                out[3*m+2] *= (int8_t)huffman;               

                m++;
                temp = 0;
                k = 0;
            }
        }
        in++;
    }
    //TODO 待完善：EOB结束
    m--;
    out[3*m] = 0;
    out[3*m+1] = 0;
    out[3*m+2] = 0;
}    


void jpeg_test(void)
{
    rgb565_to_yuv(test_image, yuv_y, yuv_u, yuv_v);
    printf("\n 原始数据：\n");
    printf_array_f(yuv_y, 8, 8);
    
    fdct2_8x8(yuv_y);
    //printf("\n DCT变换结果：\n");
    //printf_array_f(yuv_y, 8, 8);
    
    jpeg_quantify(yuv_y, dct);
    //printf("\n 量化结果：\n");   
    //printf_array_d(dct, 8, 8);
        
    jpeg_zigzag(dct);
    //printf("\n Zigzag排序结果：\n");
    //printf_array_d(dct, 8, 8);
    
    jpeg_rle(dct, rle);
    //printf("\n 游程编码结果：\n");
    //printf_array_d(rle, 10, 3);
    
    printf("\n 霍夫曼编码结果：\n");
    jpeg_huffman(rle, &ans_m, &ans_n, ans+ans_m); 
    printf_array_b(ans, ans_m);  
    
    //printf("\n 霍夫曼解码结果：\n");
    jpeg_inv_huffman(ans, &ans_m, i_rle); 
    //printf_array_d(i_rle, 10, 3); 
    
    //printf("\n 游程解码结果：\n");
    jpeg_inv_rle(i_rle, i_dct);
    //printf_array_d(i_dct, 8, 8);
    
    //printf("\n Ziazag反排序结果：\n");
    jpeg_inv_zigzag(i_dct);
    //printf_array_d(i_dct, 8, 8);
    
    //printf("\n 反量化结果：\n");
    jpeg_inv_quantify(i_dct, i_yuv_y);
    //printf_array_f(i_yuv_y, 8, 8);
    
    printf("\n DCT逆变换结果：\n");
    idct2_8x8(i_yuv_y);
    printf_array_f(i_yuv_y, 8, 8);
}



/******************************* END OF FILE *********************************/
