#pragma once

typedef struct {
	BYTE    op;
	BYTE    name1;
	BYTE    name2;
	BYTE    name3;
	SWORD   literal;
} BinOp8bits;

#define XINTERNAL_OFFSET 128
