#ifndef ML666_SIMPLE_TREE
#define ML666_SIMPLE_TREE

#include <ml666/common.h>
#include <stddef.h>
#include <stdbool.h>

/** \addtogroup ml666-simple-tree Simple Tree API
 * @{ */

/**
 * The type of an \ref ml666_st_node.
 */
enum ml666_st_node_type {
  ML666_ST_NT_DOCUMENT, ///< See \ref ml666_st_document
  ML666_ST_NT_ELEMENT, ///< See \ref ml666_st_element
  ML666_ST_NT_CONTENT, ///< See \ref ml666_st_content
  ML666_ST_NT_COMMENT, ///< See \ref ml666_st_comment
};

/**
 * \struct ml666_st_attribute
 *
 * This is an opaque type. It represents an attribute.
 * They are always part of an \ref ml666_st_element.
 *
 */
typedef struct ml666_st_attribute ml666_st_attribute_t;

/**
 * \struct ml666_st_children
 * This is an opaque type. It represents a child node list.
 * A child node list always belongs to an \ref ml666_st_document or \ref ml666_st_element.
 * All nodes are refcounted, but not necessarely GC managed.
 * The default implementation doesn't implement a GC yet.
 *
 * \see ML666_ST_CHILDREN
 * \see ML666_ST_U_CHILDREN
 */
typedef struct ml666_st_children ml666_st_children_t;

/**
 * \struct ml666_st_node
 * This is an opaque, implementation specific type. It is the base type of all entries in the simple tree.
 * It's first member must be \ref ml666_st_node_type, but there are no further requirements.
 *
 * \see ML666_ST_NODE
 */
typedef struct ml666_st_node ml666_st_node_t;

/**
 * \struct ml666_st_member
 * This is an opaque, implementation specific type. It's the base type of \ref ml666_st_element, \ref ml666_st_content, \ref ml666_st_comment.
 * An ml666_st_member can have a parent node.
 * It's first member must be \ref ml666_st_node, but there are no further requirements.
 *
 * \extends ml666_st_node
 *
 * \see ML666_ST_MEMBER
 * \see ML666_ST_U_MEMBER
 */
typedef struct ml666_st_member ml666_st_member_t;

/**
 * \struct ml666_st_document
 * This is an opaque, implementation specific type. This node represents the document.
 * It is just a collection of nodes, it represents all of them. It itself can't be a child of any other node.
 * It's first member must be \ref ml666_st_node, but there are no further requirements.
 *
 * \extends ml666_st_node
 * \extends ml666_st_children
 *
 * \see ML666_ST_U_DOCUMENT
 */
typedef struct ml666_st_document ml666_st_document_t;

/**
 * \struct ml666_st_element
 * This is an opaque, implementation specific type. This node represents an element in the simple tree.
 * Those are nodes which can have child nodes, can be a child of other nodes, and can have attributes.
 * It's first member must be \ref ml666_st_member, but there are no further requirements.
 *
 * \extends ml666_st_member
 * \extends ml666_st_children
 * \extends ml666_st_attribute
 *
 * \see ML666_ST_U_ELEMENT
 */
typedef struct ml666_st_element ml666_st_element_t;

/**
 * \struct ml666_st_content
 * This is an opaque, implementation specific type. This node represents some text or some binary data.
 * It's first member must be \ref ml666_st_member, but there are no further requirements.
 *
 * \extends ml666_st_member
 *
 * \see ML666_ST_U_CONTENT
 */
typedef struct ml666_st_content ml666_st_content_t;

/**
 * \struct ml666_st_comment
 * This is an opaque, implementation specific type. This node represents a comment.
 * It's first member must be \ref ml666_st_member, but there are no further requirements.
 *
 * \extends ml666_st_member
 *
 * \see ML666_ST_U_COMMENT
 */
typedef struct ml666_st_comment ml666_st_comment_t;

/**
 * The ml666_st_builder.
 * An instance of the default implementation can be optained using the \ref ml666_st_builder_create function.
 *
 * If you want to create your own implementation,
 * just implement the callbacks in \ref ml666_simple_tree_parser_cb
 * and create your own \ref ml666_simple_tree_parser instance.
 * There are some helpful macros for declaring and defining the necessary datastructures,
 * the compiler will tell you about any functions which still need to be implemented.
 *
 * \see \ref ml666-simple-tree
 * \see ML666_ST_DECLARATION
 * \see ML666_ST_IMPLEMENTATION
 */
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

/**
 * Safe downcast to a node.
 */
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

