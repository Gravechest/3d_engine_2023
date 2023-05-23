#include <math.h>
#include <stdio.h>
#include <intrin.h>

#include "tmath.h"

#include "source.h"
#include "phyiscs.h"
#include "ray.h"
#include "addtoscene.h"
#include "cubemap.h"
#include "lightmap.h"
#include "draw.h"
#include "draw_flat.h"

WNDCLASSA wndclass = {.lpfnWndProc = proc,.lpszClassName = "class",.lpszMenuName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,24,BI_RGB};
HWND window;
HDC context;
MSG Msg;
CAMERA camera =  {
	.infocus = TRUE,
	.pos = {1.0f,1.0f,3.0f},
	.dir_tri = {-1.0f,0.0f,-1.0f,0.0f},
	.exposure = 1.0f
};
PRIMITIVEHUB primitivehub;
BUTTONHUB buttonhub;
TEXTUREHUB texturehub;
BOXHUB boxhub;
TEXTURE font;
KEYS key;

VRAM vram;

char frame_sync;

NORMALMAP normalmap[10];
REFLECTMAP reflectmap[10];

SKYBOX skybox;

HANDLE gen_light_thread;

float grid_size = 1.0f;
int pr_selected;

int copy_mode;

int triangle_point_selected;

VEC3 triangleNormal(VEC3 p1,VEC3 p2,VEC3 p3){
	VEC3 a = VEC3subVEC3R(p2,p1);
	VEC3 b = VEC3subVEC3R(p3,p1);
	return VEC3normalizeR(VEC3cross(a,b));
}

char* loadFile(char* name){
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	int fsize = GetFileSize(h,0);
	char *file = HeapAlloc(GetProcessHeap(),8,fsize+1);
	ReadFile(h,file,fsize+1,0,0);
	CloseHandle(h);
	return file;
}

TEXTUREPROP loadBMP(char* name){
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	int fsize = GetFileSize(h,0);
	unsigned char* text = MALLOC(fsize+1);
	int offset;
	SetFilePointer(h,0x0a,0,0);
	ReadFile(h,&offset,4,0,0);
	SetFilePointer(h,offset,0,0);
	ReadFile(h,text,fsize-offset,0,0);
	CloseHandle(h);
	VEC3* text_v = MALLOC((fsize-offset)*sizeof(VEC3));
	for(int i = 0;i < fsize-offset;i+=3){
		text_v[i/3].r = (float)text[i + 0] / 255.0f;
		text_v[i/3].g = (float)text[i + 1] / 255.0f;
		text_v[i/3].b = (float)text[i + 2] / 255.0f;
	}
	MFREE(text);
	return (TEXTUREPROP){.data = text_v,.size = sqrtf((fsize - offset) / 3)};
}

char pointInBox(VEC3 point,VEC3 box_pos,VEC3 box_size){
	char x_condition = point.x > box_pos.x - box_size.x && point.x < box_pos.x + box_size.x;
	char y_condition = point.y > box_pos.y - box_size.y && point.y < box_pos.y + box_size.y;
	char z_condition = point.z > box_pos.z - box_size.z && point.z < box_pos.z + box_size.z;
	if(x_condition && y_condition && z_condition)
		return TRUE;
	return FALSE;
}

char pointInRect(VEC2 point,VEC2 box_pos,VEC2 box_size){
	char x_condition = point.x > box_pos.x - box_size.x && point.x < box_pos.x + box_size.x;
	char y_condition = point.y > box_pos.y - box_size.y && point.y < box_pos.y + box_size.y;
	if(x_condition && y_condition)
		return TRUE;
	return FALSE;
}

void boxResize(IVEC3 order,float backwards){
	SQUARE* square_x1 = &boxhub.selected->square[order.x * 2 + 0]->square;
	SQUARE* square_x2 = &boxhub.selected->square[order.x * 2 + 1]->square;
	SQUARE* square_y1 = &boxhub.selected->square[order.y * 2 + 0]->square;
	SQUARE* square_y2 = &boxhub.selected->square[order.y * 2 + 1]->square;
	SQUARE* square_z1 = &boxhub.selected->square[order.z * 2 + 0]->square;
	SQUARE* square_z2 = &boxhub.selected->square[order.z * 2 + 1]->square;
	switch(order.x){
	case VEC3_X:
		if(square_y1->size.x + grid_size * backwards <= 0.0f)
			return;
		square_y1->size.x += grid_size * backwards;
		square_y2->size.x += grid_size * backwards;
		square_z1->size.x += grid_size * backwards;
		square_z2->size.x += grid_size * backwards;
		break;
	case VEC3_Y:
		if(square_y1->size.x + grid_size * backwards <= 0.0f)
			return;
		square_y1->size.x += grid_size * backwards;
		square_y2->size.x += grid_size * backwards;
		square_z1->size.y += grid_size * backwards;
		square_z2->size.y += grid_size * backwards;
		break;
	case VEC3_Z:
		if(square_y1->size.y + grid_size * backwards <= 0.0f)
			return;
		square_y1->size.y += grid_size * backwards;
		square_y2->size.y += grid_size * backwards;
		square_z1->size.y += grid_size * backwards;
		square_z2->size.y += grid_size * backwards;
		break;
	}
	boxhub.selected->hitbox->size.a[order.x] += grid_size * backwards;
	boxhub.selected->hitbox->pos.a[order.x] += grid_size * backwards;
	square_x2->pos.a[order.x] += grid_size * backwards * 2.0f;
	square_y1->pos.a[order.x] += grid_size * backwards;
	square_y2->pos.a[order.x] += grid_size * backwards;
	square_z1->pos.a[order.x] += grid_size * backwards;
	square_z2->pos.a[order.x] += grid_size * backwards;
}

void prChangeColor(int color,float add){
	if(primitivehub.selected){
		SQUARE* square = &primitivehub.selected->square;
		switch(primitivehub.selected->type){
		case PR_SQUARE: square->color.a[color] += add; break;
		case PR_LIGHTSQUARE: square->color.a[color] += add * 10.0f; break;
		}
		square->color.a[color] = tMaxf(square->color.a[color],0.0f);
	}
}

VEC3 getLookAngle(){
	VEC3 ang;
	ang.x = cosf(camera.dir.x) * cosf(camera.dir.y);
	ang.y = sinf(camera.dir.x) * cosf(camera.dir.y);
	ang.z = sinf(camera.dir.y);
	VEC3normalize(&ang);
	return ang;
}

void movePrimitive(int axis,float ammount){
	switch(primitivehub.selected->type){
	case PR_SQUARE:
		for(int i = 0;i < 6;i++){
			SQUARE* square = &boxhub.selected->square[i]->square;
			square->pos.a[axis] += ammount;
		}
		boxhub.selected->hitbox->pos.a[axis] += ammount;
		break;
	case PR_TRIANGLE:;
		TRIANGLE* triangle = &primitivehub.selected->triangle;
		triangle->pos[triangle_point_selected].a[axis] += ammount;
		triangle->normal = triangleNormal(triangle->pos[0],triangle->pos[1],triangle->pos[2]);
		VEC2 size = {VEC3distance(triangle->pos[0],triangle->pos[1])*LM_SIZE,VEC3distance(triangle->pos[0],triangle->pos[2])*LM_SIZE};
		size.x = tCeilingf(size.x);
		size.y = tCeilingf(size.y);
		triangle->size = size;
		break;
	}
}

