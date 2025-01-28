/*
 * uart.c
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */

#include "uart.h"
#include "fsl_gpio.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_usart.h"

// Definición de las variables
volatile uint8_t flagReceived = 0;
uint8_t dataUsart = 0;
char buffer[64];
char newline[] = "\r\n";

/*--------------------------------------
 * Inicialización de UART
 *--------------------------------------*/
void Init_UART(void) {
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_USART1_TXD, kSWM_PortPin_P0_25); // TX en P0_25
    SWM_SetMovablePinSelect(SWM0, kSWM_USART1_RXD, kSWM_PortPin_P0_24); // RX en P0_24
    CLOCK_DisableClock(kCLOCK_Swm);
    CLOCK_Select(kUART1_Clk_From_MainClk);

    usart_config_t config;
    USART_GetDefaultConfig(&config);
    config.baudRate_Bps = 9600;
    config.enableRx = true;
    config.enableTx = true;
    USART_Init(USART1, &config, CLOCK_GetFreq(kCLOCK_MainClk));

    USART_EnableInterrupts(USART1, kUSART_RxReadyInterruptEnable);
    NVIC_EnableIRQ(USART1_IRQn);
}

/*--------------------------------------
 * Transmisión por UART
 *--------------------------------------*/
void Send_USART(USART_Type *base, const char *texto) {
    USART_WriteBlocking(base, (uint8_t *)texto, strlen(texto));
}

int __attribute__((weak)) _write(int handle, const char *data, int size) {
    // Redirige los datos a UART
    USART_WriteBlocking(USART1, (uint8_t *)data, size);
    return size;
}

