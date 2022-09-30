#define _GNU_SOURCE
#include <ml666/simple-tree.h>
#include <ml666/simple-tree-ml666-serializer.h>
#include <ml666/utils.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#define SERIALIZER_STATE \
  X(SERIALIZER_W_START) \
  X(SERIALIZER_W_DONE) \
  X(SERIALIZER_W_NEXT_CHILD) \
  X(SERIALIZER_W_NEXT_MEMBER) \
  X(SERIALIZER_W_TAG_START) \
  X(SERIALIZER_W_TAG) \
  X(SERIALIZER_W_ATTRIBUTE) \
  X(SERIALIZER_W_ATTRIBUTE_VALUE_START) \
  X(SERIALIZER_W_ATTRIBUTE_NEXT) \
  X(SERIALIZER_W_TAG_END) \
  X(SERIALIZER_W_END_TAG_START) \
  X(SERIALIZER_W_END_TAG) \
  X(SERIALIZER_W_END_TAG_END) \
  X(SERIALIZER_W_CONTENT_START) \
  X(SERIALIZER_W_CONTENT) \
  X(SERIALIZER_W_CONTENT_END_1) \
  X(SERIALIZER_W_CONTENT_END_2) \
  X(SERIALIZER_W_COMMENT_START) \
  X(SERIALIZER_W_COMMENT) \
  X(SERIALIZER_W_COMMENT_END_1) \
  X(SERIALIZER_W_COMMENT_END_2)

enum serializer_state {
#define X(Y) Y,
  SERIALIZER_STATE
#undef X
};
static const char*const serializer_state_name[] = {
#define X(Y) #Y,
  SERIALIZER_STATE
#undef X
};

enum data_encoding {
  ENC_RAW,
  ENC_ESCAPED,
  ENC_BASE64,
};

struct ml666_st_serializer_private {
  struct ml666_st_serializer public;
  struct ml666_st_node* node;
  int fd;
  unsigned level, spaces, lf_counter;
  const char* esc_chars;
  struct ml666_buffer_info buffer_info;

  struct ml666_st_node* cur;
  struct ml666_st_attribute* current_attribute;
  enum serializer_state state;

  enum data_encoding encoding;
  struct ml666_buffer_ro data;
  struct ml666_buffer outbuf;
  struct ml666_buffer outptr;

  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};

static ml666_st_serializer_cb_next ml666_st_ml666_serializer_next;
static ml666_st_serializer_cb_destroy ml666_st_ml666_serializer_destroy;

static const struct ml666_st_serializer_cb st_serializer_default = {
  .next = ml666_st_ml666_serializer_next,
  .destroy = ml666_st_ml666_serializer_destroy,
};

struct ml666_st_serializer* ml666_st_ml666_serializer_create_p(struct ml666_st_ml666_serializer_create_args args){
  if(!args.stb){
    fprintf(stderr, "ml666_st_serializer_create_p: mandatory argument \"stb\" not set!\n");
    return false;
  }
  if(!args.node){
    fprintf(stderr, "ml666_st_serializer_create_p: mandatory argument \"node\" not set!\n");
    return false;
  }
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;

  struct ml666_buffer outbuf = {
    .length = sysconf(_SC_PAGESIZE),
  };