char executeButton(VEC2 cur_pos){
	for(int i = 0;i < buttonhub.cnt;i++){
		BUTTON* button = &buttonhub.button[i];
		if(pointInRect(cur_pos,VEC2addVEC2R(button->pos,VEC2mulR(button->size,0.5f)),VEC2mulR(button->size,0.5f))){
			SQUARE* square;
			if(button->type == BTNTYPE_TOGGLE)
				button->toggled ^= TRUE;
			switch(button->action){
			case BTN_PLACEPRIMITIVE:
				if(camera.rd_mode)
					break;
				VEC3 ang = getLookAngle();
				VEC3 pos = VEC3addVEC3R(camera.pos,VEC3mulR(ang,2.5f));
				pos.x = tFloorf(pos.x); pos.y = tFloorf(pos.y); pos.z = tFloorf(pos.z);
				VEC3subVEC3(&pos,VEC3_SINGLE(0.5f));
				if(copy_mode && (pr_selected == 0 || pr_selected == 1)){
					BOX box = *boxhub.selected;
					VEC3 size,pos;
					size.y = box.square[0]->square.size.x * 2.0f;
					size.z = box.square[0]->square.size.y * 2.0f;
					size.x = box.square[2]->square.size.x * 2.0f;
					pos.x = box.square[2]->square.pos.x;
					pos.y = box.square[0]->square.pos.y;
					pos.z = box.square[0]->square.pos.z;
					VEC3subVEC3(&pos,VEC3mulR(size,0.5f));
					addBox(pos,size,VEC3_SINGLE(0.9f),PR_SQUARE);
					break;
				}
				switch(pr_selected){
				case 0: addBox(pos,(VEC3){1.0f,1.0f,1.0f},VEC3_SINGLE(0.9),PR_SQUARE); break;
				case 1: addBox(pos,(VEC3){1.0f,1.0f,1.0f},VEC3_SINGLE(0.9),PR_LIGHTSQUARE); break;
				case 2: addSphere(pos,VEC3_SINGLE(0.9f),1.0f,PR_SPHERE); break;
				case 3: addTriangle(pos,VEC3addVEC3R(pos,(VEC3){1.0f,0.0f,0.0f}),VEC3addVEC3R(pos,(VEC3){0.0f,1.0f,0.0f}),VEC3_SINGLE(0.9f));
				}
				break;
			case BTN_CYCLEPRIMITIVE_R:
				if(!primitivehub.selected)
					break;
				for(int i = 0;i < primitivehub.cnt;i++){
					if(&primitivehub.primitive[i] == primitivehub.selected){
						if(++i == primitivehub.cnt)
							i = 0;
						primitivehub.selected = &primitivehub.primitive[i];
					}
				}
				break;
			case BTN_CYCLEPRIMITIVE_L:
				if(!primitivehub.selected)
					break;
				for(int i = 0;i < primitivehub.cnt;i++){
					if(&primitivehub.primitive[i] == primitivehub.selected){
						if(--i == -1)
							i = primitivehub.cnt - 1;
						primitivehub.selected = &primitivehub.primitive[i];
					}
				}
				break;
			case BTN_CYCLETEXTURE_R:{
				TXMAP* texture;
				switch(primitivehub.selected->type){
				case PR_SQUARE: texture = &primitivehub.selected->square.texture; break;
				case PR_SPHERE: texture = &primitivehub.selected->sphere.texture; break;
				case PR_TRIANGLE:texture =&primitivehub.selected->triangle.texture; break; 
				}
				if(!texture->data)
					addTexture(1.0f,(VEC2){0.0f,0.0f},&texturehub.texture[texturehub.cnt-1],texture);
				else{
					for(int i = 0;i < texturehub.cnt;i++){
						if(&texturehub.texture[i] == texture->data){
							texture->data = --i == -1 ? 0 : &texturehub.texture[i];
							break;
						}
					}
				}
				break;}
			case BTN_CYCLETEXTURE_L:{
				TXMAP* texture;
				switch(primitivehub.selected->type){
				case PR_SQUARE: texture = &primitivehub.selected->square.texture; break;
				case PR_SPHERE: texture = &primitivehub.selected->sphere.texture; break;
				case PR_TRIANGLE:texture =&primitivehub.selected->triangle.texture; break; 
				}
				if(!texture->data)
					addTexture(1.0f,(VEC2){0.0f,0.0f},&texturehub.texture[0],texture);
				else{
					for(int i = 0;i < texturehub.cnt;i++){
						if(&texturehub.texture[i] == texture->data){
							texture->data = ++i == texturehub.cnt ? 0 : &texturehub.texture[i];
							break;
						}
					}
				}
				break;}
			case BTN_TOGGLELIGHT:
				camera.rd_mode ^= TRUE;
				if(camera.rd_mode){
					gen_light_thread = CreateThread(0,0,genLight,0,0,0);
					camera.exposure = 1.0f;
				}
				else
					TerminateThread(gen_light_thread,0);
				Sleep(15);
				break;
			case BTN_SUBPOS_X: movePrimitive(VEC3_X,-grid_size); break;
			case BTN_ADDPOS_X: movePrimitive(VEC3_X, grid_size); break;
			case BTN_SUBPOS_Y: movePrimitive(VEC3_Y,-grid_size); break;
			case BTN_ADDPOS_Y: movePrimitive(VEC3_Y, grid_size); break;
			case BTN_SUBPOS_Z: movePrimitive(VEC3_Z,-grid_size); break;
			case BTN_ADDPOS_Z: movePrimitive(VEC3_Z, grid_size); break;
			case BTN_SUBSIZE_X: boxResize((IVEC3){VEC3_X,VEC3_Y,VEC3_Z},-0.5f); break;
			case BTN_ADDSIZE_X: boxResize((IVEC3){VEC3_X,VEC3_Y,VEC3_Z},0.5f); break;
			case BTN_SUBSIZE_Y: boxResize((IVEC3){VEC3_Y,VEC3_X,VEC3_Z},-0.5f); break;
			case BTN_ADDSIZE_Y: boxResize((IVEC3){VEC3_Y,VEC3_X,VEC3_Z},0.5f); break;
			case BTN_SUBSIZE_Z: boxResize((IVEC3){VEC3_Z,VEC3_X,VEC3_Y},-0.5f); break;
			case BTN_ADDSIZE_Z: boxResize((IVEC3){VEC3_Z,VEC3_X,VEC3_Y},0.5f); break;
			case BTN_REDUCEGRID: grid_size *= 0.5f; break;
			case BTN_INCREASEGRID: grid_size *= 2.0f; break;
			case BTN_CYCLEOBJECT_R: if(--pr_selected == -1) pr_selected = PR_TOTAL - 1; break;
			case BTN_CYCLEOBJECT_L: if(++pr_selected == PR_TOTAL) pr_selected = 0; break;
			case BTN_SUBCOLOR_R: prChangeColor(VEC3_X,-0.1f); break;
			case BTN_ADDCOLOR_R: prChangeColor(VEC3_X, 0.1f); break;
			case BTN_SUBCOLOR_G: prChangeColor(VEC3_Y,-0.1f); break;
			case BTN_ADDCOLOR_G: prChangeColor(VEC3_Y, 0.1f); break;
			case BTN_SUBCOLOR_B: prChangeColor(VEC3_Z,-0.1f); break;
			case BTN_ADDCOLOR_B: prChangeColor(VEC3_Z, 0.1f); break;
			case 28: 
				if(!boxhub.selected)
					break;
				for(int i = 0;i < boxhub.cnt;i++){
					if(&boxhub.box[i] == boxhub.selected){
						if(--i == -1) i = boxhub.cnt - 1;
						boxhub.selected = &boxhub.box[i];
					}
				}
				break;
			case 29: 
				if(!boxhub.selected)
					break;
				for(int i = 0;i < boxhub.cnt;i++){
					if(&boxhub.box[i] == boxhub.selected){
						if(++i == boxhub.cnt) i = 0;
						boxhub.selected = &boxhub.box[i];
					}
				}
				break;
			case 30: camera.mv_mode ^= TRUE; break;
			case 31: 
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.offset.x += 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.offset.x += 0.25f; break;
				}
				break;
			case 32: 
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.offset.x -= 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.offset.x -= 0.25f; break;
				}
				break;
			case 33: 
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.offset.y += 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.offset.y += 0.25f; break;
				}
				break;
			case 34: 
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.offset.y -= 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.offset.y -= 0.25f; break;
				}
				break;
			case 35:
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.scale += 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.scale += 0.25f; break;
				}
				break;
			case 36:
				switch(primitivehub.selected->type){
				case PR_SQUARE: primitivehub.selected->square.texture.scale -= 0.25f; break;
				case PR_SPHERE: primitivehub.selected->sphere.texture.scale -= 0.25f; break;
				}
				break;
			case 37: copy_mode ^= TRUE; break;
			case 38:{
				CMMAP* texture;
				if(primitivehub.selected->type == PR_TRIANGLE){
					triangle_point_selected = ++triangle_point_selected == 3 ? 0 : triangle_point_selected;
					break;
				}
				switch(primitivehub.selected->type){
				case PR_SQUARE: texture = &primitivehub.selected->square.cm_properties; break;
				}
				if(!texture->normalmap){
					SQUARE* square = &primitivehub.selected->square;
					addReflection(&square->cubemap,&square->cm_properties,square->size,0,&normalmap[0],0.5f,0.5f);
				}
				else{
					SQUARE* square = &primitivehub.selected->square;
					for(int i = 0;i < RF_TOTAL;i++){
						if(&normalmap[i] == square->cm_properties.normalmap){
							square->cm_properties.normalmap = ++i == RF_TOTAL ? 0 : &normalmap[i];
							break;
						}
					}
				}
				break;}
			case 39:{
				if(primitivehub.selected->type == PR_TRIANGLE)
					triangle_point_selected = --triangle_point_selected == -1 ? 2 : triangle_point_selected;
				break;}
			}
			return TRUE;
		}
	}
	return FALSE;
}

