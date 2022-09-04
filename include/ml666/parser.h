#ifndef ML666_PARSER_H
#define ML666_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <ml666/common.h>


struct ml666_parser {
  const struct ml666_parser_cb*const cb;
  void* user_ptr;
  const char* error;
  bool done;
};


// What this points to is entirely up to the ml666_parser_cb callbacks.
typedef struct ml666_opaque_tag_name* ml666_opaque_tag_name;
typedef struct ml666_opaque_attribute_name* ml666_opaque_attribute_name;


typedef bool ml666_parser_cb_init(struct ml666_parser* that);
typedef void ml666_parser_cb_done(struct ml666_parser* that);
typedef void ml666_parser_cb_cleanup(struct ml666_parser* that);

typedef bool ml666_parser_cb_tag_name_append(struct ml666_parser* that, ml666_opaque_tag_name* name, struct ml666_buffer_ro data);
typedef void ml666_parser_cb_tag_name_free(struct ml666_parser* that, ml666_opaque_tag_name name);
typedef bool ml666_parser_cb_attribute_name_append(struct ml666_parser* that, ml666_opaque_attribute_name* name, struct ml666_buffer_ro data);
typedef void ml666_parser_cb_attribute_name_free(struct ml666_parser* that, ml666_opaque_attribute_name name);

typedef bool ml666_parser_cb_tag_push(struct ml666_parser* that, ml666_opaque_tag_name* name);
typedef bool ml666_parser_cb_end_tag_check(struct ml666_parser* that, ml666_opaque_tag_name name);
typedef bool ml666_parser_cb_tag_pop(struct ml666_parser* that);

typedef bool ml666_parser_cb_set_attribute(struct ml666_parser* that, ml666_opaque_attribute_name* name);

typedef bool ml666_parser_cb_value_append(struct ml666_parser* that, struct ml666_buffer_ro data);
typedef bool ml666_parser_cb_data_append(struct ml666_parser* that, struct ml666_buffer_ro data);
typedef bool ml666_parser_cb_comment_append(struct ml666_parser* that, struct ml666_buffer_ro data);


struct ml666_parser_cb {

  ml666_parser_cb_init*    init;
  ml666_parser_cb_done*    done;
  ml666_parser_cb_cleanup* cleanup;

  ml666_parser_cb_tag_name_append*       tag_name_append;
  ml666_parser_cb_tag_name_free*         tag_name_free;
  ml666_parser_cb_attribute_name_append* attribute_name_append;
  ml666_parser_cb_attribute_name_free*   attribute_name_free;

  ml666_parser_cb_tag_push*      tag_push;
  ml666_parser_cb_end_tag_check* end_tag_check;
  ml666_parser_cb_tag_pop*       tag_pop;

  ml666_parser_cb_set_attribute* set_attribute;

  ml666_parser_cb_value_append*   value_append;
  ml666_parser_cb_data_append*    data_append;
  ml666_parser_cb_comment_append* comment_append;

};


struct ml666_parser_create_args {
  const struct ml666_parser_cb* cb;
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__realloc* realloc;
  ml666__cb__free* free;
};
struct ml666_parser* ml666_parser_create_p(struct ml666_parser_create_args args);
#define ml666_parser_create(...) ml666_parser_create_p((struct ml666_parser_create_args){__VA_ARGS__})
bool ml666_parser_next(struct ml666_parser* parser);
void ml666_parser_destroy(struct ml666_parser* parser);


// The default handlers use these type definitions, just put the macro in the file where you need the type.
// If you use custom handlers everywhere, you can use your own custom type / definition instead
#define ML666_DEFAULT_OPAQUE_TAG_NAME \
  struct ml666_opaque_tag_name { struct ml666_buffer buffer; };
#define ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME \
  struct ml666_opaque_attribute_name { struct ml666_buffer buffer; };

/**
 * Malloc based tag & attribute allocation handlers.
 * These use the default definition above.
 * They allocate the data using realloc, and free it with free.
 * But the ml666_opaque_tag_name and ml666_opaque_attribute_name instance itself are allocated once per-parser.
 */
ml666_parser_cb_tag_name_append       ml666_parser__d_mal__tag_name_append;
ml666_parser_cb_tag_name_free         ml666_parser__d_mal__tag_name_free;
ml666_parser_cb_attribute_name_append ml666_parser__d_mal__attribute_name_append;
ml666_parser_cb_attribute_name_free   ml666_parser__d_mal__attribute_name_free;


#endif
