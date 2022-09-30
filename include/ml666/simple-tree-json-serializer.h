#ifndef ML666_ST_JSON_SERIALIZER
#define ML666_ST_JSON_SERIALIZER

#include <ml666/simple-tree-serializer.h>

struct ml666_st_json_serializer_create_args {
  int fd;
  struct ml666_st_builder* stb;
  struct ml666_st_node* node;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
ML666_EXPORT struct ml666_st_serializer* ml666_st_json_serializer_create_p(struct ml666_st_json_serializer_create_args args);
#define ml666_st_json_serializer_create(...) ml666_st_json_serializer_create_p((struct ml666_st_json_serializer_create_args){__VA_ARGS__})

#endif
