#ifndef ML666_H
#define ML666_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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
extern const char*const ml666__token_name[ML666_TOKEN_COUNT];

struct ml666_buffer_ro {
  const char* data;
  size_t length;
};

struct ml666_tokenizer {
  enum ml666_token token; \
  struct ml666_buffer_ro match; \
  bool complete; \
  const char* error; \
  size_t line, column; \
};

struct ml666_tokenizer* ml666_tokenizer_create(int fd);
bool ml666_tokenizer_next(struct ml666_tokenizer* state); // returns false on EOF
void ml666_tokenizer_destroy(struct ml666_tokenizer* state);

#endif
