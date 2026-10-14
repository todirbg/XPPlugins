/**
 * BetterMouseYoke - X-Plane 11 Plugin
 *
 * Does away with X-Plane's idiotic centered little box for mouse steering and
 * replaces it with a more sane system for those who, for whatever reason,
 * want to or have to use the mouse for flying.
 *
 * Copyright 2019 Torben Könke.
 */
#include "plugin.h"

#define PLUGIN_NAME         "BetterMouseYoke"
#define PLUGIN_SIG          "S22.BetterMouseYoke"
#define PLUGIN_DESCRIPTION  "Does away with X-Plane's idiotic centered little box " \
                            "for mouse steering that has caused much grieve and "   \
                            "countless loss of virtual lives."
//#define PLUGIN_VERSION      "1.6"

#define RUDDER_DEFL_DIST    200
#define RUDDER_RET_SPEED    2.0f
#define LOOK_SENSITIVITY    0.2f

static XPLMCommandRef toggle_yoke_control;
static XPLMDataRef yoke_pitch_ratio;
static XPLMDataRef yoke_roll_ratio;
static XPLMDataRef yoke_heading_ratio;
static XPLMDataRef eq_pfc_yoke;
static XPLMDataRef head_psi;
static XPLMDataRef head_the;
static XPLMDataRef view_is_external;
static XPLMFlightLoopID loop_id;
static XPLMWindowID cursor_win;
static int screen_width;
static int screen_height;
static int yoke_control_enabled;
static int rudder_control;
static float magenta[] = { 1.0f, 0, 1.0f };
static float green[] = { 0, 1.0f, 0 };
static int set_pos;
static int change_cursor;
static int cursor_pos[2];
static int set_rudder_pos;
static int rudder_return;
static int rudder_defl_dist;
static float yaw_ratio;
static float rudder_ret_spd;
static int look_control;
static int right_click_passed;
static int look_pos[2];
static float look_start[2];
static float look_sens;
static cursor_t current_cursor = CURSOR_ARROW;
static int builtin_cursors;
#if IBM
static HWND xp_hwnd;
#elif LIN
    Display *dpy;
    XEvent ev;
    XIEvent *xi_event;
    XIRawEvent *xev;
    Window window;
#endif

/**
 * X-Plane 11 Plugin Entry Point.
 *
 * Called when a plugin is initially loaded into X-Plane 11. If 0 is returned,
 * the plugin will be unloaded immediately with no further calls to any of
 * its callbacks.
 */
PLUGIN_API int XPluginStart(char *name, char *sig, char *desc) {
    /* SDK docs state buffers are at least 256 bytes. */
    sprintf(name, "%s (v%.2f)", PLUGIN_NAME, PLUGIN_VERSION);
    strcpy(sig, PLUGIN_SIG);
    strcpy(desc, PLUGIN_DESCRIPTION);
    toggle_yoke_control = XPLMCreateCommand("BetterMouseYoke/ToggleYokeControl",
        "Toggle mouse yoke control");
    yoke_pitch_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_pitch_ratio");
    yoke_roll_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_roll_ratio");
    yoke_heading_ratio = XPLMFindDataRef("sim/cockpit2/controls/yoke_heading_ratio");
    eq_pfc_yoke = XPLMFindDataRef("sim/joystick/eq_pfc_yoke");
    head_psi = XPLMFindDataRef("sim/graphics/view/pilots_head_psi");
    head_the = XPLMFindDataRef("sim/graphics/view/pilots_head_the");
    view_is_external = XPLMFindDataRef("sim/graphics/view/view_is_external");
    XPLMDataRef has_joystick = XPLMFindDataRef("sim/joystick/has_joystick");
    if (XPLMGetDatai(has_joystick)) {
        _log("init: joystick detected, unloading plugin");
        return 0;
    }
    write_default_settings();
    if (!init_menu()) {
        _log("init: could not init menu");
        return 0;
    }
    rudder_defl_dist = ini_geti("rudder_deflection_distance", RUDDER_DEFL_DIST);
    rudder_ret_spd = ini_getf("rudder_return_speed", RUDDER_RET_SPEED);
    look_sens = ini_getf("look_sensitivity", LOOK_SENSITIVITY);
    int xp_ver, xplm_ver;
    XPLMHostApplicationID host_id;
    XPLMGetVersions(&xp_ver, &xplm_ver, &host_id);
    builtin_cursors = xplm_ver >= 420;
    if (!builtin_cursors)
        _log("init: cursor icons need X-Plane 12.3 or newer");
#if IBM
    xp_hwnd = FindWindowA("X-Plane", "X-Plane");
    if (!xp_hwnd) {
        _log("could not find X-Plane 11 window");
        return 0;
    }
#elif LIN
int xi_opcode, event, error;

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
    _log("Failed to open display.\n");
    return 0;
    }

    window = XDefaultRootWindow(dpy);
    if (!XQueryExtension(dpy, "XInputExtension", &xi_opcode, &event, &error)) {
       _log("X Input extension not available.\n");
          return 0;
    }

    if (!has_xi2(dpy))
    return 0;

