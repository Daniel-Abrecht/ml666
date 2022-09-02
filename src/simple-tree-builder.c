#include <ml666/simple-tree-builder.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


struct ml666_st_attribute {
  const struct ml666_hashed_buffer_set_entry* name;
  struct ml666_buffer value;
};

struct ml666_st_children {
  struct ml666_st_member *first, *last;
};

struct ml666_st_node {
  enum ml666_st_node_type type;
  size_t refcount;
};

struct ml666_st_member {
  struct ml666_st_node node;
  struct ml666_st_node *parent;
  struct ml666_st_member *previous, *next;
};

struct ml666_st_document {
  struct ml666_st_node node;
  struct ml666_st_children children;
};

struct ml666_st_element {
  struct ml666_st_member member;
  struct ml666_st_children children;
  const struct ml666_hashed_buffer_set_entry* name;
};

struct ml666_st_content {
  struct ml666_st_member member;
  struct ml666_buffer buffer;
};

struct ml666_st_comment {
  struct ml666_st_member member;
  struct ml666_buffer buffer;
};


void ml666_st__d__node_put(struct ml666_st_builder* stb, struct ml666_st_node* node){
  (void)stb;
  if((node->refcount -= 1))
    return;
  switch(node->type){
    case ML666_ST_NT_DOCUMENT: break;
    case ML666_ST_NT_ELEMENT: {
      struct ml666_st_element* element = (struct ml666_st_element*)node;
      ml666_hashed_buffer_set.put(element->name);
    } break;
    case ML666_ST_NT_CONTENT: break;
    case ML666_ST_NT_COMMENT: break;
  }
  free(node);
}

bool ml666_st__d__node_ref(struct ml666_st_builder* stb, struct ml666_st_node* node){
  (void)stb;
  if(!(node->refcount += 1))
    return false;
  return true;
}

enum ml666_st_node_type ml666_st__d__node_get_type(struct ml666_st_builder* stb, struct ml666_st_node* node){
  (void)stb;
  return node->type;
}

struct ml666_st_children* ml666_st__d__document_get_children(struct ml666_st_builder* stb, struct ml666_st_document* document){
  (void)stb;
  return &document->children;
}

struct ml666_st_children* ml666_st__d__element_get_children(struct ml666_st_builder* stb, struct ml666_st_element* element){
  (void)stb;
  return &element->children;
}

struct ml666_st_node* ml666_st__d__member_get_parent(struct ml666_st_builder* stb, struct ml666_st_member* member){
  (void)stb;
  return member->parent;
}

struct ml666_st_member* ml666_st__d__member_get_previous(struct ml666_st_builder* stb, struct ml666_st_member* member){
  (void)stb;
  return member->previous;
}

struct ml666_st_member* ml666_st__d__member_get_next(struct ml666_st_builder* stb, struct ml666_st_member* member){
  (void)stb;
  return member->next;
}

const struct ml666_hashed_buffer_set_entry* ml666_st__d__element_get_name(struct ml666_st_builder* stb, struct ml666_st_element* element){
  (void)stb;
  return element->name;
}

bool ml666_st__d__content_set(struct ml666_st_builder* stb, struct ml666_st_content* content, struct ml666_buffer buffer){
  (void)stb;
  content->buffer = buffer;
  return true;
}

struct ml666_buffer ml666_st__d__content_get(struct ml666_st_builder* stb, const struct ml666_st_content* content){
  (void)stb;
  return content->buffer;
}

bool ml666_st__d__comment_set(struct ml666_st_builder* stb, struct ml666_st_comment* comment, struct ml666_buffer buffer){
  (void)stb;
  comment->buffer = buffer;
  return true;
}

struct ml666_buffer ml666_st__d__comment_get(struct ml666_st_builder* stb, const struct ml666_st_comment* comment){
  (void)stb;
  return comment->buffer;
}

