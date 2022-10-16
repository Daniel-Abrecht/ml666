#define _GNU_SOURCE
#include <ml666/simple-tree.h>
#include <ml666/simple-tree-json-serializer.h>
#include <ml666/utils.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

enum serializer_state {
  SERIALIZER_W_START,
  SERIALIZER_W_DONE,
  SERIALIZER_W_FINAL_NEWLINE,
  SERIALIZER_W_NEXT_CHILD,
  SERIALIZER_W_NEXT_MEMBER,
  SERIALIZER_W_TAG_START,
  SERIALIZER_W_TAG,
  SERIALIZER_W_ATTRIBUTE_LIST_START,
  SERIALIZER_W_ATTRIBUTE_START,
  SERIALIZER_W_ATTRIBUTE_NAME,
  SERIALIZER_W_ATTRIBUTE_VALUE_START,
  SERIALIZER_W_ATTRIBUTE_VALUE,
  SERIALIZER_W_ATTRIBUTE_NEXT,
  SERIALIZER_W_ATTRIBUTE_LIST_END,
  SERIALIZER_W_CHILDREN_1,
  SERIALIZER_W_CHILDREN_2,
  SERIALIZER_W_END_CHILDLIST,
  SERIALIZER_W_END_CHILDLIST_2,
  SERIALIZER_W_CONTENT,
  SERIALIZER_W_COMMENT_START,
  SERIALIZER_W_COMMENT,
  SERIALIZER_W_COMMENT_END,
};

enum data_encoding {
  ENC_RAW,
  ENC_STRING,
  ENC_STRING_UTF8,
  ENC_STRING_UTF8_2,
  ENC_STRING_BASE64,
  ENC_STRING_BASE64_2,
};

struct ml666_st_serializer_private {
  struct ml666_st_serializer public;
  struct ml666_st_node* node;
  int fd;
  unsigned level, spaces;
  struct ml666_buffer_info buffer_info;

  struct ml666_st_node* cur;
  struct ml666_st_attribute* current_attribute;
  enum serializer_state state;

  enum data_encoding encoding;
  struct ml666_buffer_ro data;
  struct ml666_buffer outbuf;
  struct ml666_buffer outptr;

  bool has_attribute, has_children;

  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};

static ml666_st_serializer_cb_next ml666_st_json_serializer_next;
static ml666_st_serializer_cb_destroy ml666_st_json_serializer_destroy;

static const struct ml666_st_serializer_cb st_serializer_default = {
  .next = ml666_st_json_serializer_next,
  .destroy = ml666_st_json_serializer_destroy,
};