#endif
    return 1;
}

/**
 * X-Plane 11 Plugin Callback
 *
 * Called when the plugin is about to be unloaded from X-Plane 11.
 */
PLUGIN_API void XPluginStop(void) {
#if LIN
    XCloseDisplay(dpy);
#endif
}

/**
 * X-Plane 11 Plugin Callback
 *
 * Called when the plugin is about to be enabled. Return 1 if the plugin
 * started successfully, otherwise 0.
 */
PLUGIN_API int XPluginEnable(void) {
    XPLMRegisterCommandHandler(toggle_yoke_control, toggle_yoke_control_cb,
        0, NULL);
    XPLMRegisterDrawCallback(draw_cb, xplm_Phase_Window, 0, NULL);
    XPLMCreateFlightLoop_t params = {
        .structSize = sizeof(XPLMCreateFlightLoop_t),
        .phase = xplm_FlightLoop_Phase_BeforeFlightModel,
        .refcon = NULL,
        .callbackFunc = loop_cb
    };
    loop_id = XPLMCreateFlightLoop(&params);
    /* Invisible window that covers the screen while mouse yoke control is
       enabled. It picks the cursor X-Plane shows and swallows mouse clicks
       so we can use the right button for looking around. */
    XPLMCreateWindow_t win = {
        .structSize = sizeof(XPLMCreateWindow_t),
        .visible = 0,
        .drawWindowFunc = win_draw_cb,
        .handleMouseClickFunc = win_click_cb,
        .handleKeyFunc = win_key_cb,
        .handleCursorFunc = win_cursor_cb,
        .handleMouseWheelFunc = win_wheel_cb,
        .refcon = NULL,
        .decorateAsFloatingWindow = xplm_WindowDecorationNone,
        .layer = xplm_WindowLayerFlightOverlay,
        .handleRightClickFunc = win_right_click_cb
    };
    cursor_win = XPLMCreateWindowEx(&win);
    if (!cursor_win) {
        _log("could not create cursor window");
        return 0;
    }
    return 1;
}

/**
 * X-Plane 11 Plugin Callback
 *
 * Called when the plugin is about to be disabled.
 */
PLUGIN_API void XPluginDisable(void) {
    XPLMUnregisterCommandHandler(toggle_yoke_control, toggle_yoke_control_cb,
        0, NULL);
    XPLMSetDatai(eq_pfc_yoke, 0);
    XPLMUnregisterDrawCallback(draw_cb, xplm_Phase_Window, 0, NULL);
    if (loop_id)
        XPLMDestroyFlightLoop(loop_id);
    loop_id = NULL;
    if (cursor_win)
        XPLMDestroyWindow(cursor_win);
    cursor_win = NULL;
    menu_deinit();
}

/**
 * X-Plane 11 Plugin Callback
 *
 * Called when a message is sent to the plugin by X-Plane 11 or another plugin.
 */
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, int msg, void *param) {
    if (from != XPLM_PLUGIN_XPLANE)
        return;
    if (msg == XPLM_MSG_PLANE_LOADED) {
        int index = (int)(intptr_t)param;
        /* user's plane */
        if (index == XPLM_USER_AIRCRAFT) {
            /* This will hide the clickable yoke control box. */
            XPLMSetDatai(eq_pfc_yoke, 1);
        }
    }
}

