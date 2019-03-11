#include <avr/io.h>

// Librairie AtMega pour I2C en mode maitre

#include "ladder.h"
#include "I2CLib.h"

#pragma weak LCD_I2C_Init           // symboles weak surchargeables
#pragma weak LCD_I2C_Erase          // si la librairie LCD_I2C est utilisée
#pragma weak LCD_I2C_Home
#pragma weak LCD_I2C_Config
#pragma weak LCD_I2C_MoveCursor
#pragma weak LCD_I2C_SendChar

void LCD_I2C_Init(int x) {};
void LCD_I2C_Erase(void) {};
void LCD_I2C_Home(void) {};
void LCD_I2C_Config(int x, int y, int z) {};
void LCD_I2C_MoveCursor(int x, int y) {};
void LCD_I2C_SendChar(char x) {};

#ifndef LCD_I2C_ADR
    #define LCD_I2C_ADR     0x20        // a adapter selon afficheur
#endif
#define LCD_I2C_REG     255             // a adapter selon preferences


/******************************** FONCTIONS I²C *******************************/

// Initialisation avec calcul des predivisions I²C
void I2C_Init(long fcpu, long ftwi)
    {
    char rating= 0;             // valeur 8 bits entre 0 et 255
    char prescal= 0;            // valeur 2 bits = 1, 4, 16 ou 64
    long q= 0;

    q= (fcpu/ftwi-16) / 2;      // devrait etre egal a rating * prescal

    if (q <= 0) {rating= 0; prescal= 0;}            // ftwi trop elevee
    else if (q <= 255) {rating= q; prescal= 0;}
    else if (q <= 4*255) {rating= q/4; prescal= 1;}
    else if (q <= 16*255) {rating= q/16; prescal= 2;}
    else if (q <= 64*255) {rating= q/64; prescal= 3;}
    else {rating= 255; prescal= 64;}                // ftwi trop faible

    I2C_MasterInit(rating, prescal);

    LCD_I2C_Init(LCD_I2C_ADR);
    }

// Initialisation I²C master
void I2C_MasterInit(char rate, char prescaler)
    {
    // Sélection du débit
    TWBR = rate;
    // Sélection du prescaler
    TWSR = prescaler;
    // Configuration du module TWI : Activation, pas d'interruptions
    TWCR = (1 << TWEN) | (0 << TWIE) | (0 << TWINT) | (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    }

// Attente de l'activation du flag TWINT
void I2C_MasterWait()
    {
    while(!(TWCR & (1 << TWINT)));
    }

// Envoi de la séquence de start + adresse
// Renvoie 0 si Ok
int I2C_MasterStart(char adresse, int mode)
    {
    char c;

    // Envoi du start
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
        (1 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    I2C_MasterWait();

    // Vérification envoi start OK
    if ((TWSR & 0xF8) != 0x08) return 1;     // Erreur => abandon

    // Envoi adresse + mode
    if (mode) // Lecture
        c = (adresse << 1) | 1;
    else // Ecriture
        c = (adresse << 1);

    TWDR = c;
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
         (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    I2C_MasterWait();

    // Vérification envoi adresse OK
    if (mode)
        {
        if ((TWSR & 0xF8) != 0x40) return 2;       // Erreur
        }
    else
        {
        if ((TWSR & 0xF8) != 0x18) return 3;       // Erreur
        }

    return 0;
    }

// Envoi de la séquence de restart + adresse
// Renvoie 0 si Ok
int I2C_MasterRestart(char adresse, int mode)
    {
    char c;

    // Envoi du (re)start
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
        (1 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    I2C_MasterWait();

    // Vérification envoi restart OK
    if ((TWSR & 0xF8) != 0x10) return 1;     // Erreur => abandon

    // Envoi adresse + mode
    if (mode) // Lecture
        c = (adresse << 1) | 1;
    else // Ecriture
        c = (adresse << 1);

    TWDR = c;
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
         (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    I2C_MasterWait();

    // Vérification envoi adresse OK
    if (mode)
        {
        if ((TWSR & 0xF8) != 0x40) return 2;       // Erreur
        }
    else
        {
        if ((TWSR & 0xF8) != 0x18) return 3;       // Erreur
        }

    return 0;
    }

// Envoi de la séquence de stop
void I2C_MasterStop()
    {
    // Envoi du stop
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
          (0 << TWSTA) | (1 << TWSTO) | (0 << TWWC);
    // Attente execution stop
    while(TWCR & (1<<TWSTO));
    }

// Ecriture d'un octet sur le bus I²C
int I2C_MasterWrite(char c)
    {
    // Placement de la donnée
    TWDR = c;
    // Lancement de l'envoi
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
         (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    // Attente fin envoi
    I2C_MasterWait();
    // Vérification envoi OK
    if ((TWSR & 0xF8) != 0x28) return 0; // Erreur

    return 1;
    }

// Lecture d'un octet sur le bus avec acknowledge à la fin
// pour indiquer au slave qu'on veut continuer de recevoir
char I2C_MasterReadNext()
    {
    // Lancement de la requete
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (1 << TWEA) |
         (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    // Attente fin réception
    I2C_MasterWait();

    return TWDR;
    }

// Lecture d'un octet sur le bus sans acknowledge à la fin
// Doit être suivi par un stop
char I2C_MasterReadLast()
    {
    // Lancement de la requete
    TWCR = (1 << TWEN) | (0 << TWIE) | (1 << TWINT) | (0 << TWEA) |
         (0 << TWSTA) | (0 << TWSTO) | (0 << TWWC);
    // Attente fin réception
    I2C_MasterWait();

    return TWDR;
    }

// Lecture d'une valeur dans registre (reg) sur peripherique (addr)
char I2C_MasterGetReg(char addr, char reg)
    {
    char c;

    I2C_MasterStart(addr, 0);       // adresse + 0= écriture
    I2C_MasterWrite(reg);           // positionne sur le registre
    I2C_MasterRestart(addr, 1);     // adresse + 1= lecture
    c= I2C_MasterReadLast();        // lecture valeur
    I2C_MasterStop();               // fin

    return c;
    }

// Ecriture d'une valeur dans registre (reg) sur peripherique (addr)
void I2C_MasterSetReg(char addr, char reg, char val)
    {
    int x, y;

    if ((addr == LCD_I2C_ADR) && (reg == LCD_I2C_REG))      // traitement special afficheur
        {
        if (val <= 0x10)        // commandes diverses
            {
            if (val == 0) LCD_I2C_Erase();              // effacement ecran
            if (val == 1) LCD_I2C_Home();               // retour en haut a gauche
            if (val == 2) LCD_I2C_Config(1, 0, 1);      // clignotement active
            if (val == 3) LCD_I2C_Config(1, 0, 0);      // clignotement desactive
            }
        else if (val >= 0x80)       // commandes move(y,x)
            {
            x= (val & 0x1F) + 1;            // x sur 5 bits => 0 < x < 33 colonnes
            y= ((val & 60) >> 5) + 1;       // y sur 2 bits => 0 < y < 5 lignes
            LCD_I2C_MoveCursor(y, x);           // val= 1yyxxxxx en binaire
            }
        else
            LCD_I2C_SendChar(val);

        return;
        }

    I2C_MasterStart(addr, 0);       // adresse + 0= écriture
    I2C_MasterWrite(reg);           // positionne sur le registre
    I2C_MasterWrite(val);           // ecriture valeur
    I2C_MasterStop();               // fin
    }

