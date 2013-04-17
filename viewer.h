// viewer.h - viewer includes.

#pragma once
#ifndef __VIEWER_H__
#define __VIEWER_H__

#pragma message("__VIEWER_H__")

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
#include "protocol.h"
#include "client_protocol.h"
#include "viewer_net.h"

#define TIMER_EVENT_ID 1
#define TANKS_TIMER_EVENT_ID 2

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

extern SOCKET s;
extern Camera camera;

extern Landscape *l;
extern ResGetTanksTankRecord tanks[MAX_CLIENTS];
extern size_t tanks_count;

extern DynamicArray *shoots;
extern DynamicArray *explosions;

// viewer_events.c
bool process_events(bool *need_redraw, Camera *camera);
void move_tanks(const Landscape *l, ResGetTanksTankRecord *tanks, const size_t tanks_count);
void process_shells(void);

// viewer_draw.c
void draw(const Landscape *l, const ResGetTanksTankRecord *tanks, size_t tanks_count);
void draw_landscape(const Landscape *l);
void draw_tank_body(void);
void draw_tank_turret(void);
void draw_tank_gun(void);
void draw_tank(const ResGetTanksTankRecord *tank);
void draw_tanks(const ResGetTanksTankRecord *tanks, size_t tanks_count);
void draw_shoot(const NotViewerShellEvent *shoot);
void draw_shoots(DynamicArray *shoots);
void draw_explosion(const NotViewerShellEvent *explosion);
void draw_explosions(DynamicArray *explosions);

double range_angle(double a);

#define DISPLAY_LISTS_COUNT 4
#define LANDSCAPE_DISPLAY_LIST   0
#define TANK_BODY_DISPLAY_LIST   1
#define TANK_TURRET_DISPLAY_LIST 2
#define TANK_GUN_DISPLAY_LIST 3
extern GLuint display_lists;

#endif /* __VIEWER_H__ */
