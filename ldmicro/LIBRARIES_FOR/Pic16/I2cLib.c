#include <htc.h>

// Librairie PIC pour I2C en mode maitre

#include "ladder.h"
#include "I2cLib.h"
#include "UsrLib.h"

#ifndef LDTARGET_pic16f628

#ifndef LCD_I2C_ADR
#define LCD_I2C_ADR 0 // a adapter selon afficheur (si utile)
#endif
#define LCD_I2C_REG 255 // a adapter selon preferences

#if LCD_I2C_ADR
void LCD_I2C_Init(int x);
void LCD_I2C_Erase(void);
void LCD_I2C_Home(void);
void LCD_I2C_Config(int x, int y, int z);
void LCD_I2C_MoveCursor(int x, int y);
void LCD_I2C_SendChar(char x);
#endif

static unsigned char Delay = 1;

// Initialisation avec calcul de la predivision I²C
void I2C_Init(long fcpu, long ftwi)
{
    char prescal = 0;
    long q = 0;

#if defined(LDTARGET_pic16f87X) || defined(LDTARGET_pic16f88X)
    q = (fcpu / ftwi) / 4; // Should be between 2 and 256

    if(q > 256)
        prescal = 255; // ftwi too slow
    else if(q < 2)
        prescal = 1; // ftwi too fast
    else
        prescal = q - 1;
#endif

#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic16f819)
    // Frequency can't be guaranteed because there's no clock !
    q = 100000 / (ftwi + 1);
    prescal = q;
#endif

    I2C_MasterInit(prescal);

#if LCD_I2C_ADR
    LCD_I2C_Init(LCD_I2C_ADR);
#endif
}

// Lecture d'une valeur dans registre (reg) sur peripherique (addr)
char I2C_MasterGetReg(char addr, char reg)
{
    char c;

    I2C_MasterStart(addr, 0);   // adresse + 0= écriture
    I2C_MasterWrite(reg);       // positionne sur le registre
    I2C_MasterRestart(addr, 1); // adresse + 1= lecture
    c = I2C_MasterReadLast();   // lecture valeur
    I2C_MasterStop();           // fin

    return c;
}

// Ecriture d'une valeur dans registre (reg) sur peripherique (addr)
void I2C_MasterSetReg(char addr, char reg, char val)
{
#if LCD_I2C_ADR
    int x, y;

    if((addr == LCD_I2C_ADR) && (reg == LCD_I2C_REG)) { // traitement special afficheur
        if(val <= 0x10) {                               // commandes diverses
            if(val == 0)
                LCD_I2C_Erase(); // effacement ecran
            if(val == 1)
                LCD_I2C_Home(); // retour en haut a gauche
            if(val == 2)
                LCD_I2C_Config(1, 0, 1); // clignotement active
            if(val == 3)
                LCD_I2C_Config(1, 0, 0); // clignotement desactive
        } else if(val >= 0x80) {         // commandes move(y,x)
            x = (val & 0x1F) + 1;        // x sur 5 bits => 0 < x < 33 colonnes
            y = ((val & 60) >> 5) + 1;   // y sur 2 bits => 0 < y < 5 lignes
            LCD_I2C_MoveCursor(y, x);    // val= 1yyxxxxx en binaire
        } else {
            LCD_I2C_SendChar(val);
        }
        return;
    }
#endif

    I2C_MasterStart(addr, 0); // adresse + 0= écriture
    I2C_MasterWrite(reg);     // positionne sur le registre
    I2C_MasterWrite(val);     // ecriture valeur
    I2C_MasterStop();         // fin
}

#if defined(LDTARGET_pic16f87X) || defined(LDTARGET_pic16f88X)

// Initialisation I²C master
void I2C_MasterInit(char prescaler)
{
    SSPCON = 0; // Reset SSPCON
    SSPM3 = 1;  // I2C Master mode

    SSPCON2 = 0;        // Default settings
    SSPADD = prescaler; // I2C frequency f = F / (4 * (pscale+1))
    SSPSTAT = 0;        // Default settings

    TRISC3 = 1; // RC3 (SCL) and RC4 (SDA) as inputs
    TRISC4 = 1; // Cf datasheet

    SSPEN = 1; // Enable I2C

    I2C_MasterWait();
}