void write_default_settings() {
    /* Gives users a settings.ini with all options to edit, if there's none
       yet. The values match the defaults used when an option is missing. */
    char buf[4096];
    snprintf(buf, sizeof(buf),
        "; BetterMouseYoke settings. Lines starting with ; are comments.\n"
        "; Remove a line to use the default value.\n"
        "[settings]\n"
        "\n"
        "; When entering mouse yoke mode, move the mouse cursor to match the\n"
        "; current yoke deflection to avoid sudden control movements.\n"
        "; Set to 0 to disable.\n"
        "set_pos = 1\n"
        "\n"
        "; When entering rudder mode, move the mouse cursor to match the current\n"
        "; rudder deflection. Set to 0 to disable.\n"
        "set_rudder_pos = 1\n"
        "\n"
        "; Change the mouse cursor symbol in yoke and rudder mode (requires\n"
        "; X-Plane 12.3 or newer). Set to 0 to disable.\n"
        "change_cursor = 1\n"
        "\n"
        "; Gradually return the rudder to neutral after leaving rudder mode.\n"
        "; Set to 0 to disable.\n"
        "rudder_return = 1\n"
        "\n"
        "; The speed (in units per second) with which the rudder returns to\n"
        "; neutral. A value of 2 takes half a second from full deflection.\n"
        "rudder_return_speed = %g\n"
        "\n"
        "; The mouse movement range for full rudder deflection in rudder mode,\n"
        "; i.e. the distance between the two green bars on the screen.\n"
        "rudder_deflection_distance = %d\n"
        "\n"
        "; How many degrees the view turns per pixel of mouse movement when\n"
        "; looking around with the right mouse button.\n"
        "look_sensitivity = %g\n"
        "\n"
        "; Write extra debug messages to X-Plane's Log.txt. Set to 1 to enable.\n"
        "debug = 0\n",
        RUDDER_RET_SPEED, RUDDER_DEFL_DIST, LOOK_SENSITIVITY);
    ini_write_default(buf);
}

int init_menu() {
    menu_item_t items[] = {
        { "Set Yoke Cursor", "set_pos", &set_pos, 1 },
        { "Set Rudder Cursor", "set_rudder_pos", &set_rudder_pos, 1 },
        { "Change Cursor Icon", "change_cursor", &change_cursor, 1 },
        { "Return Rudder", "rudder_return", &rudder_return, 1 }
    };
    int num = sizeof(items) / sizeof(items[0]);

    return menu_init(PLUGIN_NAME, items, num);
}

int toggle_yoke_control_cb(XPLMCommandRef cmd, XPLMCommandPhase phase, void *ref) {
    if (phase != xplm_CommandBegin)
        return 1;
    if (yoke_control_enabled) {
        if (change_cursor)
            set_cursor_bmp(CURSOR_ARROW);
        yoke_control_enabled = 0;
        rudder_control = 0;
        look_control = 0;
        XPLMSetWindowIsVisible(cursor_win, 0);
#if LIN
    deselect_events(dpy, window);
#endif // LIN
    } else {
#if LIN
    select_events(dpy, window);
#endif // LIN
        /* Fetch screen dimensions here because doing it from XPluginEnable
           give unrealiable results. Also the screen size may be changed by
           the user at any time. */
        XPLMGetScreenSize(&screen_width, &screen_height);
        _log("screen_width %d", screen_width);
        _log("screen_height %d", screen_height);
        /* Set cursor position to align with current deflection of yoke. */
        if (set_pos)
            set_cursor_from_yoke();
        //if (change_cursor)
        set_cursor_bmp(CURSOR_YOKE);
        int left, top, right, bottom;
        XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
        XPLMSetWindowGeometry(cursor_win, left, top, right, bottom);
        XPLMSetWindowIsVisible(cursor_win, 1);
        yoke_control_enabled = 1;
        XPLMScheduleFlightLoop(loop_id, -1.0f, 0);
    }
    return 1;
}

