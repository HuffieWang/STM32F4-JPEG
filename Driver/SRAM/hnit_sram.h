#ifndef __SRAM_H
#define __SRAM_H															    
#include "stm32_sys.h" 
										  
void fsmc_sram_init(void);
void sram_write_buffer(u8* pBuffer,u32 WriteAddr,u32 n);
void sram_read_buffer(u8* pBuffer,u32 ReadAddr,u32 n);
#endif

