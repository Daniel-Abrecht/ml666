#ifndef ML666_SIMPLE_TREE_PARSER_H
#define ML666_SIMPLE_TREE_PARSER_H

#include <ml666/common.h>
#include <stdbool.h>
#include <stdio.h>

/** \addtogroup simple-tree-builder
 * @{
 * \addtogroup ml666-simple-tree-parser ml666_simple_tree_parser API
 * @{ */

/**
 * The ml666_simple_tree_parser instance.
 * An instance of the default implementation can eb implemented using the \ref ml666_simple_tree_parser_create function.
 *
 * If you want to create your own implementation,
 * just implement the callbacks in \ref ml666_simple_tree_parser_cb
 * and create your own \ref ml666_simple_tree_parser instance.
 */
struct ml666_simple_tree_parser {
  const struct ml666_simple_tree_parser_cb*const cb; ///< The callbacks for this parser implementation
  struct ml666_st_builder*const stb; ///< The \ref ml666_st_builder for creating the ml666 document tree
  size_t line; ///< The current line being processed.
  size_t column; ///< The current column being processed.
  const char* error; ///< If an error occurs, this will be set to an error message.
  void* user_ptr; ///< A userspecified pointer
};

typedef void ml666_simple_tree_parser_cb_destroy(struct ml666_simple_tree_parser* stp); ///< \see ml666_simple_tree_parser_destroy
typedef bool ml666_simple_tree_parser_cb_next(struct ml666_simple_tree_parser* stp); ///< \see ml666_simple_tree_parser_next
typedef struct ml666_st_document* ml666_simple_tree_parser_cb_take_document(struct ml666_simple_tree_parser* stp); ///< \see ml666_simple_tree_parser_take_document

/**
 * The callbacks of the \ref ml666_simple_tree_parser implementation
 */
struct ml666_simple_tree_parser_cb {
  ml666_simple_tree_parser_cb_destroy* destroy; ///< \see ml666_simple_tree_parser_destroy
  ml666_simple_tree_parser_cb_next* next; ///< \see ml666_simple_tree_parser_next
  ml666_simple_tree_parser_cb_take_document* take_document; ///< \see ml666_simple_tree_parser_take_document
};

/** \see ml666_simple_tree_parser_create */
struct ml666_simple_tree_parser_create_args {
  struct ml666_st_builder* stb; ///< The \ref ml666_st_builder for creating the ml666 document tree
  int fd; ///< The file descriptor to use. Only used if parser and tokenizer is not set. The simple_tree_parser will take care of the cleanup.
  // Optional
  struct ml666_parser* parser; ///< Optional. The ml666 parser. The simple_tree_parser will take care of the cleanup.
  struct ml666_tokenizer* tokenizer; ///< Optional. The tokenizer. The simple_tree_parser will take care of the cleanup.
  void* user_ptr; ///< Optional. A userspecified pointer.
  ml666__cb__malloc*  malloc; ///< Optional. Custom allocator.
  ml666__cb__realloc* realloc; ///< Optional. Custom allocator.
  ml666__cb__free*    free; ///< Optional. Custom allocator.
};
/** \see ml666_simple_tree_parser_create */
ML666_EXPORT struct ml666_simple_tree_parser* ml666_simple_tree_parser_create_p(struct ml666_simple_tree_parser_create_args args);
/**
 * Creates an \ref ml666_simple_tree_parser instance. This is the default implementation.
 * \see ml666_simple_tree_parser for the arguments. Please use designated initialisers for the optional arguments.
 */
#define ml666_simple_tree_parser_create(...) ml666_simple_tree_parser_create_p((struct ml666_simple_tree_parser_create_args){__VA_ARGS__})

/**
 * Destroys the \ref ml666_simple_tree_parser instance.
 */
static inline void ml666_simple_tree_parser_destroy(struct ml666_simple_tree_parser* stp){
  stp->cb->destroy(stp);
}

/**
 * Parses more data. This may block unless the file descriptor it's reading from is non-blocking.
 * This may return multiple times until everything is parsed. If there is an error, it returns
 * false and sets stp->error.
 * \see ml666_st_parse
 * \see ml666_tokenizer_create
 * \returns true if it's not done yet, false otherwise
 */
static inline bool ml666_simple_tree_parser_next(struct ml666_simple_tree_parser* stp){
  return stp->cb->next(stp);
}

/**
 * This can only be called once (returns 0 after that).
 *
 * \returns It returns final the ml666_st_document.
 */
static inline struct ml666_st_document* ml666_simple_tree_parser_take_document(struct ml666_simple_tree_parser* stp){
  return stp->cb->take_document(stp);
}

/**
 * This is merely a convenience function for the simplest of use cases.
 * It parses the whole document all at once, using the default parser.
 * Make sure the underlying file descrition is in blocking mode, or it'll waste lots of CPU time.
 *
 * \see ml666_simple_tree_parser_create_args for the arguments
 * \returns the \ref ml666_st_document document
 */
#define ml666_st_parse(...) ml666_st_parse_p((struct ml666_simple_tree_parser_create_args){__VA_ARGS__})
/** \see ml666_st_parse */
static inline struct ml666_st_document* ml666_st_parse_p(struct ml666_simple_tree_parser_create_args args){
  struct ml666_simple_tree_parser* stp = ml666_simple_tree_parser_create_p(args);
  if(!stp){
    fprintf(stderr, "ml666_parser_create failed\n");
    return 0;
  }
  while(ml666_simple_tree_parser_next(stp));
  struct ml666_st_document* result = 0;
  if(stp->error){
    fprintf(stderr, "ml666_parser_next failed: line %zu, column %zu: %s\n", stp->line, stp->column, stp->error);
  }else{
    result = ml666_simple_tree_parser_take_document(stp);
  }
  ml666_simple_tree_parser_destroy(stp);
  return result;
}

/** @} */
/** @} */

#endif
