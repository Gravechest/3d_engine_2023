#include <math.h>

#include "ray.h"
#include "vec3.h"
#include "source.h"
#include "tmath.h"


float rayIntersectPlane(VEC3 pos,VEC3 dir,VEC3 plane){
	return -VEC3dotR(pos,plane) / VEC3dotR(dir,plane);
}

float rayIntersectSphere(VEC3 cam_pos,VEC3 sphere_pos,VEC3 cam_dir,float radius){
    VEC3 rel_pos = VEC3subVEC3R(cam_pos,sphere_pos);
	float a = VEC3dotR(cam_dir,cam_dir);
	float b = 2.0f * VEC3dotR(rel_pos,cam_dir);
	float c = VEC3dotR(rel_pos,rel_pos) - radius * radius;
	float h = b * b - 4.0f * a * c;
    if(h < 0.0f)
		return -1.0f;
	return (-b - sqrtf(h)) / (2.0f*a);
}

VEC3 rayIntersectTriangle(VEC3 ro,VEC3 rd,VEC3 v0,VEC3 v1,VEC3 v2){
    VEC3normalize(&rd);
    VEC3 v1v0 = VEC3subVEC3R(v1,v0);
    VEC3 v2v0 = VEC3subVEC3R(v2,v0);
    VEC3 rov0 = VEC3subVEC3R(ro,v0);
    VEC3  n = VEC3cross(v1v0,v2v0);
    VEC3  q = VEC3cross(rov0,rd);
    float d = 1.0f/VEC3dotR(rd,n);
    float u = d*VEC3dotR(VEC3negR(q),v2v0);
    float v = d*VEC3dotR(q,v1v0);
    float t = d*VEC3dotR(VEC3negR(n),rov0);
    if(u < 0.0f || v < 0.0f || (u + v) > 1.0f) 
        t = -1.0f;
    return (VEC3){t,u,v};
}

float rayIntersectSquare(VEC3 cam_pos,VEC3 cam_ang,SQUARE* square){
	VEC3 rel_pos = VEC3subVEC3R(cam_pos,square->pos);
    VEC3normalize(&cam_ang);
	float dst = rayIntersectPlane(rel_pos,cam_ang,square->normal);
	if(dst > 0.0f){
		VEC3 n_pos = VEC3addVEC3R(cam_pos,VEC3mulR(cam_ang,dst));
		VEC3 pos = VEC3subVEC3R(n_pos,square->pos);
		if(pos.a[square->v.x] < square->size.x && pos.a[square->v.y] < square->size.y && pos.a[square->v.x] > -square->size.x && pos.a[square->v.y] > -square->size.y)
			return dst;
		return -1.0f;
	}
	return -1.0f;
}

float rayIntersectBox(VEC3 ray_pos,VEC3 ray_dir,VEC3 box_pos,VEC3 box_size){
	VEC3 rel_pos = VEC3subVEC3R(ray_pos,box_pos);
	VEC3 delta = VEC3divFR(ray_dir,1.0f);
	VEC3 n = VEC3negR(VEC3mulVEC3R(delta,rel_pos));
	VEC3 k = VEC3mulVEC3R(VEC3absR(delta),box_size);
	VEC3 t1 = VEC3subVEC3R(n,k);
	VEC3 t2 = VEC3addVEC3R(n,k);
	float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
	float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
	if(tN > tF || tF < 0.0f)
		return -1.0f;
	return tN;
}

float rayIntersectBoxSearch(VEC3 ray_pos,VEC3 ray_dir,VEC3 box_pos,VEC3 box_size){
	VEC3 rel_pos = VEC3subVEC3R(ray_pos,box_pos);
	VEC3 delta = VEC3divFR(ray_dir,1.0f);
	VEC3 n = VEC3negR(VEC3mulVEC3R(delta,rel_pos));
	VEC3 k = VEC3mulVEC3R(VEC3absR(delta),box_size);
	VEC3 t1 = VEC3subVEC3R(n,k);
	VEC3 t2 = VEC3addVEC3R(n,k);
	float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
	float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
	if(tN > tF || tF < 0.0f)
		return -1.0f;	
	if(tN < 0.0f)
		tN = 0.0f;
	return tN;
}

