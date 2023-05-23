#pragma once

#include "vec3.h"

typedef struct{
	VEC3 pos;
	VEC3 size;
}HITBOX;

typedef struct{
	HITBOX* hitbox;
	int cnt;
}HITBOXHUB;

void walkPhysics();
void physics();

extern HITBOXHUB hitboxhub;
extern int global_tick;