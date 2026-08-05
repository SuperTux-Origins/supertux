// SPDX-FileCopyrightText: 1998-2000 Mark Probst
// SPDX-FileCopyrightText: 2002 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_LISPREADER_H
#define SUPERTUX_LISPREADER_H

#include <stdio.h>
#include <zlib.h>
#include <string>
#include <vector>

#define LISP_STREAM_FILE       1
#define LISP_STREAM_STRING     2
#define LISP_STREAM_ANY        3

#define LISP_TYPE_INTERNAL      -3
#define LISP_TYPE_PARSE_ERROR   -2
#define LISP_TYPE_EOF           -1
#define LISP_TYPE_NIL           0
#define LISP_TYPE_SYMBOL        1
#define LISP_TYPE_INTEGER       2
#define LISP_TYPE_STRING        3
#define LISP_TYPE_REAL          4
#define LISP_TYPE_CONS          5
#define LISP_TYPE_PATTERN_CONS  6
#define LISP_TYPE_BOOLEAN       7
#define LISP_TYPE_PATTERN_VAR   8

#define LISP_PATTERN_ANY        1
#define LISP_PATTERN_SYMBOL     2
#define LISP_PATTERN_STRING     3
#define LISP_PATTERN_INTEGER    4
#define LISP_PATTERN_REAL       5
#define LISP_PATTERN_BOOLEAN    6
#define LISP_PATTERN_LIST       7
#define LISP_PATTERN_OR         8

typedef struct
  {
    int type;

    union
      {
        FILE *file;
        struct
          {
            char *buf;
            int pos;
          }
        string;
        struct
          {
            void *data;
            int (*next_char) (void *data);
            void (*unget_char) (char c, void *data);
          }
        any;
      } v;
  }
lisp_stream_t;

typedef struct _lisp_object_t lisp_object_t;
struct _lisp_object_t
  {
    int type;

    union
      {
        struct
          {
            struct _lisp_object_t *car;
            struct _lisp_object_t *cdr;
          }
        cons;

        char *string;
        int integer;
        float real;

        struct
          {
            int type;
            int index;
            struct _lisp_object_t *sub;
          }
        pattern;
      } v;
  };

lisp_stream_t* lisp_stream_init_gzfile (lisp_stream_t *stream, gzFile file);
lisp_stream_t* lisp_stream_init_file (lisp_stream_t *stream, FILE *file);
lisp_stream_t* lisp_stream_init_string (lisp_stream_t *stream, char *buf);
lisp_stream_t* lisp_stream_init_any (lisp_stream_t *stream, void *data,
                                     int (*next_char) (void *data),
                                     void (*unget_char) (char c, void *data));

lisp_object_t* lisp_read (lisp_stream_t *in);
lisp_object_t* lisp_read_from_file(const std::string& filename);
void lisp_free (lisp_object_t *obj);

/**
 * True if obj is a non-nil cons whose car is a symbol equal to expected_root
 * (e.g. "supertux-level"). Safe under NDEBUG — never calls lisp_symbol/car
 * without type checks (those use assert and SIGSEGV in release on bad data).
 */
int lisp_expect_symbol_root(lisp_object_t* obj, const char* expected_root);

/**
 * True if obj is a non-nil cons whose car is a symbol (any name).
 * On success, *out_sym is set to the symbol string (owned by obj).
 */
int lisp_element_symbol(lisp_object_t* obj, const char** out_sym);

lisp_object_t* lisp_read_from_string (const char *buf);

int lisp_compile_pattern (lisp_object_t **obj, int *num_subs);
int lisp_match_pattern (lisp_object_t *pattern, lisp_object_t *obj, lisp_object_t **vars, int num_subs);
int lisp_match_string (const char *pattern_string, lisp_object_t *obj, lisp_object_t **vars);

int lisp_type (lisp_object_t *obj);
int lisp_integer (lisp_object_t *obj);
float lisp_real (lisp_object_t *obj);
char* lisp_symbol (lisp_object_t *obj);
char* lisp_string (lisp_object_t *obj);
int lisp_boolean (lisp_object_t *obj);
lisp_object_t* lisp_car (lisp_object_t *obj);
lisp_object_t* lisp_cdr (lisp_object_t *obj);

lisp_object_t* lisp_cxr (lisp_object_t *obj, const char *x);

lisp_object_t* lisp_make_integer (int value);
lisp_object_t* lisp_make_real (float value);
lisp_object_t* lisp_make_symbol (const char *value);
lisp_object_t* lisp_make_string (const char *value);
lisp_object_t* lisp_make_cons (lisp_object_t *car, lisp_object_t *cdr);
lisp_object_t* lisp_make_boolean (int value);

int lisp_list_length (lisp_object_t *obj);
lisp_object_t* lisp_list_nth_cdr (lisp_object_t *obj, int index);
lisp_object_t* lisp_list_nth (lisp_object_t *obj, int index);

void lisp_dump (lisp_object_t *obj, FILE *out);

#define lisp_nil()           ((lisp_object_t*)0)

#define lisp_nil_p(obj)      (obj == 0)
#define lisp_integer_p(obj)  (lisp_type((obj)) == LISP_TYPE_INTEGER)
#define lisp_real_p(obj)     (lisp_type((obj)) == LISP_TYPE_REAL)
#define lisp_symbol_p(obj)   (lisp_type((obj)) == LISP_TYPE_SYMBOL)
#define lisp_string_p(obj)   (lisp_type((obj)) == LISP_TYPE_STRING)
#define lisp_cons_p(obj)     (lisp_type((obj)) == LISP_TYPE_CONS)
#define lisp_boolean_p(obj)  (lisp_type((obj)) == LISP_TYPE_BOOLEAN)

/** */
class LispReader
  {
  private:
    lisp_object_t* lst;

    lisp_object_t* search_for(const char* name);
  public:
    /** cur == ((pos 1 2 3) (id 12 3 4)...) */
    LispReader (lisp_object_t* l);

    bool read_int_vector (const char* name, std::vector<int>* vec);
    bool read_char_vector (const char* name, std::vector<char>* vec);
    bool read_string_vector (const char* name, std::vector<std::string>* vec);
    bool read_string (const char* name, std::string* str);
    bool read_int (const char* name, int* i);
    bool read_float (const char* name, float* f);
    bool read_bool (const char* name, bool* b);
    bool read_lisp (const char* name, lisp_object_t** b);
  };

/** */
class LispWriter
  {
  private:
    std::vector<lisp_object_t*> lisp_objs;

    void append (lisp_object_t* obj);
    lisp_object_t* make_list3 (lisp_object_t*, lisp_object_t*, lisp_object_t*);
    lisp_object_t* make_list2 (lisp_object_t*, lisp_object_t*);
  public:
    LispWriter (const char* name);
    void write_float (const char* name, float f);
    void write_int (const char* name, int i);
    void write_boolean (const char* name, bool b);
    void write_string (const char* name, const char* str);
    void write_symbol (const char* name, const char* symname);
    void write_lisp_obj(const char* name, lisp_object_t* lst);

    /** caller is responible to free the returned lisp_object_t */
    lisp_object_t* create_lisp ();
  };

#endif /* SUPERTUX_LISPREADER_H */
