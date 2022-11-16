#ifndef ML666_UTILS_H
#define ML666_UTILS_H

#include <ml666/common.h>
#include <stdint.h>
#include <stdbool.h>

/** \addtogroup ml666-utils Utils
 * @{ */

#define ML666_B36 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" ///< Digits for base36 numbers
#define ML666_B64 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=" ///< Digits for base64 encoded content

// Hashing

/** \addtogroup ml666-buffer ml666_buffer API
 * @{ */
#define ML666_FNV_PRIME        ((uint64_t)0x100000001B3llu)      ///< This is the FNV Prime used for 64 bit hashs by the FNV algorithm
#define ML666_FNV_OFFSET_BASIS ((uint64_t)0xCBF29CE484222325llu) ///< This is the initial hash value used when calculating the FNV hash

/**
 * Updates the hash using the new data in buf
 * \param buf The buffer with the new data
 * \param hash The old hash to be updated
 * \returns the new Hash
 */
static inline uint64_t ml666_hash_FNV_1a_append(struct ml666_buffer_ro buf, uint_fast64_t hash){
  for(size_t i=0, n=buf.length; i<n; i++)
    hash = (hash ^ buf.data[i]) * ML666_FNV_PRIME;
  return hash;
}

static inline uint64_t ml666_hash_FNV_1a(struct ml666_buffer_ro buf){
  return ml666_hash_FNV_1a_append(buf, ML666_FNV_OFFSET_BASIS);
}
/** @} */

/** \addtogroup ml666-hashed-buffer ml666_hashed_buffer API
 * @{ */

/**
 * This creates an ml666_hashed_buffer from an ml666_buffer_ro buffer.
 * The data of the ml666_buffer_ro will not be copied.
 * The hash will be callculated automatically. For an empty buffer, it's set to 0.
 * \returns an ml666_hashed_buffer
 */
static inline struct ml666_hashed_buffer ml666_hashed_buffer__create(struct ml666_buffer_ro content){
  return (struct ml666_hashed_buffer){
    .buffer = content,
    // 0 for 0 length, for consistency with new zeroed out buffers
    .hash   = content.length ? ml666_hash_FNV_1a(content) : 0,
  };
}
/** @} */

// UTF-8

/** \addtogroup ml666-utf8 UTF-8 Helpers
 * These are helpful functions for working with UTF-8 Data.
 * @{ */

/**
 * This stores the current state for the \ref ml666_utf8_validate function.
 * To initialise or reset it, just make sure it's zeroed out.
 */
struct ml666_streaming_utf8_validator {
  uint_least8_t index : 3;
  uint_least8_t state : 4;
};

/**
 * For validating if a sequence of bytes is valid UTF-8.
 * \param v The validator state
 * \param ch The next character or EOF if the sequence is complete
 * \returns true if everything is OK, false if the sequence wasn't valid.
 */
ML666_EXPORT bool ml666_utf8_validate(struct ml666_streaming_utf8_validator*restrict const v, const int ch);
/** @} */

// Useful function to work with ml666_buffer

/** \addtogroup ml666-buffer ml666_buffer API
 * @{ */

/** \see ml666_buffer__dup */
struct ml666_buffer__dup_args {
  struct ml666_buffer* dest; ///< Target buffer structure
  struct ml666_buffer_ro src;///< The buffer to be copied
  // Optional
  void* that;                ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__malloc* malloc; ///< Optional. Custom allocator
};
/** \see ml666_buffer__dup */
typedef bool ml666_buffer__cb__dup_p(struct ml666_buffer__dup_args a);
/** \see ml666_buffer__dup */
ML666_EXPORT ml666_buffer__cb__dup_p ml666_buffer__dup_p;
/**
 * Allocate & copy the data of a \ref ml666_buffer.
 * It will not free the data the target buffer structure points to.
 *
 * \see ml666_buffer__dup_args for the arguments. Please use designated initialisers for the optional arguments.
 * \returns true on success, false on error
 */
#define ml666_buffer__dup(...) ml666_buffer__dup_p((struct ml666_buffer__dup_args){__VA_ARGS__})

/** \see ml666_buffer__append */
struct ml666_buffer__append_args {
  struct ml666_buffer* buffer; ///< The buffer to which the data is to be appended to
  struct ml666_buffer_ro data; ///< The buffer containing the data to be appended
  // Optional
  void* that; ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__realloc* realloc; ///< Optional. Custom allocator
};
typedef bool ml666_buffer__cb__append_p(struct ml666_buffer__append_args args);
/** \see ml666_buffer__append */
ML666_EXPORT ml666_buffer__cb__append_p ml666_buffer__append_p;
/**
 * Append data to a buffer. Make sure the buffer content is actually allocated using malloc, as it tries to realloc it.
 * The arguments can be passed by position or by name. The parameters .that and .realloc are optional.
 * \see struct ml666_buffer__append_args
 * \returns true on success, false on failure
 */
