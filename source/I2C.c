/*
 * i2c.c
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */

#include "I2C.h"
#include "fsl_gpio.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "fsl_debug_console.h"

uint32_t baudRate = 40000;
uint32_t frequency = 1000000;

void initI2C(void) {
    BOARD_InitDebugConsole();
    CLOCK_Select(kI2C1_Clk_From_MainClk);
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27); // SDA en P0_27
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26); // SCL en P0_26
    CLOCK_DisableClock(kCLOCK_Swm);

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = baudRate;
    I2C_MasterInit((I2C_Type *)I2C1_BASE, &masterConfig, frequency);
}


void I2C_ResetBus(I2C_Type *base) {
    // Deshabilitar el módulo I2C
    base->CFG &= ~I2C_CFG_MSTEN_MASK;

    // Limpiar los flags de estado
    base->STAT = I2C_STAT_MSTPENDING_MASK |
                 I2C_STAT_MSTARBLOSS_MASK |
                 I2C_STAT_MSTSTSTPERR_MASK;

    // Rehabilitar el módulo I2C
    base->CFG |= I2C_CFG_MSTEN_MASK;
}
