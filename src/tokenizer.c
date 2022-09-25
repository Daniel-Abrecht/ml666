#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ml666/tokenizer.h>
#include <ml666/utils.h>

#define ML666__STATE \
  X(ML666__STATE_EOF) \
  X(ML666__STATE_TEXT) \
  X(ML666__STATE_TEXT_EXPECT_QUOTE) \
  X(ML666__STATE_TEXT_FIRST_NEWLINE) \
  X(ML666__STATE_COMMENT_START) \
  X(ML666__STATE_COMMENT_FIRST_NEWLINE) \
  X(ML666__STATE_COMMENT) \
  X(ML666__STATE_COMMENT_END_PRE) \
  X(ML666__STATE_COMMENT_END) \
  X(ML666__STATE_COMMENT_LINE_START) \
  X(ML666__STATE_COMMENT_LINE) \
  X(ML666__STATE_SELF_CLOSE) \
  X(ML666__STATE_TAG_OR_END) \
  X(ML666__STATE_MEMBER) \
  X(ML666__STATE_TAG) \
  X(ML666__STATE_END_TAG) \
  X(ML666__STATE_ATTRIBUTE) \
  X(ML666__STATE_ATTRIBUTE_START) \
  X(ML666__STATE_ATTRIBUTE_VALUE) \
  X(ML666__STATE_ATTRIBUTE_VALUE_TEXT) \
  X(ML666__STATE_ATTRIBUTE_VALUE_FIRST_NEWLINE) \
  X(ML666__STATE_ATTRIBUTE_VALUE_TEXT_EXPECT_QUOTE) \
  X(ML666__STATE_EXPECT_LT)
enum ml666__state {
#define X(Y) Y,
  ML666__STATE
#undef X
  ML666__STATE_COUNT
};
const char*const ml666__state_name[] = {
#define X(Y) #Y,
  ML666__STATE
#undef X
};
#undef ML666__STATE

static const enum ml666_token ml666__state_token_map[ML666__STATE_COUNT] = {
  [ML666__STATE_TAG] = ML666_TAG,
  [ML666__STATE_END_TAG] = ML666_END_TAG,
  [ML666__STATE_SELF_CLOSE] = ML666_END_TAG,
  [ML666__STATE_ATTRIBUTE] = ML666_ATTRIBUTE,
  [ML666__STATE_ATTRIBUTE_VALUE_TEXT] = ML666_ATTRIBUTE_VALUE,
  [ML666__STATE_TEXT] = ML666_TEXT,
  [ML666__STATE_COMMENT] = ML666_COMMENT,
  [ML666__STATE_COMMENT_LINE] = ML666_COMMENT,
};

enum ml666__text_encoding {
  ML666__ENCODING_NONE,
  ML666__ENCODING_HEX,
  ML666__ENCODING_BASE64
};

struct ml666__tokenizer_private {
  struct ml666_tokenizer public;
  int fd;
  unsigned offset, index, length, cpo, spaces;
  enum ml666__state state;
  char* memory; // This is a ring buffer
  bool may_block, eof, spnf, ecsp, disable_utf8_validation;
  struct streaming_utf8_validator utf8_validator;
  uint8_t decode_akku, decode_index;
  union {
    enum ml666__state comment_next_state;
    enum ml666__text_encoding text_encoding;
  };
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};
static_assert(offsetof(struct ml666__tokenizer_private, public) == 0, "ml666__tokenizer_private::public must be the first member");

const char*const ml666__token_name[] = {
#define X(Y) #Y,
  ML666__TOKENS
#undef X
};

__attribute__((const))
static inline unsigned get_ringbuffer_size(void){
  const unsigned sz = sysconf(_SC_PAGESIZE);
  return (sz-1 + 4096) / sz * sz;
}

static const char* space_page;

