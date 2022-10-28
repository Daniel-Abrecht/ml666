#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/common.h>
#include <stddef.h>
#include <stdbool.h>

/** \addtogroup ml666-simple-tree Simple Tree API
 * @{ */

/**
 * The type of an \ref ml666_st_node_t.
 */
enum ml666_st_node_type {
  ML666_ST_NT_DOCUMENT, ///< See \ref ml666_st_document_t
  ML666_ST_NT_ELEMENT, ///< See \ref ml666_st_element_t
  ML666_ST_NT_CONTENT, ///< See \ref ml666_st_content_t
  ML666_ST_NT_COMMENT, ///< See \ref ml666_st_comment_t
};

/**
 * This is an opaque type. It represents an attribute.
 * They are always part of an \ref ml666_st_element_t.
 *
 * \see ml666_st_attribute_get_first
 * \see ml666_st_attribute_get_next
 * \see ml666_st_attribute_lookup
 * \see ml666_st_attribute_remove
 * \see ml666_st_attribute_get_name
 * \see ml666_st_attribute_set_value
 * \see ml666_st_attribute_get_value
 * \see ml666_st_attribute_take_value
 */
typedef struct ml666_st_attribute ml666_st_attribute_t;
/**
 * This is an opaque type. It represents a child node list.
 * A child node list always belongs to an \ref ml666_st_document_t or \ref ml666_st_element_t.
 * All nodes are refcounted, but not necessarely GC managed.
 * The default implementation doesn't implement a GC yet.
 *
 * \see ML666_ST_CHILDREN
 * \see ML666_ST_U_CHILDREN
 */
typedef struct ml666_st_children ml666_st_children_t;

/**
 * This is an opaque, implementation specific type. It is the base type of all entries in the simple tree.
 * It's first member must be \ref ml666_st_node_type, but there are no further requirements.
 * \see ML666_ST_NODE
 */
typedef struct ml666_st_node ml666_st_node_t;
/**
 * This is an opaque, implementation specific type. It's the base type of \ref ml666_st_element_t, \ref ml666_st_content_t, \ref ml666_st_comment_t.
 * An ml666_st_member can have a parent node.
 * It's first member must be \ref ml666_st_node_t, but there are no further requirements.
 * \see ML666_ST_MEMBER
 * \see ML666_ST_U_MEMBER
 */
typedef struct ml666_st_member ml666_st_member_t;
/**
 * This is an opaque, implementation specific type. This node represents the document.
 * It is just a collection of nodes, it represents all of them. It itself can't be a child of any other node.
 * It's first member must be \ref ml666_st_node_t, but there are no further requirements.
 */
typedef struct ml666_st_document ml666_st_document_t;
/**
 * This is an opaque, implementation specific type. This node represents an element in the simple tree.
 * Those are nodes which can have child nodes, can be a child of other nodes, and can have attributes.
 * It's first member must be \ref ml666_st_member_t, but there are no further requirements.
 * \see ML666_ST_U_ELEMENT
 */
typedef struct ml666_st_element ml666_st_element_t;
/**
 * This is an opaque, implementation specific type. This node represents some text or some binary data.
 * It's first member must be \ref ml666_st_member_t, but there are no further requirements.
 * \see ML666_ST_U_CONTENT
 */
typedef struct ml666_st_content ml666_st_content_t;
/**
 * This is an opaque, implementation specific type. This node represents a comment.
 * It's first member must be \ref ml666_st_member_t, but there are no further requirements.
 * \see ML666_ST_U_COMMENT
 */
typedef struct ml666_st_comment ml666_st_comment_t;

typedef struct ml666_st_builder {
  const struct ml666_st_cb*const cb;
  void* user_ptr;
} ml666_st_builder_t;

/**
 * Get the type of any node, member, element, etc.
 * \returns ml666_st_node_type
 */
