#ifndef ML666_TOKENIZER_H
#define ML666_TOKENIZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ml666/common.h>

#define ML666__TOKENS \
  X(ML666_NONE) \
  X(ML666_EOF) \
  X(ML666_TAG) \
  X(ML666_END_TAG) \
  X(ML666_ATTRIBUTE) \
  X(ML666_ATTRIBUTE_VALUE) \
  X(ML666_TEXT) \
  X(ML666_COMMENT) \

enum ml666_token {
#define X(Y) Y,
  ML666__TOKENS
#undef X
  ML666_TOKEN_COUNT
};
ML666_EXPORT extern const char*const ml666__token_name[ML666_TOKEN_COUNT];

struct ml666_tokenizer {
  const struct ml666_tokenizer_cb*const cb;
  enum ml666_token token;
  struct ml666_buffer_ro match;
  bool complete;
  const char* error;
  size_t line, column;
  void* user_ptr;
};

typedef bool ml666_tokenizer_cb_next(struct ml666_tokenizer* tokenizer);
typedef void ml666_tokenizer_cb_destroy(struct ml666_tokenizer* tokenizer);

struct ml666_tokenizer_cb {
  ml666_tokenizer_cb_next* next;
  ml666_tokenizer_cb_destroy* destroy;
};

struct ml666_tokenizer_create_args {
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
  bool disable_utf8_validation;
};
ML666_EXPORT struct ml666_tokenizer* ml666_tokenizer_create_p(struct ml666_tokenizer_create_args args);
#define ml666_tokenizer_create(...) ml666_tokenizer_create_p((struct ml666_tokenizer_create_args){__VA_ARGS__})

// returns false on EOF
static inline bool ml666_tokenizer_next(struct ml666_tokenizer* tokenizer){
  return tokenizer->cb->next(tokenizer);
}

static inline void ml666_tokenizer_destroy(struct ml666_tokenizer* tokenizer){
  tokenizer->cb->destroy(tokenizer);
}

#endif
