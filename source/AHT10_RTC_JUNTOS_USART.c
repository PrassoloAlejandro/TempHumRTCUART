/*
 * Programa para recolectar datos del AHT10 y DS1307 y enviarlos por UART.
 * Combina las funcionalidades de manejo I2C, UART y SysTick.
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
#include "fsl_usart.h"
#include "time.h"

/* Configuración del AHT10 */
#define AHT10_ADDRESS 0x38
typedef struct {
    uint32_t temp;
    uint32_t humidity;
} AHT10Data;

/* Configuración del DS1307 */
#define DS1307_I2C_ADDRESS 0x68

/* Variables compartidas */
volatile uint16_t flagTick = 0;
uint32_t baudRate = 40000;
uint32_t frequency = 1000000;

/* UART Variables */
uint8_t flagReceived = 0, dataUsart;
char buffer[64]; // Buffer para transmisión UART
char newline[] = "\r\n";

/*--------------------------------------
 * Prototipos de Funciones
 *--------------------------------------*/
void delay_ms(int msec);
void SysTick_Init(uint32_t ticks);
void initI2C(void);
void Init_UART(void);
void Send_USART(USART_Type *base, uint8_t data, const char *texto);

AHT10Data Measure_AHT10(void);

status_t DS1307_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
status_t DS1307_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
status_t DS1307_GetDate(uint8_t *day, uint8_t *month, uint8_t *year);
status_t DS1307_SetDate(uint8_t day, uint8_t month, uint8_t year);
uint8_t BCDToDec(uint8_t val);
uint8_t DecToBCD(uint8_t val);
void setCurrentTime(void);

/*--------------------------------------
 * Función Principal
 *--------------------------------------*/
int main(void) {
    uint8_t hours, minutes, seconds;
    uint8_t day, month, year;
    AHT10Data sensorData;

    /* Inicializar SysTick, I2C y UART */
    SysTick_Init((SystemCoreClock / 1000) - 1);
    initI2C();

    /* Configura la fecha y hora inicial en el DS1307 */
    setCurrentTime();
    Init_UART();
    /* Mensaje inicial */
    Send_USART(USART1, 0, "Inicio del programa AHT10 y DS1307 con UART.\r\n");

    while (1) {
        /* Lee temperatura y humedad del AHT10 */
        sensorData = Measure_AHT10();
        snprintf(buffer, sizeof(buffer), "Temperatura: %d°C\r\n", sensorData.temp);
        Send_USART(USART1, 0, buffer);
        snprintf(buffer, sizeof(buffer), "Humedad: %d%%\r\n", sensorData.humidity);
        Send_USART(USART1, 0, buffer);

        /* Lee fecha y hora del DS1307 */
        if (DS1307_GetTime(&hours, &minutes, &seconds) == kStatus_Success &&
            DS1307_GetDate(&day, &month, &year) == kStatus_Success) {
            snprintf(buffer, sizeof(buffer), "Fecha: %02d/%02d/%02d Hora: %02d:%02d:%02d\r\n",
                     day, month, year, hours, minutes, seconds);
            Send_USART(USART1, 0, buffer);
        } else {
            Send_USART(USART1, 0, "Error al leer fecha/hora del DS1307.\r\n");
        }

        delay_ms(1000); // Espera 1 segundo antes de la siguiente lectura
    }
}

/*--------------------------------------
 * Inicialización de SysTick
 *--------------------------------------*/
void SysTick_Init(uint32_t ticks) {
    SysTick->LOAD = ticks;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void) {
    flagTick++;
}

void delay_ms(int msec) {
    flagTick = 0;
    while (flagTick <= msec);
}

/*--------------------------------------
 * Inicialización de I2C
 *--------------------------------------*/
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
    I2C_MasterInit(I2C1_BASE, &masterConfig, frequency);
}

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
void Send_USART(USART_Type *base, uint8_t data, const char *texto) {
    sprintf(buffer, texto, data);
    USART_WriteBlocking(base, (uint8_t *)buffer, strlen(buffer));
}

/*--------------------------------------
 * AHT10: Lectura de Temperatura y Humedad
 *--------------------------------------*/
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

/*--------------------------------------
 * DS1307: Funciones RTC
 *--------------------------------------*/
/* Reutiliza las funciones DS1307 del archivo original */
uint8_t DecToBCD(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

uint8_t BCDToDec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

/*--------------------------------------
 * I2C Write Function
 *--------------------------------------*/
status_t DS1307_WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    status_t status;

    status = I2C_MasterStart(I2C1_BASE, DS1307_I2C_ADDRESS, kI2C_Write);
    if (status != kStatus_Success) {
        printf("I2C Start Failed! Status: %d\n", status);
        return status;
    }

    status = I2C_MasterWriteBlocking(I2C1_BASE, data, sizeof(data), kI2C_TransferDefaultFlag);
    if (status != kStatus_Success) {
        printf("I2C Write Failed! Status: %d\n", status);
        return status;
    }

    status = I2C_MasterStop(I2C1_BASE);
    if (status != kStatus_Success) {
        printf("I2C Stop Failed! Status: %d\n", status);
    }

    return status;
}

