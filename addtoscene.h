#pragma once

void addSquare(IVEC3 v,VEC3 pos,VEC3 color,VEC2 size,int type,int back);
void addTexture(float scale,VEC2 offset,TEXTURE* texture,TXMAP* txmap);
void addBoxTexture(float scale,VEC2 offset,TEXTURE* texture);
void addReflection(VEC3**** cubemap,CMMAP* cm_map,VEC2 size,REFLECTMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale);void addBoxReflection(int side,NORMALMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale);
void addBoxReflectionA(REFLECTMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale);
void addBox(VEC3 pos,VEC3 size,VEC3 color,char type);
void addSphere(VEC3 pos,VEC3 color,float radius,char type);
void addHitbox(VEC3 pos,VEC3 size);
void addBoxEffectA(int side,int effect,EFFECT prop);
void addTriangle(VEC3 p1,VEC3 p2,VEC3 p3,VEC3 color);