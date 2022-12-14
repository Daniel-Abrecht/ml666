#include <ml666/refcount.h>
#include <ml666/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUCKET_COUNT 512

static ml666_hashed_buffer_set__cb__lookup ml666_hashed_buffer_set__d__lookup;
static ml666_hashed_buffer_set__cb__put ml666_hashed_buffer_set__d__put;
static ml666_hashed_buffer_set__cb__destroy ml666_hashed_buffer_set__d__destroy;

struct ml666_hashed_buffer_set_entry {
  struct ml666_hashed_buffer data;
  struct ml666_refcount refcount;
  struct ml666_hashed_buffer_set_entry* next;
};

struct ml666_hashed_buffer_set_default {
  struct ml666_hashed_buffer_set super;
  struct ml666_refcount entry_count;
  struct ml666_hashed_buffer_set_entry* (*bucket)[BUCKET_COUNT];
  struct ml666_default_hashed_buffer_set__create_args a;
};

static unsigned hash_to_bucket(uint64_t hash){
  return ( (hash >> 0 ) ^ (hash >> 16) ^ (hash >> 32) ^ (hash >> 48) ) % BUCKET_COUNT;
}

bool ml666_buffer__dup_p(struct ml666_buffer__dup_args a){
  char* data = 0;
  if(a.src.length){
    data = (a.malloc ? a.malloc : ml666__d__malloc)(a.that, a.src.length);
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
  (a.free ? a.free : ml666__d__free)(a.that, a.buffer->data);
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
  char* content = (args.realloc ? args.realloc : ml666__d__realloc)(args.that, args.buffer->data, new_size);
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
  if(!(args.append?args.append:ml666_buffer__append_p)((struct ml666_buffer__append_args){
    .buffer = (struct ml666_buffer*)&args.buffer->buffer,
    .data = args.data,
    .that = args.that,
    .realloc = args.realloc
  })) return false;
  args.buffer->hash = hash;
  return true;
}

struct ml666_buffer_info ml666_buffer__analyze(struct ml666_buffer_ro buffer){
  struct ml666_buffer_info bi = {0};
  size_t length = buffer.length;
  while(length && buffer.data[length-1] == '\n')
    length--;
  bi.ends_with_newline = length != buffer.length;
  bi.ends_with_single_newline = buffer.length - length == 1;
  for(size_t i=0; i<length; i++){
    if(buffer.data[i] == '\n'){
      bi.really_multi_line = true;
      break;
    }
  }
  bi.multi_line = bi.ends_with_newline || bi.really_multi_line;
  bi.is_fully_ascii = true;
  for(size_t i=0; i<length; i++){
    if((unsigned char)buffer.data[i] < 0x80){
      bi.is_fully_ascii = false;
      break;
    }
  }
  for(size_t i=0; i<length; i++){
    if(!(unsigned char)buffer.data[i]){
      bi.has_null_bytes = true;
      break;
    }
  }
  bi.is_valid_utf8 = true;
  size_t approximated_escaped_overhead = 0;
  struct ml666_streaming_utf8_validator u8v;
  unsigned l = 0;
  for(size_t i=0; i<length; i++){
    {
      unsigned char ch = buffer.data[i];
      l = u8v.index ? l + 1 : 0;
      if(!ml666_utf8_validate(&u8v, ch)){
        bi.is_valid_utf8 = false;
        goto count_invalid;
      }
      // Those characters <0x20 are valid utf8, but we escape most of those anyway,
      // because having, for example, a null byte float around, isn't great.
      if(ch >= 0x20)
        continue;
    }
  count_invalid:
    for(size_t j=i-l; j<=i; j++){
      unsigned char ch = buffer.data[j];
      switch(ch){
        case ' ': case '\n': case '\t': break;
        case '\a': case '\b': case '\x1B':
        case '\v': case '\\': {
          approximated_escaped_overhead += 1;
        } break;
        default: approximated_escaped_overhead += 3; break;
      }
    }
  }
  size_t approximated_base64_overhead = (length + 2) / 3 + 3;
  bi.best_encoding = ML666_BIBE_ESCAPE;
  if(approximated_base64_overhead < approximated_escaped_overhead)
    bi.best_encoding = ML666_BIBE_BASE64;
  return bi;
}

static inline void destroy_entry(struct ml666_hashed_buffer_set_default* buffer_set, struct ml666_hashed_buffer_set_entry** it){
  struct ml666_hashed_buffer_set_entry* cur = *it;
  *it = cur->next;
  ml666_hashed_buffer__clear(&cur->data, buffer_set->a.that, buffer_set->a.free, buffer_set->a.clear_buffer);
  buffer_set->a.free(buffer_set->a.that, cur);
  if(!ml666_refcount_decrement(&buffer_set->entry_count))
    buffer_set->a.free(buffer_set->a.that, buffer_set->bucket);
}

void ml666_hashed_buffer_set__d__put(struct ml666_hashed_buffer_set* _buffer_set, const struct ml666_hashed_buffer_set_entry* _entry){
  struct ml666_hashed_buffer_set_default* buffer_set = (struct ml666_hashed_buffer_set_default*)_buffer_set;
  struct ml666_hashed_buffer_set_entry* entry = (struct ml666_hashed_buffer_set_entry*)_entry;
  if(ml666_refcount_decrement(&entry->refcount))
    return;
  unsigned bucket = hash_to_bucket(entry->data.hash);
  for(struct ml666_hashed_buffer_set_entry **it=&(*buffer_set->bucket)[bucket], *cur; (cur=*it); it=&cur->next){
    if(cur != entry)
      continue;
    destroy_entry(buffer_set, it);
    return;
  }
  fprintf(stderr, "%s:%u: error: entry not found in set\n", __FILE__, __LINE__);
  abort();
}

const struct ml666_hashed_buffer_set_entry* ml666_hashed_buffer_set__d__lookup(
  struct ml666_hashed_buffer_set* _buffer_set,
  const struct ml666_hashed_buffer* entry,
  enum ml666_hashed_buffer_set_mode mode
){
  struct ml666_hashed_buffer_set_default* buffer_set = (struct ml666_hashed_buffer_set_default*)_buffer_set;
  if(ml666_refcount_is_zero(&buffer_set->entry_count)){
    if(mode == ML666_HBS_M_GET)
      return 0;
    buffer_set->bucket = buffer_set->a.malloc(buffer_set->a.that, sizeof(*buffer_set->bucket));
    if(!buffer_set->bucket){
      perror("*malloc failed");
      return 0;
    }
    memset(buffer_set->bucket, 0, sizeof(*buffer_set->bucket));
  }
  ml666_refcount_increment(&buffer_set->entry_count);

  struct ml666_hashed_buffer c = *entry;
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
    ){
      if(mode == ML666_HBS_M_ADD_TAKE)
        ml666_hashed_buffer__clear(&c, buffer_set->a.that, buffer_set->a.free, buffer_set->a.clear_buffer);
      goto found;
    }
  }

  if(mode == ML666_HBS_M_GET)
    goto out;

  struct ml666_hashed_buffer_set_entry* result = buffer_set->a.malloc(buffer_set->a.that, sizeof(*result));
  if(!result){
    perror("*malloc failed");
    goto out;
  }
  if(mode == ML666_HBS_M_ADD_COPY){
    if(!ml666_hashed_buffer__dup(
      .dest = &result->data, .src = c,
      .that = buffer_set->a.that,
      .malloc = buffer_set->a.malloc,
      .dup = buffer_set->a.dup_buffer
    )){
      buffer_set->a.free(buffer_set->a.that, result);
      fprintf(stderr, "ml666_hashed_buffer_set__d__lookup: ml666_buffer__dup failed\n");
      goto out;
    }
    result->data.hash = c.hash;
  }else{
    result->data = *entry;
  }
  result->refcount.value = 0;
  ml666_refcount_increment(&result->refcount);
  result->next = cur;
  *it = result;
  return result;