#define ml666_buffer__append(...) ml666_buffer__append_p((struct ml666_buffer__append_args){__VA_ARGS__})

/** \see ml666_buffer__clear */
struct ml666_buffer__clear_args {
  struct ml666_buffer* buffer; ///< The buffer to be cleared
  // Optional
  void* that; ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__free* free; ///< Optional. Custom allocator
};
/** \see ml666_buffer__clear */
typedef bool ml666_buffer__cb__clear_p(struct ml666_buffer__clear_args a);
/** \see ml666_buffer__clear */
ML666_EXPORT ml666_buffer__cb__clear_p ml666_buffer__clear_p;
/**
 * Frees the data of an ml666_buffer and then zeroes out the data structure.
 *
 * \see ml666_buffer__clear_args for the arguments. Please use designated initialisers for the optional arguments.
 * \returns true on success, false on error
 */
#define ml666_buffer__clear(...) ml666_buffer__clear_p((struct ml666_buffer__clear_args){__VA_ARGS__})

ML666_EXPORT bool ml666_buffer__equal(struct ml666_buffer_ro a, struct ml666_buffer_ro b);

/** @} */

/** \addtogroup ml666-hashed-buffer ml666_hashed_buffer API
 * @{ */

/** \see ml666_hashed_buffer__clear */
struct ml666_hashed_buffer__clear_args {
  struct ml666_hashed_buffer* buffer; ///< The buffer to be cleared
  // Optional
  void* that; ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__free* free; ///< Optional. Custom allocator
  ml666_buffer__cb__clear_p* clear; ///< Optional. Custom ml666_buffer__cb__clear_p function
};
/** \see ml666_hashed_buffer__clear */
typedef bool ml666_hashed_buffer__cb__clear_p(struct ml666_hashed_buffer__clear_args a);
/** \see ml666_hashed_buffer__clear */
ML666_EXPORT ml666_hashed_buffer__cb__clear_p ml666_hashed_buffer__clear_p;
/**
 * Frees the data of an ml666_hashed_buffer and then zeroes out the data structure.
 *
 * \see ml666_hashed_buffer__clear_args for the arguments. Please use designated initialisers for the optional arguments.
 * \returns true on success, false on error
 */
#define ml666_hashed_buffer__clear(...) ml666_hashed_buffer__clear_p((struct ml666_hashed_buffer__clear_args){__VA_ARGS__})

/** \see ml666_hashed_buffer__dup */
struct ml666_hashed_buffer__dup_args {
  struct ml666_hashed_buffer* dest; ///< Target buffer structure
  struct ml666_hashed_buffer src;   ///< The buffer to be copied
  void* that; ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__malloc* malloc; ///< Optional. Custom allocator
  ml666_buffer__cb__dup_p* dup; ///< Optional. Custom ml666_buffer__cb__dup_p function
};
/** \see ml666_hashed_buffer__dup */
typedef bool ml666_hashed_buffer__cb__dup_p(struct ml666_hashed_buffer__dup_args a);
/** \see ml666_hashed_buffer__dup */
ML666_EXPORT ml666_hashed_buffer__cb__dup_p ml666_hashed_buffer__dup_p;
/**
 * Allocate & copy the data of a \ref ml666_hashed_buffer.
 * It will not free the data the target buffer structure points to.
 *
 * \see ml666_hashed_buffer__dup_args for the arguments. Please use designated initialisers for the optional arguments.
 * \returns true on success, false on error
 */
#define ml666_hashed_buffer__dup(...) ml666_hashed_buffer__dup_p((struct ml666_hashed_buffer__dup_args){__VA_ARGS__})

/** \see ml666_hashed_buffer__append */
struct ml666_hashed_buffer__append_args {
  struct ml666_hashed_buffer* buffer; ///< The buffer to which the data is to be appended to
  struct ml666_buffer_ro data; ///< The buffer containing the data to be appended
  // Optional
  void* that; ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__realloc* realloc; ///< Optional. Custom allocator
  ml666_buffer__cb__append_p* append; ///< Optional. Custom ml666_buffer__cb__append_p function
};
/** \see ml666_hashed_buffer__append */
ML666_EXPORT bool ml666_hashed_buffer__append_p(struct ml666_hashed_buffer__append_args args);
/**
 * Append data to a \ref ml666_hashed_buffer . Make sure the buffer content is actually allocated using malloc, as it tries to realloc it.
 * The arguments can be passed by position or by name. The parameters .that and .realloc are optional.
 * \see struct ml666_buffer__append_args
 * \returns true on success, false on failure
 */
