#include "vec2.h"
#include "math.h"

VEC2 VEC2subVEC2R(VEC2 v1,VEC2 v2){
	return (VEC2){v1.x-v2.x,v1.y-v2.y};
}

VEC2 VEC2addVEC2R(VEC2 v1,VEC2 v2){
	return (VEC2){v1.x+v2.x,v1.y+v2.y};
}

VEC2 VEC2mulVEC2R(VEC2 v1,VEC2 v2){
	return (VEC2){v1.x*v2.x,v1.y*v2.y};
}

void VEC2div(VEC2* v,float d){
	v->x /= d;
	v->y /= d;
}

void VEC2mul(VEC2* v,float m){
	v->x *= m;
	v->y *= m;
}

VEC2 VEC2subR(VEC2 p,float s){
	return (VEC2){p.x-s,p.y-s};
}

VEC2 VEC2mulR(VEC2 p,float f){
	return (VEC2){p.x*f,p.y*f};
}

VEC2 VEC2divR(VEC2 p,float d){
	return (VEC2){p.x/d,p.y/d};
}

void VEC2subVEC2(VEC2* v1,VEC2 v2){
	v1->x -= v2.x;
	v1->y -= v2.y;
}

void VEC2addVEC2(VEC2* v1,VEC2 v2){
	v1->x += v2.x;
	v1->y += v2.y;
}

void VEC2mulVEC2(VEC2* v1,VEC2 v2){
	v1->x *= v2.x;
	v1->y *= v2.y;
}

float VEC2dotR(VEC2 v1,VEC2 v2){
	return v1.x*v2.x+v1.y*v2.y;
}

float VEC2length(VEC2 p){
	return sqrtf(p.x*p.x+p.y*p.y);
}

void VEC2normalize(VEC2* p){
	float d = VEC2length(*p);
	p->x /= d;
	p->y /= d;
}

VEC2 VEC2normalizeR(VEC2 p){
	float d = VEC2length(p);
	p.x /= d;
	p.y /= d;
	return p;
}

float VEC2distance(VEC2 p,VEC2 p2){
	return VEC2length(VEC2subVEC2R(p,p2));
}

VEC2 VEC2divFR(VEC2 p,float p2){
	return (VEC2){p2/p.x,p2/p.y};
}

VEC2 VEC2absR(VEC2 p){
	p.x = p.x < 0.0f ? -p.x : p.x;
	p.y = p.y < 0.0f ? -p.y : p.y;
	return p;	
}

void VEC2rot(VEC2* v,float r){
	float temp = v->x * cosf(r) - v->y * sinf(r);
	v->y       = v->x * sinf(r) + v->y * cosf(r);
	v->x = temp;
}

VEC2 VEC2mixR(VEC2 v1,VEC2 v2,float mix){
	return (VEC2){v1.x * (1.0f - mix) + v2.x * mix,v1.y * (1.0f - mix) + v2.y * mix};
}