#ifndef ML666_ST_BINARY_SERIALIZER
#define ML666_ST_BINARY_SERIALIZER

#include <ml666/simple-tree-serializer.h>

/**
 * \addtogroup ml666-simple-tree
 * @{
 * \addtogroup ml666-simple-tree-serializer
 * @{
 * \addtogroup ml666-simple-tree-binary-serializer Binary Serializer
 * Implementation of \ref ml666-simple-tree-serializer for dumping \ref ml666_st_content_t raw.
 * @{
*/

struct ml666_st_binary_serializer_create_args {
  int fd;
  struct ml666_st_builder* stb;
  struct ml666_st_node* node;

  /* Optional */
  const struct ml666_hashed_buffer* attribute;
  bool recursive;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
ML666_EXPORT struct ml666_st_serializer* ml666_st_binary_serializer_create_p(struct ml666_st_binary_serializer_create_args args);
#define ml666_st_binary_serializer_create(...) ml666_st_binary_serializer_create_p((struct ml666_st_binary_serializer_create_args){__VA_ARGS__})

#endif
