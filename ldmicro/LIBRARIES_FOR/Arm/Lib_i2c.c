/**
 * Copyright (C) Tilen Majerle 2014, and JG 2016
 */

#include "lib_i2c.h"

#pragma weak LCD_I2C_Init  // symboles weak surchargeables
#pragma weak LCD_I2C_Erase // si la librairie LCD_I2C est utilisée
#pragma weak LCD_I2C_Home
#pragma weak LCD_I2C_Config
#pragma weak LCD_I2C_MoveCursor
#pragma weak LCD_I2C_SendChar

void LCD_I2C_Init(int x){};
void LCD_I2C_Erase(void){};
void LCD_I2C_Home(void){};
void LCD_I2C_Config(int x, int y, int z){};
void LCD_I2C_MoveCursor(int x, int y){};
void LCD_I2C_SendChar(char x){};

#define LCD_I2C_ADR 0x20 // a adapter selon afficheur
#define LCD_I2C_REG 255  // a adapter selon preferences

uint32_t LibI2C_Timeout;

/**
 * @brief  Initializes I2C
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  clockSpeed = Clock speed for SCL in Hertz
 * @retval None
 */
void LibI2C_MasterInit(I2C_TypeDef *I2Cx, uint32_t clockSpeed)
{
    I2C_InitTypeDef I2C_InitStruct;

    if(I2Cx == I2C1) {
        /* Enable clock */
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

        /* Enable pins */
        LibI2C_INT_InitPins(I2C1, LibI2C_PinsPack_2);

        /* Set values */
        I2C_InitStruct.I2C_ClockSpeed = clockSpeed;
        I2C_InitStruct.I2C_AcknowledgedAddress = Lib_I2Cx_7BITS;
        I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
        I2C_InitStruct.I2C_OwnAddress1 = Lib_I2Cx_MASTER; // 0 = Master
        I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
        I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    }

    if(I2Cx == I2C2) {
        /* Enable clock */
        RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

        /* Enable pins */
        LibI2C_INT_InitPins(I2C2, LibI2C_PinsPack_2);

        /* Set values */
        I2C_InitStruct.I2C_ClockSpeed = clockSpeed;
        I2C_InitStruct.I2C_AcknowledgedAddress = Lib_I2Cx_7BITS;
        I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
        I2C_InitStruct.I2C_OwnAddress1 = Lib_I2Cx_MASTER;
        I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
        I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    }

    if(I2Cx == I2C3) {
        /* Enable clock */
        RCC->APB1ENR |= RCC_APB1ENR_I2C3EN;

        /* Enable pins */
        LibI2C_INT_InitPins(I2C3, LibI2C_PinsPack_2);

        /* Set values */
        I2C_InitStruct.I2C_ClockSpeed = clockSpeed;
        I2C_InitStruct.I2C_AcknowledgedAddress = Lib_I2Cx_7BITS;
        I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
        I2C_InitStruct.I2C_OwnAddress1 = Lib_I2Cx_MASTER;
        I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
        I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    }

    /* Disable I2C first */
    I2Cx->CR1 &= ~I2C_CR1_PE;

    /* Initialize I2C */
    I2C_Init(I2Cx, &I2C_InitStruct);

    /* Enable I2C */
    I2Cx->CR1 |= I2C_CR1_PE;

    LCD_I2C_Init(LCD_I2C_ADR);
}

/**
 * @brief  Wait for I2C action to terminate
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  Action = action to wait for
 * @retval 0 if OK, 1 if time out
 */
int16_t LibI2C_WaitAction(I2C_TypeDef *I2Cx, uint16_t Action)
{
    /* Wait till I2C is busy */
    LibI2C_Timeout = LibI2C_TIMEOUT;
    while(!(I2Cx->SR1 & Action)) {
        LibI2C_Timeout--;
        if(LibI2C_Timeout == 0)
            return 1;
    }
    return 0;
}

/**
 * @brief  Wait for I2C event to occur
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  Event = event to wait for
 * @retval 0 if OK, 1 if time out
 */
int16_t LibI2C_WaitEvent(I2C_TypeDef *I2Cx, uint32_t Event)
{
    /* Wait till finished */
    LibI2C_Timeout = LibI2C_TIMEOUT;
    while(!I2C_CheckEvent(I2Cx, Event)) {
        LibI2C_Timeout--;
        if(LibI2C_Timeout == 0)
            return 1;
    }
    return 0;
}

/**
 * @brief  Send I2C Start condition
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  address = slave address (1 to 127)
 * @param  direction = I2C_SEND or I2C_RECV
 * @param  ack = I2C_ACK_ENABLE or I2C_ACK_DISABLE
 * @retval 0 if OK, 1 if error
  */
