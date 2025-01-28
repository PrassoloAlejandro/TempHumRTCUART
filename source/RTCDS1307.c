/*
 * RTCDS1307.c
 *
 *  Created on: 28 ene. 2025
 *      Author: aleja
 */

/**
 * @file    RTC_Test.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "time.h"
#include "RTCDS1307.h"
#include "I2C.h"


#include "RTCDS1307.h"
#include "fsl_i2c.h"
#include "fsl_swm.h"
#include "fsl_iocon.h"

#define I2C_BASE I2C1_BASE
#define I2C_BAUDRATE 40000U
#define I2C_CLOCK_FREQ 1000000U

static uint8_t DecToBCD(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

static uint8_t BCDToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

static status_t RTC_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    status_t status;

    status = I2C_MasterStart(I2C_BASE, DS1307_I2C_ADDRESS, kI2C_Write);
    if (status != kStatus_Success) return status;

    status = I2C_MasterWriteBlocking(I2C_BASE, data, sizeof(data), kI2C_TransferDefaultFlag);
    if (status != kStatus_Success) return status;

    return I2C_MasterStop(I2C_BASE);
}

static status_t RTC_ReadRegisters(uint8_t reg, uint8_t *data, size_t length) {
    status_t status;

    status = I2C_MasterStart(I2C_BASE, DS1307_I2C_ADDRESS, kI2C_Write);
    if (status != kStatus_Success) return status;

    status = I2C_MasterWriteBlocking(I2C_BASE, &reg, 1, kI2C_TransferDefaultFlag);
    if (status != kStatus_Success) return status;

    status = I2C_MasterRepeatedStart(I2C_BASE, DS1307_I2C_ADDRESS, kI2C_Read);
    if (status != kStatus_Success) return status;

    status = I2C_MasterReadBlocking(I2C_BASE, data, length, kI2C_TransferDefaultFlag);
    return I2C_MasterStop(I2C_BASE);
}

void RTC_Init(void) {
    CLOCK_Select(kI2C1_Clk_From_MainClk);
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUDRATE;
    I2C_MasterInit(I2C_BASE, &masterConfig, I2C_CLOCK_FREQ);
}

status_t RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    uint8_t data[3] = {DecToBCD(seconds), DecToBCD(minutes), DecToBCD(hours)};
    for (uint8_t i = 0; i < 3; i++) {
        if (RTC_WriteRegister(i, data[i]) != kStatus_Success) {
            return kStatus_Fail;
        }
    }
    return kStatus_Success;
}

status_t RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    uint8_t data[3];
    if (RTC_ReadRegisters(0x00, data, 3) != kStatus_Success) {
        return kStatus_Fail;
    }
    *seconds = BCDToDec(data[0] & 0x7F);
    *minutes = BCDToDec(data[1]);
    *hours = BCDToDec(data[2] & 0x3F);
    return kStatus_Success;
}

status_t RTC_SetDate(uint8_t day, uint8_t month, uint8_t year) {
    uint8_t data[3] = {DecToBCD(day), DecToBCD(month), DecToBCD(year)};
    for (uint8_t i = 0; i < 3; i++) {
        if (RTC_WriteRegister(0x04 + i, data[i]) != kStatus_Success) {
            return kStatus_Fail;
        }
    }
    return kStatus_Success;
}

status_t RTC_GetDate(uint8_t *day, uint8_t *month, uint8_t *year) {
    uint8_t data[3];
    if (RTC_ReadRegisters(0x04, data, 3) != kStatus_Success) {
        return kStatus_Fail;
    }
    *day = BCDToDec(data[0]);
    *month = BCDToDec(data[1]);
    *year = BCDToDec(data[2]);
    return kStatus_Success;
}
