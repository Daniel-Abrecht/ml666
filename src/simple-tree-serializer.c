#include <ml666/simple-tree.h>
#include <ml666/simple-tree-serializer.h>
#include <ml666/utils.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SERIALIZER_STATE \
  X(SERIALIZER_W_START) \
  X(SERIALIZER_W_DONE) \
  X(SERIALIZER_W_NEXT_CHILD) \
  X(SERIALIZER_W_NEXT_MEMBER) \
  X(SERIALIZER_W_TAG_START) \
  X(SERIALIZER_W_TAG) \
  X(SERIALIZER_W_ATTRIBUTE) \
  X(SERIALIZER_W_ATTRIBUTE_VALUE_START) \
  X(SERIALIZER_W_ATTRIBUTE_VALUE) \
  X(SERIALIZER_W_ATTRIBUTE_VALUE_END) \
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
  unsigned level, spaces;
  const char* esc_chars;
  struct ml666_buffer_info buffer_info;

  struct ml666_st_node* cur;
  enum serializer_state state;

  enum data_encoding encoding;
  struct ml666_buffer_ro data;
  struct ml666_buffer outbuf;
  struct ml666_buffer_ro outptr;

  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};

struct ml666_st_serializer* ml666_st_serializer_create_p(struct ml666_st_serializer_create_args args){
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
  unsigned outbuf_length = sysconf(_SC_PAGESIZE);
  struct ml666_buffer outbuf = {
    .data = args.malloc(args.user_ptr, outbuf_length),
    .length = outbuf_length,
  };
  if(!outbuf.data){
    fprintf(stderr, "malloc failed");
    return false;
  }
  struct ml666_st_serializer_private* sts = args.malloc(args.user_ptr, sizeof(*sts));
  if(!sts){
    fprintf(stderr, "malloc failed");
    args.free(args.user_ptr, outbuf.data);
    return false;
  }
  memset(sts, 0, sizeof(*sts));
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

bool ml666_st_serializer_next(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private*restrict sts = (struct ml666_st_serializer_private*)_sts;
  while(sts->state != SERIALIZER_W_DONE || sts->data.length || sts->outptr.length){
    if(sts->data.length || sts->outptr.length){
      struct ml666_buffer_ro buf = sts->data;
      struct ml666_buffer outbuf = sts->outbuf;
      size_t i=0, j=sts->outptr.data-outbuf.data+sts->outptr.length;
      if(sts->spaces){
        size_t l = sts->spaces;
        if(l > outbuf.length-j)
          l = outbuf.length-j;
        memset(outbuf.data+j, ' ', l);
        sts->spaces -= l;
        j += l;
      }
      if(!sts->spaces && buf.length){
        switch(sts->encoding){
          case ENC_RAW: {
            size_t l = buf.length;
            if(l > outbuf.length-j)
              l = outbuf.length-j;
            memcpy(outbuf.data+j, buf.data, l);
            i += l;
            j += l;
          } break;
          case ENC_ESCAPED: {
            while(i<buf.length && j<outbuf.length-6){
              unsigned char ch = buf.data[i];
              if(ch >= 0x80){
                // Check utf-8, escape if not valid utf-8
                struct streaming_utf8_validator v = {0};
                unsigned length = 2 + (ch >= 0xE0) + (ch >= 0xF0) + (ch >= 0xF8) + (ch >= 0xFC);
                size_t n = i+length < buf.length ? i+length : buf.length;
                for(unsigned k=i; k<n; k++){
                  if(!utf8_validate(&v,buf.data[k])){
                    i += 1;
                    goto outhex;
                  }
                }
                if(!utf8_validate(&v,EOF)){
                  i += 1;
                  goto outhex;
                }
                while(i<n)
                  outbuf.data[j++] = buf.data[i++];
                continue;
              }
              i += 1;
              if(ch && strchr(sts->esc_chars, ch)){
                switch(ch){
                  case '\n': ch = 'n'; break;
                  case '\t': ch = 't'; break;
                }
                outbuf.data[j++] = '\\';
                outbuf.data[j++] = ch;
              }else switch(ch){
                case '\a'  : outbuf.data[j++] = '\\'; outbuf.data[j++] = 'a'; break;
                case '\b'  : outbuf.data[j++] = '\\'; outbuf.data[j++] = 'b'; break;
                case '\x1B': outbuf.data[j++] = '\\'; outbuf.data[j++] = 'e'; break;
                case '\f'  : outbuf.data[j++] = '\\'; outbuf.data[j++] = 'f'; break;
                case '\v'  : outbuf.data[j++] = '\\'; outbuf.data[j++] = 'v'; break;
                default: {
                  if(ch >= ' ' || ch == '\t' || ch == '\n'){
                    outbuf.data[j++] = ch;
                  }else goto outhex;
                } break;
              }
              if(0) outhex:{
                outbuf.data[j++] = '\\';
                outbuf.data[j++] = 'x';
                outbuf.data[j++] = ML666_B36[ch >>  4];
                outbuf.data[j++] = ML666_B36[ch & 0xF];
              }
            }
          } break;
          case ENC_BASE64: {
            size_t len = buf.length - (buf.length % 3);
            unsigned char* data = (unsigned char*)buf.data;
            while(i<len && j<outbuf.length-5){
              if(i && i % (80/4*3-6) == 0)
                outbuf.data[j++] = '\n';
              outbuf.data[j++] = ML666_B64[                            (data[i+0] >> 2)];
              outbuf.data[j++] = ML666_B64[((data[i+0] & 0x03) << 4) | (data[i+1] >> 4)];
              outbuf.data[j++] = ML666_B64[((data[i+1] & 0x0F) << 2) | (data[i+2] >> 6)];
              outbuf.data[j++] = ML666_B64[  data[i+2] & 0x3F                          ];
              i += 3;
            }
            len = buf.length - i;
            if(len && len < 3 && j<outbuf.length-5){
              if(i && i % (80/4*3-6) == 0)
                outbuf.data[j++] = '\n';
              switch(len){
                case 1: {
                  outbuf.data[j++] = ML666_B64[                            (data[i+0] >> 2)];
                  outbuf.data[j++] = ML666_B64[((data[i+0] & 0x03) << 4)                   ];
                  outbuf.data[j++] = '=';
                  outbuf.data[j++] = '=';
                  i += 1;
                } break;
                case 2: {
                  outbuf.data[j++] = ML666_B64[                            (data[i+0] >> 2)];
                  outbuf.data[j++] = ML666_B64[((data[i+0] & 0x03) << 4) | (data[i+1] >> 4)];
                  outbuf.data[j++] = ML666_B64[((data[i+1] & 0x0F) << 2)                   ];
                  outbuf.data[j++] = '=';
                  i += 2;
                } break;
              }
            }
          } break;
        }
      }
      sts->data.length = buf.length - i;
      sts->data.data = buf.data + i;
      sts->outptr.length += j;
      if(!sts->data.length)
        sts->encoding = ENC_RAW;
      if(sts->outptr.length){
        int res = write(sts->fd, sts->outptr.data, sts->outptr.length);
        if(res < 0){
          sts->public.error = strerror(errno);
          return false;
        }
        sts->outptr.length -= res;
        sts->outptr.data   += res;
        if(!sts->outptr.length){
          sts->outptr.data = sts->outbuf.data;
        }else break;
      }
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
          sts->data = ml666_hashed_buffer_set__peek(ml666_st_element_get_name(sts->public.stb, ML666_ST_U_ELEMENT(sts->cur)))->buffer;
          sts->state = SERIALIZER_W_TAG_END;
        } break;
        case SERIALIZER_W_ATTRIBUTE: {
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE_START: {
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE: {
        } break;
        case SERIALIZER_W_ATTRIBUTE_VALUE_END: {
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
          struct ml666_buffer_ro buf = ml666_st_content_get(sts->public.stb, ML666_ST_U_CONTENT(sts->cur)).ro;
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
          sts->spaces = sts->level * 2;
          sts->state = SERIALIZER_W_CONTENT;
        } break;
        case SERIALIZER_W_CONTENT: {
          // TODO: Consider base64 encoding if overhead is lower
          sts->esc_chars = "`\\";
          switch(sts->buffer_info.best_encoding){
            case ML666_BIBE_ESCAPE: sts->encoding = ENC_ESCAPED; break;
            case ML666_BIBE_BASE64: sts->encoding = ENC_BASE64; break;
          }
          sts->data = ml666_st_content_get(sts->public.stb, ML666_ST_U_CONTENT(sts->cur)).ro;
          sts->state = sts->buffer_info.really_multi_line ? SERIALIZER_W_CONTENT_END_1 : SERIALIZER_W_CONTENT_END_2;
        } break;
        case SERIALIZER_W_CONTENT_END_1: {
          sts->data.data = "\n";
          sts->data.length = 1;
          sts->state = SERIALIZER_W_CONTENT_END_2;
        } break;
        case SERIALIZER_W_CONTENT_END_2: {
          // TODO: Check if content has any newlines, or if it should be on a single line
          sts->data.data = "`\n";
          sts->data.length = 2;
          if(sts->buffer_info.really_multi_line)
            sts->spaces = sts->level * 2;
          sts->state = SERIALIZER_W_NEXT_MEMBER;
        } break;
        case SERIALIZER_W_COMMENT_START: {
          sts->buffer_info = ml666_buffer__analyze(ml666_st_comment_get(sts->public.stb, ML666_ST_U_COMMENT(sts->cur)).ro);
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
          sts->data = ml666_st_comment_get(sts->public.stb, ML666_ST_U_COMMENT(sts->cur)).ro;
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

void ml666_st_serializer_destroy(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private* sts = (struct ml666_st_serializer_private*)_sts;
  ml666_st_node_put(sts->public.stb, sts->node);
  sts->free(sts->public.user_ptr, sts->outbuf.data);
  sts->free(sts->public.user_ptr, sts);
}
