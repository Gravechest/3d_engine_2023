#pragma once

#include "source.h"

typedef struct{
	char upside;
	char revert;
	char sideways;
}CUBEMAPTR;

VEC3 getRayAngleCB(IVEC2 rd_size,VEC4 dir_tri,int x,int y);
VEC2 sampleCube(VEC3 v,int* faceIndex);
void mixCubemapSide(VEC3* restrict side_1,VEC3* restrict side_2);
void rdCubemap(VEC3* cubemap,VEC3 l_pos,int side);
void cubemapSide(VEC3*** cubemap,VEC2 size,VEC3 pos,IVEC3 v_side,float backside);

extern CUBEMAPTR cubemaptr[6];
extern VEC4 sd_dirtri[6];
extern ZBUFFER* cm_zbuffer;