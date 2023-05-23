#include <math.h>

#include "source.h"
#include "draw_flat.h"
#include "tmath.h"
#include "cubemap.h"
#include "draw.h"

void hitRayFlat(PRIMITIVE* hitted,VEC3* color,VEC3 pos,VEC3 ang,VEC2 uv,float dst){
	switch(hitted->type){
	case PR_TRIANGLE:{
		TRIANGLE triangle = hitted->triangle;
		VEC2 l_pos = uv;
		VEC3 b_color = VEC3_SINGLE(1.0f);
		if(primitivehub.selected == hitted){
			if(VEC3distance(triangle.pos[triangle_point_selected],pos) < 0.1){
				VEC3mulVEC3(color,VEC3mulR((VEC3){1.0f,0.75f,0.0f},cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
				break;
			}
			if(uv.x < 0.1f || uv.y < 0.1f){
				VEC3mul(color,(cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
				break;
			}
		}
		TEXTURE* texture = triangle.texture.data;
		if(texture)
			drawTexture(&b_color,l_pos,dst,ang,triangle.normal,triangle.texture,texture);
		else{
			if((tFloori(l_pos.x * 8.0f + 99.0f) & 1) ^ (tFloori(l_pos.y * 8.0f + 99.0f) & 1))
				b_color = VEC3mulR(VEC3normalizeR(triangle.color),0.9f);
			else
				b_color = VEC3mulR(VEC3normalizeR(triangle.color),1.1f);
		}
		VEC3mulVEC3(color,b_color);
		break;}
	case PR_SPHERE:{
		SPHERE sphere = hitted->sphere;
		if(primitivehub.selected == hitted){
			if(VEC3dotR(VEC3normalizeR(VEC3subVEC3R(sphere.pos,pos)),ang) < 0.25f){
				VEC3mul(color,(cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
				break;
			}
		}
		TEXTURE* texture = sphere.texture.data;
		int index;
		VEC3 t_ang = VEC3normalizeR(VEC3subVEC3R(pos,sphere.pos));
		VEC3 b_color = VEC3_SINGLE(1.0f);
		VEC2 sp_pos = VEC2mulR(sampleCube(t_ang,&index),LM_SPHERESIZE);
		sp_pos.x -= 0.5f; sp_pos.y -= 0.5f;
		if(texture)
			drawTexture(&b_color,sp_pos,dst,ang,VEC3normalizeR(VEC3subVEC3R(pos,sphere.pos)),sphere.texture,texture);
		VEC3mulVEC3(color,b_color);
		break;}
	case PR_LIGHTSQUARE:{
		SQUARE square = hitted->square;
		VEC3 b_color = VEC3_SINGLE(1.0f);
		VEC2 l_pos = {pos.a[square.v.x] - square.pos.a[square.v.x],pos.a[square.v.y] - square.pos.a[square.v.y]};
		if(primitivehub.selected == hitted){
			char bound_x = l_pos.x < -square.size.x + square.size.x * 0.1f || l_pos.x > square.size.x - square.size.x * 0.1f;
			char bound_y = l_pos.y < -square.size.y + square.size.y * 0.1f || l_pos.y > square.size.y - square.size.y * 0.1f;
			if(bound_x || bound_y){
				VEC3mul(color,(cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
				break;
			}
		}
		for(int i = 0;i < 6;i++){
			if(boxhub.selected->square[i] == hitted){
				char bound_x = l_pos.x < -square.size.x + square.size.x * 0.05f || l_pos.x > square.size.x - square.size.x * 0.05f;
				char bound_y = l_pos.y < -square.size.y + square.size.y * 0.05f || l_pos.y > square.size.y - square.size.y * 0.05f;
				if(bound_x || bound_y){
					VEC3mulVEC3(color,VEC3mulR((VEC3){1.0f,0.75f,0.0f},cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
					break;
				}
			}
		}
		VEC3mul(&l_pos,0.5f);
		if((tFloori(l_pos.x * 8.0f + 99.0f) & 1) ^ (tFloori(l_pos.y * 8.0f + 99.0f) & 1))
			b_color = VEC3mulR(VEC3normalizeR(square.color),0.9f);
		else
			b_color = VEC3mulR(VEC3normalizeR(square.color),1.1f);
		VEC3mulVEC3(color,b_color);
		break;}
	case PR_SQUARE:{
		SQUARE square = hitted->square;
		VEC2 l_pos = {pos.a[square.v.x] - square.pos.a[square.v.x],pos.a[square.v.y] - square.pos.a[square.v.y]};
		if(primitivehub.selected == hitted){
			char bound_x = l_pos.x < -square.size.x + square.size.x * 0.1f || l_pos.x > square.size.x - square.size.x * 0.1f;
			char bound_y = l_pos.y < -square.size.y + square.size.y * 0.1f || l_pos.y > square.size.y - square.size.y * 0.1f;
			if(bound_x || bound_y){
				VEC3mul(color,(cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
				break;
			}
		}
		for(int i = 0;i < 6;i++){
			if(boxhub.selected->square[i] == hitted){
				char bound_x = l_pos.x < -square.size.x + square.size.x * 0.05f || l_pos.x > square.size.x - square.size.x * 0.05f;
				char bound_y = l_pos.y < -square.size.y + square.size.y * 0.05f || l_pos.y > square.size.y - square.size.y * 0.05f;
				if(bound_x || bound_y){
					VEC3mulVEC3(color,VEC3mulR((VEC3){1.0f,0.75f,0.0f},cosf(((float)(global_tick % 120) / 120.0f)*M_PI*2.0f)+1.0f));
					break;
				}
			}
		}
		VEC3mul(&l_pos,0.5f);
		TEXTURE* texture = square.texture.data;
		VEC3 b_color = VEC3_SINGLE(1.0f);
		if(texture)
			drawTexture(&b_color,l_pos,dst,ang,square.normal,square.texture,texture);			
		else{
			if((tFloori(l_pos.x * 8.0f + 99.0f) & 1) ^ (tFloori(l_pos.y * 8.0f + 99.0f) & 1))
				b_color = VEC3mulR(square.color,0.9f);
			else
				b_color = VEC3mulR(square.color,1.1f);
		}
		VEC3mulVEC3(color,b_color);
		break;}
	}
}