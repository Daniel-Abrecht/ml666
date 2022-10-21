#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ml666/json-tokenizer.h>
#include <ml666/json-token-emmiter.h>
#include <ml666/utils.h>

enum ml666_has_document {
  ML666_DOCUMENT_PRESENCE_UNKNOWN,
  ML666_DOCUMENT_PRESENT,
  ML666_DOCUMENT_NOT_PRESENT,
};

enum ml666_state {
  LS_ENTRY,
  LS_ENTRY_TYPE,
  LS_ENTRY_NAME,
  LS_PROPERTY_LIST,
  LS_PROPERTY_LIST_ENTRY,
  LS_PROPERTY_NAME,
  LS_PROPERTY_VALUE,
  LS_PROPERTY_LIST_ENTRY_END,
  LS_CHILD_LIST,
  LS_ENTRY_END,
  LS_COMMENT,
  LS_COMMENT_END,
  LS_CONTENT,
  LS_SKIP,
};

enum ml666_base64_state {
  ML666_BASE64_OFF,
  ML666_BASE64_EXPECT_MAGIC,
  ML666_BASE64_DECODE,
};

struct ml666__json_token_emmiter_private {
  struct ml666_tokenizer public;
  struct ml666_json_tokenizer* json_tokenizer;
  enum ml666_state state;
  ml666__cb__malloc* malloc;
  ml666__cb__free*   free;
  enum ml666_has_document has_document;
  size_t level;
  size_t skip;
  enum ml666_base64_state base64;
  char base64_modulo; // 0..3
  char base64_akku;
};
static_assert(offsetof(struct ml666__json_token_emmiter_private, public) == 0, "ml666__json_token_emmiter_private::public must be the first member");

static ml666_tokenizer_cb_next ml666_json_token_emmiter_d_next;
static ml666_tokenizer_cb_destroy ml666_json_token_emmiter_d_destroy;

static const struct ml666_tokenizer_cb tokenizer_cb = {
  .next = ml666_json_token_emmiter_d_next,
  .destroy = ml666_json_token_emmiter_d_destroy,
};

// +/ are part of base64, -_ is used in base64url
static signed char b64_to_num(char x){
  if(x >= 'A' && x <= 'Z')
    return x - 'A';
  if(x >= 'a' && x <= 'z')
    return x - 'a' + 26;
  if(x >= '0' && x <= '9')
    return x - '0' + 52;
  if(x == '+' || x == '-')
    return 62;
  if(x == '/' || x == '_')
    return 63;
   return -1;
}

struct ml666_tokenizer* ml666_json_token_emmiter_create_p(struct ml666_json_token_emmiter_create_args args){
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666__json_token_emmiter_private*restrict jte = args.malloc(args.user_ptr, sizeof(*jte));
  if(!jte){
    fprintf(stderr, "%s:%u: malloc failed (%d): %s\n", __FILE__, __LINE__, errno, strerror(errno));
    goto error;
  }
  memset(jte, 0, sizeof(*jte));
  *(const struct ml666_tokenizer_cb**)&jte->public.cb = &tokenizer_cb;
  jte->malloc = args.malloc;
  jte->free = args.free;
  jte->public.user_ptr = args.user_ptr;
  jte->public.line = 1;
  jte->public.column = 1;

  jte->json_tokenizer = ml666_json_tokenizer_create(
    .fd = args.fd,
    .user_ptr = args.user_ptr,
    .malloc = args.malloc,
    .free = args.free,
    .disable_utf8_validation = args.disable_utf8_validation,
    .no_objects = true
  );
  if(!jte->json_tokenizer)
    goto error_after_malloc;

  return &jte->public;

error_after_malloc:
  args.free(args.user_ptr, jte);
error:
  close(args.fd);
  return 0;
}

