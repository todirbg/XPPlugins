/**
 * Utility library for X-Plane 11 Plugins.
 *
 * Static library containing common functionality for stuff like logging and
 * dealing with configuration files. Linked against by most plugins in the
 * solution.
 *
 * Copyright 2019 Torben K�nke.
 */
#ifndef _UTIL_H_
#define _UTIL_H_

// #define XPLM200
// #define XPLM210
// #define XPLM300
#ifdef WIN32
    #define _CRT_SECURE_NO_WARNINGS
#endif /* _WIN32 */

#include "XPLMUtilities.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMPlanes.h"
#include "XPLMMenus.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef WIN32
#include <time.h>
#define _stricmp strcasecmp
#endif /* _WIN32 */
#if APL
    #include <ApplicationServices/ApplicationServices.h>
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif /* max */
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif /* min */

#ifndef MAX_PATH
#define MAX_PATH 512
#endif
#ifndef MAX_NAME
#define MAX_NAME 256
#endif

/* ini */
int ini_geti(const char *name, int def);
int ini_seti(const char *name, int val);
int ini_setf(const char *name, float val);
float ini_getf(const char *name, float def);
void ini_gets(const char *name, char *buf, int size, const char *def);
int ini_sets(const char *name, const char *val);

/* log */
void _log(const char *fmt, ...);
void _debug(const char *fmt, ...);

/* path */
int get_plugin_dir(char *buf, int size);
int get_plugin_name(char *buf, int size);
int get_acft_dir(char *buf, int size);
int get_data_path(const char *file, char *buf, int size);

/* snd */
typedef void* snd_t;
typedef enum {
    SND_VOL_MASTER,
    SND_VOL_EXTERIOR,
    SND_VOL_INTERIOR,
    SND_VOL_COPILOT,
    SND_VOL_RADIO,
    SND_VOL_ENV,
    SND_VOL_UI
} snd_vol_t;
int snd_init();
int snd_deinit();
snd_t snd_create(const char *file);
int snd_free(snd_t s);
int snd_play(snd_t s, snd_vol_t vol);

/* cmd */
XPLMCommandRef cmd_create(const char *name, const char *desc,
    XPLMCommandCallback_f cb, void *data);
void cmd_free(XPLMCommandRef *cmd, XPLMCommandCallback_f cb, void *data);

/* time */
long long get_time_ms();

/* menu */
#define MAX_MENU_ITEMS 16
typedef struct {
    const char *name;
    const char *ini_name;
    int *var;
    int value;
    void *ref;
} menu_item_t;
int menu_init(const char *name, menu_item_t *items, int num);
int menu_deinit();

#endif /* _UTIL_H_ */
