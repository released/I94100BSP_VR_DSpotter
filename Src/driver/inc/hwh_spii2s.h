

#ifndef SPI_I2S_H
#define SPI_I2S_H

#include "stdint.h"

void SPI1_I2S_Init(void);
void SPI2_I2S_Init(void);
void SPI1_I2S_Stop(void);
void SPI2_I2S_Stop(void);
void SPI1_I2S_Start(void);
void SPI2_I2S_Start(void);

void SPI2_I2S_PDMA(void);
void SPI2_I2S_TX_Send(uint32_t *add, uint32_t len);  //µ¥´Î´«ËÍ
extern uint32_t SPI2_I2S_decode[4][Frame_Len];
#endif