static void st_set_helper(struct ml666_st_builder* stb, struct ml666_st_member* member){
  struct ml666_st_node* old_parent = member->parent;
  if(!old_parent)
    return;
  struct ml666_st_children* old_children = ml666_st_node_get_children(stb, old_parent);
  if(old_children->first == member){
    old_children->first = member->next;
  }else{
    member->previous->next = member->next;
  }
  if(old_children->last == member){
    old_children->last = member->previous;
  }else{
    member->next->previous = member->previous;
  }
  if(!old_children->first)
    ml666_st__d__node_put(stb, old_parent);
}

bool ml666_st__d__member_set(
  struct ml666_st_builder* stb,
  struct ml666_st_node* parent,
  struct ml666_st_member* member,
  struct ml666_st_member* before
){
  if(!parent && !before){
    st_set_helper(stb, member);
    member->previous = 0;
    member->next = 0;
    member->parent = 0;
    ml666_st__d__node_put(stb, &member->node);
    return true;
  }
  if(before){
    if(parent){
      if(parent != before->parent)
        return false;
    }else{
      parent = before->parent;
    }
  }
  if(!parent)
    return false;
  struct ml666_st_children* children = ml666_st_node_get_children(stb, parent);
  if(!children)
    return false;
  struct ml666_st_node* old_parent = member->parent;
  if(!old_parent){
    if(!ml666_st__d__node_ref(stb, &member->node))
      return false;
    if(!children->first){
      if(!ml666_st__d__node_ref(stb, parent)){
        ml666_st__d__node_put(stb, &member->node);
        return false;
      }
    }
  }else if(old_parent != parent){
    if(!children->first)
      if(!ml666_st__d__node_ref(stb, parent))
        return false;
    st_set_helper(stb, member);
  }
  member->parent = parent;
  member->next = before;
  struct ml666_st_member* after = 0;
  if(before){
    after = before->previous;
    before->previous = member;
  }else{
    after = children->last;
    children->last = member;
  }
  member->previous = after;
  if(after){
    after->next = member;
  }else{
    children->first = member;
  }
  return true;
}

struct ml666_st_document* ml666_st__d__document_create(struct ml666_st_builder* stb){
  (void)stb;
  struct ml666_st_document* document = calloc(1, sizeof(*document));
  if(!document){
    perror("calloc failed");
    return 0;
  }
  document->node.type = ML666_ST_NT_DOCUMENT;
  document->node.refcount = 1;
  return document;
}

struct ml666_st_element* ml666_st__d__element_create(struct ml666_st_builder* stb, const struct ml666_hashed_buffer* entry, bool copy_name){
  (void)stb;
  struct ml666_st_element* element = calloc(1, sizeof(struct ml666_st_element));
  if(!element){
    perror("calloc failed");
    return 0;
  }
  element->member.node.type = ML666_ST_NT_ELEMENT;
  element->member.node.refcount = 1;
  if(!(element->name = ml666_hashed_buffer_set.add(entry, copy_name))){
    ml666_st__d__node_put(stb, &element->member.node);
    fprintf(stderr, "ml666_hashed_buffer_set::add failed");
    return 0;
  }
  return element;
}

struct ml666_st_content* ml666_st__d__content_create(struct ml666_st_builder* stb){
  (void)stb;
  struct ml666_st_content* content = calloc(1, sizeof(struct ml666_st_content));
  if(!content){
    perror("calloc failed");
    return 0;
  }
  return content;
}

struct ml666_st_comment* ml666_st__d__comment_create(struct ml666_st_builder* stb){
  (void)stb;
  struct ml666_st_comment* comment = calloc(1, sizeof(struct ml666_st_comment));
  if(!comment){
    perror("calloc failed");
    return 0;
  }
  return comment;
}

struct ml666_st_member* ml666_st__d__get_first_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  (void)stb;
  return children->first;
}

struct ml666_st_member* ml666_st__d__get_last_child(struct ml666_st_builder* stb, struct ml666_st_children* children){
  (void)stb;
  return children->last;
}

ML666_ST_IMPLEMENTATION(ml666_default, ml666_st__d_)

struct ml666_st_builder ml666_default_simple_tree_builder = {
  .cb = &ml666_default_st_api,
};
