#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/utils.h>
#include <stddef.h>

enum ml666_st_node_type {
  ML666_ST_NT_DOCUMENT,
  ML666_ST_NT_ELEMENT,
  ML666_ST_NT_CONTENT,
  ML666_ST_NT_COMMENT,
};

struct ml666_st_attribute;
struct ml666_st_children;

struct ml666_st_node; // Must be the first member of all of the below types
struct ml666_st_member; // Must be the first member of ml666_st_element, ml666_st_content and ml666_st_comment
struct ml666_st_document;
struct ml666_st_element;
struct ml666_st_content;
struct ml666_st_comment;

struct ml666_st_builder;


#define ML666_ST_MEMBER(X) _Generic((X), \
    struct ml666_st_element*: (struct ml666_st_member*)(X), \
    struct ml666_st_content*: (struct ml666_st_member*)(X), \
    struct ml666_st_comment*: (struct ml666_st_member*)(X), \
    const struct ml666_st_element*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_content*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_comment*: (const struct ml666_st_member*)(X) \
  )

#define ML666_ST_NODE(X) _Generic((X), \
    struct ml666_st_document*: (struct ml666_st_node*)(X), \
    struct ml666_st_member*: (struct ml666_st_node*)(X), \
    struct ml666_st_element*: (struct ml666_st_node*)(X), \
    struct ml666_st_content*: (struct ml666_st_node*)(X), \
    struct ml666_st_comment*: (struct ml666_st_node*)(X), \
    const struct ml666_st_document*: (const struct ml666_st_node*)(X), \
    const struct ml666_st_member*: (const struct ml666_st_node*)(X), \
    const struct ml666_st_element*: (const struct ml666_st_node*)(X), \
    const struct ml666_st_content*: (const struct ml666_st_node*)(X), \
    const struct ml666_st_comment*: (const struct ml666_st_node*)(X) \
  )


typedef void ml666_st_cb_node_put(struct ml666_st_node* node);
typedef bool ml666_st_cb_node_ref(struct ml666_st_node* node);
typedef bool ml666_st_cb_node_ref_set(struct ml666_st_node** dest, struct ml666_st_node* src);

typedef bool ml666_st_cb_member_set(struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before);
typedef void ml666_st_cb_subtree_disintegrate(struct ml666_st_children* children);

typedef struct ml666_st_document* ml666_st_cb_document_create(void);
typedef struct ml666_st_element* ml666_st_cb_content_create(void);
typedef struct ml666_st_element* ml666_st_cb_comment_create(void);
typedef struct ml666_st_element* ml666_st_cb_element_create(const struct ml666_hashed_buffer* entry, bool copy_name);

typedef enum ml666_st_node_type ml666_st_cb_node_get_type(struct ml666_st_node* node);
typedef struct ml666_st_children* ml666_st_cb_document_get_children(struct ml666_st_document* node);
typedef struct ml666_st_children* ml666_st_cb_element_get_children(struct ml666_st_element* node);
typedef struct ml666_st_children* ml666_st_cb_node_get_children(struct ml666_st_node* node);

typedef struct ml666_st_node* ml666_st_cb_member_get_parent(struct ml666_st_member* node);
typedef struct ml666_st_member* ml666_st_cb_member_get_previous(struct ml666_st_member* node);
typedef struct ml666_st_member* ml666_st_cb_member_get_next(struct ml666_st_member* node);

typedef const struct ml666_hashed_buffer_set_entry* ml666_st_cb_element_get_name(struct ml666_st_element* node);

typedef bool ml666_st_cb_content_set(struct ml666_buffer node);
typedef struct ml666_buffer ml666_st_cb_content_get(struct ml666_st_node* node);
typedef bool ml666_st_cb_comment_set(struct ml666_buffer node);
typedef struct ml666_buffer ml666_st_cb_comment_get(struct ml666_st_node* node);

#define ML666_ST_CB(X, ...) \
  X(node_put, __VA_ARGS__) \
  X(node_ref, __VA_ARGS__) \
  X(node_ref_set, __VA_ARGS__) \
  \
  X(member_set, __VA_ARGS__) \
  X(subtree_disintegrate, __VA_ARGS__) \
  \
  X(document_create, __VA_ARGS__) \
  X(content_create, __VA_ARGS__) \
  X(comment_create, __VA_ARGS__) \
  X(element_create, __VA_ARGS__) \
  \
  X(node_get_type, __VA_ARGS__) \
  X(document_get_children, __VA_ARGS__) \
  X(element_get_children, __VA_ARGS__) \
  X(node_get_children, __VA_ARGS__) \
  \
  X(member_get_parent, __VA_ARGS__) \
  X(member_get_previous, __VA_ARGS__) \
  X(member_get_next, __VA_ARGS__) \
  \
  X(element_get_name, __VA_ARGS__) \
  \
  X(content_set, __VA_ARGS__) \
  X(content_get, __VA_ARGS__) \
  X(comment_set, __VA_ARGS__) \
  X(comment_get, __VA_ARGS__) \

#define ML666__ST_DECLARATION_SUB(FUNC, PREFIX) ml666_st_cb_ ## FUNC PREFIX ## _ ## FUNC;
#define ML666__ST_IMPLEMENTATION_SUB(X, PREFIX) .X = PREFIX ## _ ## X,

#define ML666_ST_DECLARATION(NAME, PREFIX) \
  extern const struct ml666_st_cb NAME ## _st_api; \
  ML666_ST_CB(ML666__ST_DECLARATION_SUB, PREFIX)

#define ML666_ST_IMPLEMENTATION(NAME, PREFIX) \
  ML666_ST_DECLARATION(NAME, PREFIX) \
  const struct ml666_st_cb NAME ## _st_api = { \
    ML666_ST_CB(ML666__ST_IMPLEMENTATION_SUB, PREFIX) \
  };


// Interface
struct ml666_st_cb {
#define X(FUNC, _) ml666_st_cb_ ## FUNC* FUNC;
  ML666_ST_CB(X, _)
#undef X
};

// Default API
ML666_ST_DECLARATION(ml666_default, ml666_st__d_)

#endif
