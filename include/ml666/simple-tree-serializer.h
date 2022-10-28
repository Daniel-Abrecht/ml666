#ifndef ML666_SIMPLE_TREE_SERIALIZER_H
#define ML666_SIMPLE_TREE_SERIALIZER_H

#include <ml666/common.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * \addtogroup ml666-simple-tree
 * @{
 * \addtogroup ml666-simple-tree-serializer ML666 Simple Tree Serializer API
 * @{
*/

struct ml666_st_serializer {
  const struct ml666_st_serializer_cb*const cb;
  struct ml666_st_builder*const stb;
  void* user_ptr;
  const char* error;
};

typedef bool ml666_st_serializer_cb_next(struct ml666_st_serializer* sts);
typedef void ml666_st_serializer_cb_destroy(struct ml666_st_serializer* sts);

struct ml666_st_serializer_cb {
  ml666_st_serializer_cb_next* next;
  ml666_st_serializer_cb_destroy* destroy;
};

// returns false on EOF
static inline bool ml666_st_serializer_next(struct ml666_st_serializer* sts){
  return sts->cb->next(sts);
}

static inline void ml666_st_serializer_destroy(struct ml666_st_serializer* sts){
  sts->cb->destroy(sts);
}

// This is merely a convenience function for the simplest of use cases
static inline bool ml666_st_serialize(struct ml666_st_serializer* serializer){
  while(ml666_st_serializer_next(serializer));
  if(serializer->error){
    fprintf(stderr, "ml666_parser_next failed: %s\n", serializer->error);
    return false;
  }
  return true;
}

/** @} */
/** @} */

#endif
