#ifndef ML666_SIMPLE_TREE_PARSER_H
#define ML666_SIMPLE_TREE_PARSER_H

#include <stdbool.h>

struct ml666_simple_tree_parser;

struct ml666_simple_tree_parser_create_args {
  struct ml666_st_builder* stb;
  int fd;
};

struct ml666_simple_tree_parser* ml666_simple_tree_parser_create_p(struct ml666_simple_tree_parser_create_args args);
#define ml666_simple_tree_parser_create(...) ml666_simple_tree_parser_create_p((struct ml666_simple_tree_parser_create_args){__VA_ARGS__})
void ml666_simple_tree_parser_destroy(struct ml666_simple_tree_parser* stp);
bool ml666_simple_tree_parser_next(struct ml666_simple_tree_parser* stp);
struct ml666_st_document* ml666_simple_tree_parser_take_document(struct ml666_simple_tree_parser* stp);
const char* ml666_simple_tree_parser_get_error(struct ml666_simple_tree_parser* stp);

#endif
