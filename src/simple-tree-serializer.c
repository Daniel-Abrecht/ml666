#include <ml666/simple-tree.h>
#include <ml666/simple-tree-serializer.h>
#include <ml666/utils.h>
#include <string.h>
#include <unistd.h>

enum serializer_state {
  SERIALIZER_W_START,
  SERIALIZER_W_DONE,
  SERIALIZER_W_NEXT_MEMBER,
  SERIALIZER_W_TAG_START,
  SERIALIZER_W_TAG,
  SERIALIZER_W_ATTRIBUTE,
  SERIALIZER_W_ATTRIBUTE_VALUE_START,
  SERIALIZER_W_ATTRIBUTE_VALUE,
  SERIALIZER_W_ATTRIBUTE_VALUE_END,
  SERIALIZER_W_END_TAG_START,
  SERIALIZER_W_END_TAG,
  SERIALIZER_W_END_TAG_END,
  SERIALIZER_W_CONTENT_START,
  SERIALIZER_W_CONTENT_TEXT,
  SERIALIZER_W_CONTENT_BASE64,
  SERIALIZER_W_CONTENT_END,
  SERIALIZER_W_COMMENT_START,
  SERIALIZER_W_COMMENT,
  SERIALIZER_W_COMMENT_END,
};

struct ml666_st_serializer_private {
  struct ml666_st_serializer public;
  struct ml666_st_node* node;

  struct ml666_st_node* cur;
  enum serializer_state state;
  struct ml666_buffer_ro data;

  struct ml666_buffer outbuf;

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
  sts->node = args.node;
  sts->cur = args.node;
  sts->malloc = args.malloc;
  sts->free = args.free;
  ml666_st_node_ref(sts->public.stb, sts->node);
  return &sts->public;
}

bool ml666_st_serializer_next(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private*restrict sts = (struct ml666_st_serializer_private*)_sts;
  sts->public.error = "TODO: This isn't fully implemented yet";
  return false;
  if(sts->data.length || sts->outbuf.length){
    struct ml666_buffer_ro buf = sts->data;
    struct ml666_buffer out = sts->outbuf;
    size_t i=0, j=0;
    while(i<buf.length && j<out.length-4){
      unsigned char ch = buf.data[i++];
      if(ch > ' '){
        out.data[j] = ch;
      }else{
        out.data[j++] = '\\';
        out.data[j++] = 'x';
        out.data[j++] = ch >> 4;
        out.data[j++] = ch & 0xFF;
      }
    }
  }else while(sts->state != SERIALIZER_W_DONE){
    switch(sts->state){
      case SERIALIZER_W_DONE: break;
      case SERIALIZER_W_START: switch(ML666_ST_TYPE(sts->cur)){
        case ML666_ST_NT_DOCUMENT: sts->state = SERIALIZER_W_NEXT_MEMBER; break;
        case ML666_ST_NT_ELEMENT : sts->state = SERIALIZER_W_TAG_START; break;
        case ML666_ST_NT_CONTENT : sts->state = SERIALIZER_W_CONTENT_START; break;
        case ML666_ST_NT_COMMENT : sts->state = SERIALIZER_W_COMMENT_START; break;
      } continue;
      case SERIALIZER_W_NEXT_MEMBER: {
        struct ml666_st_children* children = ML666_ST_U_CHILDREN(sts->public.stb, sts->cur);
        struct ml666_st_member* first_child = children ? ml666_st_get_first_child(sts->public.stb, children) : 0;
        sts->state = SERIALIZER_W_DONE;
        if(first_child){
          sts->cur = ML666_ST_NODE(first_child);
          sts->state = SERIALIZER_W_START;
        }else if(sts->cur != sts->node){
          struct ml666_st_member* member = ML666_ST_U_MEMBER(sts->cur);
          struct ml666_st_member* next_child = member ? ml666_st_member_get_next(sts->public.stb, member) : 0;
          if(next_child){
            sts->cur = ML666_ST_NODE(next_child);
            sts->state = SERIALIZER_W_START;
          }else if(member){
            struct ml666_st_node* parent = ml666_st_member_get_parent(sts->public.stb, member);
            if(parent && parent != sts->node){
              sts->cur = parent;
              sts->state = SERIALIZER_W_START;
            }
          }
        }
      } break;
      case SERIALIZER_W_TAG_START: {
      } break;
      case SERIALIZER_W_TAG: {
      } break;
      case SERIALIZER_W_ATTRIBUTE: {
      } break;
      case SERIALIZER_W_ATTRIBUTE_VALUE_START: {
      } break;
      case SERIALIZER_W_ATTRIBUTE_VALUE: {
      } break;
      case SERIALIZER_W_ATTRIBUTE_VALUE_END: {
      } break;
      case SERIALIZER_W_END_TAG_START: {
      } break;
      case SERIALIZER_W_END_TAG: {
      } break;
      case SERIALIZER_W_END_TAG_END: {
      } break;
      case SERIALIZER_W_CONTENT_START: {
      } break;
      case SERIALIZER_W_CONTENT_TEXT: {
      } break;
      case SERIALIZER_W_CONTENT_BASE64: {
      } break;
      case SERIALIZER_W_CONTENT_END: {
      } break;
      case SERIALIZER_W_COMMENT_START: {
      } break;
      case SERIALIZER_W_COMMENT: {
      } break;
      case SERIALIZER_W_COMMENT_END: {
      } break;
    }
    break;
  }
  return sts->state != SERIALIZER_W_DONE;
}

void ml666_st_serializer_destroy(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private* sts = (struct ml666_st_serializer_private*)_sts;
  ml666_st_node_put(sts->public.stb, sts->node);
  sts->free(sts->public.user_ptr, sts->outbuf.data);
  sts->free(sts->public.user_ptr, sts);
}
