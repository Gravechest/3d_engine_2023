#include "lightmap.h"
#include "source.h"
#include "cubemap.h"
#include "tmath.h"
#include "ray.h"

char hitRayLight(PRIMITIVE* hitted,VEC3* ang,VEC3* color,VEC3 pos){
	VEC3 p,normal;
	int side_id;
	VEC2 size;
	IVEC2 tx_crd;
	VEC3 t_pos;VEC3 n_pos;
	TEXTURE* texture;
	switch(hitted->type){
	case PR_TRIANGLE:
		VEC3mulVEC3(color,hitted->triangle.color);
		*ang = VEC3reflect(*ang,hitted->triangle.normal);
		return FALSE;
	case PR_LIGHTSQUARE:;
		VEC3mulVEC3(color,hitted->square.color);
		return TRUE;
	case PR_SQUARE:;
		SQUARE square = hitted->square;
		VEC2 l_pos = {pos.a[square.v.x] - square.pos.a[square.v.x],pos.a[square.v.y] - square.pos.a[square.v.y]};
		VEC3mul(&l_pos,0.5f);
		VEC2addVEC2(&l_pos,VEC2mulR(square.size,0.5f));
		n_pos = VEC3addVEC3R(pos,square.normal);
		VEC3addVEC3(&n_pos,VEC3normalizeR(VEC3rnd()));
		*ang = VEC3normalizeR(VEC3subVEC3R(n_pos,pos));
		texture = square.texture.data;
		if(texture){
			IVEC2 tx_crd;
			int tx_size = texture->size;
			tx_crd.x = tFract(l_pos.x * square.texture.scale + square.texture.offset.x) * tx_size;
			tx_crd.y = tFract(l_pos.y * square.texture.scale + square.texture.offset.y) * tx_size;
			VEC3 tx = texture->data[0][tx_crd.y * tx_size + tx_crd.x];
			VEC3mulVEC3(color,tx);
		}
		else
			VEC3mulVEC3(color,square.color);
		return FALSE;
	case PR_SPHERE:
		VEC3mulVEC3(color,hitted->sphere.color);
		*ang = VEC3reflect(*ang,VEC3normalizeR(VEC3subVEC3R(pos,hitted->sphere.pos)));
		return FALSE;
	}
	return FALSE;
}
#include <stdio.h>

char traceInsideRay(VEC3 pos,VEC3 ang){
	for(int j = 0;j < 10;j++){
		PRIMITIVE* hitted = 0;
		float dst = 999999.0f;
		for(int i = 0;i < primitivehub.cnt;i++){
			float l_dst = 999999.0f;
			switch(primitivehub.primitive[i].type){
			case PR_SQUARE: case PR_LIGHTSQUARE:;
				SQUARE square = primitivehub.primitive[i].square;
				l_dst = rayIntersectSquare(pos,ang,&square);
				break;
			}
			if(l_dst > 0.0f && l_dst < dst){
				dst = l_dst;
				hitted = &primitivehub.primitive[i];
			}
		}
		if(hitted){
			float pointing = hitted->square.normal.a[hitted->square.v.z];
			if(pointing == ang.a[hitted->square.v.z])
				return TRUE;
		}
	}
	return FALSE;
}

VEC3 traceLightRay(VEC3 l_color,VEC3 pos,VEC3 ang){
	for(int j = 0;j < 10;j++){
		PRIMITIVE* hitted = 0;
		float dst = 999999.0f;
		for(int i = 0;i < primitivehub.cnt;i++){
			float l_dst = 999999.0f;
			switch(primitivehub.primitive[i].type){
			case PR_TRIANGLE:;
				TRIANGLE triangle = primitivehub.primitive[i].triangle;
				l_dst = rayIntersectTriangle(pos,ang,triangle.pos[0],triangle.pos[1],triangle.pos[2]).x;
				break;
			case PR_SQUARE: case PR_LIGHTSQUARE:;
				SQUARE square = primitivehub.primitive[i].square;
				l_dst = rayIntersectSquare(pos,ang,&square);
				break;
			case PR_SPHERE:;
				SPHERE sphere = primitivehub.primitive[i].sphere;
				l_dst = rayIntersectSphere(pos,sphere.pos,ang,sphere.radius);
				break;
			}
			if(l_dst > 0.0f && l_dst < dst){
				dst = l_dst;
				hitted = &primitivehub.primitive[i];
			}
		}
		if(hitted){
			float pointing = hitted->square.normal.a[hitted->square.v.z];
			if(pointing == ang.a[hitted->square.v.z])
				return VEC3_ZERO;
			VEC3addVEC3(&pos,VEC3mulR(ang,dst));
			if(hitRayLight(hitted,&ang,&l_color,pos))
				return l_color;
		}
		else{
			int index;
			VEC2 cm = VEC2mulR(sampleCube(ang,&index),skybox.size);
			IVEC2 cmi = {cm.x,cm.y};
			VEC3mulVEC3(&l_color,skybox.sides[index][cmi.x * skybox.size + cmi.y]);
			return l_color;
		} 
	}
	return VEC3_ZERO;
}

