#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree.h>
#include <ml666/parser.h>
#include <ml666/utils.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

struct ml666_simple_tree_parser_default {
  struct ml666_simple_tree_parser public;
  struct ml666_parser* parser;
  struct ml666_st_document* document;
  struct ml666_st_node* cur;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
};

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur){
    printf("ml666_parser::tag_push: invalid parser state\n");
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_element* element = ml666_st_element_create(stp->public.stb, &entry, copy_name);
  if(!copy_name)
    *name = 0;
  if(!element){
    parser->error = "ml666_parser::tag_push: ml666_st_element_create\n";
    return false;
  }
  if(!ml666_st_member_set(stp->public.stb, stp->cur, ML666_ST_MEMBER(element), 0)){
    parser->error = "ml666_parser::tag_push: ml666_st_set failed";
    ml666_st_node_put(stp->public.stb, ML666_ST_NODE(element));
    return false;
  }
  ml666_st_node_put(stp->public.stb, ML666_ST_NODE(element));
  return ml666_st_node_ref_set(stp->public.stb, &stp->cur, ML666_ST_NODE(element));
}

static bool end_tag_check(struct ml666_parser* parser, ml666_opaque_tag_name name){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur)
    return false;
  struct ml666_st_element* element = ML666_ST_U_ELEMENT(stp->cur);
  if(!element)
    return false;
  const struct ml666_hashed_buffer* cur_name = ml666_hashed_buffer_set__peek(ml666_st_element_get_name(stp->public.stb, element));
  if( cur_name->buffer.length == name->buffer.length
   && memcmp(name->buffer.data, cur_name->buffer.data, name->buffer.length) == 0
  ) return true;
  return false;
}

static bool tag_pop(struct ml666_parser* parser){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur)
    return false;
  struct ml666_st_element* element = ML666_ST_U_ELEMENT(stp->cur);
  if(!element)
    return false;
  struct ml666_st_node* parent = ml666_st_member_get_parent(stp->public.stb, ML666_ST_MEMBER(element));
  if(!parent)
    return false;
  return ml666_st_node_ref_set(stp->public.stb, &stp->cur, parent);
}

static const struct ml666_parser_cb callbacks = {
  .tag_name_append       = ml666_parser__d_mal__tag_name_append,
  .tag_name_free         = ml666_parser__d_mal__tag_name_free,
  .attribute_name_append = ml666_parser__d_mal__attribute_name_append,
  .attribute_name_free   = ml666_parser__d_mal__attribute_name_free,

  .tag_push = tag_push,
  .end_tag_check = end_tag_check,
  .tag_pop = tag_pop,
};

struct ml666_simple_tree_parser* ml666_simple_tree_parser_create_p(struct ml666_simple_tree_parser_create_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666_simple_tree_parser_default* stp = args.malloc(args.user_ptr, sizeof(*stp));
  if(!stp){
    stp->parser->error = "calloc failed";
    return false;
  }
  memset(stp, 0, sizeof(*stp));
  *(struct ml666_st_builder**)&stp->public.stb = args.stb;
  stp->public.user_ptr = args.user_ptr;
  stp->malloc = args.malloc;
  stp->free = args.free;
  stp->parser = ml666_parser_create( .fd = 0, .cb = &callbacks );
  if(!stp->parser){
    fprintf(stderr, "ml666_parser_create failed");
    return false;
  }
  stp->parser->user_ptr = stp;
  struct ml666_st_document* document = ml666_st_document_create(stp->public.stb);
  if(!document){
    args.free(args.user_ptr, stp);
    fprintf(stderr, "ml666_st_document_create failed");
    return false;
  }
  stp->document = document;
  ml666_st_node_ref_set(stp->public.stb, &stp->cur, ML666_ST_NODE(document));
  return &stp->public;
}

void ml666_simple_tree_parser_destroy(struct ml666_simple_tree_parser* _stp){
  struct ml666_simple_tree_parser_default* stp = (struct ml666_simple_tree_parser_default*)_stp;
  ml666_st_node_ref_set(stp->public.stb, &stp->cur, 0);
  if(stp->document){
    ml666_st_subtree_disintegrate(stp->public.stb, ML666_ST_CHILDREN(stp->public.stb, stp->document));
    ml666_st_node_put(stp->public.stb, ML666_ST_NODE(stp->document));
    stp->document = 0;
  }
  if(stp->parser)
    ml666_parser_destroy(stp->parser);
  stp->free(stp->public.user_ptr, stp);
}

bool ml666_simple_tree_parser_next(struct ml666_simple_tree_parser* _stp){
  struct ml666_simple_tree_parser_default* stp = (struct ml666_simple_tree_parser_default*)_stp;
  if(!stp->parser)
    return false;
  bool res = ml666_parser_next(stp->parser);
  if(stp->parser->error)
    stp->public.error = stp->parser->error;
  return res;
}


struct ml666_st_document* ml666_simple_tree_parser_take_document(struct ml666_simple_tree_parser* _stp){
  struct ml666_simple_tree_parser_default* stp = (struct ml666_simple_tree_parser_default*)_stp;
  struct ml666_st_document* document = stp->document;
  stp->document = 0;
  return document;
}
