#include <ml666/parser.h>
#include <ml666/utils.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

static struct ml666_hashed_buffer_set buffer_set;

/*struct ml666_stb_tree {
};*/

struct ml666_hashed_buffer_set_entry {
  struct ml666_hashed_buffer data;
  size_t refcount;
  struct ml666_hashed_buffer_set_entry* next;
};

struct ml666_hashed_buffer_set {
  struct ml666_hashed_buffer_set_entry* bucket[512];
};

static unsigned hash_to_bucket(uint64_t hash){
  return ( (hash >> 0 ) ^ (hash >> 16) ^ (hash >> 32) ^ (hash >> 48) ) & 0x1FF;
}

void ml666_hashed_buffer_set__put(struct ml666_hashed_buffer_set* set, const struct ml666_hashed_buffer_set_entry* _entry){
  struct ml666_hashed_buffer_set_entry* entry = (struct ml666_hashed_buffer_set_entry*)_entry;
  entry->refcount -= 1;
  if(entry->refcount)
    return;
  unsigned bucket = hash_to_bucket(entry->data.hash);
  for(struct ml666_hashed_buffer_set_entry **it=&set->bucket[bucket], *cur; (cur=*it); it=&cur->next){
    if(cur != entry)
      continue;
    *it = cur->next;
    free((void*)cur->data.buffer.data);
    free(cur);
    return;
  }
  fprintf(stderr, "%s:%u: error: entry not found in set\n", __FILE__, __LINE__);
  abort();
}

bool ml666_hashed_buffer_set__add(
  const struct ml666_hashed_buffer_set_entry** out,
  struct ml666_hashed_buffer_set* set,
  const struct ml666_hashed_buffer* entry,
  bool copy_content
){
  const struct ml666_hashed_buffer c = *entry;
  unsigned bucket = hash_to_bucket(c.hash);

  struct ml666_hashed_buffer_set_entry **it, *cur;
  for(it=&set->bucket[bucket]; (cur=*it); it=&cur->next){
    struct ml666_hashed_buffer_set_entry* cur = *it;
    if(cur->data.hash > c.hash)
      break;
    if(&cur->data == entry)
      goto found;
    if(cur->data.hash == c.hash && cur->data.buffer.length == c.buffer.length && memcmp(cur->data.buffer.data, c.buffer.data, c.buffer.length) == 0)
      goto found;
  }

  struct ml666_hashed_buffer_set_entry* result = malloc(sizeof(struct ml666_hashed_buffer_set_entry));
  if(!result){
    perror("malloc failed");
    *out = 0;
    return false;
  }
  if(copy_content){
    char* data = malloc(c.buffer.length);
    if(!data){
      free(result);
      perror("malloc failed");
      *out = 0;
      return false;
    }
    memcpy(data, c.buffer.data, c.buffer.length);
    result->data.hash = c.hash;
    result->data.buffer.data = data;
    result->data.buffer.length = c.buffer.length;
  }else{
    result->data = *entry;
  }
  result->refcount = 0;
  result->next = cur;
  *it = result;
  *out = result;
  return true;

found:
  cur->refcount += 1;
  *out = cur;
  return true;
}

struct ml666_stb_attribute {
  const struct ml666_hashed_buffer_set_entry* name;
  struct ml666_buffer value;
};

struct ml666_stb_tag {
  const struct ml666_hashed_buffer_set_entry* name;
  struct ml666_stb_tag* parent;
  struct ml666_stb_tag* previous;
  struct ml666_stb_tag* next;
  struct ml666_stb_tag* children_first;
  struct ml666_stb_tag* children_last;
};

struct ml666_simple_tree_builder {
  struct ml666_stb_tag* current_tag;
};

static bool tag_push(struct ml666_parser* parser, ml666_opaque_tag_name* name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  struct ml666_stb_tag* tag = calloc(1, sizeof(struct ml666_stb_tag));
  if(!tag){
    parser->error = "calloc failed";
    return false;
  }
  struct ml666_hashed_buffer entry;
  ml666_hashed_buffer_set(&entry, (*name)->buffer.ro);
  bool copy_content = true; // Note: "false" only works if the allocator actually used malloc. Any other case will fail. "true" always works, but is more expencive.
  if(!ml666_hashed_buffer_set__add(&tag->name, &buffer_set, &entry, copy_content)){
    free(tag);
    parser->error = "ml666_hashed_buffer_set__add failed";
    return false;
  }
  if(!copy_content)
    *name = 0;
  if(stb->current_tag){
    tag->parent = stb->current_tag;
    stb->current_tag->next = tag;
    tag->previous = stb->current_tag;
    if(stb->current_tag->parent){
      stb->current_tag->parent->children_last = tag;
      if(!stb->current_tag->parent->children_first)
        stb->current_tag->parent->children_first = tag;
    }
  }
  stb->current_tag = tag;
  return true;
}

static bool end_tag_check(struct ml666_parser* parser, ml666_opaque_tag_name name){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  const struct ml666_hashed_buffer_set_entry* current_tag_name = stb->current_tag->name;
  if(current_tag_name->data.buffer.length == name->buffer.length && memcmp(name->buffer.data, current_tag_name->data.buffer.data, name->buffer.length) == 0)
    return true;
  return false;
}

static bool tag_pop(struct ml666_parser* parser){
  struct ml666_simple_tree_builder* stb = parser->user_ptr;
  if(stb->current_tag)
    stb->current_tag = stb->current_tag->parent;
  return true;
}

static bool init(struct ml666_parser* parser){
  parser->user_ptr = calloc(1, sizeof(struct ml666_simple_tree_builder));
  if(!parser->user_ptr){
    parser->error = "calloc failed";
    return false;
  }
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

int main(){
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
  ml666_parser_destroy(parser);
  return 0;
}
