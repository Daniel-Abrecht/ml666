#ifndef ML666_UTILS_H
#define ML666_UTILS_H

#include <ml666/common.h>
#include <stdint.h>
#include <stdbool.h>

#define ML666_FNV_PRIME        ((uint64_t)0x100000001B3llu)
#define ML666_FNV_OFFSET_BASIS ((uint64_t)0xCBF29CE484222325llu)

#define ML666_B36 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ML666_B64 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="

// Hashing

static inline uint64_t ml666_hash_FNV_1a_append(struct ml666_buffer_ro buf, uint_fast64_t hash){
  for(size_t i=0, n=buf.length; i<n; i++)
    hash = (hash ^ buf.data[i]) * ML666_FNV_PRIME;
  return hash;
}

static inline uint64_t ml666_hash_FNV_1a(struct ml666_buffer_ro buf){
  return ml666_hash_FNV_1a_append(buf, ML666_FNV_OFFSET_BASIS);
}

static inline struct ml666_hashed_buffer ml666_hashed_buffer__create(struct ml666_buffer_ro content){
  return (struct ml666_hashed_buffer){
    .buffer = content,
    // 0 for 0 length, for consistency with new zeroed out buffers
    .hash   = content.length ? ml666_hash_FNV_1a(content) : 0,
  };
}

// UTF-8

struct ml666_streaming_utf8_validator {
  uint_least8_t index : 3;
  uint_least8_t state : 4;
};

ML666_EXPORT bool ml666_utf8_validate(struct ml666_streaming_utf8_validator*restrict const v, const int _ch);

// Useful function to work with ml666_buffer

struct ml666_buffer__dup_args {
  struct ml666_buffer* dest;
  struct ml666_buffer_ro src;
  void* that;
  void*(*malloc)(void* that, size_t);
};
typedef bool ml666_buffer__cb__dup_p(struct ml666_buffer__dup_args a);
#define ml666_buffer__dup(...) ml666_buffer__dup_p((struct ml666_buffer__dup_args){__VA_ARGS__})
ML666_EXPORT ml666_buffer__cb__dup_p ml666_buffer__dup_p;

struct ml666_hashed_buffer__dup_args {
  struct ml666_hashed_buffer* dest;
  struct ml666_hashed_buffer src;
  void* that;
  void*(*malloc)(void* that, size_t);
  ml666_buffer__cb__dup_p* dup;
};
typedef bool ml666_hashed_buffer__cb__dup_p(struct ml666_hashed_buffer__dup_args a);
#define ml666_hashed_buffer__dup(...) ml666_hashed_buffer__dup_p((struct ml666_hashed_buffer__dup_args){__VA_ARGS__})
ML666_EXPORT ml666_hashed_buffer__cb__dup_p ml666_hashed_buffer__dup_p;

struct ml666_buffer__clear_args {
  struct ml666_buffer* buffer;
  void* that;
  void (*free)(void* that, void* ptr);
};
typedef bool ml666_buffer__cb__clear_p(struct ml666_buffer__clear_args a);
#define ml666_buffer__clear(...) ml666_buffer__clear_p((struct ml666_buffer__clear_args){__VA_ARGS__})
ML666_EXPORT ml666_buffer__cb__clear_p ml666_buffer__clear_p;

struct ml666_hashed_buffer__clear_args {
  struct ml666_hashed_buffer* buffer;
  void* that;
  void (*free)(void* that, void* ptr);
  ml666_buffer__cb__clear_p* clear;
};
typedef bool ml666_hashed_buffer__cb__clear_p(struct ml666_hashed_buffer__clear_args a);
#define ml666_hashed_buffer__clear(...) ml666_hashed_buffer__clear_p((struct ml666_hashed_buffer__clear_args){__VA_ARGS__})
ML666_EXPORT ml666_hashed_buffer__cb__clear_p ml666_hashed_buffer__clear_p;

struct ml666_buffer__append_args {
  struct ml666_buffer* buffer;
  struct ml666_buffer_ro data;
  void* that;
  ml666__cb__realloc* realloc;
};
/**
 * Append data to a buffer. Make sure the buffer content is actually allocated using malloc, as it tries to realloc it.
 * The arguments can be passed by position or by name. The parameters .that and .realloc are optional.
 * \see struct ml666_buffer__append_args
 * \returns true on success, false on failure
 */
