#pragma once

void drawTexture(VEC3* color,VEC2 pos,float dst,VEC3 ang,VEC3 normal,TXMAP txmap,TEXTURE* texture);
void hitRayNormal(PRIMITIVE* hitted,VEC3* color,VEC3 pos,VEC3 ang,VEC2 uv,float dst);