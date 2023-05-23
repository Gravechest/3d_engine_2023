#pragma once

#include "source.h"

VEC3 traceLightRay(VEC3 l_color,VEC3 pos,VEC3 ang);
void genSphereLM(SPHERE* sphere,VEC3 pos,float quality);
void genSquareLM(SQUARE* square,float quality);
void genLight();