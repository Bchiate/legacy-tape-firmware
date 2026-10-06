#include "book.h"
#include <Arduino.h>
#include <Preferences.h>

static Preferences prefs;
static const char *NS = "ltbook";

static char s_name[BOOK_MAX_NAME_LEN + 1] = {0};
static int  s_chapter_count                = 0;
static char s_chapter_names[BOOK_MAX_CHAPTERS][BOOK_MAX_CHAPTER_LEN + 1] = {{0}};
static int  s_active_chapter               = 0;

static void persist_all(void) {
    prefs.begin(NS, false);
    prefs.putString("name", s_name);
    prefs.putInt("nch", s_chapter_count);
    prefs.putInt("act", s_active_chapter);
    for (int i = 0; i < s_chapter_count; i++) {
        char key[8];
        snprintf(key, sizeof(key), "ch%d", i);
        prefs.putString(key, s_chapter_names[i]);
    }
    prefs.end();
}

void book_begin(void) {
    prefs.begin(NS, true);   // read-only

    String n = prefs.getString("name", "");
    strncpy(s_name, n.c_str(), BOOK_MAX_NAME_LEN);
    s_name[BOOK_MAX_NAME_LEN] = 0;

    s_chapter_count = prefs.getInt("nch", 0);
    if (s_chapter_count < 0) s_chapter_count = 0;
    if (s_chapter_count > BOOK_MAX_CHAPTERS) s_chapter_count = BOOK_MAX_CHAPTERS;

    for (int i = 0; i < s_chapter_count; i++) {
        char key[8];
        snprintf(key, sizeof(key), "ch%d", i);
        String cn = prefs.getString(key, "");
        strncpy(s_chapter_names[i], cn.c_str(), BOOK_MAX_CHAPTER_LEN);
        s_chapter_names[i][BOOK_MAX_CHAPTER_LEN] = 0;
    }

    s_active_chapter = prefs.getInt("act", 0);
    if (s_active_chapter < 0 || s_active_chapter >= s_chapter_count) s_active_chapter = 0;
    prefs.end();

    // Bootstrap: always have at least one chapter
    if (s_chapter_count == 0) {
        s_chapter_count = 1;
        strncpy(s_chapter_names[0], "Chapter 1", BOOK_MAX_CHAPTER_LEN);
        persist_all();
    }

    Serial.printf("[book] loaded: name='%s' chapters=%d active=%d\n",
                  s_name, s_chapter_count, s_active_chapter);
}

const char *book_get_name(void) {
    return (s_name[0] != 0) ? s_name : "My Stories";
}

bool book_has_name(void) {
    return s_name[0] != 0;
}

void book_set_name(const char *name) {
    if (!name) return;
    strncpy(s_name, name, BOOK_MAX_NAME_LEN);
    s_name[BOOK_MAX_NAME_LEN] = 0;
    prefs.begin(NS, false);
    prefs.putString("name", s_name);
    prefs.end();
    Serial.printf("[book] name set: '%s'\n", s_name);
}

int book_get_chapter_count(void) { return s_chapter_count; }

const char *book_get_chapter_name(int idx) {
    if (idx < 0 || idx >= s_chapter_count) return nullptr;
    return s_chapter_names[idx];
}

int book_add_chapter(void) {
    if (s_chapter_count >= BOOK_MAX_CHAPTERS) return -1;
    int idx = s_chapter_count;
    snprintf(s_chapter_names[idx], BOOK_MAX_CHAPTER_LEN + 1, "Chapter %d", idx + 1);
    s_chapter_count++;
    persist_all();
    Serial.printf("[book] added '%s' (now %d chapters)\n", s_chapter_names[idx], s_chapter_count);
    return idx;
}

int  book_get_active_chapter(void)        { return s_active_chapter; }
void book_set_active_chapter(int idx) {
    if (idx < 0 || idx >= s_chapter_count) return;
    s_active_chapter = idx;
    prefs.begin(NS, false);
    prefs.putInt("act", s_active_chapter);
    prefs.end();
}