// Attente I2C pret
void I2C_MasterWait()
{
    while((SSPCON2 & 0x1F) || (SSPSTAT & 0x04))
        ;
}

// Envoi de la séquence de start + adresse
// Renvoie 0 si Ok
int I2C_MasterStart(char adresse, int mode)
{
    char a;

    SEN = 1; // start condition
    I2C_MasterWait();

    if(mode)
        a = (adresse << 1) | 1; // Read
    else                        // Write
        a = (adresse << 1);
    SSPBUF = a; // 7-bit address of slave device plus mode
    I2C_MasterWait();

    if(WCOL)
        return 2; // Erreur collision
    if(SSPOV)
        return 3; // Erreur overflow
    return 0;
}

// Envoi de la séquence de restart + adresse
// Renvoie 0 si Ok
int I2C_MasterRestart(char adresse, int mode)
{
    char a;

    RSEN = 1; // restart condition
    I2C_MasterWait();

    if(mode)
        a = (adresse << 1) | 1; // Read
    else                        // Write
        a = (adresse << 1);
    SSPBUF = a; // 7-bit address of slave device plus mode
    I2C_MasterWait();

    if(WCOL)
        return 2; // Erreur collision
    if(SSPOV)
        return 3; // Erreur overflow
    return 0;
}

// Envoi de la séquence de stop
void I2C_MasterStop()
{
    PEN = 1; // stop condition
    I2C_MasterWait();
}

// Ecriture d'un octet sur le bus I²C
int I2C_MasterWrite(char c)
{
    SSPBUF = c; // data to be sent
    I2C_MasterWait();
}

// Lecture d'un octet sur le bus avec acknowledge à la fin
// pour indiquer au slave qu'on veut continuer de recevoir
char I2C_MasterReadNext()
{
    RCEN = 1;
    I2C_MasterWait();

    ACKDT = 0; // 0 = ACK
    ACKEN = 1; // send ACK
    I2C_MasterWait();

    return SSPBUF;
}

// Lecture d'un octet sur le bus sans acknowledge à la fin
// Doit être suivi par un stop
char I2C_MasterReadLast()
{
    RCEN = 1;
    I2C_MasterWait();

    ACKDT = 1; // 1 = NACK
    ACKEN = 1; // send NACK
    I2C_MasterWait();

    return SSPBUF;
}

#endif

#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic16f819)

// Initialisation I²C master
void I2C_MasterInit(char prescaler)
{
    SSPCON = 0; // Reset SSPCON

    SSPM3 = 1; // I2C Master software mode
    SSPM1 = 1;
    SSPM0 = 1;

    // These PICs don't really implement Master I2C bus
    // Must be managed by software

    CLOCK = 0;
    DATA = 0;

    SCL = 1; // RB4 (SCL) and RB1 (SDA)
    SDA = 1; // for clock and data

    SSPEN = 1; // Enable I2C

    Delay = prescaler;
    I2C_SoftDelay(1);
}

// Attente I2C
void I2C_MasterWait()
{
    I2C_SoftDelay(1);
}

// Envoi de la séquence de start + adresse
// Renvoie 0 si Ok
int I2C_MasterStart(char adresse, int mode)
{
    unsigned char a;

    I2C_SoftStart();

    if(mode)
        a = (adresse << 1) | 1; // Read
    else                        // Write
        a = (adresse << 1);

    I2C_SoftWrite(a); // 7-bit address of slave device plus mode
    I2C_SoftGetAck();

    return 0;
}

// Envoi de la séquence de restart + adresse
// Renvoie 0 si Ok
int I2C_MasterRestart(char adresse, int mode)
{
    unsigned char a;

    I2C_SoftRestart();

    if(mode)
        a = (adresse << 1) | 1; // Read
    else                        // Write
        a = (adresse << 1);

    I2C_SoftWrite(a); // 7-bit address of slave device plus mode
    I2C_SoftGetAck();

    return 0;
}