__attribute__((constructor))
static void init(void){
  const unsigned size = get_ringbuffer_size();
  char*const x = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(x == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    abort();
  }
  x[0] = '\n';
  memset(x+1, ' ', size-1);
  // Make sure only reading is possible. This isn't really necessary, but it'll be handy to avoid & debug accidental drite access
  if(mprotect(x, size, PROT_READ)){
    fprintf(stderr, "%s:%u: mprotect failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  }
  space_page = x;
}

struct ml666_tokenizer* ml666_tokenizer_create_p(struct ml666_tokenizer_create_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666__tokenizer_private*restrict tokenizer = args.malloc(args.user_ptr, sizeof(*tokenizer));
  if(!tokenizer){
    fprintf(stderr, "%s:%u: memfd_create failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error;
  }
  memset(tokenizer, 0, sizeof(*tokenizer));
  tokenizer->fd = args.fd;
  tokenizer->malloc = args.malloc;
  tokenizer->free = args.free;
  tokenizer->public.user_ptr = args.user_ptr;
  tokenizer->public.line = 1;
  tokenizer->public.column = 1;

  const unsigned size = get_ringbuffer_size();

  const int memfd = memfd_create("ml666 tokenizer ringbuffer", MFD_CLOEXEC);
  if(memfd == -1){
    fprintf(stderr, "%s:%u: memfd_create failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_calloc;
  }

  if(ftruncate(memfd, size) == -1){
    fprintf(stderr, "%s:%u: ftruncate failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_memfd;
  }

  // Allocate any 4 free pages |A|B|C|D
  char*const mem = mmap(0, size*4, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(mem == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one, rw  |E|B|C|D|
  if(mmap(mem, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one , rw |E|E|C|D|
  if(mmap(mem+size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
    fprintf(stderr, "%s:%u: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error_mmap;
  }

  // Replace them with the same one, ro  |E|E|E|D|
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
  tokenizer->state = ML666__STATE_MEMBER;

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

// +/ are part of base64, -_ is used in base64url
static signed char b64_to_num(char x){
  if(x >= 'A' && x <= 'Z')
    return x - 'A';
  if(x >= 'a' && x <= 'z')
    return x - 'a' + 26;
  if(x >= '0' && x <= '9')
    return x - '0' + 52;
  if(x == '+' || x == '-')
    return 62;
  if(x == '/' || x == '_')
    return 63;
   return -1;
}

static signed char hex2num(char ch){
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  if(ch >= 'a' && ch <= 'z')
    return ch - 'a' + 10;
  if(ch >= 'A' && ch <= 'Z')
    return ch - 'A' + 10;
  return -1;
}

bool ml666_tokenizer_next(struct ml666_tokenizer* _tokenizer){
  struct ml666__tokenizer_private*restrict tokenizer = (struct ml666__tokenizer_private*)_tokenizer;
  tokenizer->public.token = ML666_NONE;
  tokenizer->public.match = (struct ml666_buffer_ro){0};

  const unsigned size = get_ringbuffer_size();
  size_t line = tokenizer->public.line;
  size_t column = tokenizer->public.column;

  enum ml666__state state = tokenizer->state;
  if(state == ML666__STATE_EOF)
    return false;
  if(state >= ML666__STATE_COUNT || state < 0){
    tokenizer->public.error = "Invalid state";
    goto error;
  }

  char*restrict memory = tokenizer->memory;
#define memory_ro (memory+size*2)
  const int fd = tokenizer->fd;
  unsigned offset = tokenizer->offset;
  unsigned index = tokenizer->index;
  unsigned length = tokenizer->length;
  unsigned cpo = tokenizer->cpo;
  size_t spaces = tokenizer->spaces;
  bool ecsp = tokenizer->ecsp;
  bool progress = false;
  enum ml666_token token = ML666_NONE;

  do {
    while(index < length && token == ML666_NONE){
      const char ch = memory[offset+index];
      const enum ml666_token target_token = ml666__state_token_map[state];

      if( (state == ML666__STATE_ATTRIBUTE_VALUE_TEXT || state == ML666__STATE_TEXT)
       && tokenizer->text_encoding != ML666__ENCODING_NONE
      ){ // Special case, encoded binary data of sorts

        ecsp = true;
        if(ch == ' ' || ch == '\n'){
          cpo += 1;
        }else if(ch == '`'){
          ecsp = false;
          if(tokenizer->decode_index == 1){
            tokenizer->public.error = "syntax error: encoded text incomplete";
            goto error;
          }
        }else{
          unsigned didx = tokenizer->decode_index + 1;
          if(tokenizer->text_encoding == ML666__ENCODING_HEX){
            signed char num = hex2num(ch);
            if(num < 0){
              tokenizer->public.error = "syntax error: invalid character in hex encoded text";
              goto error;
            }
            if(didx >= 2){
              memory[offset+index-cpo] = tokenizer->decode_akku<<4 | num;
              didx = 0;
            }else{
              tokenizer->decode_akku = num;
              cpo += 1;
            }
          }else if(tokenizer->text_encoding == ML666__ENCODING_BASE64){
            signed char num = b64_to_num(ch);
            if(ch == '=' && (didx == 3 || didx == 4))
              num = 0;
            if(num < 0){
              tokenizer->public.error = "syntax error: invalid character in base64/base64url encoded text";
              goto error;
            }
            switch(didx){
              case 1: {
                cpo += 1;
                tokenizer->decode_akku = num;
              } break;
              case 2: {
                memory[offset+index-cpo] = (tokenizer->decode_akku<<2) | (num>>4);
                tokenizer->decode_akku = num & 0x0F;
              } break;
              case 3: {
                if(ch != '='){
                  memory[offset+index-cpo] = (tokenizer->decode_akku<<4) | (num>>2);
                  tokenizer->decode_akku = num & 0x03;
                }else{
                  cpo += 1;
                  tokenizer->decode_akku = 64;
                }
              } break;
              case 4: {
                if(ch != '='){
                  if(tokenizer->decode_akku == 64){
                    tokenizer->public.error = "syntax error: unexpected character while decoding base64: expected =";
                    goto error;
                  }
                  didx = 0;
                  memory[offset+index-cpo] = (tokenizer->decode_akku<<6) | num;
                  tokenizer->decode_akku = 0;
                }else{
                  cpo += 1;
                }
              } break;
              case 5: {
                tokenizer->public.error = "syntax error: unexpected character while decoding base64: got a character after the final =";
                goto error;
              } break;
            }
            tokenizer->decode_akku = num;
          }else{
            tokenizer->public.error = "tokenizer error: invalid state";
            goto error;
          }
          tokenizer->decode_index = didx;
        }

      }else if(target_token && !ecsp){ // Handle escaping, if data is part of a potential token
        if(ch == '\\'){
          if(length-index < 2)
            break;
          char ch = memory[offset+index+1];
          if(ch == 'x'){
            if(length-index < 4)
              break;
            signed char a = hex2num(memory[offset+index+2]);
            signed char b = hex2num(memory[offset+index+3]);
            if(a == -1 || b == -1){
              tokenizer->public.error = "syntax error: invalid escape sequence";
              goto error;
            }
            ch = a * 16 + b;
            index += 4;
            column += 4;
            cpo += 3;
          }else{
            column += 1;
            switch(ch){
              case '*': case '/': case '>':
              case '`': case '=': case ' ':
              case '\\': break;
              case 'a': ch = '\a'  ; break;
              case 'b': ch = '\b'  ; break;
              case 'e': ch = '\x1B'; break;
              case 'f': ch = '\f'  ; break;
              case 'n': ch = '\n'  ; break;
              case 't': ch = '\t'  ; break;
              case 'v': ch = '\v'  ; break;
              default: {
                tokenizer->public.error = "syntax error: invalid escape sequence";
              } goto error;
            }
            column += 1;
            index += 2;
            cpo += 1;
          }
          memory[offset+index-cpo-1] = ch;
          if(spaces <= index){
            spaces = 0;
            continue;
          }else{
            ecsp = true;
          }
        }else if(cpo)
          memory[offset+index-cpo] = ch;
      }

      bool omit_last_spaces = (
          state == ML666__STATE_ATTRIBUTE_VALUE_TEXT
       || state == ML666__STATE_TEXT
       || state == ML666__STATE_COMMENT
      );

      bool space = false;
      if(ch==' ' || ch=='\n')
        space = true;
      bool match = false;
      bool include_final = false;

      if(!ecsp)
      switch(state){
        case ML666__STATE_EOF: abort();
        case ML666__STATE_MEMBER: {
          if(!space)
          switch(ch){
            case '<': state = ML666__STATE_TAG_OR_END; break;
            case '`': {
              state = ML666__STATE_TEXT_FIRST_NEWLINE;
              tokenizer->text_encoding = ML666__ENCODING_NONE;
            } break;
            case 'H': {
              state = ML666__STATE_TEXT_EXPECT_QUOTE;
              tokenizer->text_encoding = ML666__ENCODING_HEX;
            } break;
            case 'B': {
              state = ML666__STATE_TEXT_EXPECT_QUOTE;
              tokenizer->text_encoding = ML666__ENCODING_BASE64;
            } break;
            case '/': {
              state = ML666__STATE_COMMENT_START;
              tokenizer->comment_next_state = ML666__STATE_MEMBER;
            } break;
            default: tokenizer->public.error = "syntax error. expected one of these characters: <`/"; goto error;
          }
        } break;
        case ML666__STATE_TEXT_EXPECT_QUOTE: {
          if(ch != '`'){
            tokenizer->public.error = "syntax error: expected: `";
            goto error;
          }
          tokenizer->decode_akku = 0;
          tokenizer->decode_index = 0;
          state = ML666__STATE_TEXT_FIRST_NEWLINE;
        } break;
        case ML666__STATE_TAG_OR_END: {
          if(ch == '/'){
            state = ML666__STATE_END_TAG;
          }else{
            state = ML666__STATE_TAG;
            continue;
          }
        } break;
        case ML666__STATE_TAG: {
          match = true;
          if(space){
            state = ML666__STATE_ATTRIBUTE_START;
          }else if(ch == '>'){
            state = ML666__STATE_MEMBER;
          }else if(ch == '/'){
            state = ML666__STATE_SELF_CLOSE;
          }else match = false;
        } break;
        case ML666__STATE_END_TAG: {
          if(ch == '>' || space){
            match = true;
            state = space ? ML666__STATE_EXPECT_LT : ML666__STATE_MEMBER;
          }
        } break;
        case ML666__STATE_ATTRIBUTE_START: {
          if(!space)
          switch(ch){
            case '>': state = ML666__STATE_MEMBER; break;
            case '/': state = ML666__STATE_SELF_CLOSE; break;
            case '=': {
              tokenizer->public.error = "Empty attributes are not allowed";
              goto error;
            }; break;
            default : state = ML666__STATE_ATTRIBUTE; continue;
          }
        } break;
        case ML666__STATE_ATTRIBUTE: {
          if(space){
            match = true;
            state = ML666__STATE_ATTRIBUTE_START;
          }else if(ch == '>' || ch == '=' || ch == '/'){
            match = true;
            switch(ch){
              case '=': state = ML666__STATE_ATTRIBUTE_VALUE; break;
              case '>': state = ML666__STATE_MEMBER; break;
              case '/': state = ML666__STATE_SELF_CLOSE; break;
            }
          }
        } break;
        case ML666__STATE_ATTRIBUTE_VALUE: {
          if(ch == '`'){
            state = ML666__STATE_ATTRIBUTE_VALUE_FIRST_NEWLINE;
            tokenizer->text_encoding = ML666__ENCODING_NONE;
          }else if(ch == 'H'){
            state = ML666__STATE_ATTRIBUTE_VALUE_TEXT_EXPECT_QUOTE;
            tokenizer->text_encoding = ML666__ENCODING_HEX;
          }else if(ch == 'B'){
            state = ML666__STATE_ATTRIBUTE_VALUE_TEXT_EXPECT_QUOTE;
            tokenizer->text_encoding = ML666__ENCODING_BASE64;
          }else{
            tokenizer->public.error = "syntax error: attribute value must be introduced using `";
            goto error;
          }
        } break;
        case ML666__STATE_ATTRIBUTE_VALUE_TEXT_EXPECT_QUOTE: {
          if(ch != '`'){
            tokenizer->public.error = "syntax error: expected: `";
            goto error;
          }
          tokenizer->decode_akku = 0;
          tokenizer->decode_index = 0;
          state = ML666__STATE_ATTRIBUTE_VALUE_TEXT;
        } break;
        case ML666__STATE_ATTRIBUTE_VALUE_FIRST_NEWLINE: {
          state = ML666__STATE_ATTRIBUTE_VALUE_TEXT;
          if(ch != '\n')
            continue;
        } break;
        case ML666__STATE_ATTRIBUTE_VALUE_TEXT: {
          if(ch == '`'){
            match = true;
            state = ML666__STATE_ATTRIBUTE_START;
          }
        } break;
        case ML666__STATE_TEXT_FIRST_NEWLINE: {
          state = ML666__STATE_TEXT;
          if(ch != '\n')
            continue;
        } break;
        case ML666__STATE_TEXT: {
          if(ch == '`'){
            match = true;
            state = ML666__STATE_MEMBER;
          }
        } break;
        case ML666__STATE_COMMENT_START: {
          if(ch == '*'){
            state = ML666__STATE_COMMENT_FIRST_NEWLINE;
          }else if(ch == '/'){
            state = ML666__STATE_COMMENT_LINE_START;
          }else{
            tokenizer->public.error = "syntax error: expected * or /";
            goto error;
          }
        } break;
        case ML666__STATE_COMMENT_FIRST_NEWLINE: {
          state = ML666__STATE_COMMENT;
          if(ch != '\n' && ch != ' ')
            continue;
        } break;
        case ML666__STATE_COMMENT: {
          if(ch == '*'){
            match = true;
            state = ML666__STATE_COMMENT_END;
          }else if(ch == ' '){
            if(length-index < 2)
              break;
            if(memory[offset+index+1] == '*'){
              match = true;
              state = ML666__STATE_COMMENT_END_PRE;
            }
          }
        } break;
        case ML666__STATE_COMMENT_END_PRE: {
          if(ch != '*'){
            tokenizer->public.error = "syntax error: expected *";
            goto error;
          }
          state = ML666__STATE_COMMENT_END;
        }; break;
        case ML666__STATE_COMMENT_END: {
          if(ch != '/'){
            tokenizer->public.error = "syntax error: expected /";
            goto error;
          }
          state = tokenizer->comment_next_state;
        }; break;
        case ML666__STATE_COMMENT_LINE_START: {
          state = ML666__STATE_COMMENT_LINE;
          if(ch != ' ')
            continue;
        } break;
        case ML666__STATE_COMMENT_LINE: {
          if(ch == '\n'){
            match = true;
            state = tokenizer->comment_next_state;
            include_final = true;
          }
        } break;
        case ML666__STATE_SELF_CLOSE: {
          if(ch == '*' || ch == '/'){
            state = ML666__STATE_COMMENT_START;
            tokenizer->comment_next_state = ML666__STATE_ATTRIBUTE_START;
            continue;
          }else if(ch != '>'){
            tokenizer->public.error = "syntax error: expected: \">\"";
            goto error;
          }
          match = true;
          state = ML666__STATE_MEMBER;
        }; break;
        case ML666__STATE_EXPECT_LT: {
          if(ch != '>'){
            tokenizer->public.error = "syntax error: expected: \">\"";
            goto error;
          }
          state = ML666__STATE_MEMBER;
        }; break;
        case ML666__STATE_COUNT: abort();
      }

      unsigned advance = 0;
      if(!match){
        if(!ecsp && ch == ' ' && spaces){
          spaces += 1;
          if(!spaces){
            tokenizer->public.error = "tokenizer error: too many spaces, space counter did overflow!";
            goto error;
          }
        }else{
          if(spaces > index){
            const bool nspnf = !tokenizer->spnf;
            unsigned nspaces = spaces > size - nspnf ? size - nspnf : spaces;
            spaces -= nspaces;
            token = target_token;
            tokenizer->public.match = (struct ml666_buffer_ro){
              .data = space_page + nspnf,
              .length = nspaces,
            };
            tokenizer->public.complete = false;
            tokenizer->spnf = false;
            if(index >= spaces){
              advance = index - spaces;
              if(advance)
                goto advance;
            }
            break;
          }
          if(!ecsp && ch == '\n'){
            spaces = 1;
            tokenizer->spnf = true;
          }else{
            spaces = 0;
          }
          ecsp = false;
        }
        if(!omit_last_spaces)
          spaces = 0;
        if(!target_token){
          advance = 1;
          spaces = 0;
        }
      }else{
        token = target_token;
        tokenizer->public.complete = true;
        if(index - cpo > spaces){
          tokenizer->public.match = (struct ml666_buffer_ro){
            .data = &memory_ro[offset],
            .length = index - cpo - spaces + include_final,
          };
        }
        cpo = 0;
        spaces = 0;
        advance = index + 1;
      }

      index += 1;
      column += 1;
      if(ch == '\n'){
        line += 1;
        column = 1;
      }

    advance:
      if(advance){
        index   = advance >= index ? 0 : index - advance;
        length -= advance;
        offset += advance;
        if(offset >= size)
          offset -= size;
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
          if(!utf8_validate(&tokenizer->utf8_validator, EOF)){
            tokenizer->public.error = "the ml666 document must be valid & normalized utf-8!";
            goto error;
          }
        }else if(!tokenizer->disable_utf8_validation){
          for(int i=0; i<result; i++){
            if(!utf8_validate(&tokenizer->utf8_validator, memory[write_end+i])){
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
    const enum ml666_token target_token = ml666__state_token_map[state];
    if(target_token && index > cpo && (length >= size || tokenizer->eof)){
      if(index - cpo > spaces){
        token = target_token;
        tokenizer->public.match = (struct ml666_buffer_ro){
          .data = &memory_ro[offset],
          .length = index - cpo - spaces,
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

  if(!token && tokenizer->eof)
    token = ML666_EOF;

  if(token == ML666_EOF && state != ML666__STATE_MEMBER && state != ML666__STATE_COMMENT_LINE){
    tokenizer->public.error = "syntax error: early EOF";
    goto error;
  }

  if(token || index != tokenizer->index || offset != tokenizer->offset)
    progress = true;
  tokenizer->length = length;
  tokenizer->state = state;
  tokenizer->index = index;
  tokenizer->offset = offset;
  tokenizer->cpo = cpo;
  tokenizer->public.line = line;
  tokenizer->public.column = column;
  tokenizer->public.token = token;
  tokenizer->spaces = spaces;
  tokenizer->ecsp = ecsp;
  if(!progress){
    tokenizer->public.error = "ml666 tokenizer failed to progress, was the input incomplete?";
    goto error;
  }
  if(token == ML666_EOF)
    goto final;
  return true;

error:
  tokenizer->public.line = line;
  tokenizer->public.column = column;
  tokenizer->public.token = ML666_EOF;
  tokenizer->public.match = (struct ml666_buffer_ro){0};

final:
  // Let's free this stuff as early as possible
  if(munmap(tokenizer->memory, size*4))
    fprintf(stderr, "%s:%u: munmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  if(close(tokenizer->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  tokenizer->fd = -1;
  return false;

  #undef memory_ro
}

void ml666_tokenizer_destroy(struct ml666_tokenizer* _tokenizer){
  if(!_tokenizer) return;
  const unsigned size = get_ringbuffer_size();
  struct ml666__tokenizer_private*restrict tokenizer = (struct ml666__tokenizer_private*)_tokenizer;
  if(tokenizer->memory && munmap(tokenizer->memory, size*4))
    fprintf(stderr, "%s:%u: munmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  if(tokenizer->fd != -1 && close(tokenizer->fd))
    fprintf(stderr, "%s:%u: close failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
  tokenizer->free(tokenizer->public.user_ptr, tokenizer);
}
