#ifndef ML666_BINARY_TOKEN_EMMITER_H
#define ML666_BINARY_TOKEN_EMMITER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <ml666/tokenizer.h>

/**
 * \addtogroup tokenizer
 * @{
 * \addtogroup ml666-tokenizer
 * @{
 * \addtogroup binary-token-emmiter Binary Token Emmiter
 * Implements the \ref ml666-tokenizer on top of the \ref ml666-json-tokenizer.
 * Reads binary data and emmits an \ref ML666_TEXT token.
 * @{
 */

struct ml666_binary_token_emmiter_create_args {
  int fd;
  void* user_ptr;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
ML666_EXPORT struct ml666_tokenizer* ml666_binary_token_emmiter_create_p(struct ml666_binary_token_emmiter_create_args args);
#define ml666_binary_token_emmiter_create(...) ml666_binary_token_emmiter_create_p((struct ml666_binary_token_emmiter_create_args){__VA_ARGS__})

/** @} */
/** @} */
/** @} */

#endif
