#pragma once

#include <Windows.h>

#include "vec3.h"
#include "vec2.h"
#include "phyiscs.h"

#undef RGB

#define RF_TOTAL 3

#define PR_TOTAL 4

#define MV_FLY 0

#define CM_SPREAD 4.0f
#define CM_SIZE 16.0f
#define CM_BOUND (int)(CM_SIZE - 1.0f)

#define LM_SPHEREBOUND (int)(LM_SPHERESIZE - 1.0f)
#define LM_SPHERESIZE 8.0f

#define LM_SIZE 4.0f
#define LM_QUALITY (4096.0f * 16.0f)

#define BOUNCE_LIMIT 15
#define RESOLUTION_SCALE 2

#define WND_SIZE (IVEC2){1920,1080}
#define WND_WIDHT (WND_SIZE.x/RESOLUTION_SCALE)
#define WND_HEIGHT (WND_SIZE.y/RESOLUTION_SCALE)
#define WND_RESOLUTION (IVEC2){WND_HEIGHT,WND_WIDHT}

#define PLANE_NORMAL (VEC3){1.0f,0.0f,0.0f}
#define PLANE_X      (VEC3){0.0f,1.0f,0.0f}
#define PLANE_Y      (VEC3){0.0f,0.0f,1.0f}

#define FOV_T (500.0f/RESOLUTION_SCALE)
#define FOV (VEC2){FOV_T,FOV_T}

#define SQUARE_X (IVEC3){VEC3_Y,VEC3_Z,VEC3_X}
#define SQUARE_Y (IVEC3){VEC3_X,VEC3_Z,VEC3_Y}
#define SQUARE_Z (IVEC3){VEC3_X,VEC3_Y,VEC3_Z}

#define VK_W 0x57
#define VK_S 0x53	
#define VK_A 0x41
#define VK_D 0x44

#define FALSE 0
#define TRUE 1

#define MALLOC(MEM) HeapAlloc(GetProcessHeap(),0,MEM)
#define MALLOC_ZERO(MEM) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MEM)
#define MFREE(MEM) HeapFree(GetProcessHeap(),0,MEM)

typedef struct{
	union{
		struct{
			int x;
			int y;
		};
		int a[2];
	};
}IVEC2;

typedef struct{
	union{
		struct{
			int x;
			int y;
			int z;
		};
		int a[3];
	};
}IVEC3;

typedef struct{
	float x;
	float y;
	float z;
	float w;
}VEC4;

typedef struct{
	char w;
	char s;
	char d;
	char a;
}KEYS;

typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}RGB;

typedef struct{
	int size;
	int mipmap_amm;
	VEC3** data;
}TEXTURE;

typedef struct{
	TEXTURE* texture;
	int cnt;
}TEXTUREHUB;

typedef struct{
	VEC3* data;
	int size;
}TEXTUREPROP;

typedef struct{
	int size;
	float* value;
}REFLECTMAP;

typedef struct{
	int size;
	VEC2* value;
}NORMALMAP;

typedef struct{
	TEXTURE* data;
	VEC2 offset;
	float scale;
}TXMAP;

typedef struct{
	NORMALMAP* normalmap;
	REFLECTMAP* reflectmap;
	float scale;
	float nscale;
}CMMAP;

typedef struct{
	VEC2 radia;
}EFFECT;

typedef struct{
	VEC3 pos;
	VEC3 color;
	VEC3 box_size;
	TXMAP texture;
	VEC3** lightmap;
	VEC3** cubemap;
	float radius;
}SPHERE;

typedef struct{
	EFFECT effect_prop;
	int effect;
	VEC3 color;
	VEC3 pos;
	TXMAP texture;
	CMMAP cm_properties;
	VEC3* lightmap;
	VEC3*** cubemap;
	VEC2 size;
	IVEC3 v;
	VEC3 normal;
}SQUARE;

