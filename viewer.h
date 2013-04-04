// viewer.h - viewer includes.

#pragma once
#ifndef __VIEWER_H__
#define __VIEWER_H__

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <SDL.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>

#include "morrigan.h"
#include "landscape.h"
#include "client_net.h"

#define TIMER_EVENT_ID 1
#define TANKS_TIMER_EVENT_ID 2

typedef struct Camera
{
    double x, y, z;
    double vertical_angle, horizontal_angle;
    double move_speed, mouse_sensitivity;
    int mouse_prev_x, mouse_prev_y;
    int w, h, bpp;
    double fov_y;
} Camera;

extern Camera camera;

// viewer_events.c
bool process_events(bool *need_redraw, Camera *camera);

// viewer_draw.c
void draw(const Landscape *l, const ResGetTanksTankRecord *tanks, size_t tanks_count);
void draw_landscape(const Landscape *l);
void draw_tank_body(void);
void draw_tank_turret(void);
void draw_tank_gun(void);
void draw_tank(const ResGetTanksTankRecord *tank);
void draw_tanks(const ResGetTanksTankRecord *tanks, size_t tanks_count);

double range_angle(double a);

#define DISPLAY_LISTS_COUNT 4
#define LANDSCAPE_DISPLAY_LIST   0
#define TANK_BODY_DISPLAY_LIST   1
#define TANK_TURRET_DISPLAY_LIST 2
#define TANK_GUN_DISPLAY_LIST 3
extern GLuint display_lists;

#endif /* __VIEWER_H__ */