PRIMITIVE* rayIntersectPrimitive(VEC3 pos,VEC3 ang){
	PRIMITIVE* hitted = 0;
	float dst_f = 999999.0f;
	for(int i = 0;i < primitivehub.cnt;i++){
		switch(primitivehub.primitive[i].type){
		case PR_SQUARE: case PR_LIGHTSQUARE:
			float dst = rayIntersectSquare(camera.pos,ang,&primitivehub.primitive[i].square);
			if(dst > 0.0f && dst < dst_f){
				hitted = &primitivehub.primitive[i];
				dst_f = dst;
			}
			break;
		}
	}
	return hitted;
}

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_MBUTTONDOWN:
		primitivehub.selected = rayIntersectPrimitive(camera.pos,getLookAngle());
		for(int i = 0;i < boxhub.cnt;i++){
			for(int j = 0;j < 6;j++){
				if(primitivehub.selected == boxhub.box[i].square[j]){
					boxhub.selected = &boxhub.box[i];
					goto end;
				}
			}
		}
	end:
		break;
	case WM_DESTROY: case WM_CLOSE:
		CreateDirectoryA("lvl",0);
		HANDLE temp = CreateFileA("lvl/temp.lvl",GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
		WriteFile(temp,&hitboxhub.cnt,sizeof(int),0,0);
		WriteFile(temp,hitboxhub.hitbox,sizeof(HITBOX) * hitboxhub.cnt,0,0);
		WriteFile(temp,&primitivehub.cnt,sizeof(int),0,0);
		for(int i = 0;i < primitivehub.cnt;i++){
			PRIMITIVE pr = primitivehub.primitive[i];
			if(pr.square.cm_properties.normalmap){
				for(int j = 0;j < RF_TOTAL;j++){
					if(pr.square.cm_properties.normalmap == &normalmap[j]){
						pr.square.cm_properties.normalmap = j;
						break;
					}
				}
			}
			else
				pr.square.cm_properties.normalmap = -1;
			pr.square.cubemap = 0;
			pr.square.cm_properties.reflectmap = -1;
			if(pr.square.texture.data){
				for(int j = 0;j < texturehub.cnt;j++){
					if(pr.square.texture.data == &texturehub.texture[j]){
						pr.square.texture.data = j;
						break;
					}
				}
			}
			else
				pr.square.texture.data = -1;
			WriteFile(temp,&pr,sizeof(PRIMITIVE),0,0);
		}
		WriteFile(temp,&boxhub.cnt,sizeof(int),0,0);
		for(int i = 0;i < boxhub.cnt;i++){
			BOX box = boxhub.box[i];
			for(int j = 0;j < hitboxhub.cnt;j++){
				if(&hitboxhub.hitbox[j] == box.hitbox){
					box.hitbox = j;
					break;
				}
			}
			for(int j = 0;j < primitivehub.cnt;j++){
				if(&primitivehub.primitive[j] == box.square[0]){
					for(int k = 0;k < 6;k++)
						box.square[k] = j + k;
					break;
				}
			}
			WriteFile(temp,&box,sizeof(BOX),0,0);
		}
		WriteFile(temp,boxhub.box,boxhub.cnt * sizeof(BOX),0,0);
		for(int i = 0;i < primitivehub.cnt;i++){
			PRIMITIVE pr = primitivehub.primitive[i];
			switch(pr.type){
			case PR_SQUARE:
				WriteFile(temp,pr.square.lightmap,(int)(pr.square.size.x * LM_SIZE+2) * (int)(pr.square.size.y * LM_SIZE+2) * sizeof(VEC3),0,0);
				break;
			case PR_SPHERE:
				for(int i = 0;i < 6;i++)
					WriteFile(temp,pr.sphere.lightmap[i],LM_SPHERESIZE * LM_SPHERESIZE * sizeof(VEC3),0,0);
				break;
			}
		}
		CloseHandle(temp);
		ExitProcess(0);
	case WM_LBUTTONDOWN:
		if(camera.edit_mode){
			if(!camera.infocus){
				POINT cur;
				GetCursorPos(&cur);
				VEC2 cur_pos = {(float)cur.y / WND_SIZE.y,(float)cur.x / WND_SIZE.x};
				cur_pos.x = 1.0f - cur_pos.x;
				if(executeButton(cur_pos))
					break;
				if(cur_pos.y < 0.65f){
					if(!camera.infocus)
						ShowCursor(camera.infocus);
					camera.infocus = TRUE;
				}
			}
			else{
				if(camera.infocus)
					ShowCursor(camera.infocus);
				camera.infocus = FALSE;
			}
		}
		break;
	case WM_KEYDOWN:
		switch(wParam){
		case VK_F8:
			TerminateThread(gen_light_thread,0);
			break;
		case VK_F9:
			if(camera.infocus != camera.edit_mode)
				ShowCursor(camera.infocus);
			camera.infocus = camera.edit_mode;
			camera.edit_mode ^= TRUE;
			POINT cur;
			RECT offset;
			GetCursorPos(&cur);
			GetWindowRect(window,&offset);
			SetCursorPos(offset.left+WND_SIZE.x/2,offset.top+WND_SIZE.y/2);
		}
		break;
	case WM_MOUSEMOVE:;
		if(camera.infocus){
			POINT cur;
			RECT offset;
			VEC2 r;
			GetCursorPos(&cur);
			GetWindowRect(window,&offset);
			float mx = (float)(cur.x-(offset.left+WND_SIZE.x/2))*0.005f;
			float my = (float)(cur.y-(offset.top+WND_SIZE.y/2))*0.005f;
			camera.dir.x += mx;
			camera.dir.y -= my;
			SetCursorPos(offset.left+WND_SIZE.x/2,offset.top+WND_SIZE.y/2);
		}
		break;
	}
	return DefWindowProcA(hwnd,msg,wParam,lParam);
}

void render(){
	for(;;){
		StretchDIBits(context,0,0,WND_SIZE.x,WND_SIZE.y,0,0,WND_RESOLUTION.y,WND_RESOLUTION.x,vram.draw,&bmi,DIB_RGB_COLORS,SRCCOPY);
		Sleep(3);
	}
}

IVEC2 project(VEC3 pos,VEC3 cam_pos,VEC4 dir_tri){
	float temp;
	IVEC2 r;
	VEC3subVEC3(&pos,cam_pos);
	temp  = pos.y * dir_tri.x - pos.x * dir_tri.y;
	pos.x = pos.y * dir_tri.y + pos.x * dir_tri.x;
	pos.y = temp;
	temp  = pos.z * dir_tri.z - pos.x * dir_tri.w;
	pos.x = pos.z * dir_tri.w + pos.x * dir_tri.z;
	pos.z = temp;
	if(pos.x <= 0.0f) 
		return (IVEC2){-1,-1};
	r.x = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	r.y = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
	return r;
}