  {
    const int memfd = memfd_create("ml666 tokenizer ringbuffer", MFD_CLOEXEC);
    if(memfd == -1){
      fprintf(stderr, "%s:%u: ml666_st_serializer_create_p: memfd_create failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
      return 0;
    }
    if(ftruncate(memfd, outbuf.length) == -1){
      fprintf(stderr, "%s:%u: ml666_st_serializer_create_p: ftruncate failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
      close(memfd);
      return 0;
    }
    // Allocate any 2 free pages |A|B|
    char*const mem = mmap(0, outbuf.length*2, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(mem == MAP_FAILED){
      fprintf(stderr, "%s:%u: ml666_st_serializer_create_p: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
      close(memfd);
    }
    // Replace them with the same one, rw |E|B|
    if(mmap(mem, outbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
      fprintf(stderr, "%s:%u: ml666_st_serializer_create_p: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
      munmap(mem, outbuf.length*2);
      close(memfd);
    }
    // Replace them with the same one, rw |E|E|
    if(mmap(mem+outbuf.length, outbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, memfd, 0) == MAP_FAILED){
      fprintf(stderr, "%s:%u: ml666_st_serializer_create_p: mmap failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
      munmap(mem, outbuf.length*2);
      close(memfd);
    }
    close(memfd);
    outbuf.data = mem;
  }

  struct ml666_st_serializer_private* sts = args.malloc(args.user_ptr, sizeof(*sts));
  if(!sts){
    fprintf(stderr, "malloc failed");
    munmap(outbuf.data, outbuf.length*2);
    return 0;
  }
  memset(sts, 0, sizeof(*sts));
  *(const struct ml666_st_serializer_cb**)&sts->public.cb = &st_serializer_default;
  *(struct ml666_st_builder**)&sts->public.stb = args.stb;
  sts->public.user_ptr = args.user_ptr;
  sts->outbuf = outbuf;
  sts->outptr.data = outbuf.data;
  sts->fd = args.fd;
  sts->node = args.node;
  sts->cur = args.node;
  sts->malloc = args.malloc;
  sts->free = args.free;
  ml666_st_node_ref(sts->public.stb, sts->node);
  return &sts->public;
}

static bool ml666_st_ml666_serializer_next(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private*restrict sts = (struct ml666_st_serializer_private*)_sts;
  while(sts->state != SERIALIZER_W_DONE || sts->data.length || sts->outptr.length){
    if(sts->data.length || sts->outptr.length){
      struct ml666_buffer_ro buf = sts->data;
      struct ml666_buffer outptr = sts->outptr;
      if(sts->spaces){
        const size_t space_left = sts->outbuf.length - outptr.length;
        size_t l = sts->spaces;
        if(l > space_left)
          l = space_left;
        memset(&outptr.data[outptr.length], ' ', l);
        sts->spaces -= l;
        outptr.length += l;
      }
      if(!sts->spaces && buf.length){
        switch(sts->encoding){
          case ENC_RAW: {
            const size_t space_left = sts->outbuf.length - outptr.length;
            size_t l = buf.length;
            if(l > space_left)
              l = space_left;
            memcpy(&outptr.data[outptr.length], buf.data, l);
            outptr.length += l;
            buf.length -= l;
            buf.data += l;
          } break;
          case ENC_ESCAPED: {
            while(buf.length && sts->outbuf.length - outptr.length >= 6){
              unsigned char ch = *buf.data;
              if(ch >= 0x80){
                // Check utf-8, escape if not valid utf-8
                struct streaming_utf8_validator v = {0};
                unsigned length = 2 + (ch >= 0xE0) + (ch >= 0xF0) + (ch >= 0xF8) + (ch >= 0xFC);
                size_t n = length < buf.length ? length : buf.length;
                for(unsigned k=0; k<n; k++){
                  if(!utf8_validate(&v,buf.data[k])){
                    buf.length--;
                    buf.data++;
                    goto outhex;
                  }
                }
                if(!utf8_validate(&v,EOF)){
                  buf.length--;
                  buf.data++;
                  goto outhex;
                }
                buf.length -= n;
                while(n--)
                  outptr.data[outptr.length++] = *(buf.data++);
                continue;
              }
              buf.length--;
              buf.data++;
              if(ch && strchr(sts->esc_chars, ch)){
                switch(ch){
                  case '\n': ch = 'n'; break;
                  case '\t': ch = 't'; break;
                }
                outptr.data[outptr.length++] = '\\';
                outptr.data[outptr.length++] = ch;
              }else switch(ch){
                case '\a'  : outptr.data[outptr.length++] = '\\'; outptr.data[outptr.length++] = 'a'; break;
                case '\b'  : outptr.data[outptr.length++] = '\\'; outptr.data[outptr.length++] = 'b'; break;
                case '\x1B': outptr.data[outptr.length++] = '\\'; outptr.data[outptr.length++] = 'e'; break;
                case '\f'  : outptr.data[outptr.length++] = '\\'; outptr.data[outptr.length++] = 'f'; break;
                case '\v'  : outptr.data[outptr.length++] = '\\'; outptr.data[outptr.length++] = 'v'; break;
                default: {
                  if(ch >= ' ' || ch == '\t' || ch == '\n'){
                    outptr.data[outptr.length++] = ch;
                  }else goto outhex;
                } break;
              }
              if(0) outhex:{
                outptr.data[outptr.length++] = '\\';
                outptr.data[outptr.length++] = 'x';
                outptr.data[outptr.length++] = ML666_B36[ch >>  4];
                outptr.data[outptr.length++] = ML666_B36[ch & 0xF];
              }
            }
          } break;
          case ENC_BASE64: {
            unsigned lf_counter = sts->lf_counter;
            while(buf.length >= 3 && sts->outbuf.length - outptr.length >= 5){
              if(lf_counter == 80/4-2){
                outptr.data[outptr.length++] = '\n';
                lf_counter = 0;
              }
              const unsigned char*const data = (const unsigned char*)buf.data;
              outptr.data[outptr.length++] = ML666_B64[                          (data[0] >> 2)];
              outptr.data[outptr.length++] = ML666_B64[((data[0] & 0x03) << 4) | (data[1] >> 4)];
              outptr.data[outptr.length++] = ML666_B64[((data[1] & 0x0F) << 2) | (data[2] >> 6)];
              outptr.data[outptr.length++] = ML666_B64[  data[2] & 0x3F                        ];
              buf.data   += 3;
              buf.length -= 3;
              lf_counter += 1;
            }
            if(buf.length && buf.length < 3 && sts->outbuf.length - outptr.length >= 5){
              if(lf_counter == 80/4-2)
                outptr.data[outptr.length++] = '\n';
              switch(buf.length){
                case 1: {
                  const unsigned char*const data = (const unsigned char*)buf.data;
                  outptr.data[outptr.length++] = ML666_B64[                          (data[0] >> 2)];
                  outptr.data[outptr.length++] = ML666_B64[((data[0] & 0x03) << 4)                 ];
                  outptr.data[outptr.length++] = '=';
                  outptr.data[outptr.length++] = '=';
                } break;
                case 2: {
                  const unsigned char*const data = (const unsigned char*)buf.data;
                  outptr.data[outptr.length++] = ML666_B64[                          (data[0] >> 2)];
                  outptr.data[outptr.length++] = ML666_B64[((data[0] & 0x03) << 4) | (data[1] >> 4)];
                  outptr.data[outptr.length++] = ML666_B64[((data[1] & 0x0F) << 2)                 ];
                  outptr.data[outptr.length++] = '=';
                } break;
              }
              buf.data   = 0;
              buf.length = 0;
              lf_counter = 0;
            }
            sts->lf_counter = lf_counter;
          } break;
        }
      }
      if(!buf.length)
        sts->encoding = ENC_RAW;
      if(outptr.length){
        int res = write(sts->fd, outptr.data, outptr.length);
        if(res < 0){
          sts->data = buf;
          sts->outptr = outptr;
          sts->public.error = strerror(errno);
          return false;
        }
        outptr.length -= res;
        outptr.data   += res;
        if((size_t)(outptr.data - sts->outbuf.data) > sts->outbuf.length)
          outptr.data -= sts->outbuf.length;
      }
      sts->data = buf;
      sts->outptr = outptr;
    }
    if(!sts->data.length){
      (void)serializer_state_name;
//       printf("%s\n", serializer_state_name[sts->state]); fflush(stdout);
      switch(sts->state){
        case SERIALIZER_W_DONE: break;
        case SERIALIZER_W_START: switch(ML666_ST_TYPE(sts->cur)){
          case ML666_ST_NT_DOCUMENT: sts->state = SERIALIZER_W_NEXT_CHILD; break;
          case ML666_ST_NT_ELEMENT : sts->state = SERIALIZER_W_TAG_START; break;
          case ML666_ST_NT_CONTENT : sts->state = SERIALIZER_W_CONTENT_START; break;
          case ML666_ST_NT_COMMENT : sts->state = SERIALIZER_W_COMMENT_START; break;
        } break;
        case SERIALIZER_W_NEXT_CHILD: {
          struct ml666_st_children* children = ML666_ST_U_CHILDREN(sts->public.stb, sts->cur);
          struct ml666_st_member* first_child = children ? ml666_st_get_first_child(sts->public.stb, children) : 0;
          sts->state = SERIALIZER_W_DONE;
          if(first_child){
            if(ML666_ST_TYPE(sts->cur) == ML666_ST_NT_ELEMENT)
              sts->level += 1;
            sts->cur = ML666_ST_NODE(first_child);
            sts->state = SERIALIZER_W_START;
          }
        } break;
        case SERIALIZER_W_NEXT_MEMBER: {
          sts->state = SERIALIZER_W_DONE;
          if(sts->cur != sts->node){
            struct ml666_st_member* member = ML666_ST_U_MEMBER(sts->cur);
            struct ml666_st_member* next_child = member ? ml666_st_member_get_next(sts->public.stb, member) : 0;
            if(next_child){
              sts->cur = ML666_ST_NODE(next_child);
              sts->state = SERIALIZER_W_START;
            }else if(member){
              struct ml666_st_node* parent = ml666_st_member_get_parent(sts->public.stb, member);
              if(parent && parent != sts->node && ML666_ST_TYPE(parent) == ML666_ST_NT_ELEMENT){
                sts->cur = parent;
                sts->state = SERIALIZER_W_END_TAG_START;
                if(sts->level)
                  sts->level -= 1;
              }
            }
          }
        } break;
        case SERIALIZER_W_TAG_START: {
          sts->spaces = sts->level * 2;
          sts->data.data = "<";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_TAG;
        } break;
        case SERIALIZER_W_TAG: {
          sts->esc_chars = "/> \\\n\t";
          sts->encoding = ENC_ESCAPED;
          struct ml666_st_element* element = ML666_ST_U_ELEMENT(sts->cur);
          sts->data = ml666_hashed_buffer_set__peek(ml666_st_element_get_name(sts->public.stb, element))->buffer;
          if((sts->current_attribute = ml666_st_attribute_get_first(sts->public.stb, element))){
            sts->state = SERIALIZER_W_ATTRIBUTE;
          }else{
            sts->state = SERIALIZER_W_TAG_END;
          }
          //
        } break;
        case SERIALIZER_W_ATTRIBUTE: {
          sts->spaces = 1;
          sts->encoding = ENC_ESCAPED;
          sts->esc_chars = "=/> \\\n\t";
          sts->data = ml666_st_attribute_get_name(sts->public.stb, sts->current_attribute)->buffer;
          if(ml666_st_attribute_get_value(sts->public.stb, sts->current_attribute)){
            sts->state = SERIALIZER_W_ATTRIBUTE_VALUE_START;
          }else{
            sts->state = SERIALIZER_W_ATTRIBUTE_NEXT;
          }
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE_START: {
          sts->data.data = "=";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_CONTENT_START;
        } break;
        case SERIALIZER_W_ATTRIBUTE_NEXT: {
          if((sts->current_attribute = ml666_st_attribute_get_next(sts->public.stb, sts->current_attribute))){
            sts->state = SERIALIZER_W_ATTRIBUTE;
          }else{
            sts->state = SERIALIZER_W_TAG_END;
          }
        } break;
        case SERIALIZER_W_TAG_END: {
          if(ml666_st_get_first_child(sts->public.stb, ML666_ST_U_CHILDREN(sts->public.stb, sts->cur))){
            sts->data.data = ">\n";
            sts->data.length = 2;
            sts->state = SERIALIZER_W_NEXT_CHILD;
          }else{
            sts->data.data = "/>\n";
            sts->data.length = 3;
            sts->state = SERIALIZER_W_NEXT_MEMBER;
          }
        } break;
        case SERIALIZER_W_END_TAG_START: {
          sts->spaces = sts->level * 2;
          sts->data.data = "</";
          sts->data.length = 2;
          sts->state = SERIALIZER_W_END_TAG;
        } break;
        case SERIALIZER_W_END_TAG: {
          sts->esc_chars = "/> \\\n\t";
          sts->encoding = ENC_ESCAPED;
          sts->data = ml666_hashed_buffer_set__peek(ml666_st_element_get_name(sts->public.stb, ML666_ST_U_ELEMENT(sts->cur)))->buffer;
          sts->state = SERIALIZER_W_END_TAG_END;
        } break;
        case SERIALIZER_W_END_TAG_END: {
          sts->data.data = ">\n";
          sts->data.length = 2;
          sts->state = SERIALIZER_W_NEXT_MEMBER;
        } break;
        case SERIALIZER_W_CONTENT_START: {
          struct ml666_buffer_ro buf;
          if(sts->current_attribute){
            buf = *ml666_st_attribute_get_value(sts->public.stb, sts->current_attribute);
          }else{
            buf = ml666_st_content_get(sts->public.stb, ML666_ST_U_CONTENT(sts->cur));
            sts->spaces = sts->level * 2;
          }
          sts->buffer_info = ml666_buffer__analyze(buf);
          switch(sts->buffer_info.best_encoding){
            case ML666_BIBE_ESCAPE: {
              sts->data.data = "`\n";
              if(sts->buffer_info.really_multi_line){
                sts->data.length = 2;
              }else{
                sts->data.length = 1;
              }
            } break;
            case ML666_BIBE_BASE64: {
              sts->data.data = "B`\n";
              sts->buffer_info.really_multi_line = buf.length > 80/4*3-6;
              if(sts->buffer_info.really_multi_line){
                sts->data.length = 3;
              }else{
                sts->data.length = 2;
              }
            } break;
          }
          sts->state = SERIALIZER_W_CONTENT;
        } break;
        case SERIALIZER_W_CONTENT: {
          sts->esc_chars = "`\\";
          switch(sts->buffer_info.best_encoding){
            case ML666_BIBE_ESCAPE: sts->encoding = ENC_ESCAPED; break;
            case ML666_BIBE_BASE64: sts->encoding = ENC_BASE64; break;
          }
          if(sts->current_attribute){
            sts->data = *ml666_st_attribute_get_value(sts->public.stb, sts->current_attribute);
          }else{
            sts->data = ml666_st_content_get(sts->public.stb, ML666_ST_U_CONTENT(sts->cur));
          }
          sts->state = sts->buffer_info.really_multi_line ? SERIALIZER_W_CONTENT_END_1 : SERIALIZER_W_CONTENT_END_2;
        } break;
        case SERIALIZER_W_CONTENT_END_1: {
          sts->data.data = "\n";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_CONTENT_END_2;
        } break;
        case SERIALIZER_W_CONTENT_END_2: {
          sts->data.data = "`\n";
          if(sts->buffer_info.really_multi_line)
            sts->spaces = sts->level * 2;
          if(sts->current_attribute){
            sts->state = SERIALIZER_W_ATTRIBUTE_NEXT;
            sts->data.length = 1;
          }else{
            sts->state = SERIALIZER_W_NEXT_MEMBER;
            sts->data.length = 2;
          }
        } break;
        case SERIALIZER_W_COMMENT_START: {
          sts->buffer_info = ml666_buffer__analyze(ml666_st_comment_get(sts->public.stb, ML666_ST_U_COMMENT(sts->cur)));
          sts->spaces = sts->level * 2;
          sts->data.length = 3;
          if(sts->buffer_info.really_multi_line){
            sts->data.data = "/*\n";
          }else{
            sts->data.data = "/* ";
          }
          sts->state = SERIALIZER_W_COMMENT;
        } break;
        case SERIALIZER_W_COMMENT: {
          sts->esc_chars = "*\\";
          sts->encoding = ENC_ESCAPED;
          sts->data = ml666_st_comment_get(sts->public.stb, ML666_ST_U_COMMENT(sts->cur));
          sts->state = sts->buffer_info.really_multi_line ? SERIALIZER_W_COMMENT_END_1 : SERIALIZER_W_COMMENT_END_2;
        } break;
        case SERIALIZER_W_COMMENT_END_1: {
          sts->data.data = "\n";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_COMMENT_END_2;
        } break;
        case SERIALIZER_W_COMMENT_END_2: {
          sts->state = SERIALIZER_W_NEXT_MEMBER;
          if(sts->buffer_info.really_multi_line){
            sts->data.length = 3;
            sts->data.data = "*/\n";
            sts->spaces = sts->level * 2;
          }else{
            sts->data.length = 4;
            sts->data.data = " */\n";
          }
        } break;
      }
    }
  }
  return sts->state != SERIALIZER_W_DONE;
}

static void ml666_st_ml666_serializer_destroy(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private* sts = (struct ml666_st_serializer_private*)_sts;
  ml666_st_node_put(sts->public.stb, sts->node);
  munmap(sts->outbuf.data, sts->outbuf.length*2);
  sts->free(sts->public.user_ptr, sts);
}
