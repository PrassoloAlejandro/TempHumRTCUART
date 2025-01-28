/*
 * uart.h
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */

#ifndef UART_H_
#define UART_H_
#include <stdint.h>
#include <stddef.h>
#include "fsl_usart.h"
/* UART Variables */

extern volatile uint8_t flagReceived; // Declaración externa
extern uint8_t dataUsart;             // Declaración externa
extern char buffer[64];               // Declaración externa
extern char newline[];                // Declaración externa

void Init_UART(void);
void Send_USART(USART_Type *base, const char *texto);

#endif /* UART_H_ */