int draw_cb(XPLMDrawingPhase phase, int before, void *ref) {
    /* Show a little text indication in top left corner of screen. */
    if (yoke_control_enabled) {
        XPLMDrawString(magenta, 20, screen_height - 40, look_control ?
            "MOUSE LOOK" : rudder_control ?
            "MOUSE RUDDER CONTROL" : "MOUSE YOKE CONTROL",
            NULL, xplmFont_Proportional);
        if (rudder_control) {
            /* Draw little bars to indicate maximum rudder deflection. */
            for (int i = 1; i < 3; i++) {
                XPLMDrawString(green, cursor_pos[0] - rudder_defl_dist,
                    cursor_pos[1] + 4 - 7 * i, "|", NULL, xplmFont_Basic);
                XPLMDrawString(green, cursor_pos[0] + rudder_defl_dist,
                    cursor_pos[1] + 4 - 7 * i, "|", NULL, xplmFont_Basic);
            }
        }
    }
    return 1;
}

float loop_cb(float last_call, float last_loop, int count, void *ref) {
    static long long _last_time;
    /* If user has disabled mouse yoke control, suspend loop. */
    if (yoke_control_enabled == 0) {
        /* If rudder is still deflected, move it gradually back to zero. */
        if (yaw_ratio != 0 && rudder_return) {
            long long now = get_time_ms();
            float dt = (now - _last_time) / 1000.0f;
            _last_time = now;
            yaw_ratio = yaw_ratio > 0 ? max(0, yaw_ratio - dt * rudder_ret_spd) :
                min(0, yaw_ratio + dt * rudder_ret_spd);
            XPLMSetDataf(yoke_heading_ratio, yaw_ratio);
            /* Call us again next frame until zero. */
            return yaw_ratio ? -1.0f : 0;
        }
        /* Don't call us anymore. */
        return 0;
    }
    /* Yoke and rudder stay where they are while looking around. */
    if (look_control) {
        update_look();
        return -1.0f;
    }
    int m_x, m_y;
    get_cursor_pos(&m_x, &m_y);
    if (controlling_rudder(&m_x, &m_y)) {
        int dist = min(max(m_x - cursor_pos[0], -rudder_defl_dist),
            rudder_defl_dist);
        _last_time = get_time_ms();
        /* Save value so we don't have to continuously query the dr above. */
        yaw_ratio = dist / (float)rudder_defl_dist;
        XPLMSetDataf(yoke_heading_ratio, yaw_ratio);
    } else {
        float yoke_roll = 2 * (m_x / (float)screen_width) - 1;
        float yoke_pitch = 1 - 2 * (m_y / (float)screen_height);
        XPLMSetDataf(yoke_roll_ratio, yoke_roll);
        XPLMSetDataf(yoke_pitch_ratio, yoke_pitch);
        /* If rudder is still deflected, move it gradually back to zero. */
        if (yaw_ratio != 0 && rudder_return) {
            long long now = get_time_ms();
            float dt = (now - _last_time) / 1000.0f;
            _last_time = now;
            yaw_ratio = yaw_ratio > 0 ? max(0, yaw_ratio - dt * rudder_ret_spd) :
                min(0, yaw_ratio + dt * rudder_ret_spd);
            XPLMSetDataf(yoke_heading_ratio, yaw_ratio);
        }
    }
    /* Call us again next frame. */
    return -1.0f;
}

void update_look() {
    /* Outside views have no pilot's head to turn, and writing its datarefs
       switches back to the cockpit. So stop looking if the user switched to
       an outside view meanwhile. */
    if (XPLMGetDatai(view_is_external)) {
        set_cursor_pos(look_pos[0], look_pos[1]);
        look_control = 0;
        return;
    }
    int m_x, m_y;
    get_cursor_pos(&m_x, &m_y);
    /* Turn the pilot's head relative to where it was when the right mouse
       button went down. Pitch is clamped so the view can't flip over. */
    float psi = look_start[0] + (m_x - look_pos[0]) * look_sens;
    float the = look_start[1] + (m_y - look_pos[1]) * look_sens;
    XPLMSetDataf(head_psi, psi);
    XPLMSetDataf(head_the, min(max(the, -89.0f), 89.0f));
}

