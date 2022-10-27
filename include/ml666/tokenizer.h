#ifndef ML666_TOKENIZER_H
#define ML666_TOKENIZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ml666/common.h>

/** \addtogroup tokenizer Tokenizer
 * @{ */
/** \addtogroup ml666-tokenizer ML666 Tokenizer API
 * @{ */

#define ML666__TOKENS \
  X(ML666_NONE /**< There was no token returned */) \
  X(ML666_EOF  /**< Last token, there will be no more. */) \
  X(ML666_TAG  /**< Start of element. Element name */) \
  X(ML666_END_TAG /**< End of element. Optionally, element name. */) \
  X(ML666_ATTRIBUTE /**< Attribute name */) \
  X(ML666_ATTRIBUTE_VALUE  /**< Attribute value */) \
  X(ML666_TEXT  /**< Text/Content. Well, it can be any binary data, really. */) \
  X(ML666_COMMENT /**< Comment */) \

/**
 * These are all the tokens the tokeniser can return.
 */
enum ml666_token {
#define X(Y) Y,
  ML666__TOKENS
#undef X
  ML666_TOKEN_COUNT ///< This isn't a token, but the number of available tokens.
};
/**
 * A lookup table for the token names
 */
ML666_EXPORT extern const char*const ml666__token_name[ML666_TOKEN_COUNT];

/**
 * There are multiple implementations in this project,
 * The default one for an ml666 document can be optained using \ref ml666_tokenizer_create.
 * To create your own, just create an instance of this script and implement and set \ref ml666_tokenizer_cb.
 */
struct ml666_tokenizer {
  const struct ml666_tokenizer_cb*const cb;  ///< The callbacks for this tokenizer implementation
  enum ml666_token token; ///< set if a token was retuned to the token
  struct ml666_buffer_ro match; ///< a buffer containing a chunk of the content of the matched token
  bool complete; ///< If the token is complete, or if there is more to come
  const char* error; ///< If an error occurs, this will be set to an error message.
  size_t line; ///< The current line being processed.
  size_t column; ///< The current column being processed.
  void* user_ptr; ///< A userspecified pointer
};

typedef bool ml666_tokenizer_cb_next(struct ml666_tokenizer* tokenizer); ///< \see ml666_tokenizer_next
typedef void ml666_tokenizer_cb_destroy(struct ml666_tokenizer* tokenizer); ///< \see ml666_tokenizer_destroy

/**
 * This are the callbacks of the ml666_tokenizer implementation.
 */
struct ml666_tokenizer_cb {
  ml666_tokenizer_cb_next* next; ///< \see ml666_tokenizer_next
  ml666_tokenizer_cb_destroy* destroy; ///< \see ml666_tokenizer_destroy
};

/** \addtogroup ml666-tokenizer-default Default ML666 Tokenizer
 * @{ */

/** \see ml666_tokenizer_create */
struct ml666_tokenizer_create_args {
  int fd; ///< The file descriptor to use. Only used if parser and tokenizer is not set. The ml666_tokenizer will take care of the cleanup (close the fd).
  // Optional
  bool disable_utf8_validation; ///< Optional. Can be used to disable the utf8 validation of the ml666 document.
  void* user_ptr; ///< Optional. A userspecified pointer.
  ml666__cb__malloc* malloc; ///< Optional. Custom allocator.
  ml666__cb__free*   free; ///< Optional. Custom allocator.
};
/** \see ml666_tokenizer_create */
ML666_EXPORT struct ml666_tokenizer* ml666_tokenizer_create_p(struct ml666_tokenizer_create_args args);
/**
 * The default implementation for the tokenizer.
 * \returns an instance of the default ml666 tokenizer.
 * \see ml666_tokenizer_create_args for the arguments.
 */
#define ml666_tokenizer_create(...) ml666_tokenizer_create_p((struct ml666_tokenizer_create_args){__VA_ARGS__})

/** @} */

/**
 * Continue processing data. This may block unless the file descriptor it's reading from is non-blocking.
 * If there is a new token, tokenizer->token will be set, and the preprocessed content of the token is returned in tokeniser->match.
 * If tokeniser->complete is not set, this is only part of the token, and the next chunk of the token will be made available in future calls.
 *
 * If there is an error, it returns false and sets tokenizer->error.
 * \returns true if it's not done yet, false otherwise
 */
static inline bool ml666_tokenizer_next(struct ml666_tokenizer* tokenizer){
  return tokenizer->cb->next(tokenizer);
}

/**
 * Destroys the \ref ml666_tokenizer instance.
 */
static inline void ml666_tokenizer_destroy(struct ml666_tokenizer* tokenizer){
  tokenizer->cb->destroy(tokenizer);
}

/** @} */
/** @} */

#endif
