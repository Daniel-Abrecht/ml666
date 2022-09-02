#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/utils.h>
#include <stddef.h>

// Must be the first member of struct ml666_st_node
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

struct ml666_st_builder {
  const struct ml666_st_cb*const cb;
};

#define ML666_ST_TYPE(X) \
  _Generic((X), \
    struct ml666_st_node*: *(const enum ml666_st_node_type*)(X), \
    struct ml666_st_member*: *(const enum ml666_st_node_type*)(X), \
    struct ml666_st_document*: *(const enum ml666_st_node_type*)(X), \
    struct ml666_st_element*: *(const enum ml666_st_node_type*)(X), \
    struct ml666_st_content*: *(const enum ml666_st_node_type*)(X), \
    struct ml666_st_comment*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_node*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_member*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_document*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_element*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_content*: *(const enum ml666_st_node_type*)(X), \
    const struct ml666_st_comment*: *(const enum ml666_st_node_type*)(X) \
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

#define ML666_ST_MEMBER(X) _Generic((X), \
    struct ml666_st_element*: (struct ml666_st_member*)(X), \
    struct ml666_st_content*: (struct ml666_st_member*)(X), \
    struct ml666_st_comment*: (struct ml666_st_member*)(X), \
    const struct ml666_st_element*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_content*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_comment*: (const struct ml666_st_member*)(X) \
  )

#define ML666_ST_U_MEMBER(X) _Generic((X), \
    struct ml666_st_node*: ml666_st_is_member((X)) ? (struct ml666_st_member*)(X) : 0, \
    const struct ml666_st_node*: ml666_st_is_member((X)) ? (const struct ml666_st_member*)(X) : 0 \
  )

#define ML666_ST_U_ELEMENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (struct ml666_st_element*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (struct ml666_st_element*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const struct ml666_st_element*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const struct ml666_st_element*)(X) : 0 \
  )

#define ML666_ST_U_CONTENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (struct ml666_st_content*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (struct ml666_st_content*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const struct ml666_st_content*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const struct ml666_st_content*)(X) : 0 \
  )

#define ML666_ST_U_COMMENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (struct ml666_st_comment*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (struct ml666_st_comment*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const struct ml666_st_comment*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const struct ml666_st_comment*)(X) : 0 \
  )

#define ML666_ST_CHILDREN(STB, X) _Generic((X), \
    struct ml666_st_document*: ml666_st_document_get_children((STB), (void*)(X)), \
    struct ml666_st_element*: ml666_st_element_get_children((STB), (void*)(X)), \
    const struct ml666_st_document*: (const struct ml666_st_children*)ml666_st_document_get_children((STB), (void*)(X)), \
    const struct ml666_st_element*: (const struct ml666_st_children*)ml666_st_element_get_children((STB), (void*)(X)) \
  )

#define ML666_ST_U_CHILDREN(STB, X) _Generic((X), \
    struct ml666_st_node*: ml666_st_node_get_children((STB), (void*)(X)), \
    struct ml666_st_member*: ml666_st_node_get_children((STB), (void*)(X)), \
    const struct ml666_st_node*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)), \
    const struct ml666_st_member*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)) \
  )