int16_t LibI2C_Start(I2C_TypeDef *I2Cx, uint8_t address, uint8_t direction, uint8_t ack)
{
    /* Generate I2C start pulse */
    I2Cx->CR1 |= I2C_CR1_START;
    if(LibI2C_WaitAction(I2Cx, I2C_SR1_SB))
        return 1; // timout

    /* Enable ack if we select it */
    if(ack)
        I2Cx->CR1 |= I2C_CR1_ACK;

    /* Send write/read bit */
    if(direction == I2C_SEND) {
        /* Send address + 0 last bit (send) */
        /* Why address must be shifted by 1 */
        I2Cx->DR = (address << 1) & ~I2C_OAR1_ADD0;
        if(LibI2C_WaitAction(I2Cx, I2C_SR1_ADDR))
            return 1; // timout
    }

    if(direction == I2C_RECV) {
        /* Send address + 1 last bit (receive) */
        /* Why address must be shifted by 1 */
        I2Cx->DR = (address << 1) | I2C_OAR1_ADD0;

        if(LibI2C_WaitEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
            return 1; // timout
    }

    /* Read status register to clear ADDR flag */
    I2Cx->SR2;

    /* OK Return 0 */
    return 0;
}

/**
 * @brief  Send I2C Stop condition
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @retval 0 if OK, 1 if error
 */
int16_t LibI2C_Stop(I2C_TypeDef *I2Cx)
{
    /* Wait till transmitter empty */
    LibI2C_Timeout = LibI2C_TIMEOUT;
    while(((!(I2Cx->SR1 & I2C_SR1_TXE)) || (!(I2Cx->SR1 & I2C_SR1_BTF)))) {
        LibI2C_Timeout--;
        if(LibI2C_Timeout == 0)
            return 1;
    }

    /* Generate stop */
    I2Cx->CR1 |= I2C_CR1_STOP;

    /* Return 0, everything ok */
    return 0;
}

/**
 * @brief  Writes to slave
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  data = data to send
 * @retval None
 */
void LibI2C_Write(I2C_TypeDef *I2Cx, uint8_t data)
{
    /* Wait till I2C is not busy anymore */
    LibI2C_WaitAction(I2Cx, I2C_SR1_TXE);

    /* Send I2C data */
    I2Cx->DR = data;
}

/**
 * @brief  Read byte with ack (Another one will follow)
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @retval Byte from slave, or (int16_t) -1 if no data
 */
int16_t LibI2C_ReadNext(I2C_TypeDef *I2Cx)
{
    uint8_t data;

    /* Enable ACK */
    I2Cx->CR1 |= I2C_CR1_ACK;

    /* Wait till not received */
    if(LibI2C_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED))
        return -1; // no data

    /* Read data */
    data = I2Cx->DR;

    /* Return data */
    return data;
}

/**
 * @brief  Read byte without ack (Last one)
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @retval Byte from slave, or (int16_t) -1 if no data
 */
int16_t LibI2C_ReadLast(I2C_TypeDef *I2Cx)
{
    uint8_t data;

    /* Disable ACK */
    I2Cx->CR1 &= ~I2C_CR1_ACK;

    /* Wait till received */
    if(LibI2C_WaitEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED))
        return -1; // no data

    /* Read data */
    data = I2Cx->DR;

    /* Return data */
    return data;
}

/* Private functions */
void LibI2C_INT_InitPins(I2C_TypeDef *I2Cx, LibI2C_PinsPack_t pinspack)
{
    if(I2Cx == I2C1) {
        if(pinspack == LibI2C_PinsPack_1)
            LibGPIO_InitAlternate(
                GPIOB, GPIO_PIN_6 | GPIO_PIN_7, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C1);
        if(pinspack == LibI2C_PinsPack_2)
            LibGPIO_InitAlternate(
                GPIOB, GPIO_PIN_8 | GPIO_PIN_9, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C1);
        if(pinspack == LibI2C_PinsPack_3)
            LibGPIO_InitAlternate(
                GPIOB, GPIO_PIN_6 | GPIO_PIN_9, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C1);
    }

    if(I2Cx == I2C2) {
        if(pinspack == LibI2C_PinsPack_1)
            LibGPIO_InitAlternate(GPIOB,
                                  GPIO_PIN_10 | GPIO_PIN_11,
                                  LibGPIO_OType_OD,
                                  LibGPIO_PuPd_UP,
                                  LibGPIO_Speed_Medium,
                                  GPIO_AF_I2C2);
        if(pinspack == LibI2C_PinsPack_2)
            LibGPIO_InitAlternate(
                GPIOF, GPIO_PIN_0 | GPIO_PIN_1, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C2);
        if(pinspack == LibI2C_PinsPack_3)
            LibGPIO_InitAlternate(
                GPIOH, GPIO_PIN_4 | GPIO_PIN_5, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C2);
    }

    if(I2Cx == I2C3) {
        if(pinspack == LibI2C_PinsPack_1) {
            LibGPIO_InitAlternate(
                GPIOA, GPIO_PIN_8, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C3);
            LibGPIO_InitAlternate(
                GPIOC, GPIO_PIN_9, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C3);
        }
        if(pinspack == LibI2C_PinsPack_2)
            LibGPIO_InitAlternate(
                GPIOH, GPIO_PIN_7 | GPIO_PIN_8, LibGPIO_OType_OD, LibGPIO_PuPd_UP, LibGPIO_Speed_Medium, GPIO_AF_I2C3);
    }
}

