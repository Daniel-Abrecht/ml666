#ifndef ML666_SIMPLE_TREE_BUILDER_H
#define ML666_SIMPLE_TREE_BUILDER_H

#include <stdbool.h>

struct ml666_simple_tree_builder;

struct ml666_simple_tree_builder_create_args {
  const struct ml666_st_cb* cb;
  int fd;
};

struct ml666_simple_tree_builder* ml666_simple_tree_builder_create_p(struct ml666_simple_tree_builder_create_args args);
#define ml666_simple_tree_builder_create(...) ml666_simple_tree_builder_create_p((struct ml666_simple_tree_builder_create_args){__VA_ARGS__})
void ml666_simple_tree_builder_destroy(struct ml666_simple_tree_builder* stb);
bool ml666_simple_tree_builder_next(struct ml666_simple_tree_builder* stb);
struct ml666_st_document* ml666_simple_tree_builder_take_document(struct ml666_simple_tree_builder* stb);
const char* ml666_simple_tree_builder_get_error(struct ml666_simple_tree_builder* stb);

#endif
