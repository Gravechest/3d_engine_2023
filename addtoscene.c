#include <Windows.h>
#include <stdio.h>

#include "source.h"
#include "addtoscene.h"
#include "phyiscs.h"
#include "tmath.h"

void addHitbox(VEC3 pos,VEC3 size){
	VEC3mul(&size,0.5f);
	HITBOX* hitbox = &hitboxhub.hitbox[hitboxhub.cnt++];
	hitbox->pos = VEC3addVEC3R(pos,size);
	hitbox->size = size;
}

void addSquare(IVEC3 v,VEC3 pos,VEC3 color,VEC2 size,int type,int back){
	size.x *= 0.5f;
	size.y *= 0.5f;
	pos.a[v.x] += size.x;
	pos.a[v.y] += size.y;
	VEC3 normal;
	switch(v.z){
	case VEC3_X: normal = (VEC3){back ? -1.0f : 1.0f,0.0f,0.0f}; break;
	case VEC3_Y: normal = (VEC3){0.0f,back ? -1.0f : 1.0f,0.0f}; break;
	case VEC3_Z: normal = (VEC3){0.0f,0.0f,back ? -1.0f : 1.0f}; break;
	}
	PRIMITIVE* primitive = &primitivehub.primitive[primitivehub.cnt++];
	primitive->type = type;
	primitive->square.v = v;
	primitive->square.normal = normal;
	primitive->square.color = color;
	primitive->square.size = size;
	primitive->square.pos = pos;
	primitive->square.cm_properties.reflectmap = RF_VOID;
	primitivehub.selected = primitive;
}

void addTriangle(VEC3 p1,VEC3 p2,VEC3 p3,VEC3 color){
	PRIMITIVE* primitive = &primitivehub.primitive[primitivehub.cnt++];
	primitive->type = PR_TRIANGLE;
	primitivehub.selected = primitive;
	TRIANGLE* triangle = &primitive->triangle;
	triangle->pos[0] = p1;
	triangle->pos[1] = p2;
	triangle->pos[2] = p3;
	triangle->color = color;
	triangle->normal = triangleNormal(p1,p2,p3);
	VEC2 size = {VEC3distance(triangle->pos[0],triangle->pos[1])*LM_SIZE,VEC3distance(triangle->pos[0],triangle->pos[2])*LM_SIZE};
	size.x = tCeilingf(size.x);
	size.y = tCeilingf(size.y);
	triangle->size = size;
}

void addTexture(float scale,VEC2 offset,TEXTURE* texture,TXMAP* txmap){
	txmap->data = texture;
	txmap->scale = scale;
	txmap->offset = offset;
}

void addBoxTexture(float scale,VEC2 offset,TEXTURE* texture){
	for(int i = -6;i < 0;i++){
		SQUARE* square = &primitivehub.primitive[primitivehub.cnt + i].square;
		addTexture(scale,offset,texture,&square->texture);
	}
}

void addReflection(VEC3**** cubemap,CMMAP* cm_map,VEC2 size,REFLECTMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale){
	size.x *= CM_SPREAD;
	size.y *= CM_SPREAD;

	if(size.x < 1.0f) size.x = 1.0f;
	if(size.y < 1.0f) size.y = 1.0f;
	
	cm_map->normalmap = normalmap;
	cm_map->nscale = nscale;
	cm_map->reflectmap = reflectmap;
	cm_map->scale = scale;

	*cubemap = MALLOC(size.x * size.y * sizeof(VEC3*));

	for(int i = 0;i < size.x * size.y;i++){
		cubemap[0][i] = MALLOC(sizeof(VEC3*) * 6);
		cubemap[0][i] = MALLOC(sizeof(VEC3*) * 6);
		for(int j = 0;j < 6;j++){
			cubemap[0][i][j] = MALLOC_ZERO(CM_SIZE * CM_SIZE * sizeof(VEC3));
			cubemap[0][i][j] = MALLOC_ZERO(CM_SIZE * CM_SIZE * sizeof(VEC3));
		}
	}
}

void addBoxReflection(int side,REFLECTMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale){
	SQUARE* square = &primitivehub.primitive[primitivehub.cnt - 6 + side].square;
	addReflection(&square->cubemap,&square->cm_properties,square->size,reflectmap,normalmap,scale,nscale);
}

void addBoxReflectionA(REFLECTMAP* reflectmap,NORMALMAP* normalmap,float scale,float nscale){
	for(int i = -6;i < 0;i++){
		SQUARE* square = &primitivehub.primitive[primitivehub.cnt + i].square;
		addReflection(&square->cubemap,&square->cm_properties,square->size,reflectmap,normalmap,scale,nscale);
	}
}

void addBoxEffectA(int side,int effect,EFFECT prop){
	SQUARE* square = &primitivehub.primitive[primitivehub.cnt - 6 + side].square;
	square->effect = effect;
	square->effect_prop = prop;
}

void addBox(VEC3 pos,VEC3 size,VEC3 color,char type){
	addHitbox(pos,size);
	addSquare(SQUARE_X,pos,color,(VEC2){size.y,size.z},type,TRUE);
	addSquare(SQUARE_X,(VEC3){pos.x+size.x,pos.y,pos.z},color,(VEC2){size.y,size.z},type,FALSE);

	addSquare(SQUARE_Y,pos,color,(VEC2){size.x,size.z},type,TRUE);
	addSquare(SQUARE_Y,(VEC3){pos.x,pos.y+size.y,pos.z},color,(VEC2){size.x,size.z},type,FALSE);

	addSquare(SQUARE_Z,pos,color,(VEC2){size.x,size.y},type,TRUE);
	addSquare(SQUARE_Z,(VEC3){pos.x,pos.y,pos.z+size.z},color,(VEC2){size.x,size.y},type,FALSE);
	BOX* box = &boxhub.box[boxhub.cnt++];
	box->hitbox = &hitboxhub.hitbox[hitboxhub.cnt-1];
	for(int i = 0;i < 6;i++)
		box->square[i] = &primitivehub.primitive[primitivehub.cnt-6+i];
	boxhub.selected = box;
}

void addSphere(VEC3 pos,VEC3 color,float radius,char type){
	PRIMITIVE* primitive = &primitivehub.primitive[primitivehub.cnt++];
	primitive->type = type;
	primitive->sphere.color = color;
	primitive->sphere.pos = pos;
	primitive->sphere.radius = radius;
	primitive->sphere.box_size = VEC3_SINGLE(radius);
	primitive->sphere.lightmap = MALLOC(sizeof(VEC3*) * 6);
	primitive->sphere.lightmap = MALLOC(sizeof(VEC3*) * 6);
	for(int j = 0;j < 6;j++){
		primitive->sphere.lightmap[j] = MALLOC_ZERO(LM_SPHERESIZE * LM_SPHERESIZE * sizeof(VEC3));
		primitive->sphere.lightmap[j] = MALLOC_ZERO(LM_SPHERESIZE * LM_SPHERESIZE * sizeof(VEC3));
	}
	primitivehub.selected = primitive;
}