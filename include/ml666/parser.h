#ifndef ML666_PARSER_H
#define ML666_PARSER_H

#include <stddef.h>
#include <stdbool.h>

struct ml666_tag_name {
  char* data;
  size_t length;
};

struct ml666_attribute_name {
  char* data;
  size_t length;
};

struct ml666_parser {
  void* user_ptr;
  const char* error;
  bool done;
};

struct ml666_parser_cb {
        bool(*init   )(struct ml666_parser* that);
        bool(*done   )(struct ml666_parser* that);
        bool(*cleanup)(struct ml666_parser* that);

        struct ml666_tag_name* (*tag_name_realloc      )(struct ml666_parser* that, struct ml666_tag_name      * name, size_t new_size);
  struct ml666_attribute_name* (*attribute_name_realloc)(struct ml666_parser* that, struct ml666_attribute_name* name, size_t new_size);
                          bool (*tag_push              )(struct ml666_parser* that, struct ml666_tag_name* name);
        struct ml666_tag_name* (*tag_get_name          )(struct ml666_parser* that);
                          bool (*tag_pop               )(struct ml666_parser* that);
                          bool (*set_attribute         )(struct ml666_parser* that, struct ml666_tag_name* name);
                          bool (*append_data           )(struct ml666_parser* that, size_t length, const char data[length]);
};

struct ml666_parser_create_args {
  const struct ml666_parser_cb* cb;
  int fd;
};

struct ml666_parser* ml666_parser_create_p(struct ml666_parser_create_args args);
#define ml666_parser_create(...) ml666_parser_create_p((struct ml666_parser_create_args){__VA_ARGS__})
bool ml666_parser_next(struct ml666_parser* parser);
void ml666_parser_destroy(struct ml666_parser* parser);

#endif
