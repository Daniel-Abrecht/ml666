#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree.h>
#include <ml666/parser.h>
#include <ml666/utils.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

struct ml666_simple_tree_parser {
  const struct ml666_st_cb* cb;
  struct ml666_parser* parser;
  struct ml666_st_document* document;
  struct ml666_st_node* cur;
  const char* error;
};

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_parser* stp = parser->user_ptr;
  if(!stp->cur){
    printf("ml666_parser::tag_push: invalid parser state\n");
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_element* element = stp->cb->element_create(&entry, copy_name);
  if(!copy_name)
    *name = 0;
  if(!element){
    parser->error = "ml666_parser::tag_push: ml666_st_element_create\n";
    return false;
  }
  if(!stp->cb->member_set(stp->cur, ML666_ST_MEMBER(element), 0)){
    parser->error = "ml666_parser::tag_push: ml666_st_set failed";
    stp->cb->node_put(ML666_ST_NODE(element));
    return false;
  }
  stp->cb->node_put(ML666_ST_NODE(element));
  return stp->cb->node_ref_set(&stp->cur, ML666_ST_NODE(element));
}

static bool end_tag_check(struct ml666_parser* parser, ml666_opaque_tag_name name){
  struct ml666_simple_tree_parser* stp = parser->user_ptr;
  if(!stp->cur || stp->cb->node_get_type(stp->cur) != ML666_ST_NT_ELEMENT)
    return false;
  const struct ml666_hashed_buffer* cur_name = ml666_hashed_buffer_set__peek(stp->cb->element_get_name((struct ml666_st_element*)stp->cur));
  if( cur_name->buffer.length == name->buffer.length
   && memcmp(name->buffer.data, cur_name->buffer.data, name->buffer.length) == 0
  ) return true;
  return false;
}

static bool tag_pop(struct ml666_parser* parser){
  struct ml666_simple_tree_parser* stp = parser->user_ptr;
  if(!stp->cur || stp->cb->node_get_type(stp->cur) != ML666_ST_NT_ELEMENT)
    return false;
  struct ml666_st_member* member = (struct ml666_st_member*)stp->cur;
  struct ml666_st_node* parent = stp->cb->member_get_parent(member);
  if(!parent)
    return false;
  return stp->cb->node_ref_set(&stp->cur, parent);
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
  struct ml666_simple_tree_parser* stp = calloc(1, sizeof(*stp));
  if(!stp){
    stp->parser->error = "calloc failed";
    return false;
  }
  stp->cb = args.cb;
  stp->parser = ml666_parser_create( .fd = 0, .cb = &callbacks );
  if(!stp->parser){
    fprintf(stderr, "ml666_parser_create failed");
    return false;
  }
  stp->parser->user_ptr = stp;
  struct ml666_st_document* document = stp->cb->document_create();
  if(!document){
    free(stp);
    fprintf(stderr, "ml666_st_document_create failed");
    return false;
  }
  stp->document = document;
  stp->cb->node_ref_set(&stp->cur, ML666_ST_NODE(document));
  return stp;
}

void ml666_simple_tree_parser_destroy(struct ml666_simple_tree_parser* stp){
  stp->cb->node_ref_set(&stp->cur, 0);
  if(stp->document){
    stp->cb->subtree_disintegrate(stp->cb->document_get_children(stp->document));
    stp->cb->node_put(ML666_ST_NODE(stp->document));
    stp->document = 0;
  }
  if(stp->parser)
    ml666_parser_destroy(stp->parser);
  free(stp);
}

bool ml666_simple_tree_parser_next(struct ml666_simple_tree_parser* stp){
  if(!stp->parser)
    return false;
  bool res = ml666_parser_next(stp->parser);
  if(stp->parser->error)
    stp->error = stp->parser->error;
  return res;
}

const char* ml666_simple_tree_parser_get_error(struct ml666_simple_tree_parser* stp){
  return stp->error;
}


struct ml666_st_document* ml666_simple_tree_parser_take_document(struct ml666_simple_tree_parser* stp){
  struct ml666_st_document* document = stp->document;
  stp->document = 0;
  return document;
}
