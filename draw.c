#include "source.h"
#include "draw.h"
#include "cubemap.h"
#include "tmath.h"

#include <intrin.h>

typedef struct{
	VEC3 pos;
	VEC3 dir;
	VEC3 delta;
	VEC3 side;

	IVEC3 step;
	IVEC3 square_pos;

	int square_side;
}RAY3D;

RAY3D ray3dCreate(VEC3 pos,VEC3 dir){
	RAY3D ray;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = VEC3absR(VEC3divFR(ray.dir,1.0f));

	ray.step.x = ray.dir.x < 0.0f ? -1 : 1;
	ray.side.x = ray.dir.x < 0.0f ? (ray.pos.x-(int)ray.pos.x) * ray.delta.x : ((int)ray.pos.x + 1.0f - ray.pos.x) * ray.delta.x;

	ray.step.y = ray.dir.y < 0.0f ? -1 : 1;
	ray.side.y = ray.dir.y < 0.0f ? (ray.pos.y-(int)ray.pos.y) * ray.delta.y : ((int)ray.pos.y + 1.0f - ray.pos.y) * ray.delta.y;

	ray.step.z = ray.dir.z < 0.0f ? -1 : 1;
	ray.side.z = ray.dir.z < 0.0f ? (ray.pos.z-(int)ray.pos.z) * ray.delta.z : ((int)ray.pos.z + 1.0f - ray.pos.z) * ray.delta.z;

	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	ray.square_pos.z = ray.pos.z;
	return ray;
}

void ray3dItterate(RAY3D *ray){
	if(ray->side.x < ray->side.y){
		if(ray->side.x < ray->side.z){
			ray->square_pos.x += ray->step.x;
			ray->side.x += ray->delta.x;
			return;
		}
		ray->square_pos.z += ray->step.z;
		ray->side.z += ray->delta.z;
		return;
	}
	if(ray->side.y < ray->side.z){
		ray->square_pos.y += ray->step.y;
		ray->side.y += ray->delta.y;
		return;
	}
	ray->square_pos.z += ray->step.z;
	ray->side.z += ray->delta.z;
}

VEC3 getReflection(SQUARE* square,int index,VEC2 cm,int crd){
	IVEC2 cm_crd_1 = (IVEC2){tMaxf(cm.x + 0.0f,0.0f),tMaxf(cm.y + 0.0f,0.0f)};
	IVEC2 cm_crd_2 = (IVEC2){tMinf(cm.x + 1.0f,CM_BOUND),tMaxf(cm.y + 0.0f,0.0f)};
	IVEC2 cm_crd_3 = (IVEC2){tMaxf(cm.x + 0.0f,0.0f),tMinf(cm.y + 1.0f,CM_BOUND)};
	IVEC2 cm_crd_4 = (IVEC2){tMinf(cm.x + 1.0f,CM_BOUND),tMinf(cm.y + 1.0f,CM_BOUND)};

	VEC3 c_1 = square->cubemap[crd][index][cm_crd_1.x * (int)CM_SIZE + cm_crd_1.y];
	VEC3 c_2 = square->cubemap[crd][index][cm_crd_2.x * (int)CM_SIZE + cm_crd_2.y];
	VEC3 c_3 = square->cubemap[crd][index][cm_crd_3.x * (int)CM_SIZE + cm_crd_3.y];
	VEC3 c_4 = square->cubemap[crd][index][cm_crd_4.x * (int)CM_SIZE + cm_crd_4.y];

	VEC2 t_crd = {tFract(cm.x),tFract(cm.y)};

	VEC3 m_1 = VEC3mixR(c_1,c_2,t_crd.x);
	VEC3 m_2 = VEC3mixR(c_3,c_4,t_crd.x);

	return VEC3mixR(m_1,m_2,t_crd.y);
}

void drawTexture(VEC3* color,VEC2 pos,float dst,VEC3 ang,VEC3 normal,TXMAP txmap,TEXTURE* texture){
	int i_dst;
	_BitScanReverse(&i_dst,(unsigned int)(dst / -VEC3dotR(ang,normal) * txmap.scale * texture->size / WND_RESOLUTION.y));
	i_dst = tMin(i_dst,texture->mipmap_amm);
	IVEC2 tx_crd;
	int tx_size = texture->size;
	tx_size >>= i_dst;
	tx_crd.x = tFract(pos.x * txmap.scale + txmap.offset.x) * tx_size;
	tx_crd.y = tFract(pos.y * txmap.scale + txmap.offset.y) * tx_size;
	VEC3 tx = texture->data[i_dst][tx_crd.y * tx_size + tx_crd.x];
	VEC3mulVEC3(color,tx);	
}