void genSphereLM(SPHERE* sphere,VEC3 pos,float quality){
	for(int i = 0;i < 6;i++){
		IVEC2 c = {0,0};
		for(;c.x < LM_SPHERESIZE;c.x++){
			for(c.y = 0;c.y < LM_SPHERESIZE;c.y++){
				sphere->lightmap[i][c.x * (int)LM_SPHERESIZE + c.y] = VEC3_ZERO;
				int cxx = c.a[cubemaptr[i].sideways];
				int cyy = c.a[!cubemaptr[i].sideways];
				cxx = cubemaptr[i].upside ? LM_SPHERESIZE - cxx : cxx;
				cyy = cubemaptr[i].revert ? LM_SPHERESIZE - cyy : cyy;
				VEC3 normal = getRayAngleCB((IVEC2){LM_SPHERESIZE,LM_SPHERESIZE},sd_dirtri[i],cxx,cyy);
				VEC3 l_pos = VEC3addVEC3R(pos,VEC3mulR(normal,sphere->radius*0.5f));
				for(int k = 0;k < quality;k++){
					VEC3 n_pos = VEC3addVEC3R(l_pos,normal);
					VEC3addVEC3(&n_pos,VEC3normalizeR(VEC3rnd()));
					VEC3 ray_ang = VEC3normalizeR(VEC3subVEC3R(n_pos,l_pos));
					VEC3 l_color = VEC3mulR(sphere->color,1.0f/quality);
					l_color = traceLightRay(l_color,l_pos,ray_ang);
					VEC3addVEC3(&sphere->lightmap[i][c.x * (int)LM_SPHERESIZE + c.y],l_color);
				}
			}
		}
	}
	for(int i = 0;i < LM_SPHERESIZE;i++){
		int bound_1 = LM_SPHEREBOUND * (int)LM_SPHERESIZE + i;
		int bound_2 = (int)LM_SPHERESIZE * i + LM_SPHEREBOUND;
		mixCubemapSide(&sphere->lightmap[0][bound_1],&sphere->lightmap[5][i]);
		mixCubemapSide(&sphere->lightmap[1][bound_1],&sphere->lightmap[4][i]);
		mixCubemapSide(&sphere->lightmap[4][bound_1],&sphere->lightmap[0][i]);
		mixCubemapSide(&sphere->lightmap[5][bound_1],&sphere->lightmap[1][i]);
		mixCubemapSide(&sphere->lightmap[2][bound_1],&sphere->lightmap[0][(int)LM_SPHERESIZE * (LM_SPHEREBOUND - i)]);
		mixCubemapSide(&sphere->lightmap[3][bound_1],&sphere->lightmap[0][bound_2]);
		mixCubemapSide(&sphere->lightmap[2][i],&sphere->lightmap[1][(int)LM_SPHERESIZE * i]);
		mixCubemapSide(&sphere->lightmap[3][i],&sphere->lightmap[1][(int)LM_SPHERESIZE * (LM_SPHEREBOUND - i) + LM_SPHEREBOUND]);
		mixCubemapSide(&sphere->lightmap[2][bound_2],&sphere->lightmap[4][(int)LM_SPHERESIZE * i]);
		mixCubemapSide(&sphere->lightmap[3][bound_2],&sphere->lightmap[5][(int)LM_SPHERESIZE * (LM_SPHEREBOUND - i) + LM_SPHEREBOUND]);
		mixCubemapSide(&sphere->lightmap[2][(int)LM_SPHERESIZE * i],&sphere->lightmap[5][(int)LM_SPHERESIZE * (LM_SPHEREBOUND - i)]);
		mixCubemapSide(&sphere->lightmap[3][(int)LM_SPHERESIZE * i],&sphere->lightmap[4][(int)LM_SPHERESIZE * i + LM_SPHEREBOUND]);
	}
}

