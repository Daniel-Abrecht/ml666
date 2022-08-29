#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/utils.h>
#include <stddef.h>

struct ml666_st_attribute {
  const struct ml666_hashed_buffer_set_entry* name;
  struct ml666_buffer value;
};

enum ml666_st_node_type {
  ML666_STB_NT_DOCUMENT,
  ML666_STB_NT_ELEMENT,
  ML666_STB_NT_CONTENT,
  ML666_STB_NT_COMMENT,
};

struct ml666_st_node {
  enum ml666_st_node_type type;
  size_t refcount;
};

struct ml666_st_member {
  struct ml666_st_node node;
  struct ml666_st_node *parent;
  struct ml666_st_member *previous, *next;
};

struct ml666_st_children {
  struct ml666_st_member *first, *last;
};

struct ml666_st_document {
  struct ml666_st_node node;
  struct ml666_st_children children;
};

struct ml666_st_element {
  struct ml666_st_member member;
  struct ml666_st_children children;
  const struct ml666_hashed_buffer_set_entry* name;
};

struct ml666_st_content {
  struct ml666_st_member member;
  struct ml666_buffer content;
};

struct ml666_st_comment {
  struct ml666_st_member member;
  struct ml666_buffer content;
};

struct ml666_simple_tree_builder {
  struct ml666_st_document* document;
  struct ml666_st_node* cur;
};

struct ml666_st_api {
  void (*node_put)(struct ml666_st_node* node);
  bool (*node_ref)(struct ml666_st_node* node);
  bool (*node_ref_set)(struct ml666_st_node** dest, struct ml666_st_node* src);
  struct ml666_st_children* (*node_get_children)(struct ml666_st_node* node);
  bool (*set)(struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before);
  struct ml666_st_element* (*element_create)(const struct ml666_hashed_buffer* entry, bool copy_name);
  struct ml666_st_document* (*document_create)(void);
  void (*subtree_disintegrate)(struct ml666_st_children* children);
};

extern struct ml666_st_api ml666_st_default_api;

#endif
