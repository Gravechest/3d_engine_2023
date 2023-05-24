#include "cubemap.h"
#include "ray.h"

ZBUFFER* cm_zbuffer;

CUBEMAPTR cubemaptr[6] = {
	{TRUE,TRUE,FALSE},
	{TRUE,TRUE,FALSE},
	{FALSE,TRUE,TRUE},
	{TRUE,FALSE,TRUE},
	{TRUE,TRUE,FALSE},
	{TRUE,TRUE,FALSE}
};

VEC4 sd_dirtri[6] = {
	{ 1.0f, 0.0f, 1.0f, 0.0f},
	{ 1.0f, 0.0f,-1.0f, 0.0f},
	{ 0.0f, 1.0f, 1.0f, 0.0f},
	{ 0.0f,-1.0f, 1.0f, 0.0f},
	{ 1.0f, 0.0f, 0.0f, 1.0f},
	{ 1.0f, 0.0f, 0.0f,-1.0f}
};

VEC3 getRayAngleCB(IVEC2 rd_size,VEC4 dir_tri,int x,int y){
	VEC3 ray_ang;
	float pixelOffsetY = (((float)(x) * 2.0f / rd_size.x) - 1.0f);
	float pixelOffsetX = (((float)(y) * 2.0f / rd_size.y) - 1.0f);
	ray_ang.x = (dir_tri.x * dir_tri.z - dir_tri.x * dir_tri.w * pixelOffsetY) - dir_tri.y * pixelOffsetX;
	ray_ang.y = (dir_tri.y * dir_tri.z - dir_tri.y * dir_tri.w * pixelOffsetY) + dir_tri.x * pixelOffsetX;
	ray_ang.z = dir_tri.w + dir_tri.z * pixelOffsetY;
	return ray_ang;
}

VEC2 sampleCube(VEC3 v,int* faceIndex){
	VEC3 vAbs = VEC3absR(v);
	float ma;
	VEC2 uv;
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y){
		*faceIndex = v.z < 0.0f ? 5.0f : 4.0f;
		ma = 0.5f / vAbs.z;
		uv = (VEC2){v.z < 0.0f ? -v.x : v.x, -v.y};
	}
	else if(vAbs.y >= vAbs.x){
		*faceIndex = v.y < 0.0f ? 3.0f : 2.0f;
		ma = 0.5f / vAbs.y;
		uv = (VEC2){v.x, v.y < 0.0f ? -v.z : v.z};
	}
	else{
		*faceIndex = v.x < 0.0f ? 1.0f : 0.0f;
		ma = 0.5f / vAbs.x;
		uv = (VEC2){v.x < 0.0f ? v.z : -v.z, -v.y};
	}
	return (VEC2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
}

void mixCubemapSide(VEC3* restrict side_1,VEC3* restrict side_2){
	VEC3 mix = VEC3avgVEC3R(*side_1,*side_2);
	*side_1 = mix;
	*side_2 = mix;
}

void rdCubemap(VEC3* cubemap,VEC3 l_pos,int side){
	IVEC2 c = {0,0};
	for(int cx = 0;cx < (int)CM_SIZE;cx++){ 
		for(int cy = 0;cy < (int)CM_SIZE;cy++){
			cm_zbuffer[cx * (int)CM_SIZE + cy].dst = 999999.0f;
			cm_zbuffer[cx * (int)CM_SIZE + cy].primitive = 0;
		}
	}
	for(int i = 0;i < primitivehub.cnt;i++){
		c = (IVEC2){0,0};
		for(;c.x < CM_SIZE;c.x++){
			for(c.y = 0;c.y < CM_SIZE;c.y++){
				int cxx = c.a[cubemaptr[side].sideways];
				int cyy = c.a[!cubemaptr[side].sideways];
				cxx = cubemaptr[side].upside ? CM_BOUND - cxx : cxx;
				cyy = cubemaptr[side].revert ? CM_BOUND - cyy : cyy;
				VEC3 ray_ang = getRayAngleCB((IVEC2){CM_SIZE,CM_SIZE},sd_dirtri[side],cxx,cyy);
				ray_ang.z += 0.001f;
				ray_ang.y += 0.001f;
				ray_ang.x += 0.001f;
				float dst = 0.0f;
				switch(primitivehub.primitive[i].type){
				case PR_SQUARE: case PR_LIGHTSQUARE:;
					SQUARE square = primitivehub.primitive[i].square;
					dst = rayIntersectSquare(l_pos,ray_ang,&square); 
					break;
				case PR_SPHERE:;
					SPHERE sphere = primitivehub.primitive[i].sphere;
					dst = rayIntersectSphere(l_pos,sphere.pos,ray_ang,sphere.radius); 
					break;
				}
				if(dst > 0 && cm_zbuffer[c.x * (int)CM_SIZE + c.y].dst > dst){
					cm_zbuffer[c.x * (int)CM_SIZE + c.y].dst = dst;
					cm_zbuffer[c.x * (int)CM_SIZE + c.y].primitive = &primitivehub.primitive[i];
				}
			}
		}
	}
	c = (IVEC2){0,0};
	for(;c.x < CM_SIZE;c.x++){ 
		for(c.y = 0;c.y < CM_SIZE;c.y++){
			int cxx = c.a[cubemaptr[side].sideways];
			int cyy = c.a[!cubemaptr[side].sideways];
			cxx = cubemaptr[side].upside ? CM_BOUND - cxx : cxx;
			cyy = cubemaptr[side].revert ? CM_BOUND - cyy : cyy;
			VEC3 ray_ang = getRayAngleCB((IVEC2){CM_SIZE,CM_SIZE},sd_dirtri[side],cxx,cyy);
			if(cm_zbuffer[c.x * (int)CM_SIZE + c.y].primitive){
				VEC3 color = {1.0f,1.0f,1.0f};
				ray_ang.z += 0.001f;
				ray_ang.y += 0.001f;
				ray_ang.x += 0.001f;
				VEC3 pos = VEC3addVEC3R(l_pos,VEC3mulR(ray_ang,cm_zbuffer[c.x * (int)CM_SIZE + c.y].dst));
				hitRayNormal(cm_zbuffer[c.x * (int)CM_SIZE + c.y].primitive,&color,pos,ray_ang,cm_zbuffer[c.x * (int)CM_SIZE + c.y].dst);
				cubemap[c.x * (int)CM_SIZE + c.y] = color;
			}
			else{
				int index;
				VEC2 cm = VEC2mulR(sampleCube(ray_ang,&index),skybox.size-1);
				IVEC2 cmi = {cm.x,cm.y};
				cubemap[c.x * (int)CM_SIZE + c.y] = skybox.sides[index][cmi.x * skybox.size + cmi.y];
			}
		}
	}
}

