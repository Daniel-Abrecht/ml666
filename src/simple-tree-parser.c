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
  struct ml666_st_content* current_content;
  struct ml666_st_comment* current_comment;
  struct ml666_st_attribute* current_attribute;
  ml666__cb__malloc*  malloc;
  ml666__cb__realloc* realloc;
  ml666__cb__free*    free;
};

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur){
    parser->error = "simple_tree_parser::tag_push: invalid parser state\n";
    return false;
  }
  stp->current_content = 0;
  stp->current_comment = 0;
  stp->current_attribute = 0;
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_element* element = ml666_st_element_create(stp->public.stb, &entry, copy_name);
  if(!copy_name)
    *name = 0;
  if(!element){
    parser->error = "simple_tree_parser::tag_push: ml666_st_element_create\n";
    return false;
  }
  if(!ml666_st_member_set(stp->public.stb, stp->cur, ML666_ST_MEMBER(element), 0)){
    parser->error = "simple_tree_parser::tag_push: ml666_st_set failed\n";
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
  stp->current_content = 0;
  stp->current_comment = 0;
  stp->current_attribute = 0;
  return ml666_st_node_ref_set(stp->public.stb, &stp->cur, parent);
}

static bool data_append(struct ml666_parser* parser, struct ml666_buffer_ro data){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur){
    parser->error = "simple_tree_parser::data_append: invalid parser state\n";
    return false;
  }
  stp->current_comment = 0;
  stp->current_attribute = 0;
  if(!stp->current_content){
    struct ml666_st_content* content = ml666_st_content_create(stp->public.stb);
    if(!content){
      parser->error = "simple_tree_parser::data_append: ml666_st_content_create failed\n";
      return false;
    }
    if(!ml666_st_member_set(stp->public.stb, stp->cur, ML666_ST_MEMBER(content), 0)){
      parser->error = "simple_tree_parser::data_append: ml666_st_set failed\n";
      ml666_st_node_put(stp->public.stb, ML666_ST_NODE(content));
      return false;
    }
    ml666_st_node_put(stp->public.stb, ML666_ST_NODE(content));
    stp->current_content = content;
  }
  struct ml666_buffer buf = ml666_st_content_take(stp->public.stb, stp->current_content);
  if(!ml666_buffer__append(&buf, data, stp->public.user_ptr, stp->realloc)){
    parser->error = "simple_tree_parser::data_append: realloc failed\n";
    return false;
  }
  ml666_st_content_set(stp->public.stb, stp->current_content, buf);
  return true;
}

static bool comment_append(struct ml666_parser* parser, struct ml666_buffer_ro data){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur){
    parser->error = "simple_tree_parser::data_append: invalid parser state\n";
    return false;
  }
  stp->current_content = 0;
  stp->current_attribute = 0;
  if(!stp->current_comment){
    struct ml666_st_comment* comment = ml666_st_comment_create(stp->public.stb);
    if(!comment){
      parser->error = "simple_tree_parser::data_append: ml666_st_content_create failed\n";
      return false;
    }
    if(!ml666_st_member_set(stp->public.stb, stp->cur, ML666_ST_MEMBER(comment), 0)){
      parser->error = "simple_tree_parser::tag_push: ml666_st_set failed\n";
      ml666_st_node_put(stp->public.stb, ML666_ST_NODE(comment));
      return false;
    }
    ml666_st_node_put(stp->public.stb, ML666_ST_NODE(comment));
    stp->current_comment = comment;
  }
  struct ml666_buffer buf = ml666_st_comment_take(stp->public.stb, stp->current_comment);
  if(!ml666_buffer__append(&buf, data, stp->public.user_ptr, stp->realloc)){
    parser->error = "simple_tree_parser::data_append: realloc failed\n";
    return false;
  }
  ml666_st_comment_set(stp->public.stb, stp->current_comment, buf);
  return true;
}

