#ifndef ML666_COMMON_H
#define ML666_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef ML666_BUILD
#define ML666_EXPORT __attribute__((visibility("default")))
#else
#define ML666_EXPORT
#endif

/** \addtogroup utils Utils
 * @{
 * \addtogroup ml666-buffer ml666_buffer API
 * @{
 */

/**
 * Holds arbitrary readonly binary data. If it holds a string,
 * it is not null terminated.
 *
 * It must have the same layout as the ml666_buffer struct.
 */
struct ml666_buffer_ro {
  size_t length;
  const char* data;
};

/**
 * Holds arbitrary readonly binary data. If it holds a string,
 * it is not null terminated.
 *
 * It must have the same layout as the \ref ml666_buffer_ro struct,
 * this allows getting a reference to its data which is not to be written to.
 *
 * There are also some helper functions for working with these hashed buffers,
 * such as \ref ml666_buffer__dup, \ref ml666_buffer__clear, \ref ml666_buffer__append.
 * Note that some of those functions need the buffer to be allocated with a common allocator,
 */
struct ml666_buffer {
  union {
    struct ml666_buffer_ro ro;
    struct {
      size_t length;
      char* data;
    };
  };
};
/** @} */

/** \addtogroup ml666-hashed-buffer ml666_hashed_buffer API
 * @{
 */

/**
 * A buffer with a hash. Not to be initialised or modified directly.
 * Use the \ref ml666_hashed_buffer__create function to create one.
 *
 * There are also some helper functions for working with these hashed buffers,
 * such as \ref ml666_hashed_buffer__dup, \ref ml666_hashed_buffer__clear, \ref ml666_hashed_buffer__append.
 * Note that some of those functions need the buffer to be allocated with a common allocator,
 */
struct ml666_hashed_buffer {
  struct ml666_buffer_ro buffer;
  uint64_t hash;
};
/** @} */

/**
 * Callback for allocating memory.
 * \see ml666__d__malloc for the default implementation
 * \param that Userdefined
 * \param size The size of the memory region to be allocated
 * \returns the new memory region or 0 if an error occured
 */
typedef void* ml666__cb__malloc(void* that, size_t size);

/**
 * Callback for allocating, resizing & freeing memory.
 * \see ml666__d__realloc for the default implementation
 * \param that Userdefined
 * \param ptr The old memory region
 * \param size The new size of the memory region
 * \returns the new memory region or 0 if an error occured
 */
typedef void* ml666__cb__realloc(void* that, void* ptr, size_t size);

/**
 * Callback for freeing memory.
 * \see ml666__d__free for the default implementation
 * \param that Userdefined
 * \param ptr The memory region to be freed
 */
typedef void  ml666__cb__free(void* that, void* ptr);

/** @} */

#endif
