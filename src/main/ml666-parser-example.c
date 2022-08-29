#include <ml666/parser.h>
#include <ml666/utils.h>
#include <ml666/simple-tree.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur){
    printf("ml666_parser::tag_push: invalid parser state\n");
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer__set(&entry, (*name)->buffer.ro);
  bool copy_name = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  struct ml666_st_element* element = ml666_st_element_create(&entry, copy_name);
  if(!copy_name)
    *name = 0;
  if(!element){
    parser->error = "ml666_parser::tag_push: ml666_st_element_create\n";
    return false;
  }
  if(!ml666_st_set(stb->cur, &element->member, 0)){
    parser->error = "ml666_parser::tag_push: ml666_st_set failed";
    ml666_st_node_put(&element->member.node);
    return false;
  }
  ml666_st_node_put(&element->member.node);
  return ml666_st_node_ref_set(&stb->cur, &element->member.node);
}

static bool end_tag_check(struct ml666_parser* parser, ml666_opaque_tag_name name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur || stb->cur->type != ML666_STB_NT_ELEMENT)
    return false;
  const struct ml666_hashed_buffer* cur_name = ml666_hashed_buffer_set__peek(((struct ml666_st_element*)stb->cur)->name);
  if( cur_name->buffer.length == name->buffer.length
   && memcmp(name->buffer.data, cur_name->buffer.data, name->buffer.length) == 0
  ) return true;
  return false;
}

static bool tag_pop(struct ml666_parser* parser){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(!stb->cur || stb->cur->type != ML666_STB_NT_ELEMENT)
    return false;
  struct ml666_st_element* element = (struct ml666_st_element*)stb->cur;
  if(!element->member.parent)
    return false;
  return ml666_st_node_ref_set(&stb->cur, element->member.parent);
}

static bool init(struct ml666_parser* parser){
  struct ml666_simple_tree_builder* stb = calloc(1, sizeof(*stb));
  if(!stb){
    parser->error = "calloc failed";
    return false;
  }
  struct ml666_st_document* document = calloc(1, sizeof(*document));
  if(!document){
    free(stb);
    parser->error = "calloc failed";
    return false;
  }
  document->node.type = ML666_STB_NT_DOCUMENT;
  document->node.refcount = 1;
  stb->document = document;
  ml666_st_node_ref_set(&stb->cur, &document->node);
  parser->user_ptr = stb;
  return true;
}

static const struct ml666_parser_cb callbacks = {
  .tag_name_append       = ml666_parser__d_mal__tag_name_append,
  .tag_name_free         = ml666_parser__d_mal__tag_name_free,
  .attribute_name_append = ml666_parser__d_mal__attribute_name_append,
  .attribute_name_free   = ml666_parser__d_mal__attribute_name_free,

  .init = init,
  .tag_push = tag_push,
  .end_tag_check = end_tag_check,
  .tag_pop = tag_pop,
};

/*static bool ml666_stb_create(int fd){
}*/

void ml666_st_subtree_disintegrate(struct ml666_st_children* children){
  while(children->first){
    struct ml666_st_children* chch = ml666_st_node_get_children(&children->first->node);
    ml666_st_subtree_disintegrate(chch);
    // It's important for this to be depth first.
    // When the parent looses it's parent and children, it may get freed, as noone may be holding a reference anymore.
    // But by removing the parent here, our unknown parent still has a reference
    ml666_st_set(0, children->first, 0);
  }
}

int main(){
  setvbuf(stdout, NULL, _IOLBF, 0);
  struct ml666_parser* parser = ml666_parser_create( .fd = 0, .cb = &callbacks );
  if(!parser){
    fprintf(stderr, "ml666_parser_create failed");
    return 1;
  }
  while(ml666_parser_next(parser));
  if(parser->error){
    fprintf(stderr, "ml666_parser_next failed: %s\n", parser->error);
    return 1;
  }
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  ml666_st_node_ref_set(&stb->cur, 0);
  ml666_st_subtree_disintegrate(&stb->document->children);
  ml666_st_node_put(&stb->document->node);
  ml666_parser_destroy(parser);
  free(stb);
  return 0;
}