found:;
  ml666_refcount_increment(&cur->refcount);
  if(0) out: cur=0;
  if(!ml666_refcount_decrement(&buffer_set->entry_count))
    buffer_set->a.free(buffer_set->a.that, buffer_set->bucket);
  return cur;
}

static const struct ml666_hashed_buffer_set_cb ml666_default_hashed_buffer_cb = {
  .lookup = ml666_hashed_buffer_set__d__lookup,
  .put = ml666_hashed_buffer_set__d__put,
  .destroy = ml666_hashed_buffer_set__d__destroy,
};

__attribute__((weak))
void* ml666__d__malloc(void* that, size_t size){
  (void)that;
  return malloc(size);
}

__attribute__((weak))
void* ml666__d__realloc(void* that, void* data, size_t size){
  (void)that;
  return realloc(data, size);
}

__attribute__((weak))
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

struct ml666_hashed_buffer_set* ml666_default_hashed_buffer_set__create_p(struct ml666_default_hashed_buffer_set__create_args args){
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
  for(unsigned i=0; i<BUCKET_COUNT; i++)
    while(bucket[i])
      destroy_entry(buffer_set, &bucket[i]);
  if(buffer_set != &default_buffer_set)
    buffer_set->a.free(buffer_set->a.that, buffer_set);
}

static struct ml666_hashed_buffer_set* get_default(void){
  return &default_buffer_set.super;
}
__attribute__((weak)) ml666_hashed_buffer_set__cb__get_default* ml666_hashed_buffer_set__get_default = &get_default;

bool ml666_utf8_validate(struct ml666_streaming_utf8_validator*restrict const v, const int _ch){
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

bool ml666_buffer__equal(struct ml666_buffer_ro a, struct ml666_buffer_ro b){
  if(a.length != b.length)
    return false;
  return !memcmp(a.data, b.data, a.length);
}
