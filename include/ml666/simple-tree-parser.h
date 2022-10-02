#ifndef ML666_SIMPLE_TREE_PARSER_H
#define ML666_SIMPLE_TREE_PARSER_H

#include <ml666/common.h>
#include <stdbool.h>
#include <stdio.h>

struct ml666_simple_tree_parser {
  const struct ml666_simple_tree_parser_cb*const cb;
  struct ml666_st_builder*const stb;
  size_t line, column;
  const char* error;
  void* user_ptr;
};

typedef void ml666_simple_tree_parser_cb_destroy(struct ml666_simple_tree_parser* stp);
typedef bool ml666_simple_tree_parser_cb_next(struct ml666_simple_tree_parser* stp);
typedef struct ml666_st_document* ml666_simple_tree_parser_cb_take_document(struct ml666_simple_tree_parser* stp);

struct ml666_simple_tree_parser_cb {
  ml666_simple_tree_parser_cb_destroy* destroy;
  ml666_simple_tree_parser_cb_next* next;
  ml666_simple_tree_parser_cb_take_document* take_document;
};

struct ml666_simple_tree_parser_create_args {
  int fd;
  struct ml666_st_builder* stb;
  // Optional
  void* user_ptr;
  ml666__cb__malloc*  malloc;
  ml666__cb__realloc* realloc;
  ml666__cb__free*    free;
  struct ml666_parser* parser; // Will be freed by simple tree parser if set
};

ML666_EXPORT struct ml666_simple_tree_parser* ml666_simple_tree_parser_create_p(struct ml666_simple_tree_parser_create_args args);
#define ml666_simple_tree_parser_create(...) ml666_simple_tree_parser_create_p((struct ml666_simple_tree_parser_create_args){__VA_ARGS__})

static inline void ml666_simple_tree_parser_destroy(struct ml666_simple_tree_parser* stp){
  stp->cb->destroy(stp);
}

static inline bool ml666_simple_tree_parser_next(struct ml666_simple_tree_parser* stp){
  return stp->cb->next(stp);
}

static inline struct ml666_st_document* ml666_simple_tree_parser_take_document(struct ml666_simple_tree_parser* stp){
  return stp->cb->take_document(stp);
}

// This is merely a convenience function for the simplest of use cases
#define ml666_st_parse(...) ml666_st_parse_p((struct ml666_simple_tree_parser_create_args){__VA_ARGS__})
static inline struct ml666_st_document* ml666_st_parse_p(struct ml666_simple_tree_parser_create_args args){
  struct ml666_simple_tree_parser* stp = ml666_simple_tree_parser_create_p(args);
  if(!stp){
    fprintf(stderr, "ml666_parser_create failed\n");
    return 0;
  }
  while(ml666_simple_tree_parser_next(stp));
  struct ml666_st_document* result = 0;
  if(stp->error){
    fprintf(stderr, "ml666_parser_next failed: line %zu, column %zu: %s\n", stp->line, stp->column, stp->error);
  }else{
    result = ml666_simple_tree_parser_take_document(stp);
  }
  ml666_simple_tree_parser_destroy(stp);
  return result;
}

#endif
