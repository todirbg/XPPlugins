/**
 * BetterMouseYoke - X-Plane 11 Plugin
 *
 * Does away with X-Plane's idiotic centered little box for mouse steering and
 * replaces it with a more sane system for those who, for whatever reason,
 * want to or have to use the mouse for flying.
 *
 * Copyright 2019 Torben Könke.
 */
#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "../Util/util.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#if IBM
#pragma comment(lib, "detours.lib")
#include "detours.h"
int hook_set_cursor(int attach);
#elif LIN
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>
#endif


typedef enum {
    CURSOR_ARROW,
    CURSOR_YOKE,
    CURSOR_RUDDER
} cursor_t;

int init_menu();
int toggle_yoke_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref);
int draw_cb(XPLMDrawingPhase phase, int before, void *ref);
float loop_cb(float last_call, float last_loop, int count, void *ref);
void get_cursor_pos(int *x, int *y);
void set_cursor_from_yoke();
void set_cursor_pos(int x, int y);
void set_cursor_bmp(cursor_t cursor);
int controlling_rudder(int *x, int *y);

#if LIN
static int has_xi2(Display *dpy);
static void select_events(Display *dpy, Window win);
static void deselect_events(Display *dpy, Window win);
#endif // LIN


#endif /* _PLUGIN_H_ */
