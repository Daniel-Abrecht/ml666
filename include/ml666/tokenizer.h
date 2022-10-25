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
  const struct ml666_tokenizer_cb*const cb;
  enum ml666_token token;
  struct ml666_buffer_ro match;
  bool complete;
  const char* error;
  size_t line, column;
  void* user_ptr;
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
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
  bool disable_utf8_validation;
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

// returns false on EOF
static inline bool ml666_tokenizer_next(struct ml666_tokenizer* tokenizer){
  return tokenizer->cb->next(tokenizer);
}

static inline void ml666_tokenizer_destroy(struct ml666_tokenizer* tokenizer){
  tokenizer->cb->destroy(tokenizer);
}

/** @} */
/** @} */

#endif
