#include <ml666/utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ml666_hashed_buffer_set_entry {
  struct ml666_hashed_buffer data;
  size_t refcount;
  struct ml666_hashed_buffer_set_entry* next;
};

struct ml666_hashed_buffer_set {
  size_t entry_count;
  struct ml666_hashed_buffer_set_entry* (*bucket)[512];
};

static struct ml666_hashed_buffer_set buffer_set;

static unsigned hash_to_bucket(uint64_t hash){
  return ( (hash >> 0 ) ^ (hash >> 16) ^ (hash >> 32) ^ (hash >> 48) ) & 0x1FF;
}

static void put(const struct ml666_hashed_buffer_set_entry* _entry){
  struct ml666_hashed_buffer_set_entry* entry = (struct ml666_hashed_buffer_set_entry*)_entry;
  if((entry->refcount -= 1))
    return;
  unsigned bucket = hash_to_bucket(entry->data.hash);
  for(struct ml666_hashed_buffer_set_entry **it=&(*buffer_set.bucket)[bucket], *cur; (cur=*it); it=&cur->next){
    if(cur != entry)
      continue;
    *it = cur->next;
    free((void*)cur->data.buffer.data);
    free(cur);
    if(!(buffer_set.entry_count -= 1))
      free(buffer_set.bucket);
    return;
  }
  fprintf(stderr, "%s:%u: error: entry not found in set\n", __FILE__, __LINE__);
  abort();
}

static const struct ml666_hashed_buffer_set_entry* add(const struct ml666_hashed_buffer* entry, bool copy_content){
  if(!buffer_set.entry_count++){
    buffer_set.bucket = calloc(1, sizeof(*buffer_set.bucket));
    if(!buffer_set.bucket){
      perror("calloc failed");
      return false;
    }
  }

  const struct ml666_hashed_buffer c = *entry;
  unsigned bucket = hash_to_bucket(c.hash);

  struct ml666_hashed_buffer_set_entry **it, *cur;
  for(it=&(*buffer_set.bucket)[bucket]; (cur=*it); it=&cur->next){
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
    return 0;
  }
  if(copy_content){
    char* data = malloc(c.buffer.length);
    if(!data){
      free(result);
      perror("malloc failed");
      return 0;
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
  return result;

found:;
  const size_t refcount = cur->refcount + 1;
  if(!refcount)
    return false;
  cur->refcount = refcount;
  return cur;
}

const struct ml666_hashed_buffer_set_api ml666_hashed_buffer_set = {
  .add = add,
  .put = put,
};
