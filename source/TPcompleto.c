#include <stdio.h>
#include <string.h>
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"
#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_usart.h"
#include "uart.h"
#include "I2C.h"
#include "aht10.h"
#include "RTCDS1307.h" // Se incluye el encabezado del RTC

volatile uint16_t flagTick = 0;

// Prototipos
void delay_ms(int msec);
void SysTick_Init(uint32_t ticks);

int main(void) {
    AHT10Data sensorData;
    uint8_t hours, minutes, seconds;
    uint8_t day, month, year;
    char buffer[128];

    // Inicialización del sistema
    SysTick_Init((SystemCoreClock / 1000) - 1);
    initI2C();
    Init_UART();
    RTC_Init(); // Inicialización del RTC

    // Configuración manual de la fecha y hora
    RTC_SetTime(18, 28, 0);
    RTC_SetDate(28, 1, 25);

    // Mensaje inicial
    Send_USART(USART1, "Inicio del programa AHT10 y DS1307 con UART.\r\n");

    while (1) {
        /* Lee temperatura y humedad del AHT10 */
        sensorData = Measure_AHT10();

        /* Lee fecha y hora del RTC DS1307 */
        RTC_GetTime(&hours, &minutes, &seconds);
        RTC_GetDate(&day, &month, &year);

        /* Formatea los datos y los envía por UART */
        sprintf(buffer,
                "Temperatura: %d%cC | Humedad: %d%% | Fecha: %02d/%02d/%02d | Hora: %02d:%02d:%02d\r\n",
                sensorData.temp, 176, sensorData.humidity,
                day, month, year,
                hours, minutes, seconds);
        Send_USART(USART1, buffer);

        delay_ms(1000); // Espera 1 segundo antes de la siguiente iteración
    }

    return 0;
}

/*--------------------------------------
 * Inicialización de SysTick
 *--------------------------------------*/
void SysTick_Init(uint32_t ticks) {
    SysTick->LOAD = ticks;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}

/*--------------------------------------
 * Handler de interrupción del SysTick
 *--------------------------------------*/
void SysTick_Handler(void) {
    flagTick++;
}

/*--------------------------------------
 * Función de delay en milisegundos
 *--------------------------------------*/
void delay_ms(int msec) {
    flagTick = 0;
    while (flagTick <= msec);
}

