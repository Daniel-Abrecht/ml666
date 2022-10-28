#ifndef ML666_SIMPLE_TREE_BUILDER_H
#define ML666_SIMPLE_TREE_BUILDER_H

#include <ml666/simple-tree.h>
#include <assert.h>

/**
 * \addtogroup ml666-simple-tree Simple Tree API
 * @{
 * \addtogroup ml666-simple-tree-builder Default Simple Tree Builder
 * @{
 */


ML666_ST_DECLARATION(ml666_default, ml666_st__d_)

/**
 * ```
 * ML666_ST_DECLARATION(ml666_default, ml666_st__d_)
 * ```
 * Default implementation of \ref ml666_st_builder.
 */
ML666_EXPORT extern const struct ml666_st_cb ml666_default_st_api; // Note: ML666_ST_DECLARATION already creates this line, but doxygen refuses to document stuff from there :(

struct ml666_st_builder_create_args {
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
  struct ml666_hashed_buffer_set* buffer_set;
};

ML666_EXPORT struct ml666_st_builder* ml666_st_builder_create_p(struct ml666_st_builder_create_args args);
#define ml666_st_builder_create(...) ml666_st_builder_create_p((struct ml666_st_builder_create_args){__VA_ARGS__})

/**
 * This macro can be used for directly accessing the default simple tree data structures
 * It is recommended to use the API functions in simple-tree.h instead, because
 * those will also work with other data structures & implementations.
 */
#define ML666_DEFAULT_SIMPLE_TREE \
  struct ml666_st_children { \
    struct ml666_st_member *first, *last; \
  }; \
  \
  struct ml666_st_node { \
    enum ml666_st_node_type type; \
    size_t refcount; \
  }; \
  \
  struct ml666_st_member { \
    struct ml666_st_node node; \
    struct ml666_st_node *parent; \
    struct ml666_st_member *previous, *next; \
  }; \
  \
  struct ml666_st_document { \
    struct ml666_st_node node; \
    struct ml666_st_children children; \
  }; \
  \
  struct ml666_st_attribute_set { \
    struct ml666_st_attribute_set_entry* first; \
    struct ml666_st_attribute_set_entry* last; \
  }; \
  struct ml666_st_attribute_set_entry { \
    union { \
      struct ml666_st_attribute_set* flist; \
      struct ml666_st_attribute_set_entry* previous; \
    }; \
    union { \
      struct ml666_st_attribute_set* llist; \
      struct ml666_st_attribute_set_entry* next; \
    }; \
  }; \
  static_assert(sizeof(struct ml666_st_attribute_set) == sizeof(struct ml666_st_attribute_set_entry), "Mismatch between ml666_st_attribute_set and ml666_st_attribute_set_entry"); \
  static_assert(offsetof(struct ml666_st_attribute_set,first) == offsetof(struct ml666_st_attribute_set_entry,flist), "Mismatch between ml666_st_attribute_set and ml666_st_attribute_set_entry"); \
  static_assert(offsetof(struct ml666_st_attribute_set,last ) == offsetof(struct ml666_st_attribute_set_entry,llist), "Mismatch between ml666_st_attribute_set and ml666_st_attribute_set_entry"); \
  \
  struct ml666_st_attribute { \
    struct ml666_st_attribute_set_entry entry; \
    const struct ml666_hashed_buffer_set_entry* name; \
    struct ml666_buffer value; \
    bool has_value; \
  }; \
  \
  struct ml666_st_element { \
    struct ml666_st_member member; \
    struct ml666_st_children children; \
    const struct ml666_hashed_buffer_set_entry* name; \
    struct ml666_st_attribute_set attribute_list; \
  }; \
  \
  struct ml666_st_content { \
    struct ml666_st_member member; \
    struct ml666_buffer buffer; \
  }; \
  \
  struct ml666_st_comment { \
    struct ml666_st_member member; \
    struct ml666_buffer buffer; \
  };

/** @} */
/** @} */

#endif
