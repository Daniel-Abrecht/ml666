#include <ml666/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 512

static ml666_hashed_buffer_set__cb__add ml666_hashed_buffer_set__d__add;
static ml666_hashed_buffer_set__cb__put ml666_hashed_buffer_set__d__put;
static ml666_hashed_buffer_set__cb__destroy ml666_hashed_buffer_set__d__destroy;

struct ml666_hashed_buffer_set_entry {
  struct ml666_hashed_buffer data;
  size_t refcount;
  struct ml666_hashed_buffer_set_entry* next;
};

struct ml666_hashed_buffer_set_default {
  struct ml666_hashed_buffer_set super;
  size_t entry_count;
  struct ml666_hashed_buffer_set_entry* (*bucket)[BUCKET_COUNT];
  struct ml666_create_default_hashed_buffer_set_args a;
};

static unsigned hash_to_bucket(uint64_t hash){
  return ( (hash >> 0 ) ^ (hash >> 16) ^ (hash >> 32) ^ (hash >> 48) ) % BUCKET_COUNT;
}

bool ml666_buffer__dup_p(struct ml666_buffer__dup_args a){
  char* data = 0;
  if(a.src.length){
    data = a.malloc ? a.malloc(a.that, a.src.length) : malloc(a.src.length);
    if(!data){
      perror("*malloc failed");
      return false;
    }
    memcpy(data, a.src.data, a.src.length);
  }
  a.dest->data = data;
  a.dest->length = a.src.length;
  return true;
}

bool ml666_hashed_buffer__dup_p(struct ml666_hashed_buffer__dup_args a){
  if(!(a.dup ? a.dup : ml666_buffer__dup_p)(
    (struct ml666_buffer__dup_args){
      .dest = (struct ml666_buffer*)&a.dest->buffer,
      .src = a.src.buffer,
      .that = a.that,
      .malloc = a.malloc,
    }
  )) return false;
  a.dest->hash = a.src.hash;
  return true;
}

bool ml666_buffer__clear_p(struct ml666_buffer__clear_args a){
  if(a.free){
    a.free(a.that, a.buffer->data);
  }else{
    free(a.buffer->data);
  }
  a.buffer->data = 0;
  a.buffer->length = 0;
  return true;
}

bool ml666_hashed_buffer__clear_p(struct ml666_hashed_buffer__clear_args a){
  (a.clear ? a.clear : ml666_buffer__clear_p)(
    (struct ml666_buffer__clear_args){
      .buffer = (struct ml666_buffer*)&a.buffer->buffer,
      .that = a.that,
      .free = a.free,
    }
  );
  a.buffer->hash = 0;
  return true;
}

void ml666_hashed_buffer_set__d__put(struct ml666_hashed_buffer_set* _buffer_set, const struct ml666_hashed_buffer_set_entry* _entry){
  struct ml666_hashed_buffer_set_default* buffer_set = (struct ml666_hashed_buffer_set_default*)_buffer_set;
  struct ml666_hashed_buffer_set_entry* entry = (struct ml666_hashed_buffer_set_entry*)_entry;
  if((entry->refcount -= 1))
    return;
  unsigned bucket = hash_to_bucket(entry->data.hash);
  for(struct ml666_hashed_buffer_set_entry **it=&(*buffer_set->bucket)[bucket], *cur; (cur=*it); it=&cur->next){
    if(cur != entry)
      continue;
    *it = cur->next;
    ml666_hashed_buffer__clear(&cur->data, buffer_set->a.that, buffer_set->a.free, buffer_set->a.clear_buffer);
    buffer_set->a.free(buffer_set->a.that, cur);
    if(!(buffer_set->entry_count -= 1))
      buffer_set->a.free(buffer_set->a.that, buffer_set->bucket);
    return;
  }
  fprintf(stderr, "%s:%u: error: entry not found in set\n", __FILE__, __LINE__);
  abort();
}