#define ml666_buffer__append(...) ml666_buffer__append_p((struct ml666_buffer__append_args){__VA_ARGS__})
ML666_EXPORT bool ml666_buffer__append_p(struct ml666_buffer__append_args args);

struct ml666_hashed_buffer__append_args {
  struct ml666_hashed_buffer* buffer;
  struct ml666_buffer_ro data;
  void* that;
  ml666__cb__realloc* realloc;
};
/**
 * Append data to a hashed buffer. Make sure the buffer content is actually allocated using malloc, as it tries to realloc it.
 * The arguments can be passed by position or by name. The parameters .that and .realloc are optional.
 * \see struct ml666_buffer__append_args
 * \returns true on success, false on failure
 */
#define ml666_hashed_buffer__append(...) ml666_hashed_buffer__append_p((struct ml666_hashed_buffer__append_args){__VA_ARGS__})
ML666_EXPORT bool ml666_hashed_buffer__append_p(struct ml666_hashed_buffer__append_args args);

enum ml666_buffer_info_best_encoding {
  ML666_BIBE_ESCAPE,
  ML666_BIBE_BASE64,
};

struct ml666_buffer_info {
  bool multi_line : 1; // True if there is any newline
  bool really_multi_line : 1; // Same, but excludes a final newlines
  bool ends_with_newline : 1; // True if it ends with newlines.
  bool ends_with_single_newline : 1; // True if it ends with a single newline.
  bool is_fully_ascii : 1; // All characters are <0x80
  bool is_valid_utf8 : 1;
  bool has_null_bytes : 1; // This is commonly used to decide if some data is binary or not
  enum ml666_buffer_info_best_encoding best_encoding : 1;
};

ML666_EXPORT struct ml666_buffer_info ml666_buffer__analyze(struct ml666_buffer_ro buffer);


//// hashed_buffer_set api

// The first entry must be a struct ml666_hashed_buffer instance.
struct ml666_hashed_buffer_set;
struct ml666_hashed_buffer_set_entry;

enum ml666_hashed_buffer_set_mode {
  ML666_HBS_M_GET,
  ML666_HBS_M_ADD_TAKE,
  ML666_HBS_M_ADD_COPY,
};

typedef const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__cb__lookup(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* entry,
  enum ml666_hashed_buffer_set_mode mode
);
typedef void ml666_hashed_buffer_set__cb__put(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer_set_entry*
);

typedef void ml666_hashed_buffer_set__cb__destroy(struct ml666_hashed_buffer_set* buffer_set);


struct ml666_hashed_buffer_set_cb {
  ml666_hashed_buffer_set__cb__lookup* lookup;
  ml666_hashed_buffer_set__cb__put* put;
  ml666_hashed_buffer_set__cb__destroy* destroy;
};

struct ml666_hashed_buffer_set {
  const struct ml666_hashed_buffer_set_cb*const cb;
};


static inline const struct ml666_hashed_buffer* ml666_hashed_buffer_set__peek(const struct ml666_hashed_buffer_set_entry* entry){
  return (const struct ml666_hashed_buffer*)entry;
}

static inline const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__lookup(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* buffer,
  enum ml666_hashed_buffer_set_mode mode
){
  return buffer_set->cb->lookup(buffer_set, buffer, mode);
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
ML666_EXPORT struct ml666_hashed_buffer_set* ml666_get_default_hashed_buffer_set(void); // This returns a static buffer_set

struct ml666_create_default_hashed_buffer_set_args {
  void* that;
  ml666__cb__malloc* malloc;
  ml666__cb__free* free;
  ml666_buffer__cb__dup_p* dup_buffer;
  ml666_buffer__cb__clear_p* clear_buffer;
};
ML666_EXPORT struct ml666_hashed_buffer_set* ml666_create_default_hashed_buffer_set_p(struct ml666_create_default_hashed_buffer_set_args); // This creates a buffer set with custom parameters
#define ml666_create_default_hashed_buffer_set(...) ml666_create_default_hashed_buffer_set_p((struct ml666_create_default_hashed_buffer_set_args){__VA_ARGS__})
////


// other stuff

#if defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define ML666_LCPTR(X) (&((struct { typeof(X) x; }){X}).x)
#else
#define ML666_LCPTR(X) (&((struct { __typeof__(X) x; }){X}).x)
#endif

ML666_EXPORT ml666__cb__malloc  ml666__d__malloc;
ML666_EXPORT ml666__cb__realloc ml666__d__realloc;
ML666_EXPORT ml666__cb__free    ml666__d__free;

#endif