#define ml666_hashed_buffer__append(...) ml666_hashed_buffer__append_p((struct ml666_hashed_buffer__append_args){__VA_ARGS__})

/** @} */

/**
 * \addtogroup ml666-buffer ml666_buffer API
 * \addtogroup ml666-utf8 UTF-8 Helpers
 * @{
 */

/**
 * Used in \ref ml666_buffer_info, the best encoding for serializing the buffer.
 */
enum ml666_buffer_info_best_encoding {
  ML666_BIBE_ESCAPE, ///< Escape sequences
  ML666_BIBE_BASE64, ///< base64
};

/**
 * Various information about a buffer.
 * \see ml666_buffer__analyze
 */
struct ml666_buffer_info {
  bool multi_line : 1;        ///< True if there is any newline
  bool really_multi_line : 1; ///< Same, but excludes a final newlines
  bool ends_with_newline : 1; ///< True if it ends with newlines.
  bool ends_with_single_newline : 1; ///< True if it ends with a single newline.
  bool is_fully_ascii : 1; ///< All characters are <0x80
  bool is_valid_utf8 : 1;  ///< All the data was valid UTF-8
  bool has_null_bytes : 1; ///< This is commonly used to decide if some data is binary or not
  enum ml666_buffer_info_best_encoding best_encoding : 1; ///< Heuristic if escaping or base64 meight result in smaller output.
};

/**
 * This function checks and returns various informations about the given buffer.
 * That includes if it's entirely valid utf-8, if it contains only ascii data, if there are newlines or nullbytes in it, and much more.
 * \see ml666_buffer_info for the specifics
 */
ML666_EXPORT struct ml666_buffer_info ml666_buffer__analyze(struct ml666_buffer_ro buffer);
/** @} */


//// hashed_buffer_set api

/** \addtogroup ml666-hashed-buffer-set ml666_hashed_buffer_set API
 * @{ */

struct ml666_hashed_buffer_set;
/**
 * Opaque structure, but the first entry must be a \ref ml666_hashed_buffer instance.
 * Use \ref ml666_hashed_buffer_set__peek to access it.
 */
typedef struct ml666_hashed_buffer_set_entry ml666_hashed_buffer_set_entry;

/**
 * Used by \ref ml666_hashed_buffer_set__lookup.
 */
enum ml666_hashed_buffer_set_mode {
  ML666_HBS_M_GET, ///< Just search and return it
  ML666_HBS_M_ADD_TAKE, ///< If it does not yet exist, create the entry and initialise it with the preexisting buffer data, the hashed buffer set will be responsible for freeing it now.
  ML666_HBS_M_ADD_COPY, ///< If it does not yet exist, create the entry and initialise it with a copy of the buffer data. The caller stays resposnible for freeing it.
};

/** \see ml666_hashed_buffer_set__lookup */
typedef const ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__cb__lookup(
  struct ml666_hashed_buffer_set* buffer_set,
  const struct ml666_hashed_buffer* entry,
  enum ml666_hashed_buffer_set_mode mode
);
/** \see ml666_hashed_buffer_set__put */
typedef void ml666_hashed_buffer_set__cb__put(
  struct ml666_hashed_buffer_set* buffer_set,
  const ml666_hashed_buffer_set_entry*
);

/** \see ml666_hashed_buffer_set__destroy */
typedef void ml666_hashed_buffer_set__cb__destroy(struct ml666_hashed_buffer_set* buffer_set);

/**
 * This is the ml666_hashed_buffer_set API interface.
 * If you implement these callbacks, create your own instance of
 * the \ref ml666_hashed_buffer_set struct or a factory for that,
 * and set the cb field in there to the former, you can use your
 * own implementation with any function taking a ml666_hashed_buffer_set.
 * You can also override the global default implementation.
 * \see ml666_hashed_buffer_set__get_default
 */
struct ml666_hashed_buffer_set_cb {
  ml666_hashed_buffer_set__cb__lookup* lookup; ///< See \ref ml666_hashed_buffer_set__lookup
  ml666_hashed_buffer_set__cb__put* put; ///< See \ref ml666_hashed_buffer_set__put
  ml666_hashed_buffer_set__cb__destroy* destroy; ///< See \ref ml666_hashed_buffer_set__destroy
};

/**
 * A ml666_hashed_buffer_set. It stores unique instances of immutable buffers.
 * \see ml666_hashed_buffer_set_cb
 */
struct ml666_hashed_buffer_set {
  const struct ml666_hashed_buffer_set_cb*const cb;
};

/**
 * \returns the \ref ml666_hashed_buffer containing the data of the \ref ml666_hashed_buffer_set_entry.
 */
