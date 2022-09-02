#ifndef ML666_SIMPLE_TREE_BUILDER_H
#define ML666_SIMPLE_TREE_BUILDER_H

#include <ml666/simple-tree.h>

// Default API
ML666_ST_DECLARATION(ml666_default, ml666_st__d_)

extern struct ml666_st_builder ml666_default_simple_tree_builder;

// This macro can be used for directly accessing the default simple tree data structures
// It is recommended to use the API functions in simple-tree.h instead, because
// those will also work with other data structures & implementations.
#define ML666_DEFAULT_SIMPLE_TREE \
  struct ml666_st_attribute { \
    const struct ml666_hashed_buffer_set_entry* name; \
    struct ml666_buffer value; \
  }; \
  \
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
  struct ml666_st_element { \
    struct ml666_st_member member; \
    struct ml666_st_children children; \
    const struct ml666_hashed_buffer_set_entry* name; \
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

#endif
