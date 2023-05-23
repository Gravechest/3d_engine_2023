#pragma once

#include "vec3.h"
#include "source.h"

float rayIntersectPlane(VEC3 pos,VEC3 dir,VEC3 plane);
float rayIntersectSphere(VEC3 cam_pos,VEC3 sphere_pos,VEC3 cam_dir,float radius);
float rayIntersectSquare(VEC3 cam_pos,VEC3 cam_ang,SQUARE* square);
float rayIntersectBox(VEC3 ray_pos,VEC3 ray_dir,VEC3 box_pos,VEC3 box_size);
float rayIntersectBoxSearch(VEC3 ray_pos,VEC3 ray_dir,VEC3 box_pos,VEC3 box_size);
float rayIntersectTorus(VEC3 ray_pos,VEC3 tor_pos,VEC3 rd,VEC2 tor);
VEC3 rayIntersectTriangle(VEC3 ro,VEC3 rd,VEC3 v0,VEC3 v1,VEC3 v2);