int left_mouse_down() {
#if IBM
    /* Most significant bit is set if button is being held. */
    return GetAsyncKeyState(VK_LBUTTON) >> 15;
#elif APL
    /* Apparently you can use this also outside of the context of an event. */
    return CGEventSourceButtonState(
        kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
#elif LIN

static int mouse_but = 0;

    XGenericEventCookie *cookie = &ev.xcookie;

    XCheckTypedEvent(dpy, GenericEvent ,&ev);

    if (cookie->type != GenericEvent ||
        !XGetEventData(dpy, cookie))
        return mouse_but;

    xi_event = (XIEvent *) cookie->data;
    xev = (XIRawEvent *) xi_event;

    if(xev->detail == 1)
    {
        switch (cookie->evtype) {
            case XI_RawButtonPress:
                mouse_but = 1;
                break;
            case XI_RawButtonRelease:
                mouse_but = 0;
                break;
            }

        XFreeEventData(dpy, cookie);
    }
    return mouse_but;
#endif
}

int controlling_rudder(int *x, int *y) {
    if (left_mouse_down()) {
        /* Transitioning into rudder control */
        if (!rudder_control) {
            if (change_cursor)
                set_cursor_bmp(CURSOR_RUDDER);
            /* Remember current cursor position. */
            XPLMGetMouseLocation(cursor_pos, cursor_pos + 1);
            /* Set rudder cursor position, if enabled. */
            if (set_rudder_pos) {
                *x = *x + yaw_ratio * rudder_defl_dist;
                set_cursor_pos(*x, *y);
            }
            rudder_control = 1;
        }
    } else {
        /* Transitioning out of rudder control. */
        if (rudder_control) {
            if (change_cursor)
                set_cursor_bmp(CURSOR_YOKE);
            /* Restore previous cursor position */
            set_cursor_pos(cursor_pos[0], cursor_pos[1]);
            *x = cursor_pos[0];
            *y = cursor_pos[1];
            rudder_control = 0;
        }
    }
    return rudder_control;
}

void get_cursor_pos(int *x, int *y) {
#if APL
    /* On OSX, XPLMGetMouseLocationGlobal still returns old cursor location after
        setting its position for whatever reason, so we query the cursor position
        ourselves. */
    CGEventRef ev = CGEventCreate(NULL);
    CGPoint pt = CGEventGetLocation(ev);
    CFRelease(ev);
    *x = pt.x;
    *y = screen_height - pt.y;
#else
    XPLMGetMouseLocation(x, y);
#endif
}

void set_cursor_from_yoke() {
    set_cursor_pos(
        0.5 * screen_width  * (XPLMGetDataf(yoke_roll_ratio) + 1),
        0.5 * screen_height * (1 - XPLMGetDataf(yoke_pitch_ratio))
    );
}

void set_cursor_pos(int x, int y) {
#if IBM
    POINT pt = {
        .x = x,
        /* On windows (0,0) is the upper-left corner. */
        .y = screen_height - y
    };
    ClientToScreen(xp_hwnd, &pt);
    SetCursorPos(pt.x, pt.y);
#elif APL
    CGPoint pt = {
        .x = x,
        .y = screen_height - y
    };
    /* CGWarpMouseCursorPosition and CGDisplayMoveCursorToPoint don't generate a mouse
        movement event so they're not a good fit here. */
    CGEventRef ev = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, pt, 0);
    CGEventPost(kCGHIDEventTap, ev);
    CFRelease(ev);
#elif LIN
    _log("Cursor x = %d", x);
    _log("Cursor y = %d", screen_height - y);
    /* Xwayland ignores pointer warps while the cursor is visible. Hiding it
       around the warp makes Xwayland emulate the warp with a locked pointer
       and a cursor position hint, which the compositor applies when the
       cursor is shown again. On a plain X server this is harmless. */
    XFixesHideCursor(dpy, window);
    XWarpPointer(dpy, None, window, 0, 0, 0, 0, x, screen_height - y);
    XFixesShowCursor(dpy, window);
    XFlush(dpy);
#endif
}

void set_cursor_bmp(cursor_t cursor) {
    /* X-Plane picks up the change via win_cursor_cb. */
    current_cursor = cursor;
}

