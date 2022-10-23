#include <ml666/simple-tree-builder.h>
#include <ml666/utils.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ml666__container_of(ptr, type, member) \
  ((type*)( (ptr) ? (char*)((__typeof__(((type*)0)->member)*){ptr}) - offsetof(type, member) : 0 ))

ML666_DEFAULT_SIMPLE_TREE

struct ml666_st_builder_default {
  struct ml666_st_builder public;
  struct ml666_st_builder_create_args a;
};

void ml666_st__d__node_put(struct ml666_st_builder* _stb, struct ml666_st_node* node){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  if((node->refcount -= 1))
    return;
  switch(node->type){
    case ML666_ST_NT_DOCUMENT: break;
    case ML666_ST_NT_ELEMENT: {
      struct ml666_st_element* element = (struct ml666_st_element*)node;
      ml666_hashed_buffer_set__put(stb->a.buffer_set, element->name);
      for(struct ml666_st_attribute* attribute; (attribute=ml666_st_attribute_get_first(&stb->public, element));)
        ml666_st_attribute_remove(&stb->public, attribute);
    } break;
    case ML666_ST_NT_CONTENT: {
      struct ml666_st_content* content = (struct ml666_st_content*)node;
      stb->a.free(stb->public.user_ptr, content->buffer.data);
    } break;
    case ML666_ST_NT_COMMENT: {
      struct ml666_st_comment* comment = (struct ml666_st_comment*)node;
      stb->a.free(stb->public.user_ptr, comment->buffer.data);
    } break;
  }
  stb->a.free(stb->public.user_ptr, node);
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

bool ml666_st__d__content_set(struct ml666_st_builder* _stb, struct ml666_st_content* content, struct ml666_buffer buffer){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  ml666_buffer__clear(&content->buffer, stb->public.user_ptr, stb->a.free);
  content->buffer = buffer;
  return true;
}

struct ml666_buffer_ro ml666_st__d__content_get(struct ml666_st_builder* stb, const struct ml666_st_content* content){
  (void)stb;
  return content->buffer.ro;
}

struct ml666_buffer ml666_st__d__content_take(struct ml666_st_builder* stb, struct ml666_st_content* content){
  (void)stb;
  struct ml666_buffer ret = content->buffer;
  content->buffer = (struct ml666_buffer){0};
  return ret;
}

bool ml666_st__d__comment_set(struct ml666_st_builder* _stb, struct ml666_st_comment* comment, struct ml666_buffer buffer){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  ml666_buffer__clear(&comment->buffer, stb->public.user_ptr, stb->a.free);
  comment->buffer = buffer;
  return true;
}

struct ml666_buffer_ro ml666_st__d__comment_get(struct ml666_st_builder* stb, const struct ml666_st_comment* comment){
  (void)stb;
  return comment->buffer.ro;
}

struct ml666_buffer ml666_st__d__comment_take(struct ml666_st_builder* stb, struct ml666_st_comment* comment){
  (void)stb;
  struct ml666_buffer ret = comment->buffer;
  comment->buffer = (struct ml666_buffer){0};
  return ret;
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

struct ml666_st_document* ml666_st__d__document_create(struct ml666_st_builder* _stb){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  struct ml666_st_document* document = stb->a.malloc(stb->public.user_ptr, sizeof(*document));
  if(!document){
    perror("malloc failed");
    return 0;
  }
  memset(document, 0, sizeof(*document));
  document->node.type = ML666_ST_NT_DOCUMENT;
  document->node.refcount = 1;
  return document;
}

struct ml666_st_element* ml666_st__d__element_create(struct ml666_st_builder* _stb, const struct ml666_hashed_buffer* entry, bool copy_name){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  struct ml666_st_element* element = stb->a.malloc(stb->public.user_ptr, sizeof(*element));
  if(!element){
    perror("malloc failed");
    return 0;
  }
  memset(element, 0, sizeof(*element));
  element->member.node.type = ML666_ST_NT_ELEMENT;
  element->member.node.refcount = 1;
  if(!(element->name = ml666_hashed_buffer_set__lookup(stb->a.buffer_set, entry, copy_name ? ML666_HBS_M_ADD_COPY : ML666_HBS_M_ADD_TAKE))){
    ml666_st__d__node_put(&stb->public, &element->member.node);
    fprintf(stderr, "ml666_hashed_buffer_set::lookup failed\n");
    return 0;
  }
  return element;
}

struct ml666_st_content* ml666_st__d__content_create(struct ml666_st_builder* _stb){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  struct ml666_st_content* content = stb->a.malloc(stb->public.user_ptr, sizeof(*content));
  if(!content){
    perror("malloc failed");
    return 0;
  }
  memset(content, 0, sizeof(*content));
  content->member.node.type = ML666_ST_NT_CONTENT;
  content->member.node.refcount = 1;
  return content;
}

struct ml666_st_comment* ml666_st__d__comment_create(struct ml666_st_builder* _stb){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  struct ml666_st_comment* comment = stb->a.malloc(stb->public.user_ptr, sizeof(*comment));
  if(!comment){
    perror("malloc failed");
    return 0;
  }
  memset(comment, 0, sizeof(*comment));
  comment->member.node.type = ML666_ST_NT_COMMENT;
  comment->member.node.refcount = 1;
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

struct ml666_st_attribute* ml666_st__d__attribute_get_first(const struct ml666_st_builder* stb, const struct ml666_st_element* element){
  (void)stb;
  return ml666__container_of(element->attribute_list.first, struct ml666_st_attribute, entry);
}

struct ml666_st_attribute* ml666_st__d__attribute_get_next(const struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  (void)stb;
  if(attribute->entry.llist->last == &attribute->entry)
    return 0;
  return ml666__container_of(attribute->entry.next, struct ml666_st_attribute, entry);
}

struct ml666_st_attribute* ml666_st__d__attribute_lookup(struct ml666_st_builder* _stb, struct ml666_st_element* element, const struct ml666_hashed_buffer* name, unsigned flags){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  enum ml666_hashed_buffer_set_mode mode = ML666_HBS_M_GET;
  if((flags & ML666_ST_AOF_CREATE_NOCOPY) == ML666_ST_AOF_CREATE_NOCOPY){
    mode = ML666_HBS_M_ADD_TAKE;
  }else if((flags & ML666_ST_AOF_CREATE) == ML666_ST_AOF_CREATE){
    mode = ML666_HBS_M_ADD_COPY;
  }
  const struct ml666_hashed_buffer_set_entry* entry = ml666_hashed_buffer_set__lookup(stb->a.buffer_set, name, mode);
  if(!entry){
    if(mode != ML666_HBS_M_GET)
      fprintf(stderr, "ml666_hashed_buffer_set::lookup failed\n");
    return 0;
  }
  struct ml666_st_attribute* attribute = 0;
  for(attribute=ml666_st_attribute_get_first(&stb->public, element); attribute; attribute=ml666_st_attribute_get_next(&stb->public, attribute))
    if(attribute->name == entry)
      break;
  if(attribute){
    ml666_hashed_buffer_set__put(stb->a.buffer_set, entry);
    if((flags & ML666_ST_AOF_CREATE_EXCLUSIVE) == ML666_ST_AOF_CREATE_EXCLUSIVE){
      fprintf(stderr, "ml666_st__d__attribute_open failed: attribute already exists\n");
      return 0;
    }
  }else{
    attribute = stb->a.malloc(stb->public.user_ptr, sizeof(*attribute));
    if(!attribute){
      ml666_hashed_buffer_set__put(stb->a.buffer_set, entry);
      perror("malloc failed");
      return 0;
    }
    memset(attribute, 0, sizeof(*attribute));
    attribute->name = entry;
    if(!element->attribute_list.first)
      element->attribute_list.first = &attribute->entry;
    if(element->attribute_list.last){
      attribute->entry.previous = element->attribute_list.last;
      element->attribute_list.last->next = &attribute->entry;
    }else{
      attribute->entry.flist = &element->attribute_list;
    }
    element->attribute_list.last = &attribute->entry;
    attribute->entry.llist = &element->attribute_list;
  }
  return attribute;
}

void ml666_st__d__attribute_remove(struct ml666_st_builder* _stb, struct ml666_st_attribute* attribute){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  if(attribute->entry.llist->last == &attribute->entry && attribute->entry.flist->first == &attribute->entry){
    attribute->entry.llist->last  = 0;
    attribute->entry.llist->first = 0;
  }else if(attribute->entry.flist->first == &attribute->entry){
    attribute->entry.next->flist = attribute->entry.flist;
    attribute->entry.flist->first = attribute->entry.next;
  }else if(attribute->entry.llist->last == &attribute->entry){
    attribute->entry.previous->llist = attribute->entry.llist;
    attribute->entry.llist->last = attribute->entry.previous;
  }else{
    attribute->entry.previous->next = attribute->entry.next;
    attribute->entry.next->previous = attribute->entry.previous;
  }
  ml666_st_attribute_set_value(&stb->public, attribute, 0);
  stb->a.free(stb->public.user_ptr, attribute);
}

const struct ml666_hashed_buffer* ml666_st__d__attribute_get_name(struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  (void)stb;
  return ml666_hashed_buffer_set__peek(attribute->name);
}

bool ml666_st__d__attribute_set_value(struct ml666_st_builder* _stb, struct ml666_st_attribute* attribute, struct ml666_buffer* value){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  if(attribute->has_value)
    ml666_buffer__clear(&attribute->value, stb->public.user_ptr, stb->a.free);
  if(value)
    attribute->value = *value;
  attribute->has_value = !!value;
  return true;
}

const struct ml666_buffer_ro* ml666_st__d__attribute_get_value(struct ml666_st_builder* stb, const struct ml666_st_attribute* attribute){
  (void)stb;
  if(!attribute->has_value)
    return 0;
  return &attribute->value.ro;
}

const struct ml666_buffer* ml666_st__d__attribute_take_value(struct ml666_st_builder* stb, struct ml666_st_attribute* attribute){
  (void)stb;
  if(!attribute->has_value)
    return 0;
  attribute->has_value = false;
  return &attribute->value;
}

void ml666_st__d__builder_destroy(struct ml666_st_builder* _stb){
  struct ml666_st_builder_default* stb = (struct ml666_st_builder_default*)_stb;
  stb->a.free(stb->public.user_ptr, stb);
}

ML666_ST_IMPLEMENTATION(ml666_default, ml666_st__d_)

struct ml666_st_builder* ml666_st_builder_create_p(struct ml666_st_builder_create_args args){
  if(!args.buffer_set)
    args.buffer_set = ml666_hashed_buffer_set__get_default();
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666_st_builder_default* stb = args.malloc(args.user_ptr, sizeof(*stb));
  if(!stb){
    fprintf(stderr, "malloc failed");
    return false;
  }
  memset(stb, 0, sizeof(*stb));
  *(const struct ml666_st_cb**)&stb->public.cb = &ml666_default_st_api;
  stb->public.user_ptr = args.user_ptr;
  stb->a = args;
  return &stb->public;
}
