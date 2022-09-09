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

bool ml666_buffer__append_p(struct ml666_buffer__append_args args){
  if(!args.data.length)
    return true;
  const size_t old_size = args.buffer->length;
  const size_t new_size = old_size + args.data.length;
  if(new_size < args.data.length)
    return false;
  char* content = args.realloc ? args.realloc(args.that, args.buffer->data, new_size) : realloc(args.buffer->data, new_size);
  if(!content)
    return false;
  args.buffer->data = content;
  memcpy(&content[old_size], args.data.data, args.data.length);
  args.buffer->length = new_size;
  return true;
}

bool ml666_hashed_buffer__append_p(struct ml666_hashed_buffer__append_args args){
  if(!args.data.length)
    return true;
  uint64_t hash = ml666_hash_FNV_1a_append(args.data, args.buffer->buffer.length ? args.buffer->hash : ML666_FNV_OFFSET_BASIS);
  if(!ml666_buffer__append((struct ml666_buffer*)&args.buffer->buffer, args.data, args.that, args.realloc))
    return false;
  args.buffer->hash = hash;
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
      fprintf(stderr, "ml666_hashed_buffer_set__d__add: ml666_buffer__dup failed\n");
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

void* ml666__d__realloc(void* that, void* data, size_t size){
  (void)that;
  return realloc(data, size);
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

bool utf8_validate(struct streaming_utf8_validator*restrict const v, const int _ch){
  const uint_least8_t ch = _ch & 0xFF;
  if(_ch == EOF){
    if(v->index)
      goto bad;
    return true;
  }
  if(!v->index){
    v->state = 0;
    if(ch < 0x80)
      return true;
    if(ch < 0xC2)
      goto bad;
    if(ch >= 0xFE) // Not yet defined
      goto bad;
    if(ch >= 0xF5) // Sequences above this value are defined, but not (yet?) allowed.
      goto bad;    // If you remove this condition, make sure to check for the overlong characters for those longer sequeces
    unsigned length = 1 + (ch >= 0xE0) + (ch >= 0xF0) + (ch >= 0xF8) + (ch >= 0xFC);
    v->index = length;
    if(length == 2){
      switch(ch){
        case 0xE0: v->state = 1; break; // Overlong
        case 0xED: v->state = 2; break; // CESU-8
        case 0xEF: v->state = 3; break; // noncharacter U+FFFE U+FFFF
      }
    }
    if(length == 3){
      switch(ch){
        case 0xF0: v->state = 5; break; // noncharacter
        case 0xF1: v->state = 8; break; // noncharacter
        case 0xF2: v->state = 8; break; // noncharacter
        case 0xF3: v->state = 8; break; // noncharacter
        case 0xF4: v->state = 6; break; // U+110000 - U+13FFFF, not yet used / allowed
      }
    }
    return true;
  }
  if((ch & 0xC0) != 0x80)
    goto bad;
  unsigned state = v->state;
  v->state = 0;
  switch(state){
    case  1: if(ch >= 0x80 && ch <= 0x9F) goto bad; break;
    case  2: if(ch >= 0xA0 && ch <= 0xBF) goto bad; break;
    case  3: {
      if(ch == 0xBF)
        v->state = 4;
      if(ch == 0xB7)
        v->state = 7;
    } break;
    case  4: if(ch == 0xBE || ch == 0xBF) goto bad; break;
    case  5: {
      if(ch < 0x90 || ch > 0xBF)
        goto bad;
      if(ch == 0x9F || ch == 0xAF || ch == 0xBF)
        v->state = 9;
    } break;
    case  6: {
      if(ch < 0x80 || ch > 0x8F)
        goto bad;
      if(ch == 0x8F)
        v->state = 9;
    } break;
    case  7: if(ch >= 0x90 && ch <= 0xAF) goto bad; break;
    case  8: if(ch == 0x8F || ch == 0x9F || ch == 0xAF || ch == 0xBF) v->state = 9; break;
    case  9: if(ch == 0xBF) v->state = 4; break;
  }
  v->index -= 1;
  return true;
bad:
  v->index = 0;
  return false;
}