/** Safe downcast to an \ref ml666_st_member */
#define ML666_ST_MEMBER(X) _Generic((X), \
    struct ml666_st_element*: (struct ml666_st_member*)(X), \
    struct ml666_st_content*: (struct ml666_st_member*)(X), \
    struct ml666_st_comment*: (struct ml666_st_member*)(X), \
    const struct ml666_st_element*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_content*: (const struct ml666_st_member*)(X), \
    const struct ml666_st_comment*: (const struct ml666_st_member*)(X) \
  )

/**
 * Safe upcast to an \ref ml666_st_document
 * \returns the \ref ml666_st_document, or 0 if it was not an \ref ml666_st_document
 */
#define ML666_ST_U_DOCUMENT(X) _Generic((X), \
    struct ml666_st_document*: ML666_ST_TYPE((X)) == ML666_ST_NT_DOCUMENT ? (struct ml666_st_document*)(X) : 0, \
    const struct ml666_st_document*: ML666_ST_TYPE((X)) == ML666_ST_NT_DOCUMENT ? (const struct ml666_st_document*)(X) : 0, \
  )

/**
 * Safe upcast to an \ref ml666_st_member
 * \returns the \ref ml666_st_member, or 0 if it was not an \ref ml666_st_member
 */
#define ML666_ST_U_MEMBER(X) _Generic((X), \
    struct ml666_st_node*: ml666_st_is_member((X)) ? (struct ml666_st_member*)(X) : 0, \
    const struct ml666_st_node*: ml666_st_is_member((X)) ? (const struct ml666_st_member*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_element
 * \returns the \ref ml666_st_element, or 0 if it was not an \ref ml666_st_element
 */
#define ML666_ST_U_ELEMENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (struct ml666_st_element*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (struct ml666_st_element*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const struct ml666_st_element*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_ELEMENT ? (const struct ml666_st_element*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_content
 * \returns the \ref ml666_st_content, or 0 if it was not an \ref ml666_st_content
 */
#define ML666_ST_U_CONTENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (struct ml666_st_content*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (struct ml666_st_content*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const struct ml666_st_content*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_CONTENT ? (const struct ml666_st_content*)(X) : 0 \
  )

/**
 * Safe upcast to an \ref ml666_st_comment
 * \returns the \ref ml666_st_comment, or 0 if it was not an \ref ml666_st_comment
 */
#define ML666_ST_U_COMMENT(X) _Generic((X), \
    struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (struct ml666_st_comment*)(X) : 0, \
    struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (struct ml666_st_comment*)(X) : 0, \
    const struct ml666_st_node*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const struct ml666_st_comment*)(X) : 0, \
    const struct ml666_st_member*: ML666_ST_TYPE((X)) == ML666_ST_NT_COMMENT ? (const struct ml666_st_comment*)(X) : 0 \
  )

/** Safe downcast to an \ref ml666_st_children */
#define ML666_ST_CHILDREN(STB, X) _Generic((X), \
    struct ml666_st_document*: ml666_st_document_get_children((STB), (void*)(X)), \
    struct ml666_st_element*: ml666_st_element_get_children((STB), (void*)(X)), \
    const struct ml666_st_document*: (const struct ml666_st_children*)ml666_st_document_get_children((STB), (void*)(X)), \
    const struct ml666_st_element*: (const struct ml666_st_children*)ml666_st_element_get_children((STB), (void*)(X)) \
  )

/**
 * Safe upcast to \ref ml666_st_children.
 * \returns the \ref ml666_st_children, or 0 if it was not an \ref ml666_st_children
 */