#define ML666_ST_TYPE(X) \
  _Generic((X), \
    ml666_st_node_t*: *(const enum ml666_st_node_type*)(X), \
    ml666_st_member_t*: *(const enum ml666_st_node_type*)(X), \
    ml666_st_document_t*: *(const enum ml666_st_node_type*)(X), \
    ml666_st_element_t*: *(const enum ml666_st_node_type*)(X), \
    ml666_st_content_t*: *(const enum ml666_st_node_type*)(X), \
    ml666_st_comment_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_node_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_member_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_document_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_element_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_content_t*: *(const enum ml666_st_node_type*)(X), \
    const ml666_st_comment_t*: *(const enum ml666_st_node_type*)(X) \
  )

/**
 * Safe downcast to a node.
 */
#define ML666_ST_NODE(X) _Generic((X), \
    ml666_st_document_t*: (ml666_st_node_t*)(X), \
    ml666_st_member_t*: (ml666_st_node_t*)(X), \
    ml666_st_element_t*: (ml666_st_node_t*)(X), \
    ml666_st_content_t*: (ml666_st_node_t*)(X), \
    ml666_st_comment_t*: (ml666_st_node_t*)(X), \
    const ml666_st_document_t*: (const ml666_st_node_t*)(X), \
    const ml666_st_member_t*: (const ml666_st_node_t*)(X), \
    const ml666_st_element_t*: (const ml666_st_node_t*)(X), \
    const ml666_st_content_t*: (const ml666_st_node_t*)(X), \
    const ml666_st_comment_t*: (const ml666_st_node_t*)(X) \
  )

/** Safe downcast to an \ref ml666_st_member_t */
#define ML666_ST_MEMBER(X) _Generic((X), \
    ml666_st_element_t*: (ml666_st_member_t*)(X), \
    ml666_st_content_t*: (ml666_st_member_t*)(X), \
    ml666_st_comment_t*: (ml666_st_member_t*)(X), \
    const ml666_st_element_t*: (const ml666_st_member_t*)(X), \
    const ml666_st_content_t*: (const ml666_st_member_t*)(X), \
    const ml666_st_comment_t*: (const ml666_st_member_t*)(X) \
  )

/**
 * Safe upcast to an \ref ml666_st_member_t
 * \returns the \ref ml666_st_member_t, or 0 if it was not an \ref ml666_st_member_t
 */
#define ML666_ST_U_MEMBER(X) _Generic((X), \
    ml666_st_node_t*: ml666_st_is_member((X)) ? (ml666_st_member_t*)(X) : 0, \
    const ml666_st_node_t*: ml666_st_is_member((X)) ? (const ml666_st_member_t*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_element_t
 * \returns the \ref ml666_st_element_t, or 0 if it was not an \ref ml666_st_element_t
 */
#define ML666_ST_U_ELEMENT(X) _Generic((X), \
    ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (ml666_st_element_t*)(X) : 0, \
    ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (ml666_st_element_t*)(X) : 0, \
    const ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const ml666_st_element_t*)(X) : 0, \
    const ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const ml666_st_element_t*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_content_t
 * \returns the \ref ml666_st_content_t, or 0 if it was not an \ref ml666_st_content_t
 */
#define ML666_ST_U_CONTENT(X) _Generic((X), \
    ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (ml666_st_content_t*)(X) : 0, \
    ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (ml666_st_content_t*)(X) : 0, \
    const ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const ml666_st_content_t*)(X) : 0, \
    const ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const ml666_st_content_t*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_comment_t
 * \returns the \ref ml666_st_comment_t, or 0 if it was not an \ref ml666_st_comment_t
 */
#define ML666_ST_U_COMMENT(X) _Generic((X), \
    ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (ml666_st_comment_t*)(X) : 0, \
    ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (ml666_st_comment_t*)(X) : 0, \
    const ml666_st_node_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const ml666_st_comment_t*)(X) : 0, \
    const ml666_st_member_t*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const ml666_st_comment_t*)(X) : 0 \
  )