void win_draw_cb(XPLMWindowID id, void *ref) {
    /* Nothing to draw, the window is invisible. */
}

int win_click_cb(XPLMWindowID id, int x, int y, XPLMMouseStatus status,
    void *ref) {
    /* Make sure keystrokes keep going to X-Plane. */
    if (XPLMHasKeyboardFocus(id))
        XPLMTakeKeyboardFocus(0);
    /* Swallow left clicks. Rudder control reads the button state directly
       (see left_mouse_down), and the SDK warns against passing clicks
       through windows. */
    return 1;
}

int win_right_click_cb(XPLMWindowID id, int x, int y, XPLMMouseStatus status,
    void *ref) {
    if (XPLMHasKeyboardFocus(id))
        XPLMTakeKeyboardFocus(0);
    if (status == xplm_MouseDown) {
        /* There's no pilot's head to turn in outside views, so pass the
           click on to X-Plane, where right-drag moves the camera. */
        right_click_passed = XPLMGetDatai(view_is_external);
        if (right_click_passed)
            return 0;
        /* Remember cursor position and head angles to look relative to. */
        get_cursor_pos(look_pos, look_pos + 1);
        look_start[0] = XPLMGetDataf(head_psi);
        look_start[1] = XPLMGetDataf(head_the);
        look_control = 1;
    } else if (right_click_passed) {
        /* Keep passing on the rest of a click X-Plane got. */
        return 0;
    } else if (status == xplm_MouseUp && look_control) {
        /* Put the cursor back so the yoke continues where it was. */
        set_cursor_pos(look_pos[0], look_pos[1]);
        look_control = 0;
    }
    return 1;
}

void win_key_cb(XPLMWindowID id, char key, XPLMKeyFlags flags, char vkey,
    void *ref, int losing_focus) {
}

XPLMCursorStatus win_cursor_cb(XPLMWindowID id, int x, int y, void *ref) {
    /* The built-in cursors need X-Plane 12.3 or newer. Older versions
       don't know them, so stick with the arrow there. */
    if (!builtin_cursors)
        return xplm_CursorArrow;
    switch (current_cursor) {
    case CURSOR_YOKE:
        return xplm_CursorFourArrows;
    case CURSOR_RUDDER:
        return xplm_CursorLeftRight;
    default:
        return xplm_CursorArrow;
    }
}

int win_wheel_cb(XPLMWindowID id, int x, int y, int wheel, int clicks,
    void *ref) {
    /* Pass the mouse wheel on to X-Plane so cockpit knobs still work. */
    return 0;
}

#if LIN

static int has_xi2(Display *dpy)
{
    int major, minor;
    int rc;

    /* We support XI 2.2 */
    major = 2;
    minor = 2;

    rc = XIQueryVersion(dpy, &major, &minor);
    if (rc == BadRequest) {
    _log("No XI2 support.\n");
    return 0;
    } else if (rc != Success) {
    _log("Internal Error! This is a bug in Xlib.\n");
    }

    _log("XI2 supported.\n");

    return 1;
}

static void select_events(Display *dpy, Window win)
{
    XIEventMask evmasks[1];
    unsigned char mask1[(XI_LASTEVENT + 7)/8];

    memset(mask1, 0, sizeof(mask1));

    /* select for button and key events from all master devices */
    XISetMask(mask1, XI_RawButtonPress);
    XISetMask(mask1, XI_RawButtonRelease);

    evmasks[0].deviceid = XIAllMasterDevices;
    evmasks[0].mask_len = sizeof(mask1);
    evmasks[0].mask = mask1;

    XISelectEvents(dpy, win, evmasks, 1);
    XFlush(dpy);
}

static void deselect_events(Display *dpy, Window win)
{
    XIEventMask evmasks[1];
    unsigned char mask1[(XI_LASTEVENT + 7)/8];

    memset(mask1, 0, sizeof(mask1));

    /* select for button and key events from all master devices */
    XISetMask(mask1, 0);

    evmasks[0].deviceid = XIAllMasterDevices;
    evmasks[0].mask_len = sizeof(mask1);
    evmasks[0].mask = mask1;

    XISelectEvents(dpy, win, evmasks, 1);
    XFlush(dpy);
}

#endif
