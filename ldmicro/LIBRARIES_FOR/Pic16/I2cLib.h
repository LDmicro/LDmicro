#include <htc.h>


#if (defined LDTARGET_pic16f873) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f874) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f876) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f877) 
	#define LDTARGET_pic16f87X
#endif

#if (defined LDTARGET_pic16f882) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f883) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f884) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f886) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f887) 
	#define LDTARGET_pic16f88X
#endif


// Initialisation avec calcul des predivision I²C
// fcpu : frequence du microcontroleur
// ftwi : frequence souhaitee pour le bus I2C
void I2C_Init(long fcpu, long ftwi);

// Initialisation I²C master
// prescaler : valeur des bits du registre TWSR pour sélection du prescaler
// ces paramètres déterminent la fréquence de l'horloge I²C
void I2C_MasterInit(char prescaler);

// Envoi de la séquence de start + adresse de l'esclave
// adresse : adresse de l'esclave avec lequel démarrer la communication
// mode= 1 si lecture et 0 si écriture
int I2C_MasterStart(char adresse, int mode);

// Envoi de la séquence de restart + adresse de l'esclave
// adresse : adresse de l'esclave avec lequel démarrer la communication
// mode= 1 si lecture et 0 si écriture
int I2C_MasterRestart(char adresse, int mode);

// Envoi de la séquence stop
void I2C_MasterStop(void);

// Ecriture d'un octet sur le bus
// donnee : l'octet à écrire
int I2C_MasterWrite(char c);

// Lecture d'un octet sur le bus avec acknowledge à la fin 
// pour indiquer à l'esclave que la lecture n'est pas terminée
// Renvoie la donnée lue
char I2C_MasterReadNext(void);

// Lecture d'un octet sur le bus sans acknowledge à la fin
// pour indiquer à l'esclave que la lecture est terminée
// (doit être suivi par un stop)
// Renvoie la donnée lue
char I2C_MasterReadLast(void);

// Fonction à usage interne
void I2C_MasterWait(void);

// Lecture d'une valeur dans registre (reg) sur peripherique (addr)
char I2C_MasterGetReg(char addr, char reg);

// Ecriture d'une valeur dans registre (reg) sur peripherique (addr)
void I2C_MasterSetReg(char addr, char reg, char val);

// Fonctions et ports pour software I2C 
#if defined (LDTARGET_pic16f88) || defined (LDTARGET_pic16f819)

#define SCL		TRISB4
#define SDA		TRISB1
#define CLOCK	RB4
#define DATA	RB1

void I2C_SoftDelay(unsigned char d);
void I2C_SoftStart();
void I2C_SoftRestart();
void I2C_SoftStop(void);
void I2C_SoftWrite(unsigned char dat);
unsigned char I2C_SoftRead(void);
void I2C_SoftAck();
void I2C_SoftNack();
unsigned char I2C_SoftGetAck();

#endif