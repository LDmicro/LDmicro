#pragma once

/*
typedef struct {
	BYTE    op;
	BYTE    name1;
	BYTE    name2;
	BYTE    name3;
	SWORD   literal;
} BinOp8bits;
*/

/* Pin number ranges
 * 0 ... XINTERNAL_OFFSET-1 : direct hardware pin
 * XINTERNAL_OFFSET ... XMODBUS_OFFSET-1 : internal relays or registers
 * XMODBUS_OFFSET  ... 255 : modbus coil or H register
 */

#define MAX_PHYS_PINS		128
#define MAX_INT_RELAYS		64
#define MAX_MODBUS_COILS	64

#define XINTERNAL_OFFSET MAX_PHYS_PINS
#define XMODBUS_OFFSET (XINTERNAL_OFFSET + MAX_INT_RELAYS)

#define IS_DIRECT_PIN(pin)		( (pin)>= 0 && (pin) < XINTERNAL_OFFSET )
#define IS_INTERNAL_PIN(pin)	( (pin)>= XINTERNAL_OFFSET && (pin) < XMODBUS_OFFSET )
#define IS_MODBUS_PIN(pin)		( (pin)>= XMODBUS_OFFSET && (pin) <= 255 )
