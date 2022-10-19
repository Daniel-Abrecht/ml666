#define _GNU_SOURCE
#include <ml666/simple-tree.h>
#include <ml666/simple-tree-binary-serializer.h>
#include <ml666/utils.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

struct ml666_st_serializer_private {
  struct ml666_st_serializer public;
  struct ml666_st_node* node;
  int fd;
  struct ml666_st_member* cur;
  struct ml666_buffer_ro buf;
  bool recursive;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};

static ml666_st_serializer_cb_next ml666_st_binary_serializer_next;
static ml666_st_serializer_cb_destroy ml666_st_binary_serializer_destroy;

static const struct ml666_st_serializer_cb st_serializer_default = {
  .next = ml666_st_binary_serializer_next,
  .destroy = ml666_st_binary_serializer_destroy,
};

struct ml666_st_serializer* ml666_st_binary_serializer_create_p(struct ml666_st_binary_serializer_create_args args){
  if(!args.stb){
    fprintf(stderr, "ml666_st_binary_serializer_create_p: mandatory argument \"stb\" not set!\n");
    return false;
  }
  if(!args.node){
    fprintf(stderr, "ml666_st_binary_serializer_create_p: mandatory argument \"node\" not set!\n");
    return false;
  }
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666_st_member* first_child = 0;
  if(ML666_ST_TYPE(args.node) == ML666_ST_NT_DOCUMENT){
    struct ml666_st_children* children = ML666_ST_U_CHILDREN(args.stb, args.node);
    if(children && ml666_st_get_first_child(args.stb, children) == ml666_st_get_last_child(args.stb, children))
      args.node = ML666_ST_NODE(ml666_st_get_first_child(args.stb, children));
  }
  if(ML666_ST_TYPE(args.node) == ML666_ST_NT_DOCUMENT || ML666_ST_TYPE(args.node) == ML666_ST_NT_ELEMENT){
    struct ml666_st_children* children = ML666_ST_U_CHILDREN(args.stb, args.node);
    if(children)
      first_child = ml666_st_get_first_child(args.stb, children);
  }else{
    first_child = ML666_ST_U_MEMBER(args.node);
  }
  struct ml666_st_serializer_private* sts = args.malloc(args.user_ptr, sizeof(*sts));
  if(!sts){
    fprintf(stderr, "malloc failed");
    return 0;
  }
  memset(sts, 0, sizeof(*sts));
  *(const struct ml666_st_serializer_cb**)&sts->public.cb = &st_serializer_default;
  *(struct ml666_st_builder**)&sts->public.stb = args.stb;
  sts->public.user_ptr = args.user_ptr;
  sts->fd = args.fd;
  sts->node = args.node;
  sts->cur = first_child;
  sts->recursive = args.recursive;
  sts->malloc = args.malloc;
  sts->free = args.free;
  ml666_st_node_ref(sts->public.stb, sts->node);
  return &sts->public;
}

static inline struct ml666_st_member* next(struct ml666_st_builder* stb, struct ml666_st_node* top, struct ml666_st_member* member, bool recursive){
  if(ML666_ST_NODE(member) == top)
    return 0;
  if(recursive){
    struct ml666_st_member* first_child = ml666_st_get_first_child(stb, ML666_ST_U_CHILDREN(stb, member));
    if(first_child)
      return first_child;
  }
  struct ml666_st_member* n = ml666_st_member_get_next(stb, member);
  if(n) return n;
  if(recursive){
    struct ml666_st_node* m = ML666_ST_NODE(member);
    while(ML666_ST_U_MEMBER(m)){
      m = ml666_st_member_get_parent(stb, ML666_ST_U_MEMBER(m));
      if(!m || m == top || !ML666_ST_U_MEMBER(m))
        return 0;
      struct ml666_st_member* m2 = ml666_st_member_get_next(stb, ML666_ST_U_MEMBER(m));
      if(m2)
        return m2;

    }
  }
  return 0;
}

static bool ml666_st_binary_serializer_next(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private*restrict const sts = (struct ml666_st_serializer_private*)_sts;
  if(sts->fd == -1)
    return false;
  while(sts->cur || sts->buf.length){
    if(sts->buf.length){
      ssize_t res = write(sts->fd, sts->buf.data, sts->buf.length);
      if(res < 0){
        if(errno == EINTR)
          continue;
        break;
      }
      sts->buf.data   += res;
      sts->buf.length -= res;
      return true;
    }else{
      struct ml666_st_content* content = ML666_ST_U_CONTENT(sts->cur);
      if(content)
        sts->buf = ml666_st_content_get(sts->public.stb, content);
      sts->cur = next(sts->public.stb, sts->node, sts->cur, sts->recursive);
    }
  }
  close(sts->fd);
  sts->fd = -1;
  return false;
}

static void ml666_st_binary_serializer_destroy(struct ml666_st_serializer* _sts){
  struct ml666_st_serializer_private* sts = (struct ml666_st_serializer_private*)_sts;
  ml666_st_node_put(sts->public.stb, sts->node);
  sts->free(sts->public.user_ptr, sts);
}
