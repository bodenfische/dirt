/* dirt.c - a very basic file manager built on tint.
 * Browse folders: double-click / Enter on a folder to open it, "Up" to go to the parent.
 * Each row shows type + size + name. Read-only: it never changes your files. */
#define _GNU_SOURCE
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tint.h"
#include "fonts.h"

typedef struct { char name[NAME_MAX + 1]; int is_dir; long long size; } Entry;

static char    cwd[PATH_MAX];
static Entry  *entries;
static char  **labels;        /* strings handed to tint_list */
static int     count, sel = -1;

static int cmp(const void *a, const void *b) {
    const Entry *x = a, *y = b;
    if (x->is_dir != y->is_dir) return y->is_dir - x->is_dir;   /* folders first */
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

static void load(const char *path) {
    DIR *d = opendir(path);
    if (!d) return;                                  /* e.g. permission denied: stay where we are */
    if (!realpath(path, cwd)) snprintf(cwd, sizeof cwd, "%s", path);
    clear_list();

    int cap = 64;
    entries = malloc(cap * sizeof *entries);
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;           /* hide dotfiles (and . / ..) */
        if (count == cap) { cap *= 2; entries = realloc(entries, cap * sizeof *entries); }
        Entry *en = &entries[count++];
        snprintf(en->name, sizeof en->name, "%s", e->d_name);
        char full[PATH_MAX + NAME_MAX + 2];
        snprintf(full, sizeof full, "%s/%s", cwd, en->name);
        struct stat st;
        if (stat(full, &st) == 0) { en->is_dir = S_ISDIR(st.st_mode); en->size = st.st_size; }
        else                      { en->is_dir = 0; en->size = 0; }
    }
    closedir(d);
    qsort(entries, count, sizeof *entries, cmp);

    labels = malloc((count ? count : 1) * sizeof *labels);
    for (int i = 0; i < count; i++) {
        char sz[32], line[NAME_MAX + 48];
        if (entries[i].is_dir) sz[0] = 0;
        else human(entries[i].size, sz, sizeof sz);
        snprintf(line, sizeof line, "%-5s %9s  %s",
                 entries[i].is_dir ? "DIR" : "FILE", sz, entries[i].name);
        labels[i] = strdup(line);
    }
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

    if (argc > 1) load(argv[1]);
    if (!cwd[0]) { char *h = getenv("HOME"); load(h ? h : "/"); }
    if (!cwd[0]) load("/");

    while (tint_begin()) {
        tint_row(2, (int[]){ 80, 0 });
        if (tint_button("Up")) go_up();
        tint_label(cwd);

        tint_fill_rest = 1; tint_h = 20;             /* list fills window, leave room for status line */
        int r = tint_list((const char **)labels, count, &sel, 10);
        if ((r & TINT_SUBMIT) && sel >= 0 && sel < count && entries[sel].is_dir) {
            char next[PATH_MAX + NAME_MAX + 2];
            snprintf(next, sizeof next, "%s/%s", cwd, entries[sel].name);
            load(next);
        }

        if (sel >= 0 && sel < count) tint_labelf("%d items - selected: %s", count, entries[sel].name);
        else                         tint_labelf("%d items", count);
        tint_end();
    }
    tint_close();
    clear_list();
    return 0;
}
