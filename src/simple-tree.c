#include <ml666/simple-tree.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void ml666_st_node_put(struct ml666_st_node* node){
  if((node->refcount -= 1))
    return;
  switch(node->type){
    case ML666_STB_NT_DOCUMENT: break;
    case ML666_STB_NT_ELEMENT: {
      struct ml666_st_element* element = (struct ml666_st_element*)node;
      ml666_hashed_buffer_set__put(element->name);
    } break;
    case ML666_STB_NT_CONTENT: break;
    case ML666_STB_NT_COMMENT: break;
  }
  free(node);
}

bool ml666_st_node_ref(struct ml666_st_node* node){
  if(!(node->refcount += 1))
    return false;
  return true;
}

bool ml666_st_node_ref_set(struct ml666_st_node** dest, struct ml666_st_node* src){
  if(src && !ml666_st_node_ref(src))
    return false;
  struct ml666_st_node* old = *dest;
  if(old)
    ml666_st_node_put(old);
  *dest = src;
  return true;
}

struct ml666_st_children* ml666_st_node_get_children(struct ml666_st_node* node){
  switch(node->type){
    case ML666_STB_NT_DOCUMENT: return &((struct ml666_st_document*)node)->children;
    case ML666_STB_NT_ELEMENT : return &((struct ml666_st_element *)node)->children;
    default: return 0;
  }
}

static void ml666_st_set_helper(struct ml666_st_member* member){
  struct ml666_st_node* old_parent = member->parent;
  if(!old_parent)
    return;
  struct ml666_st_children* old_children = ml666_st_node_get_children(old_parent);
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
    ml666_st_node_put(old_parent);
}

bool ml666_st_set(
  struct ml666_st_node* parent,
  struct ml666_st_member* member,
  struct ml666_st_member* before
){
  if(!parent && !before){
    ml666_st_set_helper(member);
    member->previous = 0;
    member->next = 0;
    member->parent = 0;
    ml666_st_node_put(&member->node);
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
  struct ml666_st_children* children = ml666_st_node_get_children(parent);
  if(!children)
    return false;
  struct ml666_st_node* old_parent = member->parent;
  if(!old_parent){
    if(!ml666_st_node_ref(&member->node))
      return false;
    if(!children->first){
      if(!ml666_st_node_ref(parent)){
        ml666_st_node_put(&member->node);
        return false;
      }
    }
  }else if(old_parent != parent){
    if(!children->first)
      if(!ml666_st_node_ref(parent))
        return false;
    ml666_st_set_helper(member);
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

struct ml666_st_element* ml666_st_element_create(const struct ml666_hashed_buffer* entry, bool copy_name){
  struct ml666_st_element* element = calloc(1, sizeof(struct ml666_st_element));
  if(!element){
    fprintf(stderr, "calloc failed");
    return false;
  }
  element->member.node.type = ML666_STB_NT_ELEMENT;
  element->member.node.refcount = 1;
  if(!ml666_hashed_buffer_set__add(&element->name, entry, copy_name)){
    ml666_st_node_put(&element->member.node);
    fprintf(stderr, "ml666_hashed_buffer_set__add failed");
    return false;
  }
  return element;
}