#define ML666_ST_U_CHILDREN(STB, X) _Generic((X), \
    struct ml666_st_node*: ml666_st_node_get_children((STB), (void*)(X)), \
    struct ml666_st_member*: ml666_st_node_get_children((STB), (void*)(X)), \
    const struct ml666_st_node*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)), \
    const struct ml666_st_member*: (const struct ml666_st_children*)ml666_st_node_get_children((STB), (void*)(X)) \
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
  X(node_put /** \see ml666_st_node_put */, void, (struct ml666_st_builder* stb, struct ml666_st_node* node), __VA_ARGS__) \
  X(node_ref /** \see ml666_st_node_ref */, void, (struct ml666_st_builder* stb, struct ml666_st_node* node), __VA_ARGS__) \
  \
  X(member_set /** \see ml666_st_member_set */, bool, (struct ml666_st_builder* stb, struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before), __VA_ARGS__) \
  \
  X(document_create /** \see ml666_st_document_create */, struct ml666_st_document*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(element_create /** \see ml666_st_element_create */, struct ml666_st_element*, (struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name), __VA_ARGS__) \
  X(content_create /** \see ml666_st_content_create */, struct ml666_st_content*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  X(comment_create /** \see ml666_st_comment_create */, struct ml666_st_comment*, (struct ml666_st_builder* stb), __VA_ARGS__) \
  \
  X(document_get_children /** \see ml666_st_document_get_children */, struct ml666_st_children*, (struct ml666_st_builder* stb, struct ml666_st_document* node), __VA_ARGS__) \
  X(element_get_children /** \see ml666_st_element_get_children */, struct ml666_st_children*, (struct ml666_st_builder* stb, struct ml666_st_element* node), __VA_ARGS__) \
  \
  X(member_get_parent /** \see ml666_st_member_get_parent */, struct ml666_st_node*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  X(member_get_previous /** \see ml666_st_member_get_previous */, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  X(member_get_next /** \see ml666_st_member_get_next */, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_member* node), __VA_ARGS__) \
  \
  X(element_get_name /** \see ml666_st_element_get_name */, const struct ml666_hashed_buffer_set_entry*, (struct ml666_st_builder* stb, struct ml666_st_element* node), __VA_ARGS__) \
  \
  X(content_set /** \see ml666_st_content_set */, bool, (struct ml666_st_builder* stb, struct ml666_st_content* content, struct ml666_buffer node), __VA_ARGS__) \
  X(content_get /** \see ml666_st_content_get */, struct ml666_buffer_ro, (struct ml666_st_builder* stb, const struct ml666_st_content* content), __VA_ARGS__) \
  X(content_take /** \see ml666_st_content_take */, struct ml666_buffer, (struct ml666_st_builder* stb, struct ml666_st_content* content), __VA_ARGS__) \
  \
  X(comment_set /** \see ml666_st_comment_set */, bool, (struct ml666_st_builder* stb, struct ml666_st_comment* comment, struct ml666_buffer node), __VA_ARGS__) \
  X(comment_get /** \see ml666_st_comment_get */, struct ml666_buffer_ro, (struct ml666_st_builder* stb, const struct ml666_st_comment* comment), __VA_ARGS__) \
  X(comment_take /** \see ml666_st_comment_take */, struct ml666_buffer, (struct ml666_st_builder* stb, struct ml666_st_comment* comment), __VA_ARGS__) \
  \
  X(attribute_get_first /** \see ml666_st_attribute_get_first */, struct ml666_st_attribute*, (const struct ml666_st_builder* stb, const struct ml666_st_element* element), __VA_ARGS__) \
  X(attribute_get_next /** \see ml666_st_attribute_get_next */, struct ml666_st_attribute*, (const struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_lookup /** \see ml666_st_attribute_lookup */, struct ml666_st_attribute*, (struct ml666_st_builder* stb, struct ml666_st_element* element, const struct ml666_hashed_buffer* name, enum ml666_st_attribute_lookup_flags flags), __VA_ARGS__) \
  X(attribute_remove /** \see ml666_st_attribute_remove */, void, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_get_name /** \see ml666_st_attribute_get_name */, const struct ml666_hashed_buffer*, (struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_set_value /** \see ml666_st_attribute_set_value */, bool, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute, struct ml666_buffer* value), __VA_ARGS__) \
  X(attribute_get_value /** \see ml666_st_attribute_get_value */, const struct ml666_buffer_ro*, (struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute), __VA_ARGS__) \
  X(attribute_take_value /** \see ml666_st_attribute_take_value */, const struct ml666_buffer*, (struct ml666_st_builder* stb, struct ml666_st_attribute* attribute), __VA_ARGS__) \
  \
  X(get_first_child /** \see ml666_st_get_first_child */, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__) \
  X(get_last_child /** \see ml666_st_get_last_child */, struct ml666_st_member*, (struct ml666_st_builder* stb, struct ml666_st_children* children), __VA_ARGS__)


#define ML666__ST_DECLARATION_SUB(FUNC, _1, _2, PREFIX) ML666_EXPORT ml666_st_cb_ ## FUNC PREFIX ## _ ## FUNC;
#define ML666__ST_IMPLEMENTATION_SUB(X, _1, _2, PREFIX) .X = PREFIX ## _ ## X,

/**
 * Declares the functions & the struct containing the callbacks of the \ref ml666_st_builder implementation.
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

/**
 * Creates a ml666_st_cb and fills in all the members.
 * Also includes \ref ML666_ST_DECLARATION
 * \see ML666_ST_DECLARATION for the details on how the members are called and so on.
 */
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

/**
 * Destroys a ml666_st_builder instance. This must be done after all the nodes created using it have been destroyed.
 * \memberof ml666_st_builder
 */
static inline void ml666_st_builder_destroy(struct ml666_st_builder* stb){
  stb->cb->builder_destroy(stb);
}

/**
 * Decrements the refcount of a node. The node is freed when the ref count hits 0.
 * \memberof ml666_st_node
 * \param stb The simple tree builder instance used to create the node
 * \param node The node whos refcount is to be decremented
 */
static inline void ml666_st_node_put(struct ml666_st_builder* stb, struct ml666_st_node* node){
  stb->cb->node_put(stb, node);
}
/**
 * Increments the refcount of a node.
 * \memberof ml666_st_node
 * \param stb The simple tree builder instance used to create the node
 * \param node The node whos refcount is to be incremented
 */
static inline void ml666_st_node_ref(struct ml666_st_builder* stb, struct ml666_st_node* node){
  stb->cb->node_ref(stb, node);
}

/**
 * Add a \ref ml666_st_member to an \ref ml666_st_node, optionaly before some other \ref ml666_st_member
 * \memberof ml666_st_member
 * \param stb The simple tree builder instance used to create the nodes
 * \param parent The node to which the member node is to be added
 * \param member The node to be added to parent
 * \param before If set, this must be a child node of the parent node, the new node is to be added before this one
 */
static inline bool ml666_st_member_set(struct ml666_st_builder* stb, struct ml666_st_node* parent, struct ml666_st_member* member, struct ml666_st_member* before){
  return stb->cb->member_set(stb, parent, member, before);
}
/**
 * Get the parent of a node.
 * \memberof ml666_st_member
 * \param stb The simple tree builder instance used to create the node
 * \param node The node whos parent is to be returned
 * \returns the parent node or 0 if it doesn't have one.
 */
static inline struct ml666_st_node* ml666_st_member_get_parent(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_parent(stb, node);
}
/**
 * Get the node before this one. That is, the child node of the parent which comes before this one.
 *
 * \memberof ml666_st_member
 * \param stb The simple tree builder instance used to create the node
 * \param node The node
 * \returns the preceeding node or 0 if there is none.
 */
static inline struct ml666_st_member* ml666_st_member_get_previous(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_previous(stb, node);
}
/**
 * Get the node after this one. That is, the child node of the parent which comes after this one.
 *
 * \memberof ml666_st_member
 * \param stb The simple tree builder instance used to create the node
 * \param node The node
 * \returns the following node or 0 if there is none.
 */
static inline struct ml666_st_member* ml666_st_member_get_next(struct ml666_st_builder* stb, struct ml666_st_member* node){
  return stb->cb->member_get_next(stb, node);
}

/**
 * Create a document node.
 * \memberof ml666_st_document
 */
static inline struct ml666_st_document* ml666_st_document_create(struct ml666_st_builder* stb){
  return stb->cb->document_create(stb);
}
/**
 * Create an element node.
 * \memberof ml666_st_element
 */
static inline struct ml666_st_element* ml666_st_element_create(struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name){
  return stb->cb->element_create(stb, entry, copy_name);
}
/**
 * Create a content node.
 * \memberof ml666_st_content
 */
static inline struct ml666_st_content* ml666_st_content_create(struct ml666_st_builder* stb){
  return stb->cb->content_create(stb);
}
/**
 * Create a comment node.
 * \memberof ml666_st_comment
 */
static inline struct ml666_st_comment* ml666_st_comment_create(struct ml666_st_builder* stb){
  return stb->cb->comment_create(stb);
}

/**
 * Get the child list of a document node.
 * It may be easier to just use \ref ML666_ST_CHILDREN instead.
 * \memberof ml666_st_document
 * \param stb The simple tree builder instance used to create the node
 * \param node The document node
 * \see ML666_ST_CHILDREN
 * \see ML666_ST_U_CHILDREN
 */
static inline struct ml666_st_children* ml666_st_document_get_children(struct ml666_st_builder* stb, struct ml666_st_document* node){
  return stb->cb->document_get_children(stb, node);
}
/**
 * Get the child list of a element node.
 * It may be easier to just use \ref ML666_ST_CHILDREN instead.
 * \memberof ml666_st_element
 * \param stb The simple tree builder instance used to create the node
 * \param node The element node
 * \see ML666_ST_CHILDREN
 * \see ML666_ST_U_CHILDREN
 */
static inline struct ml666_st_children* ml666_st_element_get_children(struct ml666_st_builder* stb, struct ml666_st_element* node){
  return stb->cb->element_get_children(stb, node);
}

/**
 * Get the name of the element as an \ref ml666_hashed_buffer_set_entry.
 * This will not increment it's reference count. The element node will hold a reference already,
 * so as long as the element is referenced, so is it's name. But reference it if you need it longer than that.
 *
 * \memberof ml666_st_element
 */
static inline const struct ml666_hashed_buffer_set_entry* ml666_st_element_get_name(struct ml666_st_builder* stb, struct ml666_st_element* node){
  return stb->cb->element_get_name(stb, node);
}
/**
 * \memberof ml666_st_element
 */
static inline struct ml666_st_attribute* ml666_st_attribute_lookup(struct ml666_st_builder* stb, struct ml666_st_element* element, const struct ml666_hashed_buffer* name, enum ml666_st_attribute_lookup_flags flags){
  return stb->cb->attribute_lookup(stb, element, name, flags);
}
/**
 * \memberof ml666_st_element
 */
static inline struct ml666_st_attribute* ml666_st_attribute_get_first(const struct ml666_st_builder* stb, const struct ml666_st_element* element){
  return stb->cb->attribute_get_first(stb, element);
}

/**
 * \memberof ml666_st_content
 * @{
 */
static inline bool ml666_st_content_set(struct ml666_st_builder* stb, struct ml666_st_content* content, struct ml666_buffer node){
  return stb->cb->content_set(stb, content, node);
}
static inline struct ml666_buffer_ro ml666_st_content_get(struct ml666_st_builder* stb, const struct ml666_st_content* content){
  return stb->cb->content_get(stb, content);
}
static inline struct ml666_buffer ml666_st_content_take(struct ml666_st_builder* stb, struct ml666_st_content* content){
  return stb->cb->content_take(stb, content);
}
static inline bool ml666_st_comment_set(struct ml666_st_builder* stb, struct ml666_st_comment* comment, struct ml666_buffer node){
  return stb->cb->comment_set(stb, comment, node);
}
static inline struct ml666_buffer_ro ml666_st_comment_get(struct ml666_st_builder* stb, const struct ml666_st_comment* comment){
  return stb->cb->comment_get(stb, comment);
}
static inline struct ml666_buffer ml666_st_comment_take(struct ml666_st_builder* stb, struct ml666_st_comment* comment){
  return stb->cb->comment_take(stb, comment);
}
/** @} */

/**
 * \memberof ml666_st_attribute
 * @{
 */
static inline struct ml666_st_attribute* ml666_st_attribute_get_next(const struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  return stb->cb->attribute_get_next(stb, attribute);
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
/** @} */

/**
 * \memberof ml666_st_children
 * @{
 */
static inline struct ml666_st_member* ml666_st_get_first_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  if(!children)
    return 0;
  return stb->cb->get_first_child(stb, children);
}
static inline struct ml666_st_member* ml666_st_get_last_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  if(!children)
    return 0;
  return stb->cb->get_last_child(stb, children);
}
/** @} */


// Simple generic helper functions

/**
 * \memberof ml666_st_node
 */
static inline bool ml666_st_is_member(const struct ml666_st_node* node){
  enum ml666_st_node_type type = ML666_ST_TYPE(node);
  switch(type){
    case ML666_ST_NT_DOCUMENT: return false;
    case ML666_ST_NT_ELEMENT: return true;
    case ML666_ST_NT_CONTENT: return true;
    case ML666_ST_NT_COMMENT: return true;
  }
  return false;
}

/**
 * \memberof ml666_st_node
 */
static inline void ml666_st_node_ref_set(struct ml666_st_builder* stb, struct ml666_st_node** dest, struct ml666_st_node* src){
  if(src)
    stb->cb->node_ref(stb, src);
  struct ml666_st_node* old = *dest;
  *dest = src;
  if(old)
    stb->cb->node_put(stb, old);
}

/**
 * \memberof ml666_st_node
 */
static inline struct ml666_st_children* ml666_st_node_get_children(struct ml666_st_builder* stb, struct ml666_st_node* node){
  switch(ML666_ST_TYPE(node)){
    case ML666_ST_NT_DOCUMENT: return ml666_st_document_get_children(stb, (struct ml666_st_document*)node);
    case ML666_ST_NT_ELEMENT : return ml666_st_element_get_children(stb, (struct ml666_st_element*)node);
    default: return 0;
  }
}

/**
 * \memberof ml666_st_children
 */
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

/** @} */

#endif