struct ml666_st_serializer* ml666_st_json_serializer_create_p(struct ml666_st_json_serializer_create_args args){
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

static bool ml666_st_json_serializer_next(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private*restrict sts = (struct ml666_st_serializer_private*)_sts;
  while(sts->state != SERIALIZER_W_DONE || sts->data.length || sts->outptr.length){
    if(sts->data.length || sts->outptr.length || sts->encoding != ENC_RAW){
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
          case ENC_STRING: {
            sts->buffer_info = ml666_buffer__analyze(buf);
            if(sts->buffer_info.is_valid_utf8){
              sts->encoding = ENC_STRING_UTF8;
            }else{
              sts->encoding = ENC_STRING_BASE64;
            }
            sts->outptr = outptr;
          } continue;
          case ENC_STRING_UTF8: {
            if(sts->outbuf.length - outptr.length < 1)
              break;
            outptr.data[outptr.length++] = '"';
            sts->encoding = ENC_STRING_UTF8_2;
          } /* fallthrough */
          case ENC_STRING_UTF8_2: {
            while(buf.length && sts->outbuf.length - outptr.length >= 6){
              unsigned char ch = *buf.data;
              if(ch >= 0x80){
                // Check utf-8, emmit the unicode replacement character if not valid utf-8
                struct ml666_streaming_utf8_validator v = {0};
                unsigned length = 2 + (ch >= 0xE0) + (ch >= 0xF0) + (ch >= 0xF8) + (ch >= 0xFC);
                size_t n = length < buf.length ? length : buf.length;
                for(unsigned k=0; k<n; k++){
                  if(!ml666_utf8_validate(&v,buf.data[k])){
                    buf.length -= k+1;
                    buf.data += k+1;
                    goto badseq;
                  }
                }
                if(!ml666_utf8_validate(&v,EOF)){
                  buf.length -= n;
                  buf.data += n;
                  goto badseq;
                }
                buf.length -= n;
                while(n--)
                  outptr.data[outptr.length++] = *(buf.data++);
                if(0) badseq: {
                  outptr.data[outptr.length++] = '\xEF';
                  outptr.data[outptr.length++] = '\xBF';
                  outptr.data[outptr.length++] = '\xBD';
                }
              }else{
                buf.length--;
                buf.data++;
                if(ch >= 0x20 && ch != '\\' && ch != '"'){
                  outptr.data[outptr.length++] = ch;
                }else{
                  outptr.data[outptr.length++] = '\\';
                  switch(ch){
                    case '"' : outptr.data[outptr.length++] = '"' ; break;
                    case '\\': outptr.data[outptr.length++] = '\\'; break;
                    case '\b': outptr.data[outptr.length++] = 'b' ; break;
                    case '\f': outptr.data[outptr.length++] = 'f' ; break;
                    case '\n': outptr.data[outptr.length++] = 'n' ; break;
                    case '\r': outptr.data[outptr.length++] = 'r' ; break;
                    case '\t': outptr.data[outptr.length++] = 't' ; break;
                    default: {
                      outptr.data[outptr.length++] = 'u';
                      outptr.data[outptr.length++] = '0';
                      outptr.data[outptr.length++] = '0';
                      outptr.data[outptr.length++] = ML666_B36[ch >>  4];
                      outptr.data[outptr.length++] = ML666_B36[ch & 0xF];
                    } break;
                  }
                }
              }
            }
            if(!buf.length && sts->outbuf.length - outptr.length >= 1){
              outptr.data[outptr.length++] = '"';
              sts->encoding = ENC_RAW;
            }
          } break;
          case ENC_STRING_BASE64: {
            if(sts->outbuf.length - outptr.length < 6)
              break;
            outptr.data[outptr.length++] = '[';
            outptr.data[outptr.length++] = '"';
            outptr.data[outptr.length++] = 'B';
            outptr.data[outptr.length++] = '"';
            outptr.data[outptr.length++] = ',';
            outptr.data[outptr.length++] = '"';
            sts->outptr = outptr;
            sts->encoding = ENC_STRING_UTF8_2;
          } /* fallthrough */
          case ENC_STRING_BASE64_2: {
            while(buf.length >= 3 && sts->outbuf.length - outptr.length >= 5){
              const unsigned char*const data = (const unsigned char*)buf.data;
              outptr.data[outptr.length++] = ML666_B64[                          (data[0] >> 2)];
              outptr.data[outptr.length++] = ML666_B64[((data[0] & 0x03) << 4) | (data[1] >> 4)];
              outptr.data[outptr.length++] = ML666_B64[((data[1] & 0x0F) << 2) | (data[2] >> 6)];
              outptr.data[outptr.length++] = ML666_B64[  data[2] & 0x3F                        ];
              buf.data   += 3;
              buf.length -= 3;
            }
            if(buf.length && buf.length < 3 && sts->outbuf.length - outptr.length >= 5){
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
            }
            if(!buf.length && sts->outbuf.length - outptr.length >= 2){
              outptr.data[outptr.length++] = '"';
              outptr.data[outptr.length++] = ']';
              sts->encoding = ENC_RAW;
            }
          } break;
        }
      }
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
      switch(sts->state){
        case SERIALIZER_W_DONE: break;
        case SERIALIZER_W_START: switch(ML666_ST_TYPE(sts->cur)){
          case ML666_ST_NT_DOCUMENT: sts->state = SERIALIZER_W_TAG_START; break;
          case ML666_ST_NT_ELEMENT : sts->state = SERIALIZER_W_TAG_START; break;
          case ML666_ST_NT_CONTENT : sts->state = SERIALIZER_W_CONTENT; break;
          case ML666_ST_NT_COMMENT : sts->state = SERIALIZER_W_COMMENT_START; break;
        } break;
        case SERIALIZER_W_NEXT_CHILD: {
          struct ml666_st_children* children = ML666_ST_U_CHILDREN(sts->public.stb, sts->cur);
          struct ml666_st_member* first_child = children ? ml666_st_get_first_child(sts->public.stb, children) : 0;
          if(first_child){
            sts->cur = ML666_ST_NODE(first_child);
            sts->state = SERIALIZER_W_START;
          }else{
            sts->state = SERIALIZER_W_END_CHILDLIST;
          }
        } break;
        case SERIALIZER_W_NEXT_MEMBER: {
          sts->state = SERIALIZER_W_FINAL_NEWLINE;
          if(sts->cur == sts->node)
            break;
          struct ml666_st_member* member = ML666_ST_U_MEMBER(sts->cur);
          struct ml666_st_member* next_child = member ? ml666_st_member_get_next(sts->public.stb, member) : 0;
          if(next_child){
            sts->data.data = ", \n";
            sts->data.length = 3;
            sts->cur = ML666_ST_NODE(next_child);
            sts->state = SERIALIZER_W_START;
          }else if(member){
            struct ml666_st_node* parent = ml666_st_member_get_parent(sts->public.stb, member);
            if(parent){
              sts->cur = parent;
              sts->state = SERIALIZER_W_END_CHILDLIST;
            }
          }
        } break;
        case SERIALIZER_W_FINAL_NEWLINE: {
          sts->data.data = "\n";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_DONE;
        } break;
        case SERIALIZER_W_TAG_START: {
          sts->spaces = sts->level * 2;
          if(ML666_ST_TYPE(sts->cur) == ML666_ST_NT_DOCUMENT){
            sts->data.data = "[\"D\"";
            sts->data.length = 4;
            sts->state = SERIALIZER_W_CHILDREN_1;
          }else{
            sts->data.data = "[\"E\", ";
            sts->data.length = 6;
            sts->state = SERIALIZER_W_TAG;
          }
        } break;
        case SERIALIZER_W_TAG: {
          sts->encoding = ENC_STRING;
          struct ml666_st_element* element = ML666_ST_U_ELEMENT(sts->cur);
          sts->current_attribute = ml666_st_attribute_get_first(sts->public.stb, element);
          sts->has_attribute = !!sts->current_attribute;
          sts->has_children = !!ml666_st_get_first_child(sts->public.stb, ML666_ST_U_CHILDREN(sts->public.stb, sts->cur));
          sts->data = ml666_hashed_buffer_set__peek(ml666_st_element_get_name(sts->public.stb, element))->buffer;
          sts->state = SERIALIZER_W_ATTRIBUTE_LIST_START;
          if(sts->has_children && sts->has_attribute)
            if(!++sts->level)
              sts->level = ~0;
        } break;
        case SERIALIZER_W_ATTRIBUTE_LIST_START: {
          if(sts->current_attribute){
            sts->data.data = ", [\n";
            sts->state = SERIALIZER_W_ATTRIBUTE_START;
            if(!++sts->level)
              sts->level = ~0;
          }else{
            sts->state = SERIALIZER_W_CHILDREN_1;
            sts->data.data = ", []";
          }
          sts->data.length = 4;
        } break;
        case SERIALIZER_W_ATTRIBUTE_START: {
          sts->state = SERIALIZER_W_ATTRIBUTE_NAME;
          sts->data.data = "[";
          sts->data.length = 1;
          sts->spaces = sts->level * 2;
        } break;
        case SERIALIZER_W_ATTRIBUTE_NAME: {
          sts->encoding = ENC_STRING_UTF8;
          sts->data = ml666_st_attribute_get_name(sts->public.stb, sts->current_attribute)->buffer;
          sts->state = SERIALIZER_W_ATTRIBUTE_VALUE_START;
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE_START: {
          sts->data.data = ", ";
          sts->data.length = 2;
          sts->state = SERIALIZER_W_ATTRIBUTE_VALUE;
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE: {
          sts->state = SERIALIZER_W_ATTRIBUTE_NEXT;
          const struct ml666_buffer_ro* buf = ml666_st_attribute_get_value(sts->public.stb, sts->current_attribute);
          if(!buf){
            sts->data.data = "null";
            sts->data.length = 4;
          }else{
            sts->encoding = ENC_STRING;
            sts->data = *buf;
          }
        } break;
        case SERIALIZER_W_ATTRIBUTE_NEXT: {
          if((sts->current_attribute = ml666_st_attribute_get_next(sts->public.stb, sts->current_attribute))){
            sts->state = SERIALIZER_W_ATTRIBUTE_START;
            sts->data.data = "],\n";
            sts->data.length = 3;
          }else{
            sts->data.data = "]\n";
            sts->data.length = 2;
            sts->state = SERIALIZER_W_ATTRIBUTE_LIST_END;
          }
        } break;
        case SERIALIZER_W_ATTRIBUTE_LIST_END: {
          if(sts->level)
            sts->level--;
          sts->spaces = sts->level * 2;
          sts->data.data = "]";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_CHILDREN_1;
        } break;
        case SERIALIZER_W_CHILDREN_1: {
          if(ml666_st_get_first_child(sts->public.stb, ML666_ST_U_CHILDREN(sts->public.stb, sts->cur))){
            sts->state = SERIALIZER_W_CHILDREN_2;
            sts->data.data = ", [\n";
            sts->data.length = 4;
            if(!++sts->level)
              sts->level = ~0;
          }else{
            sts->state = SERIALIZER_W_NEXT_MEMBER;
            sts->data.data = ", []]";
            sts->data.length = 5;
            if(sts->has_children && sts->has_attribute)
              if(sts->level)
                sts->level--;
          }
        } break;
        case SERIALIZER_W_CHILDREN_2: {
          sts->spaces = sts->level * 2;
          sts->state = SERIALIZER_W_NEXT_CHILD;
        } break;
        case SERIALIZER_W_END_CHILDLIST: {
          sts->data.data = "\n";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_END_CHILDLIST_2;
        } break;
        case SERIALIZER_W_END_CHILDLIST_2: {
          if(sts->has_children && sts->has_attribute)
            if(sts->level)
              sts->level--;
          if(sts->level)
            sts->level--;
          sts->spaces = sts->level * 2;
          sts->data.data = "]]";
          sts->data.length = 2;
          sts->state = SERIALIZER_W_NEXT_MEMBER;
        } break;
        case SERIALIZER_W_CONTENT: {
          sts->spaces = sts->level * 2;
          sts->encoding = ENC_STRING;
          sts->data = ml666_st_content_get(sts->public.stb, ML666_ST_U_CONTENT(sts->cur));
          sts->state = SERIALIZER_W_NEXT_MEMBER;
        } break;
        case SERIALIZER_W_COMMENT_START: {
          sts->spaces = sts->level * 2;
          sts->data.length = 6;
          sts->data.data = "[\"C\", ";
          sts->state = SERIALIZER_W_COMMENT;
        } break;
        case SERIALIZER_W_COMMENT: {
          sts->encoding = ENC_STRING;
          sts->data = ml666_st_comment_get(sts->public.stb, ML666_ST_U_COMMENT(sts->cur));
          sts->state = SERIALIZER_W_COMMENT_END;
        } break;
        case SERIALIZER_W_COMMENT_END: {
          sts->data.length = 1;
          sts->data.data = "]";
          sts->state = SERIALIZER_W_NEXT_MEMBER;
        } break;
      }
    }
  }
  return sts->state != SERIALIZER_W_DONE;
}

static void ml666_st_json_serializer_destroy(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private* sts = (struct ml666_st_serializer_private*)_sts;
  ml666_st_node_put(sts->public.stb, sts->node);
  munmap(sts->outbuf.data, sts->outbuf.length*2);
  sts->free(sts->public.user_ptr, sts);
}
