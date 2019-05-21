/**
 * I2C Library for STM32F10x devices
 */

#ifndef I2C_H
#define I2C_H 161

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_i2c.h"
#include "lib_gpio.h"


/**
 * I2Cx     SCL   SDA
 *
 * I2C1     PB6   PB7
 * I2C2     PB10  PB11
 */

 /**
 *  Names of events used in stdperipheral library
 *
 *  I2C_EVENT_MASTER_MODE_SELECT                : EV5
 *  I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED  : EV6
 *  I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED     : EV6
 *  I2C_EVENT_MASTER_BYTE_RECEIVED              : EV7
 *  I2C_EVENT_MASTER_BYTE_TRANSMITTING          : EV8
 *  I2C_EVENT_MASTER_BYTE_TRANSMITTED           : EV8_2
 **/

//By default library supports only 7bit long address
#define Lib_I2Cx_7BITS      I2C_AcknowledgedAddress_7bit
#define Lib_I2Cx_MASTER     0x00

#define LibI2C_TIMEOUT		20000

#define LibI2C_SPEED_STND			100000      // I2C Standard speed 100 kHz
#define LibI2C_SPEED_FAST			400000      // I2C Fast speed 400 kHz
#define LibI2C_SPEED_FULL		    1000000     // I2C Full speed 1 MHz
#define LibI2C_SPEED_HIGH			3400000     // I2C High speed 3.4 MHz

#define I2C_SEND    0
#define I2C_RECV    1

#define I2C_ACK_ENABLE         1
#define I2C_ACK_DISABLE        0


/**
 * @brief  I2C pinspack enumeration
 */
typedef enum
    {
	LibI2C_PinsPack_1,      // Use Pinspack1 from Pinout table for I2Cx
	LibI2C_PinsPack_2,      // Use Pinspack2 from Pinout table for I2Cx
	LibI2C_PinsPack_3,      // Use Pinspack3 from Pinout table for I2Cx
    }
LibI2C_PinsPack_t;


//  Public functions

void LibI2C_MasterInit(I2C_TypeDef* I2Cx, uint32_t clockSpeed);

int16_t LibI2C_GetReg(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg);
void LibI2C_SetReg(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data);

void LibI2C_Stop(I2C_TypeDef* I2Cx);

uint8_t DecToBcd(uint8_t c);
uint8_t BcdToDec(uint8_t c);

//  Private functions

int16_t LibI2C_WaitAction(I2C_TypeDef* I2Cx, uint16_t Action);
int16_t LibI2C_WaitEvent(I2C_TypeDef* I2Cx, uint32_t Event);

int16_t LibI2C_SendStart(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack);
int16_t LibI2C_SendStop(I2C_TypeDef* I2Cx);


int16_t LibI2C_ReadNext(I2C_TypeDef* I2Cx); // Will be followed by another LibI2C_ReadNext() or LibI2C_ReadLast()
int16_t LibI2C_ReadLast(I2C_TypeDef* I2Cx); // Will be followed by LibI2C_Stop()
void LibI2C_Write(I2C_TypeDef* I2Cx, uint8_t data);

#endif

