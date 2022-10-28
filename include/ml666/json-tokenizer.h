#ifndef ML666_JSON_TOKENIZER_H
#define ML666_JSON_TOKENIZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ml666/common.h>

/**
 * \addtogroup tokenizer
 * @{
 * \addtogroup ml666-json-tokenizer JSON Tokenizer API
 * @{
 */

#define ML666__JSON_TOKENS \
  X(ML666_JSON_NONE) \
  X(ML666_JSON_EOF) \
  X(ML666_JSON_OBJECT_START) \
  X(ML666_JSON_OBJECT_KEY) \
  X(ML666_JSON_OBJECT_VALUE) \
  X(ML666_JSON_OBJECT_END) \
  X(ML666_JSON_ARRAY_START) \
  X(ML666_JSON_ARRAY_END) \
  X(ML666_JSON_STRING) \
  X(ML666_JSON_TRUE) \
  X(ML666_JSON_FALSE) \
  X(ML666_JSON_NULL)

enum ml666_json_token {
#define X(Y) Y,
  ML666__JSON_TOKENS
#undef X
  ML666_JSON_TOKEN_COUNT
};
ML666_EXPORT extern const char*const ml666__json_token_name[ML666_JSON_TOKEN_COUNT];

struct ml666_json_tokenizer {
  const struct ml666_json_tokenizer_cb*const cb;
  enum ml666_json_token token;
  struct ml666_buffer match;
  struct ml666_buffer_ro match_ro;
  bool complete;
  const char* error;
  size_t line, column;
  void* user_ptr;
};

typedef bool ml666_json_tokenizer_cb_next(struct ml666_json_tokenizer* tokenizer);
typedef void ml666_json_tokenizer_cb_destroy(struct ml666_json_tokenizer* tokenizer);

struct ml666_json_tokenizer_cb {
  ml666_json_tokenizer_cb_next* next;
  ml666_json_tokenizer_cb_destroy* destroy;
};

struct ml666_json_tokenizer_create_args {
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
  bool disable_utf8_validation;
  bool no_arrays, no_objects; // This saves some memory if set, because we won't have to keep track of the state.
};
ML666_EXPORT struct ml666_json_tokenizer* ml666_json_tokenizer_create_p(struct ml666_json_tokenizer_create_args args);
#define ml666_json_tokenizer_create(...) ml666_json_tokenizer_create_p((struct ml666_json_tokenizer_create_args){__VA_ARGS__})

// returns false on EOF
static inline bool ml666_json_tokenizer_next(struct ml666_json_tokenizer* tokenizer){
  return tokenizer->cb->next(tokenizer);
}

static inline void ml666_json_tokenizer_destroy(struct ml666_json_tokenizer* tokenizer){
  tokenizer->cb->destroy(tokenizer);
}

/** @} */
/** @} */

#endif