char boxInBox(VEC3 b1_pos,VEC3 b1_size,VEC3 b2_pos,VEC3 b2_size){
	VEC3 added = VEC3addVEC3R(b1_size,b2_size);
	char c1_1 = b1_pos.x >= b2_pos.x - added.x && b1_pos.x <= b2_pos.x + added.x;
	char c1_2 = b1_pos.y >= b2_pos.y - added.y && b1_pos.y <= b2_pos.y + added.y;
	char c1_3 = b1_pos.z >= b2_pos.z - added.z && b1_pos.z <= b2_pos.z + added.z;
	if(c1_1 && c1_2 && c1_3)
		return TRUE;
	return FALSE;
}

VEC3 getRayAngle(IVEC2 rd_size,VEC4 dir_tri,int x,int y){
	VEC3 ray_ang;
	float pxY = (((float)(x) * 2.0f / rd_size.x) - 1.0f);
	float pxX = (((float)(y) * 2.0f / rd_size.y) - 1.0f);
	float pixelOffsetY = pxY / (FOV.x / WND_RESOLUTION.x * 2.0f);
	float pixelOffsetX = pxX / (FOV.y / WND_RESOLUTION.y * 2.0f);
	ray_ang.x = (dir_tri.x * dir_tri.z - dir_tri.x * dir_tri.w * pixelOffsetY) - dir_tri.y * pixelOffsetX;
	ray_ang.y = (dir_tri.y * dir_tri.z - dir_tri.y * dir_tri.w * pixelOffsetY) + dir_tri.x * pixelOffsetX;
	ray_ang.z = dir_tri.w + dir_tri.z * pixelOffsetY;
	VEC3normalize(&ray_ang);
	return ray_ang;
}

void testPoint(IVEC2 rd_size,IVEC2* min,IVEC2* max,int* behind_vw,VEC3 pos,VEC3 cam_pos,VEC4 dir_tri){
	IVEC2 point = project(pos,cam_pos,dir_tri);
	if(point.x == -1){
		*behind_vw = TRUE;
		return;
	}
	if(min->x == -1){
		point.x = tMin(tMax(point.x,0),rd_size.x);
		point.y = tMin(tMax(point.y,0),rd_size.y);
		*min = point;
		*max = point;
		return;
	}
	min->x = tMax(tMin(min->x,point.x),0);
	min->y = tMax(tMin(min->y,point.y),0);
	max->x = tMin(tMax(max->x,point.x+1),rd_size.x);
	max->y = tMin(tMax(max->y,point.y+1),rd_size.y);
}

REGION boundRegion(IVEC2 rd_size,VEC3 pos,VEC3 size,VEC3 cam_pos,VEC4 dir_tri,int gen){
	if(gen == 15)
		return (REGION){{0,0},{rd_size.x,rd_size.y}};

	IVEC2 min = {-1,-1},max = {-1,-1};
	int behind_vw = FALSE;

	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){ size.x, size.y, size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){-size.x, size.y, size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){ size.x,-size.y, size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){-size.x,-size.y, size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){ size.x, size.y,-size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){-size.x, size.y,-size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){ size.x,-size.y,-size.z}),cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,VEC3addVEC3R(pos,(VEC3){-size.x,-size.y,-size.z}),cam_pos,dir_tri);

	if(behind_vw){
		if(min.x == -1)
			return (REGION){{-1,-1},{-1,-1}};

		size.a[gen % 3] *= 0.5f;
		pos.a[gen % 3]  -= size.a[gen % 3];
		REGION region1 = boundRegion(rd_size,pos,size,cam_pos,dir_tri,gen + 1);

		pos.a[gen % 3]  += size.a[gen % 3] * 2.0f;
		REGION region2 = boundRegion(rd_size,pos,size,cam_pos,dir_tri,gen + 1);

		if(region1.min.x != -1){
			min.x = tMin(min.x,region1.min.x);
			min.y = tMin(min.y,region1.min.y);
			max.x = tMax(max.x,region1.max.x);
			max.y = tMax(max.y,region1.max.y);
		}
		if(region2.min.x != -1){
			min.x = tMin(min.x,region2.min.x);
			min.y = tMin(min.y,region2.min.y);
			max.x = tMax(max.x,region2.max.x);
			max.y = tMax(max.y,region2.max.y);
		}
	}
	return (REGION){min,{max.x,max.y}};
}

REGION boundRegionSquare(IVEC2 rd_size,VEC3 pos,IVEC3 side,VEC2 size,VEC3 cam_pos,VEC4 dir_tri,int gen){
	if(gen == 15)
		return (REGION){{-1,-1},{-1,-1}};

	IVEC2 min = {-1,-1},max = {-1,-1};
	int behind_vw = FALSE;

	VEC3 p_1 = pos; p_1.a[side.x] += size.x; p_1.a[side.y] += size.y;
	VEC3 p_2 = pos; p_2.a[side.x] -= size.x; p_2.a[side.y] += size.y;
	VEC3 p_3 = pos; p_3.a[side.x] += size.x; p_3.a[side.y] -= size.y;
	VEC3 p_4 = pos; p_4.a[side.x] -= size.x; p_4.a[side.y] -= size.y;

	testPoint(rd_size,&min,&max,&behind_vw,p_1,cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,p_2,cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,p_3,cam_pos,dir_tri);
	testPoint(rd_size,&min,&max,&behind_vw,p_4,cam_pos,dir_tri);

	if(behind_vw){
		if(min.x == -1)
			return (REGION){{-1,-1},{-1,-1}};

		size.a[gen & 1] *= 0.5f;
		pos.a[side.a[gen & 1]]  -= size.a[gen & 1];
		REGION region1 = boundRegionSquare(rd_size,pos,side,size,cam_pos,dir_tri,gen + 1);

		pos.a[side.a[gen & 1]] += size.a[gen & 1] * 2.0f;
		REGION region2 = boundRegionSquare(rd_size,pos,side,size,cam_pos,dir_tri,gen + 1);

		if(region1.min.x != -1){
			min.x = tMin(min.x,region1.min.x);
			min.y = tMin(min.y,region1.min.y);
			max.x = tMax(max.x,region1.max.x);
			max.y = tMax(max.y,region1.max.y);
		}
		if(region2.min.x != -1){
			min.x = tMin(min.x,region2.min.x);
			min.y = tMin(min.y,region2.min.y);
			max.x = tMax(max.x,region2.max.x);
			max.y = tMax(max.y,region2.max.y);
		}
	}
	return (REGION){min,{max.x,max.y}};
}

REGION boundRegionTriangle(VEC3 p1,VEC3 p2,VEC3 p3,VEC3 cam_pos,VEC4 dir_tri,int gen){
	if(gen == 15)
		return (REGION){{-1,-1},{-1,-1}};

	IVEC2 min = {-1,-1},max = {-1,-1};
	int behind_vw = FALSE;

	testPoint(WND_RESOLUTION,&min,&max,&behind_vw,p1,cam_pos,dir_tri);
	testPoint(WND_RESOLUTION,&min,&max,&behind_vw,p2,cam_pos,dir_tri);
	testPoint(WND_RESOLUTION,&min,&max,&behind_vw,p3,cam_pos,dir_tri);

	if(behind_vw){
		if(min.x == -1)
			return (REGION){{-1,-1},{-1,-1}};
	}
	return (REGION){min,{max.x,max.y}};
}