float rayIntersectTorus(VEC3 ray_pos,VEC3 tor_pos,VEC3 rd,VEC2 tor){
	VEC3 p = VEC3subVEC3R(ray_pos,tor_pos);
    float po = 1.0f;
    float Ra2 = tor.x*tor.x;
    float ra2 = tor.y*tor.y;
    float m = VEC3dotR(p,p);
    float n = VEC3dotR(p,rd);
    float k = (m + Ra2 - ra2)/2.0f;
    float k3 = n;
    float k2 = n*n - Ra2*VEC2dotR((VEC2){rd.x,rd.y},(VEC2){rd.x,rd.y}) + k;
    float k1 = n*k - Ra2*VEC2dotR((VEC2){rd.x,rd.y},(VEC2){p.x,p.y});
    float k0 = k*k - Ra2*VEC2dotR((VEC2){p.x,p.y},(VEC2){p.x,p.y});
    
    if(tAbsf(k3*(k3*k3-k2)+k1) < 0.01f){
        po = -1.0f;
        float tmp=k1; k1=k3; k3=tmp;
        k0 = 1.0f/k0;
        k1 = k1*k0;
        k2 = k2*k0;
        k3 = k3*k0;
    }
    
    float c2 = k2*2.0f - 3.0f*k3*k3;
    float c1 = k3*(k3*k3-k2)+k1;
    float c0 = k3*(k3*(c2+2.0f*k2)-8.0f*k1)+4.0f*k0;
    c2 /= 3.0f;
    c1 *= 2.0f;
    c0 /= 3.0f;
    float Q = c2*c2 + c0;
    float R = c2*c2*c2 - 3.0f*c2*c0 + c1*c1;
    float h = R*R - Q*Q*Q;
    
    if( h>=0.0f){
        h = sqrtf(h);
        float v = R+h < 0.0f ? -1.0f : 1.0f*powf(tAbsf(R+h),1.0/3.0); // cube root
        float u = R-h < 0.0f ? -1.0f : 1.0f*powf(tAbsf(R-h),1.0/3.0); // cube root
		VEC2 s = { (v+u)+4.0f*c2, (v-u)*sqrtf(3.0f)};
        float y = sqrtf(0.5f*(VEC2length(s)+s.x));
        float x = 0.5f*s.y/y;
        float r = 2.0f*c1/(x*x+y*y);
        float t1 =  x - r - k3; t1 = (po<0.0f)?2.0f/t1:t1;
        float t2 = -x - r - k3; t2 = (po<0.0f)?2.0f/t2:t2;
        float t = 1e20;
        if( t1>0.0f ) t=t1;
        if( t2>0.0f ) t=tMinf(t,t2);
        return t;
    }
    
    float sQ = sqrtf(Q);
    float w = sQ*cosf( acosf(-R/(sQ*Q)) / 3.0f );
    float d2 = -(w+c2); if( d2<0.0f ) return -1.0f;
    float d1 = sqrtf(d2);
    float h1 = sqrtf(w - 2.0f*c2 + c1/d1);
    float h2 = sqrtf(w - 2.0f*c2 - c1/d1);
    float t1 = -d1 - h1 - k3; t1 = (po<0.0f)?2.0f/t1:t1;
    float t2 = -d1 + h1 - k3; t2 = (po<0.0f)?2.0f/t2:t2;
    float t3 =  d1 - h2 - k3; t3 = (po<0.0f)?2.0f/t3:t3;
    float t4 =  d1 + h2 - k3; t4 = (po<0.0f)?2.0f/t4:t4;
    float t = 1e20;
    if( t1>0.0f ) t=t1;
    if( t2>0.0f ) t=tMinf(t,t2);
    if( t3>0.0f ) t=tMinf(t,t3);
    if( t4>0.0f ) t=tMinf(t,t4);
    return t;
}