void genSquareLM(SQUARE* square,float quality){
	VEC3 pos = square->pos;
	pos.a[square->v.x] -= square->size.x;
	pos.a[square->v.y] -= square->size.y;
	VEC2 size = square->size;
	size.x = tCeilingf(size.x * LM_SIZE);
	size.y = tCeilingf(size.y * LM_SIZE);
	for(int x = 1;x < size.x+1;x++){
		for(int y = 1;y < size.y+1;y++){
			VEC3 m_pos = pos;
			m_pos.a[square->v.x] += (float)(x-1) / LM_SIZE * 2.0f + 0.5f / LM_SIZE;
			m_pos.a[square->v.y] += (float)(y-1) / LM_SIZE * 2.0f + 0.5f / LM_SIZE;
			if(traceInsideRay(m_pos,square->normal)){
				square->lightmap[y*(int)(size.x+2)+x] = VEC3_ZERO;
				continue;
			}
			VEC3 t_color;
			for(int k = 0;k < quality;k++){
				VEC3 n_pos = square->normal;
				VEC3addVEC3(&n_pos,VEC3normalizeR(VEC3rnd()));
				VEC3 ang = VEC3normalizeR(n_pos);
				VEC3 l_pos = pos;
				l_pos.a[square->v.x] += (float)(x-1) / LM_SIZE * 2.0f + tRnd1() / LM_SIZE;
				l_pos.a[square->v.y] += (float)(y-1) / LM_SIZE * 2.0f + tRnd1() / LM_SIZE;
				VEC3 l_color;
				if(!square->texture.data)
					l_color = VEC3mulR(square->color,1.0f);
				else
					l_color = (VEC3){1.0f,1.0f,1.0f};
				l_color = traceLightRay(l_color,l_pos,ang);
				VEC3addVEC3(&t_color,l_color);
			}
			VEC3div(&t_color,quality);
			square->lightmap[y*(int)(size.x+2)+x] = t_color;
		}
	}
	for(int i = 1;i < size.x+1;i++){
		square->lightmap[i] = square->lightmap[i+(int)(square->size.x+2)];
		square->lightmap[i+(int)(size.x+2)*(int)(size.y+1)] = square->lightmap[i+(int)(square->size.x * LM_SIZE+2)*(int)(square->size.y * LM_SIZE)];
	}
	for(int i = 0;i < size.y+2;i++){
		square->lightmap[i*(int)(size.x+2)] = square->lightmap[i*(int)(size.x+2)+1];
		square->lightmap[i*(int)(size.x+2)+(int)(size.x+1)] = square->lightmap[i*(int)(size.x+2)+(int)size.x];
	}
	int lm_size = (int)(size.x+2) * (int)(size.y+2);
	VEC3* t_lm_map = MALLOC(lm_size * sizeof(VEC3));
	for(int x = 0;x < size.x+2;x++){
		for(int y = 0;y < size.y+2;y++){
			if(!square->lightmap[y*(int)(size.x+2)+x].x){
				VEC3 color = VEC3_ZERO;
				int edge_c = 0;
				if(square->lightmap[tMax(y-1,0)*(int)(size.x+2)+x].x){
					VEC3addVEC3(&color,square->lightmap[tMax(y-1,0)*(int)(size.x+2)+x]);
					edge_c++;
				}
				if(square->lightmap[y*(int)(size.x+2)+tMax(x-1,0)].x){
					VEC3addVEC3(&color,square->lightmap[y*(int)(size.x+2)+tMax(x-1,0)]);
					edge_c++;
				}
				if(square->lightmap[tMin(y+1,size.y+2)*(int)(size.x+2)+x].x){
					VEC3addVEC3(&color,square->lightmap[tMin(y+1,size.y+2)*(int)(size.x+2)+x]);
					edge_c++;
				}
				if(square->lightmap[y*(int)(size.x+2)+tMin(x+1,size.x+2)].x){
					VEC3addVEC3(&color,square->lightmap[y*(int)(size.x+2)+tMin(x+1,size.x+2)]);
					edge_c++;
				}
				if(edge_c){
					VEC3div(&color,edge_c);
					VEC3mul(&color,0.9f);
					t_lm_map[y*(int)(size.x+2)+x] = color;
					continue;
				}
			}
			t_lm_map[y*(int)(size.x+2)+x] = square->lightmap[y*(int)(size.x+2)+x];
		}
	}
	memcpy(square->lightmap,t_lm_map,lm_size * sizeof(VEC3));
	MFREE(t_lm_map);
}

