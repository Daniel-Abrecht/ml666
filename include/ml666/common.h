#ifndef ML666_COMMON_H
#define ML666_COMMON_H

#include <stddef.h>
#include <stdint.h>

// This must match struct ml666_buffer
struct ml666_buffer_ro {
  size_t length;
  const char* data;
};

struct ml666_buffer {
  union {
    struct ml666_buffer_ro ro;
    struct {
      size_t length;
      char* data;
    };
  };
};

struct ml666_hashed_buffer {
  struct ml666_buffer_ro buffer;
  uint64_t hash;
};

typedef void* ml666__cb__malloc(void* that, size_t size);
typedef void* ml666__cb__realloc(void* that, void* ptr, size_t size);
typedef void  ml666__cb__free(void* that, void* ptr);

#endif
