#ifndef ML666_SIMPLE_TREE_SERIALIZER_H
#define ML666_SIMPLE_TREE_SERIALIZER_H

#include <ml666/common.h>
#include <stdbool.h>
#include <stdio.h>

struct ml666_st_serializer {
  struct ml666_st_builder*const stb;
  void* user_ptr;
  const char* error;
};

struct ml666_st_serializer_create_args {
  int fd;
  struct ml666_st_builder* stb;
  struct ml666_st_node* node;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
struct ml666_st_serializer* ml666_st_serializer_create_p(struct ml666_st_serializer_create_args args);
#define ml666_st_serializer_create(...) ml666_st_serializer_create_p((struct ml666_st_serializer_create_args){__VA_ARGS__})
bool ml666_st_serializer_next(struct ml666_st_serializer* sts); // returns false on EOF
void ml666_st_serializer_destroy(struct ml666_st_serializer* sts);

// This is merely a convenience function for the simplest of use cases
#define ml666_st_serialize(...) ml666_st_serialize_p((struct ml666_st_serializer_create_args){__VA_ARGS__})
static inline bool ml666_st_serialize_p(struct ml666_st_serializer_create_args args){
  struct ml666_st_serializer* sts = ml666_st_serializer_create_p(args);
  if(!sts){
    fprintf(stderr, "ml666_parser_create failed\n");
    return 0;
  }
  while(ml666_st_serializer_next(sts));
  if(sts->error){
    fprintf(stderr, "ml666_parser_next failed: %s\n", sts->error);
    ml666_st_serializer_destroy(sts);
    return false;
  }
  ml666_st_serializer_destroy(sts);
  return true;
}

#endif
