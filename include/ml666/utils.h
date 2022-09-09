#ifndef ML666_UTILS_H
#define ML666_UTILS_H

#include <ml666/common.h>
#include <stdint.h>
#include <stdbool.h>

#define ML666_FNV_PRIME        ((uint64_t)0x100000001B3llu)
#define ML666_FNV_OFFSET_BASIS ((uint64_t)0xCBF29CE484222325llu)

#define ML666_B36 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

// Hashing

static inline uint64_t ml666_hash_FNV_1a_append(struct ml666_buffer_ro buf, uint_fast64_t hash){
  for(size_t i=0, n=buf.length; i<n; i++)
    hash = (hash ^ buf.data[i]) * ML666_FNV_PRIME;
  return hash;
}

static inline uint64_t ml666_hash_FNV_1a(struct ml666_buffer_ro buf){
  return ml666_hash_FNV_1a_append(buf, ML666_FNV_OFFSET_BASIS);
}

static inline void ml666_hashed_buffer__set(struct ml666_hashed_buffer* hashed, struct ml666_buffer_ro content){
  hashed->buffer = content;
  hashed->hash   = ml666_hash_FNV_1a(content);
}

// UTF-8

struct streaming_utf8_validator {
  uint_least8_t index : 3;
  uint_least8_t state : 4;
};

bool utf8_validate(struct streaming_utf8_validator*restrict const v, const int _ch);

// Useful function to work with ml666_buffer

struct ml666_buffer__dup_args {
  struct ml666_buffer* dest;
  struct ml666_buffer_ro src;
  void* that;
  void*(*malloc)(void* that, size_t);
};
typedef bool ml666_buffer__cb__dup_p(struct ml666_buffer__dup_args a);
#define ml666_buffer__dup(...) ml666_buffer__dup_p((struct ml666_buffer__dup_args){__VA_ARGS__})
ml666_buffer__cb__dup_p ml666_buffer__dup_p;

struct ml666_hashed_buffer__dup_args {
  struct ml666_hashed_buffer* dest;
  struct ml666_hashed_buffer src;
  void* that;
  void*(*malloc)(void* that, size_t);
  ml666_buffer__cb__dup_p* dup;
};
typedef bool ml666_hashed_buffer__cb__dup_p(struct ml666_hashed_buffer__dup_args a);
#define ml666_hashed_buffer__dup(...) ml666_hashed_buffer__dup_p((struct ml666_hashed_buffer__dup_args){__VA_ARGS__})
ml666_hashed_buffer__cb__dup_p ml666_hashed_buffer__dup_p;

struct ml666_buffer__clear_args {
  struct ml666_buffer* buffer;
  void* that;
  void (*free)(void* that, void* ptr);
};
typedef bool ml666_buffer__cb__clear_p(struct ml666_buffer__clear_args a);
#define ml666_buffer__clear(...) ml666_buffer__clear_p((struct ml666_buffer__clear_args){__VA_ARGS__})
ml666_buffer__cb__clear_p ml666_buffer__clear_p;

struct ml666_hashed_buffer__clear_args {
  struct ml666_hashed_buffer* buffer;
  void* that;
  void (*free)(void* that, void* ptr);
  ml666_buffer__cb__clear_p* clear;
};
typedef bool ml666_hashed_buffer__cb__clear_p(struct ml666_hashed_buffer__clear_args a);
#define ml666_hashed_buffer__clear(...) ml666_hashed_buffer__clear_p((struct ml666_hashed_buffer__clear_args){__VA_ARGS__})
ml666_hashed_buffer__cb__clear_p ml666_hashed_buffer__clear_p;

ml666__cb__malloc  ml666__d__malloc;
ml666__cb__realloc ml666__d__realloc;
ml666__cb__free    ml666__d__free;


//// hashed_buffer_set api

// The first entry must be a struct ml666_hashed_buffer instance.
struct ml666_hashed_buffer_set;
struct ml666_hashed_buffer_set_entry;

typedef const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__cb__add(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* entry,
  bool copy_content
);
typedef void ml666_hashed_buffer_set__cb__put(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer_set_entry*
);

typedef void ml666_hashed_buffer_set__cb__destroy(struct ml666_hashed_buffer_set* buffer_set);


struct ml666_hashed_buffer_set_cb {
  ml666_hashed_buffer_set__cb__add* add;
  ml666_hashed_buffer_set__cb__put* put;
  ml666_hashed_buffer_set__cb__destroy* destroy;
};

struct ml666_hashed_buffer_set {
  const struct ml666_hashed_buffer_set_cb*const cb;
};


static inline const struct ml666_hashed_buffer* ml666_hashed_buffer_set__peek(const struct ml666_hashed_buffer_set_entry* entry){
  return (const struct ml666_hashed_buffer*)entry;
}

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

static inline void ml666_hashed_buffer_set__destroy(struct ml666_hashed_buffer_set* buffer_set){
  buffer_set->cb->destroy(buffer_set);
}

// Default implementation
struct ml666_hashed_buffer_set* ml666_get_default_hashed_buffer_set(void); // This returns a static buffer_set

struct ml666_create_default_hashed_buffer_set_args {
  void* that;
  ml666__cb__malloc* malloc;
  ml666__cb__free* free;
  ml666_buffer__cb__dup_p* dup_buffer;
  ml666_buffer__cb__clear_p* clear_buffer;
};
struct ml666_hashed_buffer_set* ml666_create_default_hashed_buffer_set_p(struct ml666_create_default_hashed_buffer_set_args); // This creates a buffer set with custom parameters
#define ml666_create_default_hashed_buffer_set(...) ml666_create_default_hashed_buffer_set_p((struct ml666_create_default_hashed_buffer_set_args){__VA_ARGS__})
////

#endif