static inline const struct ml666_hashed_buffer* ml666_hashed_buffer_set__peek(const ml666_hashed_buffer_set_entry* entry){
  return (const struct ml666_hashed_buffer*)entry;
}

/**
 * Lookup the \ref ml666_hashed_buffer_set_entry matching the buffer.
 * It will take a reference which needs to be released using \ref ml666_hashed_buffer_set__put.
 * Depending on the mode parameter, it may create the entry if none is found.
 */
static inline const ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__lookup(
  struct ml666_hashed_buffer_set* buffer_set, ///< The buffer set. See \ref ml666_hashed_buffer_set__get_default for obtaining the default buffer set
  const struct ml666_hashed_buffer* buffer, ///< The buffer to be searched
  enum ml666_hashed_buffer_set_mode mode ///< If the buffer is just to be looked up, or if it is to be added if not found. See \ref ml666_hashed_buffer_set_mode
){
  return buffer_set->cb->lookup(buffer_set, buffer, mode);
}

/**
 * Decreases the reference count of \ref ml666_hashed_buffer_set_entry and removes it when it hits 0.
 */
static inline void ml666_hashed_buffer_set__put(
  struct ml666_hashed_buffer_set* buffer_set, ///< The buffer set. See \ref ml666_hashed_buffer_set__get_default for obtaining the default buffer set
  const ml666_hashed_buffer_set_entry* entry ///< The buffer reference to be dereferenced
){
  buffer_set->cb->put(buffer_set, entry);
}

/**
 * Frees the \ref ml666_hashed_buffer_set including all the entries.
 * \param buffer_set The buffer set. See \ref ml666_hashed_buffer_set__get_default for obtaining the default buffer set
 */
static inline void ml666_hashed_buffer_set__destroy(struct ml666_hashed_buffer_set* buffer_set){
  buffer_set->cb->destroy(buffer_set);
}

/** \see ml666_hashed_buffer_set__get_default */
typedef struct ml666_hashed_buffer_set* ml666_hashed_buffer_set__cb__get_default(void);

/**
 * Get the global default instance or the ml666_hashed_buffer_set. It's statically allocated.
 *
 * Usually, a weak symbol would be used to make this replacable, and I do like those a lot,
 * but the behaviour of static and dynamic linkers isn't straight forward, making that a real pain.
 * So this is a pointer. To replace it, just set it. You can use __attribute__((constructor)) to do so as early as possible.
 * (It's also still a weak symbol, so if you do want to redefine it, you can do so)
 */
ML666_EXPORT extern ml666_hashed_buffer_set__cb__get_default* ml666_hashed_buffer_set__get_default;

/** \see ml666_default_hashed_buffer_set__create */
struct ml666_default_hashed_buffer_set__create_args {
  // Optional
  void* that;                              ///< Optional. Userdefined parameter. Passed to all userdefined callbacks
  ml666__cb__malloc* malloc;               ///< Optional. Custom allocator.
  ml666__cb__free* free;                   ///< Optional. Custom allocator.
  ml666_buffer__cb__dup_p* dup_buffer;     ///< Optional. Custom function for dublicating buffers.
  ml666_buffer__cb__clear_p* clear_buffer; ///< Optional. Custom function for clearing buffers.
};
/** \see ml666_default_hashed_buffer_set__create */
ML666_EXPORT struct ml666_hashed_buffer_set* ml666_default_hashed_buffer_set__create_p(struct ml666_default_hashed_buffer_set__create_args); // This creates a buffer set with custom parameters
/**
 * Create a an instance of the default implementation of a hashed_buffer_set.
 * \see ml666_default_hashed_buffer_set__create_args for the arguments.  Please use designated initialisers for the optional arguments. (they're all optional, though).
 */
#define ml666_default_hashed_buffer_set__create(...) ml666_default_hashed_buffer_set__create_p((struct ml666_default_hashed_buffer_set__create_args){__VA_ARGS__})
////

/** @} */

// other stuff

#if defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define ML666_LCPTR(X) (&((struct { typeof(X) x; }){X}).x) ///< Expression which makes a local copy of any value passed to it and returns a pointer to it
#else
#define ML666_LCPTR(X) (&((struct { __typeof__(X) x; }){X}).x) ///< Expression which makes a local copy of any value passed to it and returns a pointer to it
#endif

ML666_EXPORT ml666__cb__malloc  ml666__d__malloc;  ///< Default malloc implementation
ML666_EXPORT ml666__cb__realloc ml666__d__realloc; ///< Default realloc implementation
ML666_EXPORT ml666__cb__free    ml666__d__free;    ///< Default free implementation

/** @} */

#endif