typedef struct{
	VEC3 pos[3];
	VEC3 color;
	TXMAP texture;
	VEC3 normal;
	VEC3* lightmap;
	VEC2 size;
}TRIANGLE;

typedef struct PRIMITIVE{
	int type;
	union{
		SPHERE sphere;
		SQUARE square;
		TRIANGLE triangle;
	};
}PRIMITIVE;

typedef struct{
	PRIMITIVE* square[6];
	HITBOX* hitbox;
}BOX;

typedef struct{
	BOX* box;
	int cnt;
	BOX* selected;
}BOXHUB;

typedef struct{
	VEC3 pos;
	VEC3 ang;
	PRIMITIVE* primitive;
}RAYINFO;

typedef struct{
	PRIMITIVE* primitive;
	float dst;
}ZBUFFER;

typedef struct{
	ZBUFFER* zbuffer;
	VEC3* color;
	RGB* draw;
	RGB* render;
	VEC3* ray_ang;
	VEC2* uv;
}VRAM;

typedef struct{
	VEC3 pos;
	VEC2 dir;
	VEC4 dir_tri;
	VEC3 vel;
	float exposure;
	int rd_mode;
	int mv_mode;
	int edit_mode;
	int infocus;
}CAMERA;

typedef struct{
	int cnt;
	PRIMITIVE* selected;
	PRIMITIVE* primitive;
}PRIMITIVEHUB;

typedef struct{
	IVEC2 min;
	IVEC2 max;
}REGION;

typedef struct{
	int size;
	VEC3* sides[6];
}SKYBOX;

typedef struct{
	VEC2 pos;
	VEC2 size;
	int action;
	char ch;
	int type;
	int toggled;
}BUTTON;

typedef struct{
	BUTTON* button;
	int cnt;
}BUTTONHUB;

enum{
	BTNTYPE_NORMAL,
	BTNTYPE_TOGGLE
};

enum{
	BTN_PLACEPRIMITIVE,
	BTN_CYCLEPRIMITIVE_R,
	BTN_CYCLEPRIMITIVE_L,
	BTN_CYCLETEXTURE_R,
	BTN_CYCLETEXTURE_L,
	BTN_TOGGLELIGHT,
	BTN_SUBPOS_X,
	BTN_ADDPOS_X,
	BTN_SUBPOS_Y,
	BTN_ADDPOS_Y,
	BTN_SUBPOS_Z,
	BTN_ADDPOS_Z,
	BTN_SUBSIZE_X,
	BTN_ADDSIZE_X,
	BTN_SUBSIZE_Y,
	BTN_ADDSIZE_Y,
	BTN_SUBSIZE_Z,
	BTN_ADDSIZE_Z,
	BTN_REDUCEGRID,
	BTN_INCREASEGRID,
	BTN_CYCLEOBJECT_R,
	BTN_CYCLEOBJECT_L,
	BTN_SUBCOLOR_R,
	BTN_ADDCOLOR_R,
	BTN_SUBCOLOR_G,
	BTN_ADDCOLOR_G,
	BTN_SUBCOLOR_B,
	BTN_ADDCOLOR_B,
};

enum{
	CAM_FLAT,
	CAM_NORMAL
};

enum{
	SD_X1,
	SD_X2,
	SD_Y1,
	SD_Y2,
	SD_Z1,
	SD_Z2
};

enum{
	RF_VOID = -1,
	RF_DEFAULT
};

enum{
	PR_PLANE,
	PR_SPHERE,
	PR_LIGHTSPHERE,
	PR_SQUARE,
	PR_LIGHTSQUARE,
	PR_TRIANGLE
};

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);
VEC3 triangleNormal(VEC3 p1,VEC3 p2,VEC3 p3);

extern PRIMITIVEHUB primitivehub;
extern KEYS key;
extern CAMERA camera;
extern SKYBOX skybox;
extern BOXHUB boxhub;
extern int triangle_point_selected;