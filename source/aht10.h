/*
 * aht10.h
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */

#ifndef AHT10_H_
#define AHT10_H_

#include <stdint.h>
#include <stddef.h>
#include "fsl_i2c.h"

#define AHT10_ADDRESS 0x38

typedef struct {
    uint32_t temp;
    uint32_t humidity;
} AHT10Data;

AHT10Data Measure_AHT10(void);

#endif /* AHT10_H_ */