void cubemapSide(VEC3*** cubemap,VEC2 size,VEC3 pos,IVEC3 v_side,float backside){
	pos.a[v_side.x] -= size.x;
	pos.a[v_side.y] -= size.y;
	int max_x = (int)(size.x * CM_SPREAD);
	for(int x = 0;x < size.x * CM_SPREAD;x++){
		for(int y = 0;y < size.y * CM_SPREAD;y++){
			int crd = y * max_x + x;
			VEC3 l_pos = pos;								   
			l_pos.a[v_side.x] += (float)x / CM_SPREAD * 2.0f + 1.0f / CM_SPREAD;
			l_pos.a[v_side.y] += (float)y / CM_SPREAD * 2.0f + 1.0f / CM_SPREAD;

			rdCubemap(cubemap[crd][0],l_pos,0);
			rdCubemap(cubemap[crd][1],l_pos,1);
			rdCubemap(cubemap[crd][2],l_pos,2);
			rdCubemap(cubemap[crd][3],l_pos,3);
			rdCubemap(cubemap[crd][4],l_pos,4);
			rdCubemap(cubemap[crd][5],l_pos,5);
			
			for(int i = 0;i < CM_SIZE;i++){
				int bound_1 = CM_BOUND * (int)CM_SIZE + i;
				int bound_2 = (int)CM_SIZE * i + CM_BOUND;
				
				mixCubemapSide(&cubemap[crd][0][bound_1],&cubemap[crd][5][i]);
				mixCubemapSide(&cubemap[crd][1][bound_1],&cubemap[crd][4][i]);
				mixCubemapSide(&cubemap[crd][4][bound_1],&cubemap[crd][0][i]);
				mixCubemapSide(&cubemap[crd][5][bound_1],&cubemap[crd][1][i]);
				mixCubemapSide(&cubemap[crd][2][bound_1],&cubemap[crd][0][(int)CM_SIZE * (CM_BOUND - i)]);
				mixCubemapSide(&cubemap[crd][3][bound_1],&cubemap[crd][0][bound_2]);
				mixCubemapSide(&cubemap[crd][2][i],&cubemap[crd][1][(int)CM_SIZE * i]);
				mixCubemapSide(&cubemap[crd][3][i],&cubemap[crd][1][(int)CM_SIZE * (CM_BOUND - i) + CM_BOUND]);
				mixCubemapSide(&cubemap[crd][2][bound_2],&cubemap[crd][4][(int)CM_SIZE * i]);
				mixCubemapSide(&cubemap[crd][3][bound_2],&cubemap[crd][5][(int)CM_SIZE * (CM_BOUND - i) + CM_BOUND]);
				mixCubemapSide(&cubemap[crd][2][(int)CM_SIZE * i],&cubemap[crd][5][(int)CM_SIZE * (CM_BOUND - i)]);
				mixCubemapSide(&cubemap[crd][3][(int)CM_SIZE * i],&cubemap[crd][4][(int)CM_SIZE * i + CM_BOUND]);
			}
		}
	}
}