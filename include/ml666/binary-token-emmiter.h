#ifndef ML666_BINARY_TOKEN_EMMITER_H
#define ML666_BINARY_TOKEN_EMMITER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ml666/tokenizer.h>

struct ml666_binary_token_emmiter_create_args {
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
ML666_EXPORT struct ml666_tokenizer* ml666_binary_token_emmiter_create_p(struct ml666_binary_token_emmiter_create_args args);
#define ml666_binary_token_emmiter_create(...) ml666_binary_token_emmiter_create_p((struct ml666_binary_token_emmiter_create_args){__VA_ARGS__})

#endif
