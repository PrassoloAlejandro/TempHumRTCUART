/*
 * aht10.c
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */

#include "aht10.h"
#include "fsl_usart.h"
#include "fsl_common.h"
#include "board.h"
#include "fsl_debug_console.h"

extern uint8_t buffer[64];
extern void Send_USART(USART_Type *base, uint8_t data, const char *texto);
extern void delay_ms(int msec);

AHT10Data Measure_AHT10(void) {
    uint8_t command[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];
    AHT10Data result = {0};

    if (kStatus_Success != I2C_MasterStart(I2C1_BASE, AHT10_ADDRESS, kI2C_Write)) {
        Send_USART(USART1, 0, "Error iniciando I2C para AHT10.\r\n");
        return result;
    }

    I2C_MasterWriteBlocking(I2C1_BASE, command, 3, 0);
    I2C_MasterStop(I2C1_BASE);
    delay_ms(75);

    I2C_MasterRepeatedStart(I2C1_BASE, AHT10_ADDRESS, kI2C_Read);
    I2C_MasterReadBlocking(I2C1_BASE, data, 6, 0);
    I2C_MasterStop(I2C1_BASE);

    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t raw_temperature = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    result.humidity = ((float)raw_humidity / 1048576) * 100;
    result.temp = ((float)raw_temperature / 1048576) * 200 - 50;

    return result;
}

