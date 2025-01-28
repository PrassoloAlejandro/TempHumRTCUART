/*
 * RTCDS1307.h
 *
 *  Created on: 28 ene. 2025
 *      Author: aleja
 */

#ifndef RTC_H_
#define RTC_H_

#include <stdint.h>
#include "fsl_common.h"

#define DS1307_I2C_ADDRESS 0x68

void RTC_Init(void);
status_t RTC_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
status_t RTC_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
status_t RTC_SetDate(uint8_t day, uint8_t month, uint8_t year);
status_t RTC_GetDate(uint8_t *day, uint8_t *month, uint8_t *year);

#endif /* RTC_H_ */
