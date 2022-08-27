#ifndef ML666_UTILS_H
#define ML666_UTILS_H

#include <stdint.h>
#include <ml666/common.h>

#define ML666_FNV_PRIME        ((uint64_t)0x100000001B3llu)
#define ML666_FNV_OFFSET_BASIS ((uint64_t)0xCBF29CE484222325llu)


static inline uint64_t ml666_hash_FNV_1a_append(struct ml666_buffer_ro buf, uint_fast64_t hash){
  for(size_t i=0, n=buf.length; i<n; i++)
    hash = (hash ^ buf.data[i]) * ML666_FNV_PRIME;
  return hash;
}

static inline uint64_t ml666_hash_FNV_1a(struct ml666_buffer_ro buf){
  return ml666_hash_FNV_1a_append(buf, ML666_FNV_OFFSET_BASIS);
}

struct ml666_hashed_buffer {
  struct ml666_buffer_ro buffer;
  uint64_t hash;
};

static inline void ml666_hashed_buffer_set(struct ml666_hashed_buffer* hashed, struct ml666_buffer_ro content){
  hashed->buffer = content;
  hashed->hash   = ml666_hash_FNV_1a(content);
}

#endif
