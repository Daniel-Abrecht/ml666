#ifndef ML666_UTILS_H
#define ML666_UTILS_H

#include <ml666/common.h>
#include <stdint.h>
#include <stdbool.h>

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

static inline void ml666_hashed_buffer__set(struct ml666_hashed_buffer* hashed, struct ml666_buffer_ro content){
  hashed->buffer = content;
  hashed->hash   = ml666_hash_FNV_1a(content);
}


//// hashed_buffer_set api

// The first entry must be a struct ml666_hashed_buffer instance.
struct ml666_hashed_buffer_set_entry;

static inline const struct ml666_hashed_buffer* ml666_hashed_buffer_set__peek(const struct ml666_hashed_buffer_set_entry* entry){
  return (const struct ml666_hashed_buffer*)entry;
}

struct ml666_hashed_buffer_set;

typedef const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__cb__add(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* entry,
  bool copy_content
);
typedef void ml666_hashed_buffer_set__cb__put(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer_set_entry*
);

struct ml666_hashed_buffer_set_cb {
  ml666_hashed_buffer_set__cb__add* add;
  ml666_hashed_buffer_set__cb__put* put;
};

struct ml666_hashed_buffer_set {
  const struct ml666_hashed_buffer_set_cb*const cb;
};

static inline const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__add(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* buffer,
  bool copy_content
){
  return buffer_set->cb->add(buffer_set, buffer, copy_content);
}

static inline void ml666_hashed_buffer_set__put(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer_set_entry* entry
){
  buffer_set->cb->put(buffer_set, entry);
}


// Default implementation
ml666_hashed_buffer_set__cb__add ml666_hashed_buffer_set__d__add;
ml666_hashed_buffer_set__cb__put ml666_hashed_buffer_set__d__put;

extern const struct ml666_hashed_buffer_set_cb ml666_default_hashed_buffer_cb;

struct ml666_hashed_buffer_set* ml666_get_default_hashed_buffer_set(void);

////

#endif
