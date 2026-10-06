// Legacy Tape — book + chapter persistence (NVS).
//
// Stores the name the user types on Screen3's keyboard plus the list of
// chapters. Eventually this will be the local cache of what Supabase says,
// but for now it's authoritative on the device side.
#ifndef LEGACYTAPE_BOOK_H
#define LEGACYTAPE_BOOK_H

#ifdef __cplusplus
extern "C" {
#endif

#define BOOK_MAX_NAME_LEN     48
#define BOOK_MAX_CHAPTERS     32
#define BOOK_MAX_CHAPTER_LEN  32

// Call once at boot to load whatever's stored. Safe to call multiple times.
void book_begin(void);

// Book name — what's printed on the cassette label.
// If never set, returns "My Stories".
const char *book_get_name(void);
void        book_set_name(const char *name);

// True if the user has explicitly set a book name (vs. the default fallback).
// Used by Screen8 to decide whether to show a real book tile.
bool        book_has_name(void);

// Chapter list. Index 0 = first chapter.
// On fresh boot returns 1 chapter named "Chapter 1".
int         book_get_chapter_count(void);
const char *book_get_chapter_name(int idx);     // null if idx out of range

// Add a new chapter with default name "Chapter N". Returns the new index,
// or -1 if BOOK_MAX_CHAPTERS reached. Persisted to NVS immediately.
int  book_add_chapter(void);

// Set the active (currently-playing/recording) chapter index. Persisted.
int  book_get_active_chapter(void);
void book_set_active_chapter(int idx);

#ifdef __cplusplus
}
#endif
#endif
