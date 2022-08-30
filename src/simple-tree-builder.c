#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree.h>
#include <ml666/parser.h>
#include <ml666/utils.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

struct ml666_simple_tree_builder {
  const struct ml666_st_cb* cb;
  struct ml666_parser* parser;
  struct ml666_st_document* document;
  struct ml666_st_node* cur;
  const char* error;
};

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur){
    printf("ml666_parser::tag_push: invalid parser state\n");
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_element* element = stb->cb->element_create(&entry, copy_name);
  if(!copy_name)
    *name = 0;
  if(!element){
    parser->error = "ml666_parser::tag_push: ml666_st_element_create\n";
    return false;
  }
  if(!stb->cb->member_set(stb->cur, &element->member, 0)){
    parser->error = "ml666_parser::tag_push: ml666_st_set failed";
    stb->cb->node_put(&element->member.node);
    return false;
  }
  stb->cb->node_put(&element->member.node);
  return stb->cb->node_ref_set(&stb->cur, &element->member.node);
}

static bool end_tag_check(struct ml666_parser* parser, ml666_opaque_tag_name name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur || stb->cur->type != ML666_ST_NT_ELEMENT)
    return false;
  const struct ml666_hashed_buffer* cur_name = ml666_hashed_buffer_set__peek(((struct ml666_st_element*)stb->cur)->name);
  if( cur_name->buffer.length == name->buffer.length
   && memcmp(name->buffer.data, cur_name->buffer.data, name->buffer.length) == 0
  ) return true;
  return false;
}

static bool tag_pop(struct ml666_parser* parser){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur || stb->cur->type != ML666_ST_NT_ELEMENT)
    return false;
  struct ml666_st_element* element = (struct ml666_st_element*)stb->cur;
  if(!element->member.parent)
    return false;
  return stb->cb->node_ref_set(&stb->cur, element->member.parent);
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

struct ml666_simple_tree_builder* ml666_simple_tree_builder_create_p(struct ml666_simple_tree_builder_create_args args){
  struct ml666_simple_tree_builder* stb = calloc(1, sizeof(*stb));
  if(!stb){
    stb->parser->error = "calloc failed";
    return false;
  }
  stb->cb = args.cb;
  stb->parser = ml666_parser_create( .fd = 0, .cb = &callbacks );
  if(!stb->parser){
    fprintf(stderr, "ml666_parser_create failed");
    return false;
  }
  stb->parser->user_ptr = stb;
  struct ml666_st_document* document = stb->cb->document_create();
  if(!document){
    free(stb);
    fprintf(stderr, "ml666_st_document_create failed");
    return false;
  }
  stb->document = document;
  stb->cb->node_ref_set(&stb->cur, &document->node);
  return stb;
}

void ml666_simple_tree_builder_destroy(struct ml666_simple_tree_builder* stb){
  stb->cb->node_ref_set(&stb->cur, 0);
  if(stb->document){
    stb->cb->subtree_disintegrate(&stb->document->children);
    stb->cb->node_put(&stb->document->node);
    stb->document = 0;
  }
  if(stb->parser)
    ml666_parser_destroy(stb->parser);
  free(stb);
}

bool ml666_simple_tree_builder_next(struct ml666_simple_tree_builder* stb){
  if(!stb->parser)
    return false;
  bool res = ml666_parser_next(stb->parser);
  if(stb->parser->error)
    stb->error = stb->parser->error;
  return res;
}

const char* ml666_simple_tree_builder_get_error(struct ml666_simple_tree_builder* stb){
  return stb->error;
}


struct ml666_st_document* ml666_simple_tree_builder_take_document(struct ml666_simple_tree_builder* stb){
  struct ml666_st_document* document = stb->document;
  stb->document = 0;
  return document;
}