void mapSphereRegion(PRIMITIVE* primitive,int b_x,CAMERA camera_rd){
	SPHERE sphere = primitive->sphere;
	VEC3 box_size = {sphere.radius,sphere.radius,sphere.radius};
	REGION region = boundRegion(WND_RESOLUTION,sphere.pos,box_size,camera_rd.pos,camera_rd.dir_tri,0);
	region.min.x += (region.min.x & 1) ? !(b_x & 1) : (b_x & 1);
	region.min.y += (region.min.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	for(int x = region.min.x;x < region.max.x;x+=2){ 
		for(int y = region.min.y;y < region.max.y;y+=2){
			VEC3 ray_ang = vram.ray_ang[x * WND_RESOLUTION.y + y];
			float dst = rayIntersectSphere(camera_rd.pos,sphere.pos,ray_ang,sphere.radius);
			if(dst > 0 && vram.zbuffer[x * WND_RESOLUTION.y + y].dst > dst){
				vram.zbuffer[x * WND_RESOLUTION.y + y].dst = dst;
				vram.zbuffer[x * WND_RESOLUTION.y + y].primitive = primitive;
			}
		}
	}
}

void mapTriangleRegion(PRIMITIVE* primitive,int b_x,CAMERA camera_rd){
	TRIANGLE triangle = primitive->triangle;
	if(VEC3dotR(triangle.normal,VEC3subVEC3R(camera_rd.pos,triangle.pos[0])) < 0.0f)
		return;
	REGION region = boundRegionTriangle(triangle.pos[0],triangle.pos[1],triangle.pos[2],camera_rd.pos,camera_rd.dir_tri,0);;
	region.min.x += (region.min.x & 1) ? !(b_x & 1) : (b_x & 1);
	region.min.y += (region.min.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	for(int x = region.min.x;x < region.max.x;x+=2){ 
		for(int y = region.min.y;y < region.max.y;y+=2){
			VEC3 ray_ang = vram.ray_ang[x * WND_RESOLUTION.y + y];
			VEC3 dst = rayIntersectTriangle(camera_rd.pos,ray_ang,triangle.pos[0],triangle.pos[1],triangle.pos[2]);
			if(dst.x > 0 && vram.zbuffer[x * WND_RESOLUTION.y + y].dst > dst.x){
				vram.zbuffer[x * WND_RESOLUTION.y + y].dst = dst.x;
				vram.zbuffer[x * WND_RESOLUTION.y + y].primitive = primitive;
				vram.uv[x * WND_RESOLUTION.y + y] = (VEC2){dst.y,dst.z};
			}
		}
	}
}

void mapSquareRegion(PRIMITIVE* primitive,int b_x,CAMERA camera_rd){
	SQUARE square = primitive->square;
	if(square.normal.x == -1.0f && camera_rd.pos.x > square.pos.x)
		return;
	if(square.normal.x ==  1.0f && camera_rd.pos.x < square.pos.x)
		return;
	if(square.normal.y == -1.0f && camera_rd.pos.y > square.pos.y)
		return;
	if(square.normal.y ==  1.0f && camera_rd.pos.y < square.pos.y)
		return;
	if(square.normal.z == -1.0f && camera_rd.pos.z > square.pos.z)
		return;
	if(square.normal.z ==  1.0f && camera_rd.pos.z < square.pos.z)
		return;
	REGION region = boundRegionSquare(WND_RESOLUTION,square.pos,square.v,square.size,camera_rd.pos,camera_rd.dir_tri,0);
	region.min.x += (region.min.x & 1) ? !(b_x & 1) : (b_x & 1);
	region.min.y += (region.min.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	for(int x = region.min.x;x < region.max.x;x+=2){
		for(int y = region.min.y;y < region.max.y;y+=2){
			VEC3 ray_ang = vram.ray_ang[x * WND_RESOLUTION.y + y];
			float dst = rayIntersectSquare(camera_rd.pos,ray_ang,&square);
			if(dst > 0 && vram.zbuffer[x * WND_RESOLUTION.y + y].dst > dst){
				vram.zbuffer[x * WND_RESOLUTION.y + y].dst = dst;
				vram.zbuffer[x * WND_RESOLUTION.y + y].primitive = primitive;
			}
		}
	}
}

void camFlat(int b_x,CAMERA camera_rd){
	for(int x = b_x & 1;x < WND_RESOLUTION.x;x+=2){ 
		for(int y = b_x >> 1 & 1;y < WND_RESOLUTION.y;y+=2){
			VEC3 ray_ang = vram.ray_ang[x * WND_RESOLUTION.y + y];
			float dst = vram.zbuffer[x * WND_RESOLUTION.y + y].dst;
			if(!dst)
				continue;
			if(dst != 999999.0f){
				VEC3 color = {127.0f,127.0f,127.0f};
				VEC3 pos = VEC3addVEC3R(camera_rd.pos,VEC3mulR(ray_ang,vram.zbuffer[x * WND_RESOLUTION.y + y].dst));
				hitRayFlat(vram.zbuffer[x * WND_RESOLUTION.y + y].primitive,&color,pos,ray_ang,vram.uv[x * WND_RESOLUTION.y + y],dst);
				vram.draw[x * WND_RESOLUTION.y + y].r = tMinf(color.r,255.0f);
				vram.draw[x * WND_RESOLUTION.y + y].g = tMinf(color.g,255.0f);
				vram.draw[x * WND_RESOLUTION.y + y].b = tMinf(color.b,255.0f);
				continue;
			}
			int index;
			VEC2 cm = VEC2mulR(sampleCube(ray_ang,&index),skybox.size);
			IVEC2 cmi = {cm.x,cm.y};
			VEC3 color = VEC3mulR(skybox.sides[index][cmi.x * skybox.size + cmi.y],255.0f);
			vram.draw[x * WND_RESOLUTION.y + y].r = tMinf(color.r,255.0f);
			vram.draw[x * WND_RESOLUTION.y + y].g = tMinf(color.g,255.0f);
			vram.draw[x * WND_RESOLUTION.y + y].b = tMinf(color.b,255.0f);
		}
	}
}

void camNormal(int b_x,CAMERA camera_rd){
	float brightness = 0.0f;
	for(int x = b_x & 1;x < WND_RESOLUTION.x;x+=2){ 
		for(int y = b_x >> 1 & 1;y < WND_RESOLUTION.y;y+=2){
			VEC3 ray_ang = vram.ray_ang[x * WND_RESOLUTION.y + y];
			float dst = vram.zbuffer[x * WND_RESOLUTION.y + y].dst;
			if(!dst)
				continue;
			if(dst != 999999.0f){
				VEC3 color = {127.0f,127.0f,127.0f};
				VEC3 pos = VEC3addVEC3R(camera_rd.pos,VEC3mulR(ray_ang,vram.zbuffer[x * WND_RESOLUTION.y + y].dst));
				hitRayNormal(vram.zbuffer[x * WND_RESOLUTION.y + y].primitive,&color,pos,ray_ang,vram.uv[x * WND_RESOLUTION.y + y],dst);
				brightness += tMax(tMaxf(color.r,color.g),color.b);
				VEC3mul(&color,camera.exposure);
				vram.draw[x * WND_RESOLUTION.y + y].r = tMinf(color.r,255.0f);
				vram.draw[x * WND_RESOLUTION.y + y].g = tMinf(color.g,255.0f);
				vram.draw[x * WND_RESOLUTION.y + y].b = tMinf(color.b,255.0f);
				continue;
			}
			int index;
			VEC2 cm = VEC2mulR(sampleCube(ray_ang,&index),skybox.size);
			IVEC2 cmi = {cm.x,cm.y};
			brightness += tMax(tMaxf(skybox.sides[index][cmi.x * skybox.size + cmi.y].x,skybox.sides[index][cmi.x * skybox.size + cmi.y].y),skybox.sides[index][cmi.x * skybox.size + cmi.y].z);
			VEC3 color = VEC3mulR(skybox.sides[index][cmi.x * skybox.size + cmi.y],255.0f);
			brightness += tMax(tMaxf(color.r,color.g),color.b);
			VEC3mul(&color,camera.exposure);
			vram.draw[x * WND_RESOLUTION.y + y].r = tMinf(color.r,255.0f);
			vram.draw[x * WND_RESOLUTION.y + y].g = tMinf(color.g,255.0f);
			vram.draw[x * WND_RESOLUTION.y + y].b = tMinf(color.b,255.0f);
		}
	}
	float a = (float)brightness / (WND_RESOLUTION.x / 2 * WND_RESOLUTION.y / 2 * 127.0f) * camera.exposure;
	camera.exposure = (camera.exposure * 299.0f + (camera.exposure * tMaxf(2.0f - a,0.0f))) / 300.0f;
	camera.exposure = (camera.exposure * 199.0f + (1.0f)) / 200.0f;
	if(!camera.exposure) camera.exposure = 0.0001f;
}

void drawRect(VEC2 pos,VEC2 size,RGB color,int b_x){
	VEC2mulVEC2(&pos,(VEC2){WND_RESOLUTION.x,WND_RESOLUTION.y});
	VEC2mulVEC2(&size,(VEC2){WND_RESOLUTION.x,WND_RESOLUTION.y});
	pos.x += ((int)pos.x & 1) ? !(b_x & 1) : (b_x & 1);
	pos.y += ((int)pos.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	for(int x = pos.x;x < pos.x + size.x;x+=2){
		for(int y = pos.y;y < pos.y + size.y;y+=2){
			if(vram.zbuffer[x * WND_RESOLUTION.y + y].dst){
				vram.draw[x * WND_RESOLUTION.y + y] = color;
				vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 0.0f;
			}
		}
	}
}

void drawGuiTexture(VEC2 pos,VEC2 size,TEXTURE* texture,int b_x){
	VEC2mulVEC2(&pos,(VEC2){WND_RESOLUTION.x,WND_RESOLUTION.y});
	VEC2mulVEC2(&size,(VEC2){WND_RESOLUTION.x,WND_RESOLUTION.y});
	VEC2 o_pos = pos;
	pos.x += ((int)pos.x & 1) ? !(b_x & 1) : (b_x & 1);
	pos.y += ((int)pos.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	if(texture){
		for(int x = pos.x;x < pos.x + size.x;x+=2){
			for(int y = pos.y;y < pos.y + size.y;y+=2){
				if(vram.zbuffer[x * WND_RESOLUTION.y + y].dst){
					IVEC2 crd;
					crd.x = tMax(tMin((x - o_pos.x) / (size.x) * texture->size,texture->size-1),0);
					crd.y = tMax(tMin((y - o_pos.y) / (size.y) * texture->size,texture->size-1),0);
					VEC3 color = texture->data[0][crd.x * texture->size + crd.y];
					vram.draw[x * WND_RESOLUTION.y + y] = (RGB){color.r*255.0f,color.g*255.0f,color.b*255.0f};
					vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 0.0f;
				}
			}
		}
	}
	else{
		for(int x = pos.x;x < pos.x + size.x;x+=2){
			for(int y = pos.y;y < pos.y + size.y;y+=2){
				if(vram.zbuffer[x * WND_RESOLUTION.y + y].dst){
					RGB color = (x & 4) ^ (y & 4) ? (RGB){100,100,100} : (RGB){140,140,140};
					vram.draw[x * WND_RESOLUTION.y + y] = color;
					vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 0.0f;
				}
			}
		}
	}
}

void drawString(char* str,VEC2 pos,int b_x){
	VEC2mulVEC2(&pos,(VEC2){WND_RESOLUTION.x,WND_RESOLUTION.y});
	VEC2 o_pos = pos;
	pos.x += ((int)pos.x & 1) ? !(b_x & 1) : (b_x & 1);
	pos.y += ((int)pos.y & 1) ? !(b_x >> 1 & 1) : (b_x >> 1 & 1);
	while(*str++){
		int offset = (str[-1] - 0x21) % 10 * 8 + (9 - (str[-1] - 0x21) / 10) * 80 * 8;
		for(int x = pos.x;x < o_pos.x + 7;x+=2){
			for(int y = pos.y;y < o_pos.y + 7;y+=2){
				VEC3 color = font.data[0][offset + (x - (int)o_pos.x) * 80 + (y - (int)o_pos.y)];
				if(!color.x){
					vram.draw[x * WND_RESOLUTION.y + y] = (RGB){255,255,255};
					vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 0.0f;
				}
			}
		}
		o_pos.y += 8.0f;
		pos.y += 8.0f;
	}
}

void drawPixel(int x,int y,RGB color){
	vram.draw[x * WND_RESOLUTION.y + y] = color;
	vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 0.0f;
}

void draw(int b_x){
	for(;;){
		CAMERA camera_rd = camera;
		for(int x = b_x & 1;x < WND_RESOLUTION.x;x+=2){ 
			for(int y = b_x >> 1 & 1;y < WND_RESOLUTION.y;y+=2){
				vram.zbuffer[x * WND_RESOLUTION.y + y].dst = 999999.0f;
				vram.ray_ang[x * WND_RESOLUTION.y + y] = getRayAngle(WND_RESOLUTION,camera_rd.dir_tri,x,y);
			}
		}
		for(int i = 0;i < primitivehub.cnt;i++){
			PRIMITIVE* primitive = &primitivehub.primitive[i];
			switch(primitive->type){
			case PR_SPHERE:				         mapSphereRegion(primitive,b_x,camera_rd); break;
			case PR_SQUARE: case PR_LIGHTSQUARE: mapSquareRegion(primitive,b_x,camera_rd); break;
			case PR_TRIANGLE: mapTriangleRegion(primitive,b_x,camera_rd); break;
			}
		}
		drawPixel(WND_RESOLUTION.x/2,WND_RESOLUTION.y/2,(RGB){0,255,0});
		drawPixel(WND_RESOLUTION.x/2,WND_RESOLUTION.y/2+1,(RGB){0,0,0});
		drawPixel(WND_RESOLUTION.x/2,WND_RESOLUTION.y/2-1,(RGB){0,0,0});
		drawPixel(WND_RESOLUTION.x/2+1,WND_RESOLUTION.y/2,(RGB){0,0,0});
		drawPixel(WND_RESOLUTION.x/2-1,WND_RESOLUTION.y/2,(RGB){0,0,0});
		if(camera.edit_mode){
			char tstr[20] = {[19] = '\0'};
			switch(pr_selected){
			case 0: drawString("cube",(VEC2){0.917f,0.8125f},b_x); break;
			case 1: drawString("lightcube",(VEC2){0.917f,0.79f},b_x); break;
			case 2: drawString("sphere",(VEC2){0.917f,0.8f},b_x); break;
			case 3: drawString("triangle",(VEC2){0.917f,0.795f},b_x); break;
			}

			sprintf(tstr,"%f",grid_size);
			tstr[5] = '\0';
			drawString(tstr,(VEC2){0.82f,0.92f},b_x);

			drawString("gridsize",(VEC2){0.86f,0.912f},b_x);

			drawString("position",(VEC2){0.445f,0.75f},b_x);
			drawString("X",(VEC2){0.407f,0.74f},b_x);
			drawString("Y",(VEC2){0.357f,0.74f},b_x);
			drawString("Z",(VEC2){0.307f,0.74f},b_x);

			drawString("size",(VEC2){0.445f,0.84f},b_x);
			drawString("X",(VEC2){0.407f,0.83f},b_x);
			drawString("Y",(VEC2){0.357f,0.83f},b_x);
			drawString("Z",(VEC2){0.307f,0.83f},b_x);

			drawString("color",(VEC2){0.445f,0.93f},b_x);
			drawString("R",(VEC2){0.407f,0.92f},b_x);
			drawString("G",(VEC2){0.357f,0.92f},b_x);
			drawString("B",(VEC2){0.307f,0.92f},b_x);

			if(primitivehub.selected && primitivehub.selected->type != PR_TRIANGLE)
				drawGuiTexture((VEC2){0.49f,0.795f},VEC2_SQUARE(0.07f),primitivehub.selected->square.texture.data,b_x);

			for(int i = 0;i < buttonhub.cnt;i++){
				BUTTON button = buttonhub.button[i];
				if(button.ch){
					char str[2] = {button.ch,'\0'};
					drawString(str,VEC2addVEC2R(VEC2subVEC2R(button.pos,VEC2_SQUARE(0.0057f)),VEC2mulR(button.size,0.5f)),b_x);
				}
				drawRect((VEC2){button.pos.x,button.pos.y},(VEC2){button.size.x*0.1f,button.size.y},(RGB){0,0,0},b_x);
				drawRect((VEC2){button.pos.x,button.pos.y},(VEC2){button.size.x,button.size.y*0.1f},(RGB){0,0,0},b_x);
				drawRect((VEC2){button.pos.x+button.size.x*0.9f,button.pos.y},(VEC2){button.size.x*0.1f,button.size.y},(RGB){0,0,0},b_x);
				drawRect((VEC2){button.pos.x,button.pos.y+button.size.y*0.9f},(VEC2){button.size.x,button.size.y*0.1f},(RGB){0,0,0},b_x);
				RGB color;
				switch(button.type){
				case BTNTYPE_NORMAL: color = (RGB){127,127,127}; break;
				case BTNTYPE_TOGGLE:
					color = button.toggled ? (RGB){40,127,40} : (RGB){40,40,127};
					break;
				}
				VEC2 pos = VEC2addVEC2R(button.pos,VEC2mulR(button.size,0.125f));
				drawRect(pos,VEC2mulR(button.size,0.75f),color,b_x);
			}
		}
		switch(camera.rd_mode){
		case CAM_FLAT:   camFlat(b_x,camera_rd); break;
		case CAM_NORMAL: camNormal(b_x,camera_rd); break;
		}
	}
}

void draw_1(){
	draw(0);
}

void draw_2(){
	draw(1);
}

void draw_3(){
	draw(2);
}

void draw_4(){
	draw(3);
}

void addButton(VEC2 pos,VEC2 size,int action,char ch,int type){
	BUTTON* button = &buttonhub.button[buttonhub.cnt++];
	button->pos = pos;
	button->size = size;
	button->action = action;
	button->ch = ch;
	button->type = type;
}

void addTextureH(TEXTUREPROP prop){
	TEXTURE* texture = &texturehub.texture[texturehub.cnt++];
	texture->data = MALLOC(sizeof(VEC3)*14);
	texture->data[0] = prop.data;
	texture->size = prop.size;
	texture->mipmap_amm = 0;
	int m_size = texture->size >> 1;
	for(int i = 1;m_size != 1;i++,m_size >>= 1,texture->mipmap_amm++){
		texture->data[i] = MALLOC(sizeof(VEC3) * m_size * m_size);
		for(int x = 0;x < m_size;x++){
			for(int y = 0;y < m_size;y++){	
				VEC3 c_1 = texture->data[i - 1][(x << 1) * (m_size << 1) + (y << 1)];
				VEC3 c_2 = texture->data[i - 1][(x << 1) * (m_size << 1) + (y << 1) + 1];
				VEC3 c_3 = texture->data[i - 1][(x << 1) * (m_size << 1) + (y << 1) + (m_size << 1)];
				VEC3 c_4 = texture->data[i - 1][(x << 1) * (m_size << 1) + (y << 1) + (m_size << 1) + 1];

				VEC3 avg = VEC3avgVEC3R4(c_1,c_2,c_3,c_4);
				texture->data[i][x * m_size + y] = avg;
			}
		}
	}
}

void main(){
	hitboxhub.hitbox = MALLOC(sizeof(HITBOX) * 1024);
	buttonhub.button = MALLOC(sizeof(BUTTON) * 1024);
	texturehub.texture = MALLOC(sizeof(TEXTURE) * 1024);
	boxhub.box = MALLOC(sizeof(BOX) * 1024);

	vram.render       = MALLOC(sizeof(RGB)    *WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.draw         = MALLOC(sizeof(RGB)    *WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.color        = MALLOC(sizeof(VEC3)   *WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.zbuffer      = MALLOC_ZERO(sizeof(ZBUFFER)*WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.ray_ang      = MALLOC(sizeof(VEC3)   *WND_RESOLUTION.x*WND_RESOLUTION.y);
	vram.uv           = MALLOC(sizeof(VEC2) *WND_RESOLUTION.x*WND_RESOLUTION.y);
	cm_zbuffer        = MALLOC_ZERO(sizeof(ZBUFFER)*CM_SIZE*CM_SIZE);
	skybox.size = 128;
	for(int i = 0;i < 6;i++)
		skybox.sides[i] = MALLOC_ZERO(sizeof(VEC3) * skybox.size * skybox.size);
	
	for(int i = 0;i < skybox.size * skybox.size;i++){
		float x = (float)(i % skybox.size) * 0.01f;
		skybox.sides[SD_X2][i] = (VEC3){0.5f,0.5f,0.5f};
		skybox.sides[SD_X1][i] = (VEC3){2.0f,4.0f,6.0f};
		skybox.sides[SD_Y1][i] = (VEC3){0.5f,0.5f,0.5f};
		skybox.sides[SD_Y2][i] = (VEC3){0.5f,0.5f,0.5f};
		skybox.sides[SD_Z1][i] = (VEC3){0.5f,0.5f,0.5f};
		skybox.sides[SD_Z2][i] = (VEC3){0.5f,0.5f,0.5f};
	}
	
	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;
	primitivehub.primitive = MALLOC(sizeof(PRIMITIVE)*1024);
	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE|WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);

	WIN32_FIND_DATAA windt;
	HANDLE h = FindFirstFileA("img/*.bmp",&windt);
	if(h!=-1){
		char str[256];
		strcpy(str,"img/");
		strcpy(str+4,windt.cFileName);
		addTextureH(loadBMP(str));
		while(FindNextFileA(h,&windt)){
			strcpy(str+4,windt.cFileName);
			addTextureH(loadBMP(str));
		}
	}
	TEXTUREPROP font_prop = loadBMP("font/font.bmp");
	font.data = MALLOC(sizeof(void*));
	font.data[0] = font_prop.data;
	font.size = font_prop.size;

	normalmap[0].size = 256;
	normalmap[0].value = MALLOC_ZERO(sizeof(VEC2) * normalmap[0].size * normalmap[0].size);

	for(int x = 0;x < 256;x++){
		for(int y = 0;y < 32;y++){
			normalmap[0].value[x * 256 + (31 - y)].y = (float)y * 0.02f;
			normalmap[0].value[x * 256 + 255 - (31 - y)].y = -(float)y * 0.02f;

			normalmap[0].value[x + (31 - y) * 256].x = (float)y * 0.02f;
			normalmap[0].value[x + 255 * 256 - (31 - y) * 256].x = -(float)y * 0.02f;
		}
	}

	normalmap[1].size = 256;
	normalmap[1].value = MALLOC_ZERO(sizeof(VEC2) * normalmap[0].size * normalmap[0].size);

	for(int x = 0;x < 256;x++){
		for(int y = 0;y < 128;y++){
			normalmap[1].value[x * 256 + y].y = -(float)y * 0.001f;
			normalmap[1].value[x * 256 + 255 - y].y = (float)y * 0.001f;
		}
	}

	normalmap[2].size = 256;
	normalmap[2].value = MALLOC_ZERO(sizeof(VEC2) * normalmap[2].size * normalmap[2].size);

	for(int x = 0;x < 256;x++){
		for(int y = 0;y < 32;y++){
			normalmap[2].value[x * 256 + (31 - y)].y = (float)y * 0.02f;
			normalmap[2].value[x * 256 + 255 - (31 - y)].y = -(float)y * 0.02f;
			
			normalmap[2].value[x * 256 + (31 - y) + 128].y = (float)y * 0.02f;
			normalmap[2].value[x * 256 + 255 - (31 - y) + 128].y = -(float)y * 0.02f;
			if(x < 128){
				normalmap[2].value[x + (31 - y) * 256].x = (float)y * 0.02f;
				normalmap[2].value[x + 255 * 256 - (31 - y) * 256].x = -(float)y * 0.02f;
			}
			else{
				normalmap[2].value[x + (31 - y + 127) * 256].x = (float)y * 0.02f;
				normalmap[2].value[x + 255 * 256 - (31 - y + 127) * 256].x = -(float)y * 0.02f;
			}
		}
	}

	reflectmap[0].size = 16;
	reflectmap[0].value = MALLOC_ZERO(sizeof(float) * reflectmap[0].size * reflectmap[0].size);

	for(int x = 0;x < 8;x++){
		for(int y = 0;y < 8;y++){
			reflectmap[0].value[x * 16 + y] = 0.5f;
			reflectmap[0].value[x * 16 + y + 136] = 0.5f;
		}
	}

	HANDLE temp = CreateFileA("lvl/temp.lvl",GENERIC_READ,0,0,OPEN_EXISTING,0,0);
	int size = GetFileSize(temp,0);
	ReadFile(temp,&hitboxhub.cnt,sizeof(int),0,0);
	ReadFile(temp,hitboxhub.hitbox,sizeof(HITBOX) * hitboxhub.cnt,0,0);
	ReadFile(temp,&primitivehub.cnt,sizeof(int),0,0);
	ReadFile(temp,primitivehub.primitive,primitivehub.cnt * sizeof(PRIMITIVE),0,0);
	ReadFile(temp,&boxhub.cnt,sizeof(int),0,0);
	ReadFile(temp,boxhub.box,boxhub.cnt * sizeof(BOX),0,0);

	if(primitivehub.cnt)
		primitivehub.selected = primitivehub.primitive;
	for(int i = 0;i < primitivehub.cnt;i++){
		PRIMITIVE* primitive = &primitivehub.primitive[i];
		switch(primitive->type){
		case PR_SQUARE:;
			SQUARE* square = &primitive->square;
			VEC2 size = square->size;
			size.x *= CM_SPREAD;
			size.y *= CM_SPREAD;
			if(square->cm_properties.reflectmap != RF_VOID){
				square->cubemap = MALLOC(size.x * size.y * sizeof(VEC3*));
				for(int j = 0;j < size.x * size.y;j++){
					square->cubemap[j] = MALLOC(sizeof(VEC3*) * 6);
					square->cubemap[j] = MALLOC(sizeof(VEC3*) * 6);
					for(int k = 0;k < 6;k++){
						square->cubemap[j][k] = MALLOC_ZERO(CM_SIZE * CM_SIZE * sizeof(VEC3));
						square->cubemap[j][k] = MALLOC_ZERO(CM_SIZE * CM_SIZE * sizeof(VEC3));
					}
				}
			}
			if(square->cm_properties.normalmap == -1)
				square->cm_properties.normalmap = 0;
			else
				square->cm_properties.normalmap = 0;
			if(square->texture.data == -1)
				square->texture.data = 0;
			else
				square->texture.data = &texturehub.texture[(int)square->texture.data];
			int lm_size = (int)(square->size.x * LM_SIZE+2) * (int)(square->size.y * LM_SIZE+2) * sizeof(VEC3);
			square->lightmap = MALLOC(lm_size);
			ReadFile(temp,square->lightmap,lm_size,0,0);
			break;
		case PR_SPHERE:
			primitive->sphere.lightmap = MALLOC(sizeof(void*) * 6);
			for(int j = 0;j < 6;j++){
				primitive->sphere.lightmap[j] = MALLOC(LM_SPHERESIZE * LM_SPHERESIZE * sizeof(VEC3));
				ReadFile(temp,primitive->sphere.lightmap[j],LM_SPHERESIZE * LM_SPHERESIZE * sizeof(VEC3),0,0);
			}
			break;
		}
	}

	if(boxhub.cnt)
		boxhub.selected = boxhub.box;
	for(int i = 0;i < boxhub.cnt;i++){
		BOX* box = &boxhub.box[i];
		for(int j = 0;j < 6;j++)
			box->square[j] = &primitivehub.primitive[(int)box->square[j]];
		box->hitbox = &hitboxhub.hitbox[(int)box->hitbox];
	}

	CloseHandle(temp);

	addButton((VEC2){0.53f,0.9f},VEC2_SQUARE(0.025f),31,'-',BTNTYPE_NORMAL);
	addButton((VEC2){0.53f,0.925f},VEC2_SQUARE(0.025f),32,'+',BTNTYPE_NORMAL);

	addButton((VEC2){0.495f,0.9f},VEC2_SQUARE(0.025f),33,'-',BTNTYPE_NORMAL);
	addButton((VEC2){0.495f,0.925f},VEC2_SQUARE(0.025f),34,'+',BTNTYPE_NORMAL);

	addButton((VEC2){0.495f,0.975f},VEC2_SQUARE(0.025f),35,'-',BTNTYPE_NORMAL);
	addButton((VEC2){0.53f,0.975f},VEC2_SQUARE(0.025f),36,'+',BTNTYPE_NORMAL);

	addButton((VEC2){0.2f,0.75f},VEC2_SQUARE(0.05f),38,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.2f,0.85f},VEC2_SQUARE(0.05f),39,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.8f,0.8f},VEC2_SQUARE(0.05f),37,'C',BTNTYPE_TOGGLE);

	addButton((VEC2){0.9f,0.75f},VEC2_SQUARE(0.05f),20,'<',BTNTYPE_NORMAL);

	addButton((VEC2){0.9f,0.875f},VEC2_SQUARE(0.05f),21,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.7f,0.75f},VEC2_SQUARE(0.05f),28,'<',BTNTYPE_NORMAL);

	addButton((VEC2){0.7f,0.85f},VEC2_SQUARE(0.05f),29,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.8f,0.75f},VEC2_SQUARE(0.05f),BTN_PLACEPRIMITIVE,'+',BTNTYPE_NORMAL);

	addButton((VEC2){0.6f,0.75f},VEC2_SQUARE(0.05f),BTN_CYCLEPRIMITIVE_R,'<',BTNTYPE_NORMAL);

	addButton((VEC2){0.6f,0.85f},VEC2_SQUARE(0.05f),BTN_CYCLEPRIMITIVE_L,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.5f,0.75f},VEC2_SQUARE(0.05f),BTN_CYCLETEXTURE_R,'<',BTNTYPE_NORMAL);

	addButton((VEC2){0.5f,0.85f},VEC2_SQUARE(0.05f),BTN_CYCLETEXTURE_L,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.948f,0.972f},VEC2_SQUARE(0.05f),BTN_TOGGLELIGHT,'L',BTNTYPE_TOGGLE);
	addButton((VEC2){0.9f,0.972f},VEC2_SQUARE(0.05f),30,'M',BTNTYPE_TOGGLE);

	addButton((VEC2){0.8f,0.89f},VEC2_SQUARE(0.05f),18,'-',BTNTYPE_NORMAL);
	addButton((VEC2){0.8f,0.97f},VEC2_SQUARE(0.05f),19,'+',BTNTYPE_NORMAL);

	addButton((VEC2){0.4f,0.75f},VEC2_SQUARE(0.025f),BTN_SUBPOS_X,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.4f,0.775f},VEC2_SQUARE(0.025f),BTN_ADDPOS_X,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.75f},VEC2_SQUARE(0.025f),BTN_SUBPOS_Y,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.775f},VEC2_SQUARE(0.025f),BTN_ADDPOS_Y,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.75f},VEC2_SQUARE(0.025f),BTN_SUBPOS_Z,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.775f},VEC2_SQUARE(0.025f),BTN_ADDPOS_Z,'>',BTNTYPE_NORMAL);
	
	addButton((VEC2){0.4f,0.84f},VEC2_SQUARE(0.025f),BTN_SUBSIZE_X,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.4f,0.865f},VEC2_SQUARE(0.025f),BTN_ADDSIZE_X,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.84f},VEC2_SQUARE(0.025f),BTN_SUBSIZE_Y,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.865f},VEC2_SQUARE(0.025f),BTN_ADDSIZE_Y,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.84f},VEC2_SQUARE(0.025f),BTN_SUBSIZE_Z,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.865f},VEC2_SQUARE(0.025f),BTN_ADDSIZE_Z,'>',BTNTYPE_NORMAL);

	addButton((VEC2){0.4f,0.93f},VEC2_SQUARE(0.025f),22,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.4f,0.955f},VEC2_SQUARE(0.025f),23,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.93f},VEC2_SQUARE(0.025f),24,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.35f,0.955f},VEC2_SQUARE(0.025f),25,'>',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.93f},VEC2_SQUARE(0.025f),26,'<',BTNTYPE_NORMAL);
	addButton((VEC2){0.3f,0.955f},VEC2_SQUARE(0.025f),27,'>',BTNTYPE_NORMAL);

	HANDLE thread_render = CreateThread(0,0,render,0,0,0);

	HANDLE thread_draw_1 = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,0,0,0);
	HANDLE thread_draw_2 = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_2,0,0,0);
	HANDLE thread_draw_3 = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_3,0,0,0);
	HANDLE thread_draw_4 = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_4,0,0,0);

	SetThreadPriority(thread_draw_1,THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(thread_draw_2,THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(thread_draw_3,THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(thread_draw_4,THREAD_PRIORITY_ABOVE_NORMAL);

	CreateThread(0,0,physics,0,0,0);

	while(GetMessageA(&Msg,window,0,0)){
		TranslateMessage(&Msg);
 		DispatchMessageA(&Msg);
	}
}