/** Safe downcast to an \ref ml666_st_children_t */
#define ML666_ST_CHILDREN(STB, X) _Generic((X), \
    ml666_st_document_t*: ml666_st_document_get_children((STB), (void*)(X)), \
    ml666_st_element_t*: ml666_st_element_get_children((STB), (void*)(X)), \
    const ml666_st_document_t*: (const struct ml666_st_children*)ml666_st_document_get_children((STB), (void*)(X)), \
    const ml666_st_element_t*: (const struct ml666_st_children*)ml666_st_element_get_children((STB), (void*)(X)) \
  )

/**
 * Safe upcast to \ref ml666_st_children_t.
 * \returns the \ref ml666_st_children_t, or 0 if it was not an \ref ml666_st_children_t
 */
#define ML666_ST_U_CHILDREN(STB, X) _Generic((X), \
    ml666_st_node_t*: ml666_st_node_get_children((STB), (void*)(X)), \
    ml666_st_member_t*: ml666_st_node_get_children((STB), (void*)(X)), \
    const ml666_st_node_t*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)), \
    const ml666_st_member_t*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)) \
  )

/**
 * The flags for \ref ml666_st_attribute_lookup.
 * These can be combined.
 */
enum ml666_st_attribute_lookup_flags {
  ML666_ST_AOF_CREATE           = (1<<0), ///< If the attribute was not found, create it
  ML666_ST_AOF_CREATE_EXCLUSIVE = (1<<1) | ML666_ST_AOF_CREATE, ///< Implies ML666_ST_AOF_CREATE. Create the attribute, and fail if it already exists.
  ML666_ST_AOF_CREATE_NOCOPY    = (2<<1) | ML666_ST_AOF_CREATE, ///< Implies ML666_ST_AOF_CREATE. Take ownership of the attribute names buffer data, do not copy it.
};