#define ML666_ST_CB(X, ...) \
  X(node_put, void, (struct ml666_st_builder* stb, struct ml666_st_node* node), __VA_ARGS__) \
  X(node_ref, bool, (struct ml666_st_builder* stb, struct ml666_st_node* node), __VA_ARGS__) \
  \
  X(member_set, bool, (struct ml666_st_builder* stb, struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before), __VA_ARGS__) \
  \
  X(document_create, struct ml666_st_document*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(element_create, struct ml666_st_element*, (struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name), __VA_ARGS__) \
  X(content_create, struct ml666_st_content*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(comment_create, struct ml666_st_comment*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  \
  X(document_get_children, struct ml666_st_children*, (struct ml666_st_builder* stb, struct ml666_st_document* node), __VA_ARGS__) \
  X(element_get_children, struct ml666_st_children*, (struct ml666_st_builder* stb, struct ml666_st_element* node), __VA_ARGS__) \
  \
  X(member_get_parent, struct ml666_st_node*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  X(member_get_previous, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  X(member_get_next, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  \
  X(element_get_name, const struct ml666_hashed_buffer_set_entry*, (struct ml666_st_builder* stb, struct ml666_st_element* node), __VA_ARGS__) \
  \
  X(content_set, bool, (struct ml666_st_builder* stb, struct ml666_st_content* content, struct ml666_buffer node), __VA_ARGS__) \
  X(content_get, struct ml666_buffer, (struct ml666_st_builder* stb, const struct ml666_st_content* content), __VA_ARGS__) \
  X(comment_set, bool, (struct ml666_st_builder* stb, struct ml666_st_comment* comment, struct ml666_buffer node), __VA_ARGS__) \
  X(comment_get, struct ml666_buffer, (struct ml666_st_builder* stb, const struct ml666_st_comment* comment), __VA_ARGS__) \
  \
  X(get_first_child, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__) \
  X(get_last_child, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__) \


#define ML666__ST_DECLARATION_SUB(FUNC, _1, _2, PREFIX) ml666_st_cb_ ## FUNC PREFIX ## _ ## FUNC;
#define ML666__ST_IMPLEMENTATION_SUB(X, _1, _2, PREFIX) .X = PREFIX ## _ ## X,

#define ML666_ST_DECLARATION(NAME, PREFIX) \
  extern const struct ml666_st_cb NAME ## _st_api; \
  ML666_ST_CB(ML666__ST_DECLARATION_SUB, PREFIX)

#define ML666_ST_IMPLEMENTATION(NAME, PREFIX) \
  ML666_ST_DECLARATION(NAME, PREFIX) \
  const struct ml666_st_cb NAME ## _st_api = { \
    ML666_ST_CB(ML666__ST_IMPLEMENTATION_SUB, PREFIX) \
  };


// Callback function types
#define X(FUNC, RET, PARAMS, _) typedef RET ml666_st_cb_ ## FUNC PARAMS;
ML666_ST_CB(X, _)
#undef X

// Interface
struct ml666_st_cb {
#define X(FUNC, _1, _2, _3) ml666_st_cb_ ## FUNC* FUNC;
  ML666_ST_CB(X, _)
#undef X
};


// Simple wrappers

static inline void ml666_st_node_put(struct ml666_st_builder* stb, struct ml666_st_node* node){
  stb->cb->node_put(stb, node);
}
static inline bool ml666_st_node_ref(struct ml666_st_builder* stb, struct ml666_st_node* node){
  return stb->cb->node_ref(stb, node);
}

static inline bool ml666_st_member_set(struct ml666_st_builder* stb, struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before){
  return stb->cb->member_set(stb, parent, member, before);
}

static inline struct ml666_st_document* ml666_st_document_create(struct ml666_st_builder* stb){
  return stb->cb->document_create(stb);
}
static inline struct ml666_st_element* ml666_st_element_create(struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name){
  return stb->cb->element_create(stb, entry, copy_name);
}
static inline struct ml666_st_content* ml666_st_content_create(struct ml666_st_builder* stb){
  return stb->cb->content_create(stb);
}
static inline struct ml666_st_comment* ml666_st_comment_create(struct ml666_st_builder* stb){
  return stb->cb->comment_create(stb);
}

static inline struct ml666_st_children* ml666_st_document_get_children(struct ml666_st_builder* stb, struct ml666_st_document* node){
  return stb->cb->document_get_children(stb, node);
}
static inline struct ml666_st_children* ml666_st_element_get_children(struct ml666_st_builder* stb, struct ml666_st_element* node){
  return stb->cb->element_get_children(stb, node);
}

static inline struct ml666_st_node* ml666_st_member_get_parent(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_parent(stb, node);
}
static inline struct ml666_st_member* ml666_st_member_get_previous(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_previous(stb, node);
}
static inline struct ml666_st_member* ml666_st_member_get_next(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_next(stb, node);
}

static inline const struct ml666_hashed_buffer_set_entry* ml666_st_element_get_name(struct ml666_st_builder* stb, struct ml666_st_element* node){
  return stb->cb->element_get_name(stb, node);
}

static inline bool ml666_st_content_set(struct ml666_st_builder* stb, struct ml666_st_content* content, struct ml666_buffer node){
  return stb->cb->content_set(stb, content, node);
}
static inline struct ml666_buffer ml666_st_content_get(struct ml666_st_builder* stb, const struct ml666_st_content* content){
  return stb->cb->content_get(stb, content);
}
static inline bool ml666_st_comment_set(struct ml666_st_builder* stb, struct ml666_st_comment* comment, struct ml666_buffer node){
  return stb->cb->comment_set(stb, comment, node);
}
static inline struct ml666_buffer ml666_st_comment_get(struct ml666_st_builder* stb, const struct ml666_st_comment* comment){
  return stb->cb->comment_get(stb, comment);
}

static inline struct ml666_st_member* ml666_st_get_first_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  return stb->cb->get_first_child(stb, children);
}
static inline struct ml666_st_member* ml666_st_get_last_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  return stb->cb->get_last_child(stb, children);
}


// Simple generic helper functions
static inline bool ml666_st_is_member(const struct ml666_st_node* node){
  enum ml666_st_node_type type = ML666_ST_TYPE(node);
  switch(type){
    case ML666_ST_NT_DOCUMENT: return false;
    case ML666_ST_NT_ELEMENT: return true;
    case ML666_ST_NT_CONTENT: return true;
    case ML666_ST_NT_COMMENT: return true;
  }
}

static inline bool ml666_st_node_ref_set(struct ml666_st_builder* stb, struct ml666_st_node** dest, struct ml666_st_node* src){
  if(src && !stb->cb->node_ref(stb, src))
    return false;
  struct ml666_st_node* old = *dest;
  if(old)
    stb->cb->node_put(stb, old);
  *dest = src;
  return true;
}

static inline struct ml666_st_children* ml666_st_node_get_children(struct ml666_st_builder* stb, struct ml666_st_node* node){
  switch(ML666_ST_TYPE(node)){
    case ML666_ST_NT_DOCUMENT: return ml666_st_document_get_children(stb, (struct ml666_st_document*)node);
    case ML666_ST_NT_ELEMENT : return ml666_st_element_get_children(stb, (struct ml666_st_element*)node);
    default: return 0;
  }
}

static inline void ml666_st_subtree_disintegrate(struct ml666_st_builder* stb, struct ml666_st_children* children){
  for(struct ml666_st_member* it; (it=ml666_st_get_first_child(stb, children)); ){
    struct ml666_st_children* chch = ml666_st_node_get_children(stb, ML666_ST_NODE(it));
    if(chch)
      ml666_st_subtree_disintegrate(stb, chch);
    // It's important for this to be depth first.
    // When the parent looses it's parent and children, it may get freed, as noone may be holding a reference anymore.
    // But by removing the parent here, our unknown parent still has a reference
    ml666_st_member_set(stb, 0, it, 0);
  }
}

#endif
