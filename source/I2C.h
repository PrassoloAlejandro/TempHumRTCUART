/*
 * I2C.h
 *
 *  Created on: 27 ene. 2025
 *      Author: aleja
 */


#ifndef I2C_H
#define I2C_H

#include "fsl_i2c.h"
extern uint32_t baudRate;
extern uint32_t frequency;
void initI2C(void);

void I2C_ResetBus(I2C_Type *base);



#endif /* I2C_H_ */
