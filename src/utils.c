#include <ml666/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ml666_hashed_buffer_set {
  struct ml666_hashed_buffer_set_entry* bucket[512];
};

static struct ml666_hashed_buffer_set buffer_set;

static unsigned hash_to_bucket(uint64_t hash){
  return ( (hash >> 0 ) ^ (hash >> 16) ^ (hash >> 32) ^ (hash >> 48) ) & 0x1FF;
}

void ml666_hashed_buffer_set__put(const struct ml666_hashed_buffer_set_entry* _entry){
  struct ml666_hashed_buffer_set_entry* entry = (struct ml666_hashed_buffer_set_entry*)_entry;
  if((entry->refcount -= 1))
    return;
  unsigned bucket = hash_to_bucket(entry->data.hash);
  for(struct ml666_hashed_buffer_set_entry **it=&buffer_set.bucket[bucket], *cur; (cur=*it); it=&cur->next){
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
  const struct ml666_hashed_buffer* entry,
  bool copy_content
){
  const struct ml666_hashed_buffer c = *entry;
  unsigned bucket = hash_to_bucket(c.hash);

  struct ml666_hashed_buffer_set_entry **it, *cur;
  for(it=&buffer_set.bucket[bucket]; (cur=*it); it=&cur->next){
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
  result->refcount = 1;
  result->next = cur;
  *it = result;
  *out = result;
  return true;

found:;
  const size_t refcount = cur->refcount + 1;
  if(!refcount)
    return false;
  cur->refcount = refcount;
  *out = cur;
  return true;
}
