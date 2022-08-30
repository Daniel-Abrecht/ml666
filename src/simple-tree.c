#include <ml666/simple-tree.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void ml666_st_node_put(struct ml666_st_node* node){
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
    case ML666_ST_NT_DOCUMENT: return &((struct ml666_st_document*)node)->children;
    case ML666_ST_NT_ELEMENT : return &((struct ml666_st_element *)node)->children;
    default: return 0;
  }
}

static void st_set_helper(struct ml666_st_member* member){
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

bool ml666_st_member_set(
  struct ml666_st_node* parent,
  struct ml666_st_member* member,
  struct ml666_st_member* before
){
  if(!parent && !before){
    st_set_helper(member);
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
    st_set_helper(member);
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

struct ml666_st_document* ml666_st_document_create(void){
  struct ml666_st_document* document = calloc(1, sizeof(*document));
  if(!document){
    perror("calloc failed");
    return false;
  }
  document->node.type = ML666_ST_NT_DOCUMENT;
  document->node.refcount = 1;
  return document;
}

struct ml666_st_element* ml666_st_element_create(const struct ml666_hashed_buffer* entry, bool copy_name){
  struct ml666_st_element* element = calloc(1, sizeof(struct ml666_st_element));
  if(!element){
    perror("calloc failed");
    return false;
  }
  element->member.node.type = ML666_ST_NT_ELEMENT;
  element->member.node.refcount = 1;
  if(!(element->name = ml666_hashed_buffer_set.add(entry, copy_name))){
    ml666_st_node_put(&element->member.node);
    fprintf(stderr, "ml666_hashed_buffer_set::add failed");
    return false;
  }
  return element;
}

void ml666_st_subtree_disintegrate(struct ml666_st_children* children){
  while(children->first){
    struct ml666_st_children* chch = ml666_st_default_api.node_get_children(&children->first->node);
    ml666_st_default_api.subtree_disintegrate(chch);
    // It's important for this to be depth first.
    // When the parent looses it's parent and children, it may get freed, as noone may be holding a reference anymore.
    // But by removing the parent here, our unknown parent still has a reference
    ml666_st_default_api.member_set(0, children->first, 0);
  }
}

const struct ml666_st_cb ml666_st_default_api = {
  .node_put = ml666_st_node_put,
  .node_ref = ml666_st_node_ref,
  .node_ref_set = ml666_st_node_ref_set,
  .node_get_children = ml666_st_node_get_children,
  .member_set = ml666_st_member_set,
  .element_create = ml666_st_element_create,
  .document_create = ml666_st_document_create,
  .subtree_disintegrate = ml666_st_subtree_disintegrate,
};
