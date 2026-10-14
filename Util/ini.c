/**
 * Utility library for X-Plane 11 Plugins.
 *
 * Static library containing common functionality for stuff like logging and
 * dealing with configuration files. Linked against by most plugins in the
 * solution.
 *
 * Copyright 2019 Torben K�nke.
 */
#include "util.h"

static char ini_path[MAX_PATH];

#define INI_FILE_NAME "settings.ini"
#define INI_SECT_NAME "settings"

static char *ini_get_path() {
    if (ini_path[0])
        return ini_path;
    if (!get_plugin_dir(ini_path, MAX_PATH)) {
        _log("ini_get_path: could not get plugin dir");
        return NULL;
    }
    return strcat(ini_path, INI_FILE_NAME);
}

#if !IBM
/* Returns 1 if line assigns a value to name, i.e. reads "name = ...". This
   keeps e.g. "rudder_return" from matching "rudder_return_speed". */
static int ini_is_key(const char *line, const char *name) {
    size_t len = strlen(name);
    while (*line == ' ' || *line == '\t')
        line++;
    if (strncmp(line, name, len))
        return 0;
    line += len;
    while (*line == ' ' || *line == '\t')
        line++;
    return *line == '=';
}

/* Returns 1 if line is the header of our section. */
static int ini_is_section(const char *line) {
    const char *header = "[" INI_SECT_NAME "]";
    while (*line == ' ' || *line == '\t')
        line++;
    return !strncmp(line, header, strlen(header));
}
#endif

int ini_geti(const char *name, int def) {
#if IBM
    return GetPrivateProfileIntA(INI_SECT_NAME, name, def,
        ini_get_path());
#else
    char buf[32];
    ini_gets(name, buf, 32, "nan");
    if (!strcmp(buf, "nan"))
        return def;
    return atoi(buf);
#endif
}

int ini_seti(const char *name, int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%i", val);
    return ini_sets(name, buf);
}

float ini_getf(const char *name, float def) {
    char buf[32];
    ini_gets(name, buf, 32, "nan");
    if (!strcmp(buf, "nan"))
        return def;
    return atof(buf);
}

int ini_setf(const char *name, float val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%g", val);
    return ini_sets(name, buf);
}

void ini_gets(const char *name, char *buf, int size, const char *def) {
    char *f = ini_get_path();
#if IBM
    GetPrivateProfileStringA(INI_SECT_NAME, name, def, buf, size, f);
#else
    FILE *fp = fopen(f, "r");
    if (!fp) {
        strcpy(buf, def);
        return;
    }
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p && (*p == ' ' || *p == '\t'))
            p++;
        if (*p == '[' || *p == ';')
            continue;
        if (!ini_is_key(p, name))
            continue;
        char n[128] = { 0 };
        char *q = strchr(p, '=') + 1;
        while (*q && (*q == ' ' || *q == '\t'))
            q++;
        strcpy(n, q);
        if((q = strrchr(n, ';')))
            *q = '\0';
        else {
            q = n + strlen(n) - 1;
            if (*q == '\n') {
                *q = '\0';
                q--;
                /* remove carriage if it's a windows file */
                if (*q == '\r')
                    *q = '\0';
            }
        }
        strcpy(buf, n);
        fclose(fp);
        return;
    }
    fclose(fp);
    strcpy(buf, def);
#endif
}

int ini_sets(const char *name, const char *val) {
    char *f = ini_get_path();
    if (!f)
        return 0;
#if IBM
    return WritePrivateProfileStringA(INI_SECT_NAME, name, val, f);
#else
    /* Read the whole file so we can write it back with only this setting
       changed, keeping comments and everything else as is. */
    char *data = NULL;
    long size = 0;
    FILE *fp = fopen(f, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        data = malloc(size + 1);
        if (!data) {
            fclose(fp);
            return 0;
        }
        size = fread(data, 1, size, fp);
        data[size] = '\0';
        fclose(fp);
    }
    int found = 0;
    for (char *line = data; line && *line && !found; ) {
        found = ini_is_key(line, name);
        line = strchr(line, '\n');
        if (line)
            line++;
    }
    /* Write to a temporary file first so a failed write can't destroy the
       existing settings. */
    char tmp[MAX_PATH + 8];
    snprintf(tmp, sizeof(tmp), "%s.tmp", f);
    fp = fopen(tmp, "wb");
    if (!fp) {
        free(data);
        return 0;
    }
    const char *eol = data && strstr(data, "\r\n") ? "\r\n" : "\n";
    int written = 0;
    for (char *line = data; line && *line; ) {
        char *next = strchr(line, '\n');
        size_t len = next ? (size_t)(next - line + 1) : strlen(line);
        if (!written && ini_is_key(line, name)) {
            fprintf(fp, "%s = %s%s", name, val, eol);
            written = 1;
        } else {
            fwrite(line, 1, len, fp);
            /* Add a new setting right below the section header. */
            if (!written && !found && ini_is_section(line)) {
                if (!next)
                    fputs(eol, fp);
                fprintf(fp, "%s = %s%s", name, val, eol);
                written = 1;
            }
        }
        line = next ? next + 1 : NULL;
    }
    if (!written) {
        /* There's no section header yet, so start one. */
        if (size > 0 && data[size - 1] != '\n')
            fputs(eol, fp);
        fprintf(fp, "[%s]%s%s = %s%s", INI_SECT_NAME, eol, name, val, eol);
    }
    free(data);
    int err = ferror(fp);
    if (fclose(fp) || err || rename(tmp, f)) {
        _log("ini_sets: could not write %s", f);
        remove(tmp);
        return 0;
    }
    return 1;
#endif
}

int ini_write_default(const char *contents) {
    char *f = ini_get_path();
    if (!f)
        return 0;
    /* Leave an existing file alone. */
    FILE *fp = fopen(f, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    /* Text mode gives the file native line endings. */
    fp = fopen(f, "w");
    if (!fp) {
        _log("ini_write_default: could not create %s", f);
        return 0;
    }
    fputs(contents, fp);
    return fclose(fp) == 0;
}