void genTriangleLM(TRIANGLE* triangle,float quality){
	VEC2 size = {VEC3distance(triangle->pos[0],triangle->pos[1]),VEC3distance(triangle->pos[0],triangle->pos[2])};
	size.x = tCeilingf(size.x * LM_SIZE);
	size.y = tCeilingf(size.y * LM_SIZE);
	VEC3 dx = VEC3subVEC3R(triangle->pos[1],triangle->pos[0]);
	VEC3 dy = VEC3subVEC3R(triangle->pos[2],triangle->pos[0]);
	VEC3div(&dx,size.x);
	VEC3div(&dy,size.y);
	VEC3 pos = triangle->pos[0];
	for(int x = 1;x < size.x+1;x++){
		for(int y = 1;y < size.y+1;y++){
			VEC3 t_color;
			for(int k = 0;k < quality;k++){
				VEC3 n_pos = triangle->normal;
				VEC3addVEC3(&n_pos,VEC3normalizeR(VEC3rnd()));
				VEC3 ang = VEC3normalizeR(n_pos);
				VEC3 l_pos = pos;
				VEC3 l_color;	
				if(!triangle->texture.data)
					l_color = VEC3mulR(triangle->color,1.0f);
				else
					l_color = VEC3_SINGLE(1.0f);
				l_color = traceLightRay(l_color,l_pos,ang);
				VEC3addVEC3(&t_color,l_color);
			}
			VEC3div(&t_color,quality);
			triangle->lightmap[y*(int)(size.x+2)+x] = t_color;
			VEC3addVEC3(&pos,dy);
		}
		VEC3subVEC3(&pos,VEC3mulR(dy,size.y));
		VEC3addVEC3(&pos,dx);
	}
}

void genLight(){
	for(int i = 0;i < primitivehub.cnt;i++){
		switch(primitivehub.primitive[i].type){
		case PR_TRIANGLE:{
			TRIANGLE* triangle = &primitivehub.primitive[i].triangle;
			if(triangle->lightmap)
				MFREE(triangle->lightmap);
			VEC2 size = triangle->size;
			triangle->lightmap = MALLOC_ZERO((int)(size.x+2) * (int)(size.y+2) * sizeof(VEC3));
			break;}
		case PR_SQUARE:;
			SQUARE* square = &primitivehub.primitive[i].square;
			if(square->lightmap)
				MFREE(square->lightmap);
			VEC2 size = square->size;
			size.x = tCeilingf(size.x * LM_SIZE);
			size.y = tCeilingf(size.y * LM_SIZE);
			square->lightmap = MALLOC_ZERO((int)(size.x+2) * (int)(size.y+2) * sizeof(VEC3));
		}
	}
	camera.rd_mode = TRUE;
	for(float quality = 1.0f;quality < LM_QUALITY;quality *= 4.0f){
		for(int i = 0;i < primitivehub.cnt;i++){
			switch(primitivehub.primitive[i].type){
			case PR_TRIANGLE: genTriangleLM(&primitivehub.primitive[i].triangle,quality);
			case PR_SQUARE: genSquareLM(&primitivehub.primitive[i].square,quality); break;
			case PR_SPHERE:;
				SPHERE* sphere = &primitivehub.primitive[i].sphere;
				genSphereLM(sphere,sphere->pos,quality);
				break;
			}
		}
		for(int i = 0;i < primitivehub.cnt;i++){
			switch(primitivehub.primitive[i].type){
			case PR_SQUARE:;
				SQUARE* square = &primitivehub.primitive[i].square;
				if(square->cubemap)
					cubemapSide(square->cubemap,square->size,square->pos,square->v,square->normal.a[square->v.z]);
				break;
			case PR_SPHERE:;
				SPHERE* sphere = &primitivehub.primitive[i].sphere;
				if(sphere->cubemap){
					rdCubemap(sphere->cubemap[0],sphere->pos,0);
					rdCubemap(sphere->cubemap[1],sphere->pos,1);
					rdCubemap(sphere->cubemap[2],sphere->pos,2);
					rdCubemap(sphere->cubemap[3],sphere->pos,3);
					rdCubemap(sphere->cubemap[4],sphere->pos,4);
					rdCubemap(sphere->cubemap[5],sphere->pos,5);
				}
				break;
			}
		}
	}
}