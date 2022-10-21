#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ml666/json-tokenizer.h>
#include <ml666/utils.h>

enum json_state {
  JS_ELEMENT,
  JS_OBJECT_FIRST,
  JS_OBJECT_KEY,
  JS_OBJECT_VALUE,
  JS_ARRAY_FIRST,
  JS_ARRAY,
  JS_NUMBER,
  JS_NUMBER_FRACTION,
  JS_NUMBER_EXPONENT,
  JS_STRING,
  JS_STRING_2,
  JS_TRUE,
  JS_FALSE,
  JS_NULL,
  JS_EOF
};

static signed char hex2num(char ch){
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  if(ch >= 'a' && ch <= 'z')
    return ch - 'a' + 10;
  if(ch >= 'A' && ch <= 'Z')
    return ch - 'A' + 10;
  return -1;
}

struct ml666__json_tokenizer_private {
  struct ml666_json_tokenizer public;
  int fd;
  char* memory; // This is a ring buffer
  unsigned offset, index, length, cpo;
  enum json_state j_state, next_j_state;
  bool may_block, eof, disable_utf8_validation;
  bool skip_spaces;
  struct ml666_streaming_utf8_validator utf8_validator;
  bool no_arrays, no_objects; // This saves some memory if set, because we won't have to keep track of the state in aobmap.
  size_t aobmap_length;
  uint64_t* aobmap;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
static_assert(offsetof(struct ml666__json_tokenizer_private, public) == 0, "ml666__json_tokenizer_private::public must be the first member");

const char*const ml666__json_token_name[] = {
#define X(Y) #Y,
  ML666__JSON_TOKENS
#undef X
};

static bool aobmap_push(struct ml666__json_tokenizer_private*restrict tokenizer, bool ao){
  size_t i = tokenizer->aobmap_length;
  if(!(size_t)(i+1))
    return false;
  if(!tokenizer->no_arrays && !tokenizer->no_objects){
    if(i%(64*64) == 0){
      uint64_t* aobmap = realloc(tokenizer->aobmap, (i/64+64)*sizeof(uint64_t));
      if(!aobmap)
        return false;
      tokenizer->aobmap = aobmap;
    }
    if(ao){
      tokenizer->aobmap[i/64] |=  (((uint64_t)1)<<(i%64));
    }else{
      tokenizer->aobmap[i/64] &= ~(((uint64_t)1)<<(i%64));
    }
  }else if((tokenizer->no_arrays && !ao) || (tokenizer->no_objects && ao))
    return false;
  tokenizer->aobmap_length = i + 1;
  return true;
}

static bool aobmap_pop(struct ml666__json_tokenizer_private*restrict tokenizer){
  if(!tokenizer->aobmap_length)
    return false;
  tokenizer->aobmap_length -= 1;
  return true;
}

static enum json_state aobmap_peek(struct ml666__json_tokenizer_private*restrict tokenizer){
  size_t i = tokenizer->aobmap_length;
  if(!i) return JS_EOF;
  i -= 1;
  if(tokenizer->no_arrays)
    return JS_OBJECT_VALUE;
  if(tokenizer->no_objects)
    return JS_ARRAY;
  return tokenizer->aobmap[i/64] & (((uint64_t)1)<<(i%64)) ? JS_OBJECT_VALUE : JS_ARRAY;
}

static ml666_json_tokenizer_cb_next ml666_json_tokenizer_d_next;
static ml666_json_tokenizer_cb_destroy ml666_json_tokenizer_d_destroy;

static const struct ml666_json_tokenizer_cb tokenizer_cb = {
  .next = ml666_json_tokenizer_d_next,
  .destroy = ml666_json_tokenizer_d_destroy,
};

__attribute__((const))
static inline unsigned get_ringbuffer_size(void){
  const unsigned sz = sysconf(_SC_PAGESIZE);
  return (sz-1 + 4096) / sz * sz;
}

struct ml666_json_tokenizer* ml666_json_tokenizer_create_p(struct ml666_json_tokenizer_create_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666__json_tokenizer_private*restrict tokenizer = args.malloc(args.user_ptr, sizeof(*tokenizer));
  if(!tokenizer){
    fprintf(stderr, "%s:%u: malloc failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error;
  }
  memset(tokenizer, 0, sizeof(*tokenizer));
  *(const struct ml666_json_tokenizer_cb**)&tokenizer->public.cb = &tokenizer_cb;
  tokenizer->fd = args.fd;
  tokenizer->malloc = args.malloc;
  tokenizer->free = args.free;
  tokenizer->disable_utf8_validation = args.disable_utf8_validation;
  tokenizer->public.user_ptr = args.user_ptr;
  tokenizer->public.line = 1;
  tokenizer->public.column = 1;

  const unsigned size = get_ringbuffer_size();

  const int memfd = memfd_create("ml666 json token emmiter ringbuffer", MFD_CLOEXEC);
  if(memfd == -1){
    fprintf(stderr, "%s:%u: memfd_create failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_calloc;
  }

  if(ftruncate(memfd, size) == -1){
    fprintf(stderr, "%s:%u: ftruncate failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_memfd;
  }

  // Allocate any 4 free pages |A|B|C|D|
  char*const mem = mmap(0, size*4, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(mem == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_memfd;
  }

  // Replace them with the same one, rw |E|B|C|D|
  if(mmap(mem, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one, rw |E|E|C|D|
  if(mmap(mem+size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one, ro |E|E|E|D|
  if(mmap(mem+size*2, size, PROT_READ, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one, ro |E|E|E|E|
  if(mmap(mem+size*3, size, PROT_READ, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  close(memfd);

  tokenizer->memory = mem;
  tokenizer->j_state = JS_ELEMENT;
  tokenizer->next_j_state = JS_EOF;
  tokenizer->skip_spaces = true;
  tokenizer->no_arrays = args.no_arrays;
  tokenizer->no_objects = args.no_objects;

  return &tokenizer->public;

error_mmap:
  munmap(mem, size*4);
error_memfd:
  close(memfd);
error_calloc:
  args.free(args.user_ptr, tokenizer);
error:
  close(args.fd);
  return 0;
}

static bool ml666_json_tokenizer_d_next(struct ml666_json_tokenizer* _tokenizer){
  struct ml666__json_tokenizer_private*restrict const tokenizer = (struct ml666__json_tokenizer_private*)_tokenizer;
  tokenizer->public.token = ML666_JSON_NONE;
  tokenizer->public.match = (struct ml666_buffer){0};
  tokenizer->public.match_ro = (struct ml666_buffer_ro){0};

  const int fd = tokenizer->fd;
  if(fd == -1) return false;

  const unsigned size = get_ringbuffer_size();
  size_t line = tokenizer->public.line;
  size_t column = tokenizer->public.column;

  char*restrict memory = tokenizer->memory;
#define memory_ro (memory+size*2)
  unsigned offset = tokenizer->offset;
  unsigned index = tokenizer->index;
  unsigned length = tokenizer->length;
  unsigned cpo = tokenizer->cpo;
  bool progress = false;
  enum ml666_json_token token = ML666_JSON_NONE;

  do {
    while(index < length && token == ML666_JSON_NONE){
      const char ch = memory[offset+index];
      const enum ml666_json_token target_token = tokenizer->j_state == JS_STRING_2 ? ML666_JSON_STRING : ML666_JSON_NONE;
      unsigned advance = index+1;

      if(tokenizer->skip_spaces){
        while(index < length)
        switch(memory[offset+index]){
          case ' ': case '\t': case '\r': {
            index += 1;
            column += 1;
          } continue;
          case '\n': {
            index += 1;
            column = 0;
            line += 1;
          } continue;
          default: {
            tokenizer->skip_spaces = false;
            advance = index;
          } goto advance;
        }
        offset = 0;
        index = 0;
        length = 0;
        break;
      }

      if(target_token == ML666_JSON_STRING && ch == '\\'){
        if(length-index < 2)
          break;
        char ch = memory[offset+index+1];
        if(ch == 'u'){
          if(length-index < 6)
            break;
          signed char num[] = {
            hex2num(memory[offset+index+2]),
            hex2num(memory[offset+index+2]),
            hex2num(memory[offset+index+2]),
            hex2num(memory[offset+index+2]),
          };
          if(num[0] == -1 || num[1] == -1 || num[2] == -1 || num[3] == -1){
            tokenizer->public.error = "syntax error: invalid escape sequence";
            goto error;
          }
          index += 6;
          column += 6;
          uint16_t codepoint = num[0] * 0x1000u + num[1] * 0x100u + num[2] * 0x10u + num[3];
          // FIXME: Do surrogates need to be handled?
          if(codepoint < 0x80){
            ch = codepoint;
            cpo += 5;
          }else if(codepoint < 0x800u){
            cpo += 4;
            memory[offset+index-cpo-2] = 0xC0 | (codepoint >> 6);
            memory[offset+index-cpo-1] = 0x80 | (codepoint & 0x3F);
            continue;
          }else{
            cpo += 3;
            memory[offset+index-cpo-3] = 0xE0 | (codepoint >> 12);
            memory[offset+index-cpo-2] = 0x80 | ((codepoint >> 6) & 0x3F);
            memory[offset+index-cpo-1] = 0x80 | (codepoint & 0x3F);
            continue;
          }
        }else if(ch == 'x'){
          if(length-index < 4)
            break;
          signed char num[] = {
            hex2num(memory[offset+index+2]),
            hex2num(memory[offset+index+3]),
          };
          if(num[0] == -1 || num[1] == -1){
            tokenizer->public.error = "syntax error: invalid escape sequence";
            goto error;
          }
          ch = num[0] * 0x10 + num[1];
          index += 4;
          column += 4;
          cpo += 3;
        }else{
          column += 1;
          switch(ch){
            case '"': case '/': case '\\': break;
            case 'b': ch = '\b'  ; break;
            case 'f': ch = '\f'  ; break;
            case 'n': ch = '\n'  ; break;
            case 'r': ch = '\r'  ; break;
            case 't': ch = '\t'  ; break;
            default: {
              tokenizer->public.error = "syntax error: invalid escape sequence";
            } goto error;
          }
          column += 1;
          index += 2;
          cpo += 1;
        }
        memory[offset+index-cpo-1] = ch;
        continue;
      }else{
        if((unsigned char)ch < 0x20){
          tokenizer->public.error = "syntax error: Control characters must be escaped";
          goto error;
        }
        if(cpo)
          memory[offset+index-cpo] = ch;
      }

      bool match = false;

      switch(tokenizer->j_state){
        case JS_ELEMENT: {
          switch(ch){
            case '{': {
              token = ML666_JSON_OBJECT_START;
              tokenizer->public.complete = true;
              tokenizer->skip_spaces = true;
              tokenizer->j_state = JS_OBJECT_FIRST;
              tokenizer->next_j_state = JS_OBJECT_KEY;
              if(!aobmap_push(tokenizer, true)){
                tokenizer->public.error = "aobmap_push failed";
                goto error;
              }
            } break;
            case '[': {
              token = ML666_JSON_ARRAY_START;
              tokenizer->public.complete = true;
              tokenizer->skip_spaces = true;
              tokenizer->j_state = JS_ARRAY_FIRST;
              tokenizer->next_j_state = JS_ARRAY;
              if(!aobmap_push(tokenizer, false)){
                tokenizer->public.error = "aobmap_push failed";
                goto error;
              }
            } break;
            case '"': {
              tokenizer->j_state = JS_STRING_2;
            } break;
            case 't': tokenizer->j_state = JS_TRUE; break;
            case 'f': tokenizer->j_state = JS_FALSE; break;
            case 'n': tokenizer->j_state = JS_NULL; break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            case '-': tokenizer->j_state = JS_NUMBER; break;
            default: tokenizer->public.error = "syntax error"; goto error;
          }
        } break;
        case JS_OBJECT_FIRST: {
          tokenizer->skip_spaces = true;
          if(ch == '}'){
            if(!aobmap_pop(tokenizer)){
              tokenizer->public.error = "bad tokenizer state";
              goto error;
            }
            tokenizer->j_state = aobmap_peek(tokenizer);
            token = ML666_JSON_OBJECT_END;
            tokenizer->public.complete = true;
          }else{
            tokenizer->j_state = JS_STRING;
            tokenizer->next_j_state = JS_OBJECT_KEY;
            token = ML666_JSON_OBJECT_KEY;
            tokenizer->public.complete = true;
            continue;
          }
        } break;
        case JS_OBJECT_KEY: {
          if(ch != ':'){
            tokenizer->public.error = "syntax error. Expected ':'";
            goto error;
          }
          tokenizer->skip_spaces = true;
          tokenizer->j_state = JS_ELEMENT;
          tokenizer->next_j_state = JS_OBJECT_VALUE;
        } break;
        case JS_OBJECT_VALUE: {
          tokenizer->skip_spaces = true;
          if(ch == ','){
            tokenizer->j_state = JS_STRING;
            tokenizer->next_j_state = JS_OBJECT_KEY;
            token = ML666_JSON_OBJECT_KEY;
            tokenizer->public.complete = true;
          }else if(ch == '}'){
            if(!aobmap_pop(tokenizer)){
              tokenizer->public.error = "bad tokenizer state";
              goto error;
            }
            tokenizer->j_state = aobmap_peek(tokenizer);
            token = ML666_JSON_OBJECT_END;
            tokenizer->public.complete = true;
          }else{
            tokenizer->public.error = "syntax error. Expected ',' or '}'";
            goto error;
          }
        } break;
        case JS_ARRAY_FIRST: {
          tokenizer->skip_spaces = true;
          if(ch == ']'){
            if(!aobmap_pop(tokenizer)){
              tokenizer->public.error = "bad tokenizer state";
              goto error;
            }
            tokenizer->j_state = aobmap_peek(tokenizer);
            token = ML666_JSON_ARRAY_END;
            tokenizer->public.complete = true;
          }else{
            tokenizer->j_state = JS_ELEMENT;
            tokenizer->next_j_state = JS_ARRAY;
            continue;
          }
        } break;
        case JS_ARRAY: {
          tokenizer->skip_spaces = true;
          if(ch == ','){
            tokenizer->j_state = JS_ELEMENT;
            tokenizer->next_j_state = JS_ARRAY;
          }else if(ch == ']'){
            if(!aobmap_pop(tokenizer)){
              tokenizer->public.error = "bad tokenizer state";
              goto error;
            }
            token = ML666_JSON_ARRAY_END;
            tokenizer->public.complete = true;
            tokenizer->j_state = aobmap_peek(tokenizer);
          }else{
            tokenizer->public.error = "syntax error. Expected ',' or '}'";
            goto error;
          }
        } break;
        case JS_NUMBER: puts("JS_NUMBER"); break;
        case JS_NUMBER_FRACTION: puts("JS_NUMBER_FRACTION"); break;
        case JS_NUMBER_EXPONENT: puts("JS_NUMBER_EXPONENT"); break;
        case JS_STRING: {
          if(ch != '"'){
            tokenizer->public.error = "syntax error: expected \"";
            goto error;
          }
          tokenizer->j_state = JS_STRING_2;
        } break;
        case JS_STRING_2: {
          advance = 0;
          if(ch == '"'){
            match = true;
            tokenizer->skip_spaces = true;
            tokenizer->j_state = tokenizer->next_j_state;
          }
        } break;
        case JS_EOF: {
          tokenizer->public.error = "syntax error: expected eof";
          goto error;
        } break;
        case JS_TRUE: {
          if(index < 2){
            advance = 0;
          }else if(memcmp(&memory[offset], "rue", 3)){
            tokenizer->public.error = "syntax error. Expected 'true'.";
            goto error;
          }else{
            tokenizer->skip_spaces = true;
            tokenizer->j_state = tokenizer->next_j_state;
            token = ML666_JSON_TRUE;
            tokenizer->public.complete = true;
          }
        } break;
        case JS_FALSE: {
          if(index < 3){
            advance = 0;
          }else if(memcmp(&memory[offset], "alse", 4)){
            tokenizer->public.error = "syntax error. Expected 'false'.";
            goto error;
          }else{
            tokenizer->skip_spaces = true;
            tokenizer->j_state = tokenizer->next_j_state;
            token = ML666_JSON_FALSE;
            tokenizer->public.complete = true;
          }
        } break;
        case JS_NULL: {
          if(index < 2){
            advance = 0;
          }else if(memcmp(&memory[offset], "ull", 3)){
            tokenizer->public.error = "syntax error. Expected 'null'.";
            goto error;
          }else{
            tokenizer->skip_spaces = true;
            tokenizer->j_state = tokenizer->next_j_state;
            token = ML666_JSON_NULL;
            tokenizer->public.complete = true;
          }
        } break;
      }

      if(match){
        token = target_token;
        tokenizer->public.complete = true;
        if(index - cpo){
          tokenizer->public.match = (struct ml666_buffer){
            .data = &memory[offset],
            .length = index - cpo,
          };
          tokenizer->public.match_ro = (struct ml666_buffer_ro){
            .data = &memory_ro[offset],
            .length = index - cpo,
          };
        }
        cpo = 0;
      }

      index += 1;
      column += 1;
      if(ch == '\n'){
        line += 1;
        column = 1;
      }

      advance: if(advance){
        length -= advance;
        if(!length){
          index = 0;
          offset = 0;
        }else{
          index   = advance >= index ? 0 : index - advance;
          offset += advance;
          if(offset >= size)
            offset -= size;
        }
      }
    }

    if(length >= size || tokenizer->eof)
      break;

    if(!token)
    while(length < size && !tokenizer->eof && (!tokenizer->may_block || length - index == 0 || !progress)){
      unsigned write_end = offset + length;
      if(write_end >= size)
        write_end -= size;
      int result = read(fd, &memory[write_end], size - length);
      if(result < 0 && errno == EWOULDBLOCK){
        result = 0;
      }else{
        if(result < 0){
          if(errno == EINTR)
            continue;
          tokenizer->public.error = strerror(errno);
          goto error;
        }
        if(result == 0){
          tokenizer->eof = true;
          if(!ml666_utf8_validate(&tokenizer->utf8_validator, EOF)){
            tokenizer->public.error = "the ml666 document must be valid & normalized utf-8!";
            goto error;
          }
        }else if(!tokenizer->disable_utf8_validation){
          for(int i=0; i<result; i++){
            if(!ml666_utf8_validate(&tokenizer->utf8_validator, memory[write_end+i])){
              tokenizer->public.error = "the ml666 document must be valid & normalized utf-8!";
              goto error;
            }
          }
        }
      }
      tokenizer->may_block = (unsigned)result < size - length;
      length += result;
      progress = true;
      break;
    }

  } while(!token);

  if(!token){
    const enum ml666_json_token target_token = tokenizer->j_state == JS_STRING_2 ? ML666_JSON_STRING : ML666_JSON_NONE;
    if(target_token && index > cpo && (length >= size || tokenizer->eof)){
      if(index - cpo){
        token = target_token;
        tokenizer->public.match = (struct ml666_buffer){
          .data = &memory[offset],
          .length = index - cpo,
        };
        tokenizer->public.match_ro = (struct ml666_buffer_ro){
          .data = &memory_ro[offset],
          .length = index - cpo,
        };
        tokenizer->public.complete = false;
      }
      cpo = 0;
      offset += index;
      length -= index;
      if(offset >= size)
        offset -= size;
      index = 0;
    }
  }

  if(!token && !tokenizer->length && tokenizer->j_state == JS_EOF)
    token = ML666_JSON_EOF;

  if(token || index != tokenizer->index || offset != tokenizer->offset)
    progress = true;
  tokenizer->length = length;
  tokenizer->index = index;
  tokenizer->offset = offset;
  tokenizer->cpo = cpo;
  tokenizer->public.line = line;
  tokenizer->public.column = column;
  tokenizer->public.token = token;
  if(!progress){
    tokenizer->public.error = "ml666 json token emmiter failed to progress, was the input incomplete?";
    goto error;
  }
  if(token == ML666_JSON_EOF)
    goto final;
  return true;

error:
  tokenizer->public.line = line;
  tokenizer->public.column = column;
  tokenizer->public.token = ML666_JSON_EOF;
  tokenizer->public.match = (struct ml666_buffer){0};
  tokenizer->public.match_ro = (struct ml666_buffer_ro){0};

final:
  // Let's free this stuff as early as possible
  if(munmap(tokenizer->memory, size*4))
    fprintf(stderr, "%s:%u: munmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  tokenizer->memory = 0;
  if(close(tokenizer->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  free(tokenizer->aobmap);
  tokenizer->aobmap = 0;
  tokenizer->fd = -1;
  return false;

  #undef memory_ro
}

static void ml666_json_tokenizer_d_destroy(struct ml666_json_tokenizer* _tokenizer){
  if(!_tokenizer) return;
  const unsigned size = get_ringbuffer_size();
  struct ml666__json_tokenizer_private*restrict tokenizer = (struct ml666__json_tokenizer_private*)_tokenizer;
  if(tokenizer->memory && munmap(tokenizer->memory, size*4))
    fprintf(stderr, "%s:%u: munmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  if(tokenizer->fd != -1 && close(tokenizer->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  free(tokenizer->aobmap);
  tokenizer->aobmap = 0;
  tokenizer->free(tokenizer->public.user_ptr, tokenizer);
}