void rayMarch(VEC3* ang,VEC2* l_pos,IVEC3 v,int effect,EFFECT prop,VEC2 size,VEC3 normal){
	switch(effect){
	case 2:{
		VEC3normalize(ang);
		VEC3 rpos;
		rpos.a[v.x] = tFract(l_pos->x);
		rpos.a[v.y] = tFract(l_pos->y);
		rpos.a[v.z] = 1.0f;
		RAY3D ray = ray3dCreate(rpos,*ang);
		ray3dItterate(&ray);
		int z = ray.square_pos.a[v.z];
		for(int i = 0;i < 50;i++){
			VEC3 box_pos = VEC3addVEC3R((VEC3){ray.square_pos.x,ray.square_pos.y,ray.square_pos.z},VEC3_SINGLE(0.5f));
			float d = rayIntersectBox(rpos,*ang,box_pos,VEC3_SINGLE(0.3f));
			if(d > 0.0f){
				VEC3addVEC3(&rpos,VEC3mulR(*ang,d));
				l_pos->x += ang->a[v.x] * d;
				l_pos->y += ang->a[v.y] * d;
				l_pos->x = tFract(l_pos->x / size.x) * size.x;
				l_pos->y = tFract(l_pos->y / size.y) * size.y;
				VEC3 l = VEC3subVEC3R(VEC3_SINGLE(0.5f),rpos);
				VEC2 y = VEC2mulR(VEC2normalizeR((VEC2){l.x,l.y}),prop.radia.x);
				VEC3 normal = (VEC3){(l.x - y.x),(l.y - y.y),l.z};
				*ang = VEC3reflect(*ang,VEC3normalizeR(normal));
				return;
			}
			ray3dItterate(&ray);
			if(ray.square_pos.a[v.z] != z){
				float dst = rayIntersectPlane(rpos,*ang,normal);
				l_pos->x += ang->a[v.x] * dst;
				l_pos->y += ang->a[v.y] * dst;
				if(l_pos->x < -size.x) 
					l_pos->x += -(l_pos->x + size.x) * 2.0f; 
				if(l_pos->x > size.x) 
					l_pos->x -= (l_pos->x - size.x) * 2.0f; 
				if(l_pos->y < -size.y) 
					l_pos->y += -(l_pos->y + size.y) * 2.0f;
				if(l_pos->y > size.y) 
					l_pos->y -= (l_pos->y - size.y) * 2.0f; 
				return;
			}
		}
		return;}
	case 0:
		return;
	case 1:
		VEC3normalize(ang);
		VEC3 rpos;
		rpos.a[v.x] = tFract(l_pos->x);
		rpos.a[v.y] = tFract(l_pos->y);
		rpos.a[v.z] = 1.0f;
		RAY3D ray = ray3dCreate(rpos,*ang);
		ray3dItterate(&ray);
		int z = ray.square_pos.a[v.z];
		for(int i = 0;i < 50;i++){
			VEC3 sphere_pos = VEC3addVEC3R((VEC3){ray.square_pos.x,ray.square_pos.y,ray.square_pos.z},VEC3_SINGLE(0.5f));
			float d = rayIntersectSphere(rpos,sphere_pos,*ang,0.25f);
			if(d > 0.0f){
				VEC3addVEC3(&rpos,VEC3mulR(*ang,d));
				l_pos->x += ang->a[v.x] * d;
				l_pos->y += ang->a[v.y] * d;
				l_pos->x = tFract(l_pos->x / size.x) * size.x;
				l_pos->y = tFract(l_pos->y / size.y) * size.y;
				VEC3 l = VEC3subVEC3R(VEC3_SINGLE(0.5f),rpos);
				VEC2 y = VEC2mulR(VEC2normalizeR((VEC2){l.x,l.y}),prop.radia.x);
				VEC3 normal = (VEC3){(l.x - y.x),(l.y - y.y),l.z};
				*ang = VEC3reflect(*ang,VEC3normalizeR(normal));
				return;
			}
			ray3dItterate(&ray);
			if(ray.square_pos.a[v.z] != z){
				float dst = rayIntersectPlane(rpos,*ang,normal);
				l_pos->x += ang->a[v.x] * dst;
				l_pos->y += ang->a[v.y] * dst;
				if(l_pos->x < -size.x) 
					l_pos->x += -(l_pos->x + size.x) * 2.0f; 
				if(l_pos->x > size.x) 
					l_pos->x -= (l_pos->x - size.x) * 2.0f; 
				if(l_pos->y < -size.y) 
					l_pos->y += -(l_pos->y + size.y) * 2.0f;
				if(l_pos->y > size.y) 
					l_pos->y -= (l_pos->y - size.y) * 2.0f; 
				return;
			}
		}
	}
}
#include <stdio.h>
void hitRayNormal(PRIMITIVE* hitted,VEC3* color,VEC3 pos,VEC3 ang,VEC2 uv,float dst){
	VEC3 p,b_color;
	RGB tx;
	int texture_id,index;
	VEC3 c_1;
	VEC3 c_2;
	VEC3 c_3;
	VEC3 c_4;
	VEC3 m_1,m_2;
	VEC2 t_crd;
	VEC3 normal;
	VEC3 t_pos;
	TEXTURE* texture;
	IVEC2 lm_crd_1;
	IVEC2 lm_crd_2;
	IVEC2 lm_crd_3;
	IVEC2 lm_crd_4;
	VEC3 rf_ang = ang;
	switch(hitted->type){
	case PR_TRIANGLE:{
		TRIANGLE triangle = hitted->triangle;
		t_crd = VEC2mulVEC2R(uv,triangle.size);
		t_crd.x = tFract(t_crd.x + 0.5f);
		t_crd.y = tFract(t_crd.y + 0.5f);
		IVEC2 crd = {uv.x * triangle.size.x + 0.5f,uv.y * triangle.size.y + 0.5f};
		lm_crd_1 = (IVEC2){crd.x+0,crd.y+0};
		lm_crd_2 = (IVEC2){crd.x+1,crd.y+0};
		lm_crd_3 = (IVEC2){crd.x+0,crd.y+1};
		lm_crd_4 = (IVEC2){crd.x+1,crd.y+1};

		printf("%i\n",lm_crd_4.y * (int)(triangle.size.x+2) + lm_crd_4.x);

		c_1 = triangle.lightmap[lm_crd_1.y * (int)(triangle.size.x+2) + lm_crd_1.x];
		c_2 = triangle.lightmap[lm_crd_2.y * (int)(triangle.size.x+2) + lm_crd_2.x];
		c_3 = triangle.lightmap[lm_crd_3.y * (int)(triangle.size.x+2) + lm_crd_3.x];
		c_4 = triangle.lightmap[lm_crd_4.y * (int)(triangle.size.x+2) + lm_crd_4.x];

		m_1 = VEC3mixR(c_1,c_2,t_crd.x);
		m_2 = VEC3mixR(c_3,c_4,t_crd.x);

		b_color = VEC3mixR(m_1,m_2,t_crd.y);
		VEC3mulVEC3(color,b_color);
		break;}
	case PR_SQUARE:;
		SQUARE square = hitted->square;
		VEC2 l_pos = {pos.a[square.v.x] - square.pos.a[square.v.x],pos.a[square.v.y] - square.pos.a[square.v.y]};
		rayMarch(&rf_ang,&l_pos,square.v,square.effect,square.effect_prop,square.size,square.normal);

		l_pos.x = tMinf(tMaxf(l_pos.x,-square.size.x),square.size.x);
		l_pos.y = tMinf(tMaxf(l_pos.y,-square.size.y),square.size.y);
		VEC3mul(&l_pos,0.5f);
		
		VEC2addVEC2(&l_pos,VEC2mulR(square.size,0.5f));
		texture = square.texture.data;
		IVEC2 crd = {l_pos.x * LM_SIZE + 0.5f,l_pos.y * LM_SIZE + 0.5f};

		t_crd = (VEC2){tFract(l_pos.x * LM_SIZE + 0.5f),tFract(l_pos.y * LM_SIZE + 0.5f)};
		lm_crd_1 = (IVEC2){crd.x+0,crd.y+0};
		lm_crd_2 = (IVEC2){crd.x+1,crd.y+0};
		lm_crd_3 = (IVEC2){crd.x+0,crd.y+1};
		lm_crd_4 = (IVEC2){crd.x+1,crd.y+1};

		c_1 = square.lightmap[lm_crd_1.y * (int)(square.size.x * LM_SIZE+2) + lm_crd_1.x];
		c_2 = square.lightmap[lm_crd_2.y * (int)(square.size.x * LM_SIZE+2) + lm_crd_2.x];
		c_3 = square.lightmap[lm_crd_3.y * (int)(square.size.x * LM_SIZE+2) + lm_crd_3.x];
		c_4 = square.lightmap[lm_crd_4.y * (int)(square.size.x * LM_SIZE+2) + lm_crd_4.x];

		m_1 = VEC3mixR(c_1,c_2,t_crd.x);
		m_2 = VEC3mixR(c_3,c_4,t_crd.x);

		b_color = VEC3mixR(m_1,m_2,t_crd.y);

		REFLECTMAP* reflectmap = square.cm_properties.reflectmap;

		if(reflectmap != RF_VOID){
			rf_ang = ang;
			NORMALMAP* normalmap = square.cm_properties.normalmap;
			if(normalmap){
				IVEC2 np_pos;
				np_pos.x = tFract(l_pos.x * square.cm_properties.nscale) * normalmap->size;
				np_pos.y = tFract(l_pos.y * square.cm_properties.nscale) * normalmap->size;
				VEC2 reflect_v;
				reflect_v.x = normalmap->value[np_pos.x * normalmap->size + np_pos.y].x;
				reflect_v.y = normalmap->value[np_pos.x * normalmap->size + np_pos.y].y;
				if(reflect_v.x || reflect_v.y){
					VEC3 n;
					n.a[square.v.x] = reflect_v.x * -square.normal.a[square.v.z];
					n.a[square.v.y] = reflect_v.y * -square.normal.a[square.v.z];
					n.a[square.v.z] = square.normal.a[square.v.z];
					rf_ang = VEC3reflect(rf_ang,n);
				}
			}

			VEC2 cm = VEC2mulR(sampleCube(rf_ang,&index),CM_BOUND);

			IVEC2 size_c = {square.size.x * CM_SPREAD,square.size.y * CM_SPREAD};

			VEC2 crd_c = {l_pos.x * CM_SPREAD - 0.5f,l_pos.y * CM_SPREAD - 0.5f};

			if(size_c.x < 1) size_c.x = 1;
			if(size_c.y < 1) size_c.y = 1;

			VEC2 t_crd_c = {tFract(l_pos.x * CM_SPREAD + 0.5f),tFract(l_pos.y * CM_SPREAD + 0.5f)};

			IVEC2 cm_crd_1 = (IVEC2){tMaxf(crd_c.x+0.0f,0.0f),tMaxf(crd_c.y+0.0f,0.0f)};
			IVEC2 cm_crd_2 = (IVEC2){tMinf(crd_c.x+1.0f,size_c.x-1),tMaxf(crd_c.y+0.0f,0.0f)};
			IVEC2 cm_crd_3 = (IVEC2){tMaxf(crd_c.x+0.0f,0.0f),tMinf(crd_c.y+1.0f,size_c.y-1)};
			IVEC2 cm_crd_4 = (IVEC2){tMinf(crd_c.x+1.0f,size_c.x-1),tMinf(crd_c.y+1.0f,size_c.y-1)};

			VEC3 reflect_1 = getReflection(&hitted->square,index,cm,cm_crd_1.y * size_c.x + cm_crd_1.x);
			VEC3 reflect_2 = getReflection(&hitted->square,index,cm,cm_crd_2.y * size_c.x + cm_crd_2.x);
			VEC3 reflect_3 = getReflection(&hitted->square,index,cm,cm_crd_3.y * size_c.x + cm_crd_3.x);
			VEC3 reflect_4 = getReflection(&hitted->square,index,cm,cm_crd_4.y * size_c.x + cm_crd_4.x);
		
			VEC3 m_1_c = VEC3mixR(reflect_1,reflect_2,t_crd_c.x);
			VEC3 m_2_c = VEC3mixR(reflect_3,reflect_4,t_crd_c.x);

			VEC3 reflect = VEC3mixR(m_1_c,m_2_c,t_crd_c.y);

			if(reflectmap){
				IVEC2 tx_crd;
				tx_crd.x = tFract(t_pos.a[square.v.x] * square.cm_properties.scale) * reflectmap->size;
				tx_crd.y = tFract(t_pos.a[square.v.y] * square.cm_properties.scale) * reflectmap->size;

				b_color = VEC3mixR(b_color,reflect,reflectmap->value[tx_crd.x * reflectmap->size + tx_crd.y]);
			}
			else
				b_color = VEC3mixR(b_color,reflect,square.cm_properties.scale);
		}
		if(texture)
			drawTexture(&b_color,l_pos,dst,ang,square.normal,square.texture,texture);			
		VEC3mulVEC3(color,b_color);
		break;
	case PR_SPHERE:;
		SPHERE sphere = hitted->sphere;
		VEC3 t_ang = VEC3normalizeR(VEC3subVEC3R(pos,sphere.pos));
		
		VEC2 sp_pos = VEC2mulR(sampleCube(t_ang,&index),LM_SPHERESIZE);
		sp_pos.x -= 0.5f; sp_pos.y -= 0.5f;
		IVEC2 cm_crd_1 = (IVEC2){tMaxf(sp_pos.x + 0.0f,0.0f),tMaxf(sp_pos.y + 0.0f,0.0f)};
		IVEC2 cm_crd_2 = (IVEC2){tMinf(sp_pos.x + 1.0f,LM_SPHERESIZE-1.0f),tMaxf(sp_pos.y + 0.0f,0.0f)};
		IVEC2 cm_crd_3 = (IVEC2){tMaxf(sp_pos.x + 0.0f,0.0f),tMinf(sp_pos.y + 1.0f,LM_SPHERESIZE-1.0f)};
		IVEC2 cm_crd_4 = (IVEC2){tMinf(sp_pos.x + 1.0f,LM_SPHERESIZE-1.0f),tMinf(sp_pos.y + 1.0f,LM_SPHERESIZE-1.0f)};

		c_1 = sphere.lightmap[index][cm_crd_1.x * (int)LM_SPHERESIZE + cm_crd_1.y];
		c_2 = sphere.lightmap[index][cm_crd_2.x * (int)LM_SPHERESIZE + cm_crd_2.y];
		c_3 = sphere.lightmap[index][cm_crd_3.x * (int)LM_SPHERESIZE + cm_crd_3.y];
		c_4 = sphere.lightmap[index][cm_crd_4.x * (int)LM_SPHERESIZE + cm_crd_4.y];
		
		t_crd = (VEC2){tFract(sp_pos.x),tFract(sp_pos.y)};

		m_1 = VEC3mixR(c_1,c_2,t_crd.x);
		m_2 = VEC3mixR(c_3,c_4,t_crd.x);

		VEC3 lm_color = VEC3mixR(m_1,m_2,t_crd.y);
	
		VEC3 r_ang = VEC3reflect(ang,VEC3normalizeR(VEC3subVEC3R(pos,sphere.pos)));
		VEC2 cm = VEC2mulR(sampleCube(r_ang,&index),CM_SIZE);
		cm.x -= 0.5f; cm.y -= 0.5f;
		cm_crd_1 = (IVEC2){tMaxf(cm.x + 0.0f,0.0f),tMaxf(cm.y + 0.0f,0.0f)};
		cm_crd_2 = (IVEC2){tMinf(cm.x + 1.0f,CM_SIZE-1.0f),tMaxf(cm.y + 0.0f,0.0f)};
		cm_crd_3 = (IVEC2){tMaxf(cm.x + 0.0f,0.0f),tMinf(cm.y + 1.0f,CM_SIZE-1.0f)};
		cm_crd_4 = (IVEC2){tMinf(cm.x + 1.0f,CM_SIZE-1.0f),tMinf(cm.y + 1.0f,CM_SIZE-1.0f)};

		if(sphere.cubemap){
			c_1 = sphere.cubemap[index][cm_crd_1.x * (int)CM_SIZE + cm_crd_1.y];
			c_2 = sphere.cubemap[index][cm_crd_2.x * (int)CM_SIZE + cm_crd_2.y];
			c_3 = sphere.cubemap[index][cm_crd_3.x * (int)CM_SIZE + cm_crd_3.y];
			c_4 = sphere.cubemap[index][cm_crd_4.x * (int)CM_SIZE + cm_crd_4.y];
		
			t_crd = (VEC2){tFract(cm.x),tFract(cm.y)};

			m_1 = VEC3mixR(c_1,c_2,t_crd.x);
			m_2 = VEC3mixR(c_3,c_4,t_crd.x);
		
			VEC3mulVEC3(color,VEC3mixR(m_1,m_2,t_crd.y));
		}
		VEC3mulVEC3(color,lm_color);
		texture = sphere.texture.data;
		if(texture)
			drawTexture(color,sp_pos,dst,ang,VEC3normalizeR(VEC3subVEC3R(pos,sphere.pos)),sphere.texture,texture);
		break;
	case PR_LIGHTSQUARE:
		VEC3mulVEC3(color,hitted->square.color);
		break;
	}
}