const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__d__add(struct ml666_hashed_buffer_set* _buffer_set, const struct ml666_hashed_buffer* entry, bool copy_content){
  struct ml666_hashed_buffer_set_default* buffer_set = (struct ml666_hashed_buffer_set_default*)_buffer_set;
  if(!buffer_set->entry_count++){
    buffer_set->bucket = buffer_set->a.malloc(buffer_set->a.that, sizeof(*buffer_set->bucket));
    if(!buffer_set->bucket){
      perror("*malloc failed");
      return false;
    }
    memset(buffer_set->bucket, 0, sizeof(*buffer_set->bucket));
  }

  const struct ml666_hashed_buffer c = *entry;
  unsigned bucket = hash_to_bucket(c.hash);

  struct ml666_hashed_buffer_set_entry **it, *cur;
  for(it=&(*buffer_set->bucket)[bucket]; (cur=*it); it=&cur->next){
    struct ml666_hashed_buffer_set_entry* cur = *it;
    if(cur->data.hash > c.hash)
      break;
    if(&cur->data == entry)
      goto found;
    if( cur->data.hash == c.hash
     && cur->data.buffer.length == c.buffer.length
     && memcmp(cur->data.buffer.data, c.buffer.data, c.buffer.length) == 0
    ) goto found;
  }

  struct ml666_hashed_buffer_set_entry* result = buffer_set->a.malloc(buffer_set->a.that, sizeof(*result));
  if(!result){
    perror("*malloc failed");
    return 0;
  }
  if(copy_content){
    if(!ml666_hashed_buffer__dup(
      .dest = &result->data, .src = c,
      .that = buffer_set->a.that,
      .malloc = buffer_set->a.malloc,
      .dup = buffer_set->a.dup_buffer
    )){
      buffer_set->a.free(buffer_set->a.that, result);
      fprintf(stderr, "ml666_hashed_buffer_set__d__add: ml666_buffer__dup failed");
      return 0;
    }
    result->data.hash = c.hash;
  }else{
    result->data = *entry;
  }
  result->refcount = 1;
  result->next = cur;
  *it = result;
  return result;

found:;
  const size_t refcount = cur->refcount + 1;
  if(!refcount)
    return false;
  cur->refcount = refcount;
  return cur;
}

static const struct ml666_hashed_buffer_set_cb ml666_default_hashed_buffer_cb = {
  .add = ml666_hashed_buffer_set__d__add,
  .put = ml666_hashed_buffer_set__d__put,
  .destroy = ml666_hashed_buffer_set__d__destroy,
};

void* ml666__d__malloc(void* that, size_t size){
  (void)that;
  return malloc(size);
}

void ml666__d__free(void* that, void* data){
  (void)that;
  free(data);
}

static struct ml666_hashed_buffer_set_default default_buffer_set = {
  .super.cb = &ml666_default_hashed_buffer_cb,
  .a = {
    .malloc = ml666__d__malloc,
    .free = ml666__d__free,
    .dup_buffer = ml666_buffer__dup_p,
    .clear_buffer = ml666_buffer__clear_p,
  }
};

struct ml666_hashed_buffer_set* ml666_create_default_hashed_buffer_set_p(struct ml666_create_default_hashed_buffer_set_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  if(!args.dup_buffer)
    args.dup_buffer = ml666_buffer__dup_p;
  if(!args.clear_buffer)
    args.clear_buffer = ml666_buffer__clear_p;
  struct ml666_hashed_buffer_set_default* buffer_set = args.malloc(args.that, sizeof(*buffer_set));
  if(!buffer_set){
    perror("*malloc failed");
    return 0;
  }
  memset(buffer_set, 0, sizeof(*buffer_set));
  *(const struct ml666_hashed_buffer_set_cb**)&buffer_set->super.cb = &ml666_default_hashed_buffer_cb;
  buffer_set->a = args;
  return &buffer_set->super;
}

void ml666_hashed_buffer_set__d__destroy(struct ml666_hashed_buffer_set* _buffer_set){
  struct ml666_hashed_buffer_set_default* buffer_set = (struct ml666_hashed_buffer_set_default*)_buffer_set;
  struct ml666_hashed_buffer_set_entry** bucket = *buffer_set->bucket;
  for(unsigned i=0; i<BUCKET_COUNT; i++){
    for(struct ml666_hashed_buffer_set_entry* entry; (entry=bucket[i]); ){
      entry->refcount = 1;
      ml666_hashed_buffer_set__put(&buffer_set->super, entry);
    }
  }
  if(buffer_set != &default_buffer_set)
    buffer_set->a.free(buffer_set->a.that, buffer_set);
}

struct ml666_hashed_buffer_set* ml666_get_default_hashed_buffer_set(void){
  return &default_buffer_set.super;
}
