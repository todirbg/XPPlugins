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
#if LIN
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xfixes.h>
#endif


typedef enum {
    CURSOR_ARROW,
    CURSOR_YOKE,
    CURSOR_RUDDER
} cursor_t;

void write_default_settings();
int init_menu();
int toggle_yoke_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref);
int draw_cb(XPLMDrawingPhase phase, int before, void *ref);
float loop_cb(float last_call, float last_loop, int count, void *ref);
void get_cursor_pos(int *x, int *y);
void set_cursor_from_yoke();
void set_cursor_pos(int x, int y);
void set_cursor_bmp(cursor_t cursor);
int controlling_rudder(int *x, int *y);
void update_look();
void win_draw_cb(XPLMWindowID id, void *ref);
int win_click_cb(XPLMWindowID id, int x, int y, XPLMMouseStatus status,
    void *ref);
int win_right_click_cb(XPLMWindowID id, int x, int y, XPLMMouseStatus status,
    void *ref);
void win_key_cb(XPLMWindowID id, char key, XPLMKeyFlags flags, char vkey,
    void *ref, int losing_focus);
XPLMCursorStatus win_cursor_cb(XPLMWindowID id, int x, int y, void *ref);
int win_wheel_cb(XPLMWindowID id, int x, int y, int wheel, int clicks,
    void *ref);

#if LIN
static int has_xi2(Display *dpy);
static void select_events(Display *dpy, Window win);
static void deselect_events(Display *dpy, Window win);
#endif // LIN


#endif /* _PLUGIN_H_ */
