#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/utils.h>
#include <stddef.h>

struct ml666_st_attribute {
  const struct ml666_hashed_buffer_set_entry* name;
  struct ml666_buffer value;
};

enum ml666_st_node_type {
  ML666_ST_NT_DOCUMENT,
  ML666_ST_NT_ELEMENT,
  ML666_ST_NT_CONTENT,
  ML666_ST_NT_COMMENT,
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


typedef void ml666_st_cb_node_put(struct ml666_st_node* node);
typedef bool ml666_st_cb_node_ref(struct ml666_st_node* node);
typedef bool ml666_st_cb_node_ref_set(struct ml666_st_node** dest, struct ml666_st_node* src);

typedef bool ml666_st_cb_member_set(struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before);
typedef void ml666_st_cb_subtree_disintegrate(struct ml666_st_children* children);

typedef struct ml666_st_document* ml666_st_cb_document_create(void);
typedef struct ml666_st_element* ml666_st_cb_content_create(void);
typedef struct ml666_st_element* ml666_st_cb_comment_create(void);
typedef struct ml666_st_element* ml666_st_cb_element_create(const struct ml666_hashed_buffer* entry, bool copy_name);

typedef struct ml666_st_children* ml666_st_cb_node_get_type(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_document_get_children(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_element_get_children(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_node_get_children(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_member_get_parent(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_member_get_previous(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_member_get_next(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_element_get_name(struct ml666_st_node* node);

typedef bool ml666_st_cb_content_set(struct ml666_buffer node);
typedef struct ml666_buffer ml666_st_cb_content_get(struct ml666_st_node* node);
typedef bool ml666_st_cb_comment_set(struct ml666_buffer node);
typedef struct ml666_buffer ml666_st_cb_comment_get(struct ml666_st_node* node);



struct ml666_st_cb {
  ml666_st_cb_node_put* node_put;
  ml666_st_cb_node_ref* node_ref;
  ml666_st_cb_node_ref_set* node_ref_set;

  ml666_st_cb_node_get_children* node_get_children;
  ml666_st_cb_member_set* member_set;
  ml666_st_cb_subtree_disintegrate* subtree_disintegrate;

  ml666_st_cb_document_create* document_create;
  ml666_st_cb_element_create* element_create;
};


extern const struct ml666_st_cb ml666_st_default_api;

ml666_st_cb_node_put ml666_st_node_put;
ml666_st_cb_node_ref ml666_st_node_ref;
ml666_st_cb_node_ref_set ml666_st_node_ref_set;

ml666_st_cb_node_get_children ml666_st_node_get_children;
ml666_st_cb_member_set ml666_st_set;
ml666_st_cb_subtree_disintegrate ml666_st_subtree_disintegrate;

ml666_st_cb_document_create ml666_st_document_create;
ml666_st_cb_element_create ml666_st_element_create;


#endif
