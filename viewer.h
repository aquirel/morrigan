// viewer.h - viewer includes.

#pragma once
#ifndef __VIEWER_H__
#define __VIEWER_H__

//#pragma message("__VIEWER_H__")

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>

#pragma warn(push)
#pragma warn(disable: 2185)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warn(pop)

#include <SDL.h>

#include <gl/gl.h>
#pragma warn(push)
#pragma warn(disable: 2027)
#include <gl/glu.h>
#pragma warn(pop)
#include <gl/glaux.h>

#include "morrigan.h"
#include "dynamic_array.h"
#include "landscape.h"
#include "tank.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "client_protocol.h"
#include "viewer_net.h"

#pragma pack(push, 8)

typedef struct Camera
{
    double x, y, z;
    double vertical_angle, horizontal_angle;
    double move_speed, mouse_sensitivity;
    unsigned short mouse_prev_x, mouse_prev_y;
    unsigned short w, h, bpp;
    double fov_y;
} Camera;

#pragma pack(pop)

// viewer_events.c
bool process_events(bool *need_redraw, Camera *camera, ResGetTanksTankRecord *tanks, size_t *tanks_count, ClientProtocol *viewer_protocol);
//void move_tanks(const Landscape *l, ResGetTanksTankRecord *tanks, const size_t tanks_count);
void process_shells(void);

DynamicArray *viewer_get_shoots(void);
DynamicArray *viewer_get_explosions(void);

// viewer_draw.c
void draw_landscape(const Landscape *l);
void draw_tank_body(void);
void draw_tank_turret(void);
void draw_tank_gun(void);
void draw(const Landscape *l,
          const ResGetTanksTankRecord *tanks,
          size_t tanks_count,
          const Camera *camera,
          const GLuint *display_lists,
          const DynamicArray *shoots,
          const DynamicArray *explosions);

#define TIMER_EVENT_ID 1
#define TANKS_TIMER_EVENT_ID 2

#define DISPLAY_LISTS_COUNT 4
#define LANDSCAPE_DISPLAY_LIST   0
#define TANK_BODY_DISPLAY_LIST   1
#define TANK_TURRET_DISPLAY_LIST 2
#define TANK_GUN_DISPLAY_LIST    3

#endif /* __VIEWER_H__ */