static bool ml666_json_token_emmiter_d_next(struct ml666_tokenizer* _tokenizer){
  struct ml666__json_token_emmiter_private*restrict const jte = (struct ml666__json_token_emmiter_private*)_tokenizer;
  jte->public.token = ML666_NONE;
  jte->public.match = (struct ml666_buffer_ro){0};
  struct ml666_json_tokenizer*restrict const json = jte->json_tokenizer;
  if(!json)
    return false;
  while(!jte->public.token){
    bool res = ml666_json_tokenizer_next(json);
    jte->public.line = json->line;
    jte->public.column = json->column;
    if(!res){
      jte->public.error = json->error;
      return false;
    }
    if(!json->token)
      break;
  retry:
    switch(jte->base64){
      case ML666_BASE64_OFF: break;
      case ML666_BASE64_EXPECT_MAGIC: {
        if( json->token == ML666_JSON_STRING && json->complete
         && json->match_ro.length == 1 && json->match_ro.data[0] == 'B'
        ){
          jte->base64 = ML666_BASE64_DECODE;
        }else{
          jte->public.error = "schema error: expected string \"B\"";
          goto error;
        }
      } continue;
      case ML666_BASE64_DECODE: {
        if(json->token == ML666_JSON_STRING){
          size_t out = 0;
          unsigned mod = jte->base64_modulo % 4;
          char* memory = json->match.data;
          for(size_t in=0,n=json->match_ro.length; in<n; in++){
            unsigned char ch = json->match_ro.data[in];
            char num = b64_to_num(ch);
            mod += 1;
            if(ch == '=' && (mod == 3 || mod == 4))
              num = 0;
            if(num < 0){
              jte->public.error = "syntax error: invalid character in base64/base64url encoded text";
              goto error;
            }
            switch(mod){
              case 1: {
                jte->base64_akku = num;
              } break;
              case 2: {
                memory[out++] = (jte->base64_akku<<2) | (num>>4);
                jte->base64_akku = num & 0x0F;
              } break;
              case 3: {
                if(ch != '='){
                  memory[out++] = (jte->base64_akku<<4) | (num>>2);
                  jte->base64_akku = num & 0x03;
                }else{
                  jte->base64_akku = 64;
                }
              } break;
              case 4: {
                if(ch != '='){
                  if(jte->base64_akku == 64){
                    jte->public.error = "syntax error: unexpected character while decoding base64: expected =";
                    goto error;
                  }
                  mod = 0;
                  memory[out++] = (jte->base64_akku<<6) | num;
                  jte->base64_akku = 0;
                }
              } break;
              case 5: {
                jte->public.error = "syntax error: unexpected character while decoding base64: got a character after the final =";
                goto error;
              } break;
            }
            jte->base64_akku = num;
          }
          jte->base64_modulo = mod;
          json->match.length = out;
          json->match_ro.length = out;
        }else if(json->token == ML666_JSON_ARRAY_END){
          jte->base64_modulo = 0;
          jte->base64 = ML666_BASE64_OFF;
          continue;
        }else{
          jte->public.error = "schema error: expected string or end of array";
          goto error;
        }
      } break;
    }
    switch(jte->state){
      case LS_ENTRY: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->state = LS_ENTRY_TYPE;
        }else if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_ENTRY_END;
        }else if(json->token == ML666_JSON_STRING){
          jte->state = LS_CONTENT;
          if(jte->has_document == ML666_DOCUMENT_PRESENCE_UNKNOWN)
            jte->has_document = ML666_DOCUMENT_NOT_PRESENT;
          goto retry;
        }else{
          jte->public.error = "schema error: expected object, array or string";
          goto error;
        }
      } break;
      case LS_ENTRY_TYPE: {
        if(json->token != ML666_JSON_STRING || !json->complete || json->match_ro.length != 1){
          jte->public.error = "schema error: expected entry type (as a 1 char big string)";
          goto error;
        }
        switch(json->match_ro.data[0]){
          case 'D': {
            if(jte->has_document != ML666_DOCUMENT_PRESENCE_UNKNOWN){
              jte->public.error = "schema error: there can only be 1 ml666 document";
              goto error;
            }
            jte->has_document = ML666_DOCUMENT_PRESENT;
            jte->state = LS_CHILD_LIST;
            jte->level += 1;
          } break;
          case 'E': {
            if(jte->has_document == ML666_DOCUMENT_PRESENCE_UNKNOWN)
              jte->has_document = ML666_DOCUMENT_NOT_PRESENT;
            jte->state = LS_ENTRY_NAME;
          } break;
          case 'C': {
            jte->state = LS_COMMENT;
          } break;
          case 'B': {
            jte->base64 = ML666_BASE64_DECODE;
            jte->state = LS_CONTENT;
            if(jte->has_document == ML666_DOCUMENT_PRESENCE_UNKNOWN)
              jte->has_document = ML666_DOCUMENT_NOT_PRESENT;
          } break;
        }
      } break;
      case LS_ENTRY_NAME: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->base64 = ML666_BASE64_EXPECT_MAGIC;
        }else if(json->token == ML666_JSON_STRING){
          jte->public.token = ML666_TAG;
          jte->public.match = json->match_ro;
          jte->public.complete = json->complete;
          if(json->complete){
            jte->state = LS_PROPERTY_LIST;
            if(!++jte->level){
              jte->public.error = "error: tree level overflow";
              goto error;
            }
          }
        }else{
          jte->public.error = "schema error: expected element name";
          goto error;
        }
      } break;
      case LS_PROPERTY_LIST: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->state = LS_PROPERTY_LIST_ENTRY;
        }else if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_ENTRY_END;
          goto retry;
        }else{
          jte->public.error = "schema error: expected array of properties";
          goto error;
        }
      } break;
      case LS_PROPERTY_LIST_ENTRY: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->state = LS_PROPERTY_NAME;
        }else if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_CHILD_LIST;
        }else{
          jte->public.error = "schema error: expected property (a key value pair as a JSON array)";
          goto error;
        }
      } break;
      case LS_PROPERTY_NAME: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->base64 = ML666_BASE64_EXPECT_MAGIC;
        }else if(json->token == ML666_JSON_STRING && json->match_ro.length){
          jte->public.token = ML666_ATTRIBUTE;
          jte->public.match = json->match_ro;
          jte->public.complete = json->complete;
          if(json->complete)
            jte->state = LS_PROPERTY_VALUE;
        }else{
          jte->public.error = "schema error: expected attribute name";
          goto error;
        }
      } break;
      case LS_PROPERTY_VALUE: {
        if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_PROPERTY_LIST_ENTRY;
        }else if(json->token == ML666_JSON_NULL){
          jte->state = LS_PROPERTY_LIST_ENTRY_END;
        }else if(json->token == ML666_JSON_ARRAY_START){
          jte->base64 = ML666_BASE64_EXPECT_MAGIC;
        }else if(json->token == ML666_JSON_STRING){
          jte->public.token = ML666_ATTRIBUTE_VALUE;
          jte->public.match = json->match_ro;
          jte->public.complete = json->complete;
          if(json->complete)
            jte->state = LS_PROPERTY_LIST_ENTRY_END;
        }else{
          jte->public.error = "schema error: expected attribute name";
          goto error;
        }
      } break;
      case LS_PROPERTY_LIST_ENTRY_END: {
        if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_PROPERTY_LIST_ENTRY;
        }else{
          jte->public.error = "schema error: expected end of array";
          goto error;
        }
      } break;
      case LS_CHILD_LIST: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->state = LS_ENTRY;
        }else if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_ENTRY_END;
          goto retry;
        }else{
          jte->public.error = "schema error: expected array of child nodes";
          goto error;
        }
      } break;
      case LS_CONTENT: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->base64 = ML666_BASE64_EXPECT_MAGIC;
        }else if(json->token == ML666_JSON_STRING && json->match_ro.length){
          jte->public.token = ML666_TEXT;
          jte->public.match = json->match_ro;
          jte->public.complete = json->complete;
          if(json->complete)
            jte->state = LS_ENTRY;
        }else{
          jte->public.error = "schema error: expected content";
          goto error;
        }
      } break;
      case LS_COMMENT: {
        if(json->token == ML666_JSON_ARRAY_START){
          jte->base64 = ML666_BASE64_EXPECT_MAGIC;
        }else if(json->token == ML666_JSON_STRING && json->match_ro.length){
          jte->public.token = ML666_COMMENT;
          jte->public.match = json->match_ro;
          jte->public.complete = json->complete;
          if(json->complete)
            jte->state = LS_COMMENT_END;
        }else{
          jte->public.error = "schema error: expected comment";
          goto error;
        }
      } break;
      case LS_COMMENT_END: {
        if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_ENTRY;
        }else{
          jte->public.error = "schema error: expected end of array";
          goto error;
        }
      } break;
      case LS_ENTRY_END: {
        if(!jte->level){
          jte->public.error = "error: tree level underflow";
          goto error;
        }
        jte->level -= 1;
        if(json->token == ML666_JSON_ARRAY_END){
          jte->state = LS_ENTRY;
          if(jte->level || jte->has_document == ML666_DOCUMENT_NOT_PRESENT){
            jte->public.token = ML666_END_TAG;
            jte->public.complete = true;
          }
        }else{
          jte->public.error = "schema error: expected end of array";
          goto error;
        }
      } break;
      case LS_SKIP: {
        if( json->token == ML666_JSON_OBJECT_START
         || json->token == ML666_JSON_ARRAY_START
        ){
          if(!++jte->skip){
            jte->public.error = "error: can't handle a JSON nested so deep";
            goto error;
          }
        }else if( json->token == ML666_JSON_OBJECT_END
               || json->token == ML666_JSON_ARRAY_END
        ){
          if(!--jte->skip)
            jte->state = LS_ENTRY;
        }
      } break;
    }
  }
  return true;
error:
  ml666_json_tokenizer_destroy(json);
  jte->json_tokenizer = 0;
  return false;
}

static void ml666_json_token_emmiter_d_destroy(struct ml666_tokenizer* _tokenizer){
  if(!_tokenizer) return;
  struct ml666__json_token_emmiter_private*restrict jte = (struct ml666__json_token_emmiter_private*)_tokenizer;
  if(jte->json_tokenizer)
    ml666_json_tokenizer_destroy(jte->json_tokenizer);
  jte->free(jte->public.user_ptr, jte);
}