/**
 * @brief  Read register from slave
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  address = 7 bit slave address
 * @param  reg = register to read
 * @retval Byte from slave, or (int16_t) -1 if no data
 */
int16_t LibI2C_GetReg(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg)
{
    int16_t data;

    LibI2C_Start(I2Cx, address, I2C_SEND, I2C_ACK_DISABLE);
    LibI2C_Write(I2Cx, reg);
    LibI2C_Stop(I2Cx);
    LibI2C_Start(I2Cx, address, I2C_RECV, I2C_ACK_DISABLE);
    data = LibI2C_ReadLast(I2Cx);
    LibI2C_Stop(I2Cx);

    return data;
}

/**
 * @brief  Read multiple registers from slave
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  address = 7 bit slave address
 * @param  reg = first register to read
 * @param  data = pointer to data array to store read data
 * @param  count = number of registers ro read
 * @retval 0 if OK, 1 if error
 */
int16_t LibI2C_GetMultiReg(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg, uint8_t *data, uint16_t nb)
{
    uint8_t i;
    int16_t dat, res = 0;

    LibI2C_Start(I2Cx, address, I2C_SEND, I2C_ACK_ENABLE);
    LibI2C_Write(I2Cx, reg);
    LibI2C_Stop(I2Cx);
    LibI2C_Start(I2Cx, address, I2C_RECV, I2C_ACK_ENABLE);
    for(i = 0; i < nb; i++) {
        if(i + 1 < nb) {
            dat = LibI2C_ReadNext(I2Cx);
            if(dat == -1)
                res = 1; // error
            data[i] = dat;
        } else { // Last byte
            dat = LibI2C_ReadLast(I2Cx);
            if(dat == -1)
                res = 1; // error
            data[i] = dat;
            LibI2C_Stop(I2Cx);
        }
    }
    return res; // 0 if OK
}

/**
 * @brief  Write register to slave
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  address = 7 bit slave address
 * @param  reg = register to write
 * @param  data = data to be written
 * @retval None
 */
void LibI2C_SetReg(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg, uint8_t data)
{
    int x, y;

    if((address == LCD_I2C_ADR) && (reg == LCD_I2C_REG)) // traitement special afficheur
    {
        if(data <= 0x10) // commandes diverses
        {
            if(data == 0)
                LCD_I2C_Erase(); // effacement ecran
            if(data == 1)
                LCD_I2C_Home(); // retour en haut a gauche
            if(data == 2)
                LCD_I2C_Config(1, 0, 1); // clignotement active
            if(data == 3)
                LCD_I2C_Config(1, 0, 0); // clignotement desactive
        } else if(data >= 0x80) {        // commandes move(y,x)
            x = (data & 0x1F) + 1;       // x sur 5 bits => 0 < x < 33 colonnes
            y = ((data & 60) >> 5) + 1;  // y sur 2 bits => 0 < y < 5 lignes
            LCD_I2C_MoveCursor(y, x);    // val= 1yyxxxxx en binaire
        } else {
            LCD_I2C_SendChar(data);
        }
        return;
    }

    LibI2C_Start(I2Cx, address, I2C_SEND, I2C_ACK_DISABLE);
    LibI2C_Write(I2Cx, reg);
    LibI2C_Write(I2Cx, data);
    LibI2C_Stop(I2Cx);
}

/**
 * @brief  Write multiple registers to slave
 * @param  I2Cx = I2C1, I2C2 or I2C3
 * @param  address = 7 bit slave address
 * @param  reg = first register to write
 * @param  data = pointer to data array to write
 * @param  count = number of registers to write
 * @retval None
 */
void LibI2C_SetMultiReg(I2C_TypeDef *I2Cx, uint8_t address, uint8_t reg, uint8_t *data, uint16_t nb)
{
    uint8_t i;

    LibI2C_Start(I2Cx, address, I2C_SEND, I2C_ACK_DISABLE);
    LibI2C_Write(I2Cx, reg);
    for(i = 0; i < nb; i++) {
        LibI2C_Write(I2Cx, data[i]);
    }
    LibI2C_Stop(I2Cx);
}

/**
 * @brief  Convert values from decimal to BCD format
 * @param  c = decimal value to convert
 * @retval Converted BCD value
 */
uint8_t DecToBcd(uint8_t c)
{
    uint8_t bcd;

    bcd = ((c / 10) << 4) + (c % 10);
    return bcd;
}

/**
 * @brief  Convert values from BCD to decimal format
 * @param  c = BCD value to convert
 * @retval Converted decimal value
 */
uint8_t BcdToDec(uint8_t c)
{
    uint8_t dec;

    dec = 10 * (c >> 4) + (c & 0x0F);
    return dec;
}