#define ML666__ST_CB(X, ...) \
  X(builder_destroy /** \see ml666_st_builder_destroy */, void, (struct ml666_st_builder* stb), __VA_ARGS__) \
  \
  X(node_put /** \see ml666_st_node_put */, void, (struct ml666_st_builder* stb, ml666_st_node_t* node), __VA_ARGS__) \
  X(node_ref /** \see ml666_st_node_ref */, bool, (struct ml666_st_builder* stb, ml666_st_node_t* node), __VA_ARGS__) \
  \
  X(member_set /** \see ml666_st_member_set */, bool, (struct ml666_st_builder* stb, ml666_st_node_t* parent, ml666_st_member_t* member, ml666_st_member_t* before), __VA_ARGS__) \
  \
  X(document_create /** \see ml666_st_document_create */, ml666_st_document_t*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(element_create /** \see ml666_st_element_create */, ml666_st_element_t*, (struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name), __VA_ARGS__) \
  X(content_create /** \see ml666_st_content_create */, ml666_st_content_t*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(comment_create /** \see ml666_st_comment_create */, ml666_st_comment_t*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  \
  X(document_get_children /** \see ml666_st_document_get_children */, struct ml666_st_children*, (struct ml666_st_builder* stb, ml666_st_document_t* node), __VA_ARGS__) \
  X(element_get_children /** \see ml666_st_element_get_children */, struct ml666_st_children*, (struct ml666_st_builder* stb, ml666_st_element_t* node), __VA_ARGS__) \
  \
  X(member_get_parent /** \see ml666_st_member_get_parent */, ml666_st_node_t*, (struct ml666_st_builder* stb, ml666_st_member_t* node), __VA_ARGS__) \
  X(member_get_previous /** \see ml666_st_member_get_previous */, ml666_st_member_t*, (struct ml666_st_builder* stb, ml666_st_member_t* node), __VA_ARGS__) \
  X(member_get_next /** \see ml666_st_member_get_next */, ml666_st_member_t*, (struct ml666_st_builder* stb, ml666_st_member_t* node), __VA_ARGS__) \
  \
  X(element_get_name /** \see ml666_st_element_get_name */, const struct ml666_hashed_buffer_set_entry*, (struct ml666_st_builder* stb, ml666_st_element_t* node), __VA_ARGS__) \
  \
  X(content_set /** \see ml666_st_content_set */, bool, (struct ml666_st_builder* stb, ml666_st_content_t* content, struct ml666_buffer node), __VA_ARGS__) \
  X(content_get /** \see ml666_st_content_get */, struct ml666_buffer_ro, (struct ml666_st_builder* stb, const ml666_st_content_t* content), __VA_ARGS__) \
  X(content_take /** \see ml666_st_content_take */, struct ml666_buffer, (struct ml666_st_builder* stb, ml666_st_content_t* content), __VA_ARGS__) \
  X(comment_set /** \see ml666_st_comment_set */, bool, (struct ml666_st_builder* stb, ml666_st_comment_t* comment, struct ml666_buffer node), __VA_ARGS__) \
  X(comment_get /** \see ml666_st_comment_get */, struct ml666_buffer_ro, (struct ml666_st_builder* stb, const ml666_st_comment_t* comment), __VA_ARGS__) \
  X(comment_take /** \see ml666_st_comment_take */, struct ml666_buffer, (struct ml666_st_builder* stb, ml666_st_comment_t* comment), __VA_ARGS__) \
  \
  X(attribute_get_first /** \see ml666_st_attribute_get_first */, struct ml666_st_attribute*, (const struct ml666_st_builder* stb, const ml666_st_element_t* element), __VA_ARGS__) \
  X(attribute_get_next /** \see ml666_st_attribute_get_next */, struct ml666_st_attribute*, (const struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_lookup /** \see ml666_st_attribute_lookup */, struct ml666_st_attribute*, (struct ml666_st_builder* stb, ml666_st_element_t* element, const struct ml666_hashed_buffer* name, enum ml666_st_attribute_lookup_flags flags), __VA_ARGS__) \
  X(attribute_remove /** \see ml666_st_attribute_remove */, void, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_get_name /** \see ml666_st_attribute_get_name */, const struct ml666_hashed_buffer*, (struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_set_value /** \see ml666_st_attribute_set_value */, bool, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute, struct ml666_buffer* value), __VA_ARGS__) \
  X(attribute_get_value /** \see ml666_st_attribute_get_value */, const struct ml666_buffer_ro*, (struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_take_value /** \see ml666_st_attribute_take_value */, const struct ml666_buffer*, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute), __VA_ARGS__) \
  \
  X(get_first_child /** \see ml666_st_get_first_child */, ml666_st_member_t*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__) \
  X(get_last_child /** \see ml666_st_get_last_child */, ml666_st_member_t*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__)


#define ML666__ST_DECLARATION_SUB(FUNC, _1, _2, PREFIX) ML666_EXPORT ml666_st_cb_ ## FUNC PREFIX ## _ ## FUNC;
#define ML666__ST_IMPLEMENTATION_SUB(X, _1, _2, PREFIX) .X = PREFIX ## _ ## X,

/**
 * Declares the functions & the struct contaiing the callbacks of the \ref ml666_st_builder implementation.
 *
 * Declares the following:
 * ```
 * ML666_EXPORT extern const struct ml666_st_cb NAME##_st_api;
 * ml666_st_cb_##FUNC PREFIX##_##FUNC;
 * ```
 * Where `NAME` and `PREFIX` are parameters of this macro, and `FUNC` represents all
 * the callback function names from \ref ml666_st_cb.
 *
 * \see ml666_default_st_api for the default implementation
 */
#define ML666_ST_DECLARATION(NAME, PREFIX) \
  ML666_EXPORT extern const struct ml666_st_cb NAME ## _st_api; \
  ML666__ST_CB(ML666__ST_DECLARATION_SUB, PREFIX)

#define ML666_ST_IMPLEMENTATION(NAME, PREFIX) \
  ML666_ST_DECLARATION(NAME, PREFIX) \
  const struct ml666_st_cb NAME ## _st_api = { \
    ML666__ST_CB(ML666__ST_IMPLEMENTATION_SUB, PREFIX) \
  };


// Callback function types
#define X(FUNC, RET, PARAMS, _) typedef RET ml666_st_cb_ ## FUNC PARAMS;
ML666__ST_CB(X, _)
#undef X

/**
 * The callbacks of the \ref ml666_st_builder implementation,
 * implementing the \ref ml666-simple-tree.
 * \see ml666_st_builder for more information
 */
struct ml666_st_cb {
#define X2(FUNC, _1, _2, _3) ml666_st_cb_ ## FUNC* FUNC;
  ML666__ST_CB(X2, _)
#undef X2
};


// Simple wrappers

static inline void ml666_st_builder_destroy(struct ml666_st_builder* stb){
  stb->cb->builder_destroy(stb);
}

static inline void ml666_st_node_put(struct ml666_st_builder* stb, ml666_st_node_t* node){
  stb->cb->node_put(stb, node);
}
static inline bool ml666_st_node_ref(struct ml666_st_builder* stb, ml666_st_node_t* node){
  return stb->cb->node_ref(stb, node);
}

static inline bool ml666_st_member_set(struct ml666_st_builder* stb, ml666_st_node_t* parent, ml666_st_member_t* member, ml666_st_member_t* before){
  return stb->cb->member_set(stb, parent, member, before);
}

static inline ml666_st_document_t* ml666_st_document_create(struct ml666_st_builder* stb){
  return stb->cb->document_create(stb);
}
static inline ml666_st_element_t* ml666_st_element_create(struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name){
  return stb->cb->element_create(stb, entry, copy_name);
}
static inline ml666_st_content_t* ml666_st_content_create(struct ml666_st_builder* stb){
  return stb->cb->content_create(stb);
}
static inline ml666_st_comment_t* ml666_st_comment_create(struct ml666_st_builder* stb){
  return stb->cb->comment_create(stb);
}

static inline ml666_st_children_t* ml666_st_document_get_children(struct ml666_st_builder* stb, ml666_st_document_t* node){
  return stb->cb->document_get_children(stb, node);
}
static inline ml666_st_children_t* ml666_st_element_get_children(struct ml666_st_builder* stb, ml666_st_element_t* node){
  return stb->cb->element_get_children(stb, node);
}

static inline ml666_st_node_t* ml666_st_member_get_parent(struct ml666_st_builder* stb, ml666_st_member_t* node){
  return stb->cb->member_get_parent(stb, node);
}
static inline ml666_st_member_t* ml666_st_member_get_previous(struct ml666_st_builder* stb, ml666_st_member_t* node){
  return stb->cb->member_get_previous(stb, node);
}
static inline ml666_st_member_t* ml666_st_member_get_next(struct ml666_st_builder* stb, ml666_st_member_t* node){
  return stb->cb->member_get_next(stb, node);
}

static inline const struct ml666_hashed_buffer_set_entry* ml666_st_element_get_name(struct ml666_st_builder* stb, ml666_st_element_t* node){
  return stb->cb->element_get_name(stb, node);
}

static inline bool ml666_st_content_set(struct ml666_st_builder* stb, ml666_st_content_t* content, struct ml666_buffer node){
  return stb->cb->content_set(stb, content, node);
}
static inline struct ml666_buffer_ro ml666_st_content_get(struct ml666_st_builder* stb, const ml666_st_content_t* content){
  return stb->cb->content_get(stb, content);
}
static inline struct ml666_buffer ml666_st_content_take(struct ml666_st_builder* stb, ml666_st_content_t* content){
  return stb->cb->content_take(stb, content);
}
static inline bool ml666_st_comment_set(struct ml666_st_builder* stb, ml666_st_comment_t* comment, struct ml666_buffer node){
  return stb->cb->comment_set(stb, comment, node);
}
static inline struct ml666_buffer_ro ml666_st_comment_get(struct ml666_st_builder* stb, const ml666_st_comment_t* comment){
  return stb->cb->comment_get(stb, comment);
}
static inline struct ml666_buffer ml666_st_comment_take(struct ml666_st_builder* stb, ml666_st_comment_t* comment){
  return stb->cb->comment_take(stb, comment);
}

static inline ml666_st_attribute_t* ml666_st_attribute_get_first(const struct ml666_st_builder* stb, const ml666_st_element_t* element){
  return stb->cb->attribute_get_first(stb, element);
}

static inline ml666_st_attribute_t* ml666_st_attribute_get_next(const struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  return stb->cb->attribute_get_next(stb, attribute);
}

static inline ml666_st_attribute_t* ml666_st_attribute_lookup(struct ml666_st_builder* stb, ml666_st_element_t* element, const struct ml666_hashed_buffer* name, enum ml666_st_attribute_lookup_flags flags){
  return stb->cb->attribute_lookup(stb, element, name, flags);
}
static inline void ml666_st_attribute_remove(struct ml666_st_builder* stb, struct ml666_st_attribute* attribute){
  stb->cb->attribute_remove(stb, attribute);
}
static inline const struct ml666_hashed_buffer* ml666_st_attribute_get_name(struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  return stb->cb->attribute_get_name(stb, attribute);
}
static inline bool ml666_st_attribute_set_value(struct ml666_st_builder* stb, struct ml666_st_attribute* attribute, struct ml666_buffer* value){
  return stb->cb->attribute_set_value(stb, attribute, value);
}
static inline const struct ml666_buffer_ro* ml666_st_attribute_get_value(struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  return stb->cb->attribute_get_value(stb, attribute);
}
static inline const struct ml666_buffer* ml666_st_attribute_take_value(struct ml666_st_builder* stb, struct ml666_st_attribute* attribute){
  return stb->cb->attribute_take_value(stb, attribute);
}

static inline ml666_st_member_t* ml666_st_get_first_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  if(!children)
    return 0;
  return stb->cb->get_first_child(stb, children);
}
static inline ml666_st_member_t* ml666_st_get_last_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  if(!children)
    return 0;
  return stb->cb->get_last_child(stb, children);
}


// Simple generic helper functions
static inline bool ml666_st_is_member(const ml666_st_node_t* node){
  enum ml666_st_node_type type = ML666_ST_TYPE(node);
  switch(type){
    case ML666_ST_NT_DOCUMENT: return false;
    case ML666_ST_NT_ELEMENT: return true;
    case ML666_ST_NT_CONTENT: return true;
    case ML666_ST_NT_COMMENT: return true;
  }
  return false;
}

static inline bool ml666_st_node_ref_set(struct ml666_st_builder* stb, ml666_st_node_t** dest, ml666_st_node_t* src){
  if(src && !stb->cb->node_ref(stb, src))
    return false;
  ml666_st_node_t* old = *dest;
  if(old)
    stb->cb->node_put(stb, old);
  *dest = src;
  return true;
}

static inline ml666_st_children_t* ml666_st_node_get_children(struct ml666_st_builder* stb, ml666_st_node_t* node){
  switch(ML666_ST_TYPE(node)){
    case ML666_ST_NT_DOCUMENT: return ml666_st_document_get_children(stb, (ml666_st_document_t*)node);
    case ML666_ST_NT_ELEMENT : return ml666_st_element_get_children(stb, (ml666_st_element_t*)node);
    default: return 0;
  }
}

static inline void ml666_st_subtree_disintegrate(struct ml666_st_builder* stb, struct ml666_st_children* children){
  for(ml666_st_member_t* it; (it=ml666_st_get_first_child(stb, children)); ){
    struct ml666_st_children* chch = ml666_st_node_get_children(stb, ML666_ST_NODE(it));
    if(chch)
      ml666_st_subtree_disintegrate(stb, chch);
    // It's important for this to be depth first.
    // When the parent looses it's parent and children, it may get freed, as noone may be holding a reference anymore.
    // But by removing the parent here, our unknown parent still has a reference
    ml666_st_member_set(stb, 0, it, 0);
  }
}

/** @} */

#endif
