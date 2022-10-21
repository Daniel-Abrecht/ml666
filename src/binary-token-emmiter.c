#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ml666/binary-token-emmiter.h>
#include <ml666/utils.h>


struct ml666__tokenizer_private {
  struct ml666_tokenizer public;
  int fd;
  struct ml666_buffer buffer;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
static_assert(offsetof(struct ml666__tokenizer_private, public) == 0, "ml666__tokenizer_private::public must be the first member");


static ml666_tokenizer_cb_next ml666_binary_token_emmiter_d_next;
static ml666_tokenizer_cb_destroy ml666_binary_token_emmiter_d_destroy;

static const struct ml666_tokenizer_cb tokenizer_cb = {
  .next = ml666_binary_token_emmiter_d_next,
  .destroy = ml666_binary_token_emmiter_d_destroy,
};

__attribute__((const))
static inline unsigned get_ringbuffer_size(void){
  const unsigned sz = sysconf(_SC_PAGESIZE);
  return (sz-1 + 4096) / sz * sz;
}

struct ml666_tokenizer* ml666_binary_token_emmiter_create_p(struct ml666_binary_token_emmiter_create_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666__tokenizer_private*restrict bte = args.malloc(args.user_ptr, sizeof(*bte));
  if(!bte){
    fprintf(stderr, "%s:%u: malloc failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error;
  }
  memset(bte, 0, sizeof(*bte));
  *(const struct ml666_tokenizer_cb**)&bte->public.cb = &tokenizer_cb;
  bte->fd = args.fd;
  bte->malloc = args.malloc;
  bte->free = args.free;
  bte->public.user_ptr = args.user_ptr;

  const unsigned size = get_ringbuffer_size();
  bte->buffer.length = size;
  if(!(bte->buffer.data=args.malloc(args.user_ptr, size))){
    fprintf(stderr, "%s:%u: malloc failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_calloc;
  }

  return &bte->public;

error_calloc:
  args.free(args.user_ptr, bte);
error:
  close(args.fd);
  return 0;
}

static bool ml666_binary_token_emmiter_d_next(struct ml666_tokenizer* _tokenizer){
  struct ml666__tokenizer_private*restrict bte = (struct ml666__tokenizer_private*)_tokenizer;
  bte->public.token = ML666_NONE;
  bte->public.match = (struct ml666_buffer_ro){0};

  const int fd = bte->fd;
  if(fd == -1) return false;

  while(true){
    int result = read(fd, bte->buffer.data, bte->buffer.length);
    if(result < 0 && errno == EWOULDBLOCK)
      break;
    if(result < 0){
      if(errno == EINTR)
        continue;
      bte->public.error = strerror(errno);
      goto error;
    }
    bte->public.token = ML666_TEXT;
    if(result == 0){
      bte->public.complete = true;
      goto final;
    }else{
      bte->public.complete = false;
      bte->public.match.data = bte->buffer.ro.data;
      bte->public.match.length = result;
      break;
    }
  }

  return true;

error:
  bte->public.token = ML666_EOF;

final:
  // Let's free this stuff as early as possible
  if(close(bte->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  bte->free(bte->public.user_ptr, bte->buffer.data);
  bte->buffer.data = 0;
  bte->fd = -1;
  return false;
}

static void ml666_binary_token_emmiter_d_destroy(struct ml666_tokenizer* _tokenizer){
  if(!_tokenizer) return;
  struct ml666__tokenizer_private*restrict bte = (struct ml666__tokenizer_private*)_tokenizer;
  if(bte->fd != -1 && close(bte->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  if(bte->buffer.data)
    bte->free(bte->public.user_ptr, bte->buffer.data);
  bte->fd = -1;
  bte->free(bte->public.user_ptr, bte);
}