static bool set_attribute(struct ml666_parser* parser, ml666_opaque_attribute_name* name){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  struct ml666_st_element* element = ML666_ST_U_ELEMENT(stp->cur);
  if(!stp->cur || !element){
    parser->error = "simple_tree_parser::data_append: invalid parser state\n";
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_attribute* attribute = ml666_st_attribute_lookup(stp->public.stb, element, &entry, ML666_ST_AOF_CREATE_EXCLUSIVE | (copy_name?0:ML666_ST_AOF_CREATE_NOCOPY));
  stp->current_attribute = attribute;
  return !!attribute;
}

static bool value_append(struct ml666_parser* parser, struct ml666_buffer_ro data){
  struct ml666_simple_tree_parser_default* stp = parser->user_ptr;
  if(!stp->cur || !stp->current_attribute){
    parser->error = "simple_tree_parser::data_append: invalid parser state\n";
    return false;
  }
  const struct ml666_buffer* pvalue = ml666_st_attribute_take_value(stp->public.stb, stp->current_attribute);
  struct ml666_buffer buf = {0};
  if(pvalue)
    buf = *pvalue;
  if(!ml666_buffer__append(&buf, data, stp->public.user_ptr, stp->realloc)){
    parser->error = "simple_tree_parser::data_append: realloc failed\n";
    return false;
  }
  return ml666_st_attribute_set_value(stp->public.stb, stp->current_attribute, &buf);
}

static ml666_simple_tree_parser_cb_destroy ml666_simple_tree_parser_d_destroy;
static ml666_simple_tree_parser_cb_next ml666_simple_tree_parser_d_next;
static ml666_simple_tree_parser_cb_take_document ml666_simple_tree_parser_d_take_document;

static const struct ml666_simple_tree_parser_cb simple_tree_parser_cb = {
  .destroy = ml666_simple_tree_parser_d_destroy,
  .next = ml666_simple_tree_parser_d_next,
  .take_document = ml666_simple_tree_parser_d_take_document,
};

static const struct ml666_parser_api callbacks = {
  .tag_name_append       = ml666_parser__d_mal__tag_name_append,
  .tag_name_free         = ml666_parser__d_mal__tag_name_free,
  .attribute_name_append = ml666_parser__d_mal__attribute_name_append,
  .attribute_name_free   = ml666_parser__d_mal__attribute_name_free,

  .set_attribute = set_attribute,

  .data_append = data_append,
  .comment_append = comment_append,
  .value_append = value_append,

  .tag_push = tag_push,
  .end_tag_check = end_tag_check,
  .tag_pop = tag_pop,
};

// FIXME: Free parser on error
struct ml666_simple_tree_parser* ml666_simple_tree_parser_create_p(struct ml666_simple_tree_parser_create_args args){
  if(!args.stb){
    fprintf(stderr, "ml666_simple_tree_parser_create_p: mandatory argument \"stb\" not set!\n");
    if(args.parser) ml666_parser_destroy(args.parser);
    return false;
  }
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.realloc)
    args.realloc = ml666__d__realloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666_simple_tree_parser_default* stp = args.malloc(args.user_ptr, sizeof(*stp));
  if(!stp){
    fprintf(stderr, "malloc failed");
    if(args.parser) ml666_parser_destroy(args.parser);
    return false;
  }
  memset(stp, 0, sizeof(*stp));
  *(const struct ml666_simple_tree_parser_cb**)&stp->public.cb = &simple_tree_parser_cb;
  *(struct ml666_st_builder**)&stp->public.stb = args.stb;
  stp->public.user_ptr = args.user_ptr;
  stp->malloc = args.malloc;
  stp->realloc = args.realloc;
  stp->free = args.free;
  if(args.parser){
    stp->parser = args.parser;
  }else{
    stp->parser = ml666_parser_create(
      .fd = args.fd,
      .tokenizer = args.tokenizer,
      .api = &callbacks,
      .user_ptr = stp
    );
    if(!stp->parser){
      fprintf(stderr, "ml666_parser_create failed\n");
      return false;
    }
  }
  struct ml666_st_document* document = ml666_st_document_create(stp->public.stb);
  if(!document){
    if(stp->parser) ml666_parser_destroy(stp->parser);
    args.free(args.user_ptr, stp);
    fprintf(stderr, "ml666_st_document_create failed\n");
    return false;
  }
  stp->document = document;
  ml666_st_node_ref_set(stp->public.stb, &stp->cur, ML666_ST_NODE(document));
  return &stp->public;
}

static void ml666_simple_tree_parser_d_destroy(struct ml666_simple_tree_parser* _stp){
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

static bool ml666_simple_tree_parser_d_next(struct ml666_simple_tree_parser* _stp){
  struct ml666_simple_tree_parser_default* stp = (struct ml666_simple_tree_parser_default*)_stp;
  if(!stp->parser)
    return false;
  bool res = ml666_parser_next(stp->parser);
  stp->public.line = stp->parser->line;
  stp->public.column = stp->parser->column;
  if(stp->parser->error)
    stp->public.error = stp->parser->error;
  return res;
}


static struct ml666_st_document* ml666_simple_tree_parser_d_take_document(struct ml666_simple_tree_parser* _stp){
  struct ml666_simple_tree_parser_default* stp = (struct ml666_simple_tree_parser_default*)_stp;
  struct ml666_st_document* document = stp->document;
  stp->document = 0;
  return document;
}