/*--------------------------------------
 * I2C Read Function
 *--------------------------------------*/
status_t DS1307_ReadRegisters(uint8_t reg, uint8_t *data, size_t length) {
    status_t status;

    if (kStatus_Success == I2C_MasterStart(I2C1_BASE, DS1307_I2C_ADDRESS, kI2C_Write)) {
        status = I2C_MasterWriteBlocking(I2C1_BASE, &reg, 1, kI2C_TransferDefaultFlag);

        if (kStatus_Success == I2C_MasterRepeatedStart(I2C1_BASE, DS1307_I2C_ADDRESS, kI2C_Read)) {
            status = I2C_MasterReadBlocking(I2C1_BASE, data, length, kI2C_TransferDefaultFlag);
            status = I2C_MasterStop(I2C1_BASE);
        }
    }

    return status;
}

/*--------------------------------------
 * Set RTC Time Function
 *--------------------------------------*/
status_t DS1307_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    uint8_t data[3];
    data[0] = DecToBCD(seconds); // Register 0x00
    data[1] = DecToBCD(minutes); // Register 0x01
    data[2] = DecToBCD(hours);   // Register 0x02

    for (uint8_t i = 0; i < 3; i++) {
        if (DS1307_WriteRegister(i, data[i]) != kStatus_Success) {
            return kStatus_Fail;
        }
    }

    return kStatus_Success;
}

/*--------------------------------------
 * Get RTC Time Function
 *--------------------------------------*/
status_t DS1307_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds) {
    uint8_t data[3];

    if (DS1307_ReadRegisters(0x00, data, 3) != kStatus_Success) {
        return kStatus_Fail;
    }

    *seconds = BCDToDec(data[0] & 0x7F); // Mask CH bit
    *minutes = BCDToDec(data[1]);
    *hours = BCDToDec(data[2] & 0x3F);  // 24-hour format

    return kStatus_Success;
}

/*--------------------------------------
 * Set System Time Function
 *--------------------------------------*/
void setCurrentTime(void) {
    time_t rawTime;
    struct tm *utcTime;

    rawTime = time(NULL);
    if (rawTime == -1) {
        printf("Failed to get the current time.\n");
        return;
    }

    utcTime = gmtime(&rawTime);
    if (utcTime == NULL) {
        printf("Failed to convert the current time to UTC.\n");
        return;
    }

    utcTime->tm_hour -= 3; //Ajuste UTC-3
    if (utcTime->tm_hour < 0) {
        utcTime->tm_hour += 24;
        utcTime->tm_mday -= 1;
    }

    printf("UTC-3 Date: %02d/%02d/%02d Time: %02d:%02d:%02d\n",
               utcTime->tm_mday, utcTime->tm_mon + 1, utcTime->tm_year % 100,
               utcTime->tm_hour, utcTime->tm_min, utcTime->tm_sec);
    //printf("UTC-3 Time: %02d:%02d:%02d\n", utcTime->tm_hour, utcTime->tm_min, utcTime->tm_sec);

    if (DS1307_SetTime(utcTime->tm_hour, utcTime->tm_min, utcTime->tm_sec) == kStatus_Success &&
		DS1307_SetDate(utcTime->tm_mday, utcTime->tm_mon + 1, utcTime->tm_year % 100) == kStatus_Success) {
		printf("Date and Time set successfully!\n");
	} else {
		printf("Failed to set date and time!\n");
	}
}

/*--------------------------------------
 * Set RTC Date Function
 *--------------------------------------*/
status_t DS1307_SetDate(uint8_t day, uint8_t month, uint8_t year) {
    uint8_t data[3];
    data[0] = DecToBCD(day);   // Register 0x04
    data[1] = DecToBCD(month); // Register 0x05
    data[2] = DecToBCD(year);  // Register 0x06


    for (uint8_t i = 0; i < 3; i++) {
        if (DS1307_WriteRegister(0x04 + i, data[i]) != kStatus_Success) {
            return kStatus_Fail;
        }
    }

    return kStatus_Success;
}

/*--------------------------------------
 * Get RTC Date Function
 *--------------------------------------*/
status_t DS1307_GetDate(uint8_t *day, uint8_t *month, uint8_t *year) {
    uint8_t data[3];

    if (DS1307_ReadRegisters(0x04, data, 3) != kStatus_Success) {
        return kStatus_Fail;
    }

    *day = BCDToDec(data[0]);
    *month = BCDToDec(data[1]);
    *year = BCDToDec(data[2]);

    return kStatus_Success;
}