// Envoi de la séquence de stop
void I2C_MasterStop()
{
    I2C_SoftStop(); // stop condition
}

// Ecriture d'un octet sur le bus I²C
int I2C_MasterWrite(char c)
{
    I2C_SoftWrite(c); // data to be sent
    I2C_SoftGetAck();
}

// Lecture d'un octet sur le bus avec acknowledge à la fin
// pour indiquer au slave qu'on veut continuer de recevoir
char I2C_MasterReadNext()
{
    char c = 0;

    c = I2C_SoftRead();
    I2C_SoftAck();

    return c;
}

// Lecture d'un octet sur le bus sans acknowledge à la fin
// Doit être suivi par un stop
char I2C_MasterReadLast()
{
    char c = 0;

    c = I2C_SoftRead();
    I2C_SoftNack();

    return c;
}

// SCL= 0 toggles SCL Line to Output, at level CLOCK= 0
// SCL= 1 toggles SCL line to Input, so that
// pull-up brings it high but lets Slave able to manage it too

// SDA= 0 toggles SDA Line to Output, at level DATA= 0
// SDA= 1 toggles SDA line to Input, so that
// pull-up brings it high but lets Slave able to manage it too

void I2C_SoftDelay(unsigned char d)
{
    while(d > 0) {
        delay_us(1);
        d--;
    }
}

void I2C_SoftStart()
{
    SCL = 1;
    SDA = 0;
    I2C_SoftDelay(Delay);
    SCL = 0;
    I2C_SoftDelay(Delay);
}

void I2C_SoftRestart()
{
    SCL = 0;
    SDA = 1;
    I2C_SoftDelay(Delay);
    SCL = 1;
    I2C_SoftDelay(Delay);
    SDA = 0;
    I2C_SoftDelay(Delay);
    SCL = 0;
    I2C_SoftDelay(Delay);
}

void I2C_SoftStop(void)
{
    SDA = 0;
    SCL = 1;
    I2C_SoftDelay(Delay);
    SDA = 1;
    I2C_SoftDelay(Delay);
}

void I2C_SoftWrite(unsigned char dat)
{
    unsigned char i;

    for(i = 0; i < 8; i++) // Loop 8 times to send 1-byte of data
    {
        if(dat & 0x80) // Send Bit by Bit on SDA line
            SDA = 1;
        else
            SDA = 0;
        SCL = 1;
        I2C_SoftDelay(Delay);
        SCL = 0;
        I2C_SoftDelay(Delay);

        dat = dat << 1;
    }
}

unsigned char I2C_SoftRead(void)
{
    unsigned char i, dat = 0x00;

    SDA = 1; // Input

    for(i = 0; i < 8; i++) // Loop 8 times to read 1-byte of data
    {
        SCL = 1;
        I2C_SoftDelay(2 * Delay);

        dat = dat << 1;   // dat is Shifted each time and
        dat = dat | DATA; // ORed with the received bit to pack into byte

        SCL = 0;
        I2C_SoftDelay(Delay);
    }

    return dat; // Finally return the received Byte
}

void I2C_SoftAck()
{
    SDA = 0; // ACK = 0
    SCL = 1;
    I2C_SoftDelay(Delay);
    SCL = 0;
    I2C_SoftDelay(Delay);
}

void I2C_SoftNack()
{
    SDA = 1; // NAK = 1
    SCL = 1;
    I2C_SoftDelay(Delay);
    SCL = 0;
    I2C_SoftDelay(Delay);
}

unsigned char I2C_SoftGetAck()
{
    unsigned char ack = 0;

    SDA = 1; // Input
    I2C_SoftDelay(Delay);
    SCL = 1;
    I2C_SoftDelay(Delay);
    ack = DATA;
    SCL = 0;
    I2C_SoftDelay(Delay);

    return ack; // ACK = 0 / NAK = 1
}

#endif

#endif