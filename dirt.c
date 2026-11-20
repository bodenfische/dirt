/* dirt.c - a very basic file manager built on tint.
 * Browse folders: double-click / Enter on a folder to open it, "Up" to go to the parent.
 * Each row shows type + size + name. Read-only: it never changes your files.
 *
 * Copyright (C) 2026 bodenfische
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Built on tint (https://github.com/FacePafes/tint), LGPL-3.0-or-later. */
#define _GNU_SOURCE
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "tint.h"
#include "fonts.h"

typedef struct { char name[NAME_MAX + 1]; int is_dir; long long size; time_t mtime; } Entry;

enum { SORT_NAME, SORT_SIZE, SORT_DATE };

static char    cwd[PATH_MAX];
static char    pathbuf[PATH_MAX];     /* text in the path bar */
static char    status_err[128];       /* shown in the status line when something fails */
static Entry  *entries;
static char  **labels;                /* strings handed to tint_list */
static int     count, sel = -1;
static int     show_hidden = 0;
static int     sort_mode = SORT_NAME;

static int cmp(const void *a, const void *b) {
    const Entry *x = a, *y = b;
    if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;   /* folders first, always */
    if (sort_mode == SORT_SIZE && x->size != y->size)   return x->size < y->size ? 1 : -1;     /* biggest first */
    if (sort_mode == SORT_DATE && x->mtime != y->mtime) return x->mtime < y->mtime ? 1 : -1;   /* newest first */
    return strcasecmp(x->name, y->name);
}

static void human(long long n, char *out, size_t cap) {
    const char *u[] = { "B", "K", "M", "G", "T" };
    double v = (double)n; int i = 0;
    while (v >= 1024 && i < 4) { v /= 1024; i++; }
    if (i == 0) snprintf(out, cap, "%lld B", n);
    else        snprintf(out, cap, "%.1f %s", v, u[i]);
}

static void clear_list(void) {
    for (int i = 0; i < count; i++) free(labels[i]);
    free(labels); free(entries);
    labels = NULL; entries = NULL; count = 0; sel = -1;
}

/* Reads a folder into the list. Returns 1 on success; on failure nothing changes except status_err. */
static int load(const char *path) {
    char target[PATH_MAX], home_expanded[PATH_MAX];
    if (path[0] == '~' && (path[1] == '/' || path[1] == 0) && getenv("HOME")) {
        snprintf(home_expanded, sizeof home_expanded, "%s%s", getenv("HOME"), path + 1);
        path = home_expanded;
    }
    snprintf(target, sizeof target, "%s", path);          /* path may point at cwd itself */

    DIR *d = opendir(target);
    if (!d) {
        snprintf(status_err, sizeof status_err, "Cannot open: %.90s", target);
        snprintf(pathbuf, sizeof pathbuf, "%s", cwd);     /* put the bar back */
        return 0;
    }
    status_err[0] = 0;
    if (!realpath(target, cwd)) snprintf(cwd, sizeof cwd, "%s", target);
    snprintf(pathbuf, sizeof pathbuf, "%s", cwd);
    clear_list();

    int cap = 64;
    entries = malloc(cap * sizeof *entries);
    struct dirent *e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (e->d_name[0] == '.' && !show_hidden) continue;
        if (count == cap) { cap *= 2; entries = realloc(entries, cap * sizeof *entries); }
        Entry *en = &entries[count++];
        snprintf(en->name, sizeof en->name, "%s", e->d_name);
        char full[PATH_MAX + NAME_MAX + 2];
        snprintf(full, sizeof full, "%s/%s", cwd, en->name);
        struct stat st;
        if (stat(full, &st) == 0) { en->is_dir = S_ISDIR(st.st_mode); en->size = st.st_size; en->mtime = st.st_mtime; }
        else                      { en->is_dir = 0; en->size = 0; en->mtime = 0; }
    }
    closedir(d);
    qsort(entries, count, sizeof *entries, cmp);

    labels = malloc((count ? count : 1) * sizeof *labels);
    for (int i = 0; i < count; i++) {
        char sz[32], when[24], line[NAME_MAX + 64];
        if (entries[i].is_dir) sz[0] = 0;
        else human(entries[i].size, sz, sizeof sz);
        struct tm tm;
        localtime_r(&entries[i].mtime, &tm);
        strftime(when, sizeof when, "%Y-%m-%d %H:%M", &tm);
        snprintf(line, sizeof line, "%-5s %s %9s  %s",
                 entries[i].is_dir ? "DIR" : "FILE", when, sz, entries[i].name);
        labels[i] = strdup(line);
    }
    return 1;
}

static void go_up(void) {
    char parent[PATH_MAX + 4];
    snprintf(parent, sizeof parent, "%s/..", cwd);
    load(parent);
}

int main(int argc, char **argv) {
    tint_set_font(&tint_font_eu);                    /* Latin/Greek/Cyrillic so UTF-8 names show */
    tint_list_activate = 1;                          /* double-click / Enter => TINT_SUBMIT */
    if (tint_open(640, 480, "dirt")) return 1;

    const char *home = getenv("HOME") ? getenv("HOME") : "/";
    if (argc > 1) load(argv[1]);
    if (!cwd[0]) load(home);
    if (!cwd[0]) load("/");

    const char *sort_names[] = { "Name", "Size", "Date" };

    while (tint_begin()) {
        /* path bar: Up / Home / editable path (Enter to go) */
        tint_row(3, (int[]){ 60, 70, 0 });
        if (tint_button("Up")) go_up();
        if (tint_button("Home")) load(home);
        if (tint_input(pathbuf, sizeof pathbuf) & TINT_SUBMIT) load(pathbuf);

        /* sort + hidden files */
        if (tint_tabs(sort_names, 3, &sort_mode)) load(cwd);
        if (tint_check("Show hidden files", &show_hidden)) load(cwd);

        tint_fill_rest = 1; tint_h = 20;             /* list fills window, leave room for status line */
        int r = tint_list((const char **)labels, count, &sel, 10);
        if ((r & TINT_SUBMIT) && sel >= 0 && sel < count && entries[sel].is_dir) {
            char next[PATH_MAX + NAME_MAX + 2];
            snprintf(next, sizeof next, "%s/%s", cwd, entries[sel].name);
            load(next);
        }

        if (status_err[0])           tint_label(status_err);
        else if (sel >= 0 && sel < count) tint_labelf("%d items - selected: %s", count, entries[sel].name);
        else                         tint_labelf("%d items", count);
        tint_end();
    }
    tint_close();
    clear_list();
    return 0;
}