#include "vec3.h"
#include "math.h"
#include "tmath.h"

VEC3 VEC3subVEC3R(VEC3 v1,VEC3 v2){
	return (VEC3){v1.x - v2.x,v1.y - v2.y,v1.z - v2.z};
}

VEC3 VEC3addVEC3R(VEC3 v1,VEC3 v2){
	return (VEC3){v1.x+v2.x,v1.y+v2.y,v1.z+v2.z};
}

VEC3 VEC3mulVEC3R(VEC3 v1,VEC3 v2){
	return (VEC3){v1.x*v2.x,v1.y*v2.y,v1.z*v2.z};
}

void VEC3div(VEC3* v,float d){
	v->x /= d;
	v->y /= d;
	v->z /= d;
}

void VEC3mul(VEC3* v1,float m){
	v1->x *= m;
	v1->y *= m;
	v1->z *= m;
}

VEC3 VEC3addR(VEC3 p,float s){
	return (VEC3){p.x + s,p.y + s,p.z + s};
}

VEC3 VEC3subR(VEC3 p,float s){
	return (VEC3){p.x - s,p.y - s,p.z - s};
}

VEC3 VEC3mulR(VEC3 p,float f){
	return (VEC3){p.x * f,p.y * f,p.z * f};
}

VEC3 VEC3divR(VEC3 p,float d){
	return (VEC3){p.x / d,p.y / d,p.z / d};
}

void VEC3subVEC3(VEC3* v1,VEC3 v2){
	v1->x -= v2.x;
	v1->y -= v2.y;
	v1->z -= v2.z;
}

void VEC3addVEC3(VEC3* v1,VEC3 v2){
	v1->x += v2.x;
	v1->y += v2.y;
	v1->z += v2.z;
}

void VEC3mulVEC3(VEC3* v1,VEC3 v2){
	v1->x *= v2.x;
	v1->y *= v2.y;
	v1->z *= v2.z;
}

void VEC3divVEC3(VEC3* v1,VEC3 v2){
	v1->x /= v2.x;
	v1->y /= v2.y;
	v1->z /= v2.z;
}

float VEC3dotR(VEC3 v1,VEC3 v2){
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float VEC3length(VEC3 p){
	return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}

void VEC3normalize(VEC3* p){
	float d = 1.0f / VEC3length(*p);
	p->x *= d;
	p->y *= d;
	p->z *= d;
}

VEC3 VEC3normalizeR(VEC3 p){
	float d = 1.0f / VEC3length(p);
	p.x *= d;
	p.y *= d;
	p.z *= d;
	return p;
}

float VEC3distance(VEC3 p,VEC3 p2){
	return VEC3length(VEC3subVEC3R(p,p2));
}

VEC3 VEC3divFR(VEC3 p,float p2){
	return (VEC3){p2/p.x,p2/p.y,p2/p.z};
}

VEC3 VEC3absR(VEC3 p){
	p.x = p.x < 0.0f ? -p.x : p.x;
	p.y = p.y < 0.0f ? -p.y : p.y;
	p.z = p.z < 0.0f ? -p.z : p.z;
	return p;	
}

VEC3 VEC3avgVEC3R(VEC3 v1,VEC3 v2){
	return (VEC3){(v1.x+v2.x)*0.5f,(v1.y+v2.y)*0.5f,(v1.z+v2.z)*0.5f};
}

VEC3 VEC3avgVEC3R4(VEC3 v1,VEC3 v2,VEC3 v3,VEC3 v4){
	return (VEC3){(v1.x + v2.x + v3.x + v4.x) * 0.25f,(v1.y + v2.y + v3.y + v4.y) * 0.25f,(v1.z + v2.z + v3.z + v4.z) * 0.25f};
}

VEC3 VEC3mixR(VEC3 v1,VEC3 v2,float mix){
	return (VEC3){v1.x * (1.0f - mix) + v2.x * mix,v1.y * (1.0f - mix) + v2.y * mix,v1.z * (1.0f - mix) + v2.z * mix};
}

VEC3 VEC3cross(VEC3 v1,VEC3 v2){
	VEC3 v3;
	v3.x = v1.y * v2.z - v2.y * v1.z;
	v3.y = v1.z * v2.x - v2.z * v1.x;
	v3.z = v1.x * v2.y - v2.x * v1.y;
	return v3;
}

VEC3 VEC3rnd(){
	return (VEC3){tRnd()+tRnd()+tRnd()-4.5f,tRnd()+tRnd()+tRnd()-4.5f,tRnd()+tRnd()+tRnd()-4.5f};
}

VEC3 VEC3negR(VEC3 v){
	return (VEC3){-v.x,-v.y,-v.z};
}

VEC3 VEC3reflect(VEC3 i,VEC3 n){
	return VEC3subVEC3R(i,VEC3mulR(n,2.0f * VEC3dotR(n,i)));
}