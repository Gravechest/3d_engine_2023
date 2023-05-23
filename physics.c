#include <Windows.h>
#include <math.h>

#include "phyiscs.h"
#include "source.h"
#include "tmath.h"

HITBOXHUB hitboxhub;
int global_tick;

void walkPhysics(){
	VEC3addVEC3(&camera.pos,camera.vel);
	VEC3 hitbox_pos = {camera.pos.x,camera.pos.y,camera.pos.z - 1.0f};
	char hx = FALSE,hy = FALSE,hz = FALSE;
	float hx_stair = 0.0f,hy_stair = 0.0f;
	for(int i = 0;i < hitboxhub.cnt;i++){
		HITBOX hitbox = hitboxhub.hitbox[i];
		if(boxInBox(hitbox_pos,(VEC3){0.2f,0.2f,0.5f},hitbox.pos,hitbox.size)){
			hitbox_pos.x -= camera.vel.x;
			if(!boxInBox(hitbox_pos,(VEC3){0.2f,0.2f,0.5f},hitbox.pos,hitbox.size)){
				hx = TRUE;
				hx_stair = (hitbox.pos.z + hitbox.size.z) - (hitbox_pos.z - 0.5f) ;
			}
			hitbox_pos.x += camera.vel.x;
			hitbox_pos.y -= camera.vel.y;
			if(!boxInBox(hitbox_pos,(VEC3){0.2f,0.2f,0.5f},hitbox.pos,hitbox.size)){
				hy = TRUE;
				hy_stair = (hitbox.pos.z + hitbox.size.z) - (hitbox_pos.z - 0.5f);
			}
			hitbox_pos.y += camera.vel.y;
			hitbox_pos.z -= camera.vel.z;
			if(!boxInBox(hitbox_pos,(VEC3){0.2f,0.2f,0.5f},hitbox.pos,hitbox.size))
				hz = TRUE;
			hitbox_pos.z += camera.vel.z;
		}
	}
	if(hx){
		if(hx_stair < 0.6f){
			camera.pos.z += hx_stair;
		}
		else{
			camera.pos.x -= camera.vel.x;
			camera.vel.x = 0.0f;
		}
	}
	if(hy){
		if(hy_stair < 0.6f){
			camera.pos.z += hy_stair;
		}
		else{
			camera.pos.y -= camera.vel.y;
			camera.vel.y = 0.0f;
		}
	}
	if(hz){
		camera.pos.z -= camera.vel.z;
		camera.vel.z = 0.0f;
		if(key.w){
			float mod = key.d || key.a ? 0.0042f : 0.006f;
			camera.vel.x += cosf(camera.dir.x) * mod * 2.0f;
			camera.vel.y += sinf(camera.dir.x) * mod * 2.0f;
		}
		if(key.s){
			float mod = key.d || key.a ? 0.0042f : 0.006f;
			camera.vel.x -= cosf(camera.dir.x) * mod * 2.0f;
			camera.vel.y -= sinf(camera.dir.x) * mod * 2.0f;
		}
		if(key.d){
			float mod = key.s || key.w ? 0.0042f : 0.006f;
			camera.vel.x += cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		}
		if(key.a){
			float mod = key.s || key.w ? 0.0042f : 0.006f;
			camera.vel.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		}
		if(GetKeyState(VK_SPACE) & 0x80)
			camera.vel.z = 0.25f;
		VEC3mul(&camera.vel,0.9f);
	}
	else
		VEC3mul(&camera.vel,0.99f);
	camera.vel.z -= 0.015f;
}

void physics(){
	for(;;){
		key.w = GetKeyState(VK_W) & 0x80;
		key.s = GetKeyState(VK_S) & 0x80;
		key.d = GetKeyState(VK_D) & 0x80;
		key.a = GetKeyState(VK_A) & 0x80;
		global_tick++;
		if(!camera.mv_mode){
			float mod = (GetKeyState(VK_LCONTROL) & 0x80) ? 3.0f : 1.0f;
			if(key.w){
				float mod_2 = key.d || key.a ? mod * 0.042f : mod * 0.06f;
				camera.pos.x += cosf(camera.dir.x) * mod_2;
				camera.pos.y += sinf(camera.dir.x) * mod_2;
			}
			if(key.s){
				float mod_2 = key.d || key.a ? mod * 0.042f : mod * 0.06f;
				camera.pos.x -= cosf(camera.dir.x) * mod_2;
				camera.pos.y -= sinf(camera.dir.x) * mod_2;
			}
			if(key.d){
				float mod_2 = key.s || key.w ? mod * 0.042f : mod * 0.06f;
				camera.pos.x += cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
				camera.pos.y += sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
			}
			if(key.a){
				float mod_2 = key.s || key.w ? mod * 0.042f : mod * 0.06f;
				camera.pos.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
				camera.pos.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
			}
			if(GetKeyState(VK_SPACE) & 0x80) 
				camera.pos.z += 0.04f * mod;
			if(GetKeyState(VK_LSHIFT) & 0x80) 
				camera.pos.z -= 0.04f * mod;
			}
		else
			walkPhysics();
		
		camera.dir_tri = (VEC4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		Sleep(3);
	}
}