#include <ml666/parser.h>
#include <ml666/tokenizer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

ML666_DEFAULT_OPAQUE_TAG_NAME
ML666_DEFAULT_OPAQUE_ATTRIBUTE_NAME

struct ml666__parser_private {
  struct ml666_parser public;
  struct ml666_tokenizer* tokenizer;
  bool nonempty_token;

  // Opaque pointers. The callback handlers may store a pointer to anything in them.
  struct {
    union {
      ml666_opaque_tag_name tag_name;
      ml666_opaque_attribute_name attribute_name;
    };
  } state;

  // This is what the default handlers will let the state point to.
  struct {
    union {
      struct ml666_opaque_tag_name tag_name;
      struct ml666_opaque_attribute_name attribute_name;
    };
  } default_hander_state;
};
static_assert(offsetof(struct ml666__parser_private, public) == 0);

static bool ml666_parser_a_init(struct ml666__parser_private* that){
  if(that->public.cb->init)
    return that->public.cb->init(&that->public);
  return true;
}

static void ml666_parser_a_done(struct ml666__parser_private* that){
  if(that->public.cb->done)
    that->public.cb->done(&that->public);
  ml666_tokenizer_destroy(that->tokenizer);
  that->tokenizer = 0;
}

static void ml666_parser_a_cleanup(struct ml666__parser_private* that){
  if(that->public.cb->cleanup)
    that->public.cb->cleanup(&that->public);
  ml666_tokenizer_destroy(that->tokenizer);
}

static void ml666_parser__buffer_free(struct ml666_buffer* name){
  if(!name)
    return;
  if(name->data){
    free(name->data);
    name->data = 0;
    name->length = 0;
  }
}

static bool ml666_parser__buffer_append(struct ml666_buffer* buffer, struct ml666_buffer_ro data){
  if(!data.length)
    return true;
  const size_t old_size = buffer->length;
  const size_t new_size = old_size + data.length;
  if(new_size < data.length)
    return false;
  char* content = realloc(buffer->data, new_size);
  if(!content)
    return false;
  buffer->data = content;
  memcpy(&content[old_size], data.data, data.length);
  buffer->length = new_size;
  return true;
}

bool ml666_parser__d_mal__tag_name_append(struct ml666_parser* _that, ml666_opaque_tag_name* name, struct ml666_buffer_ro data){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!*name)
    *name = &that->default_hander_state.tag_name;
  return ml666_parser__buffer_append(&(*name)->buffer, data);
}

bool ml666_parser__d_mal__attribute_name_append(struct ml666_parser* _that, ml666_opaque_attribute_name* name, struct ml666_buffer_ro data){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!*name)
    *name = &that->default_hander_state.attribute_name;
  return ml666_parser__buffer_append(&(*name)->buffer, data);
}

void ml666_parser__d_mal__tag_name_free(struct ml666_parser* that, ml666_opaque_tag_name name){
  (void)that;
  ml666_parser__buffer_free(&name->buffer);
}

void ml666_parser__d_mal__attribute_name_free(struct ml666_parser* that, ml666_opaque_attribute_name name){
  (void)that;
  ml666_parser__buffer_free(&name->buffer);
}

static bool ml666_parser_a_tag_name_append(struct ml666__parser_private* that, ml666_opaque_tag_name* name, struct ml666_buffer_ro data){
  return that->public.cb->tag_name_append(&that->public, name, data);
}

static void ml666_parser_a_tag_name_free(struct ml666__parser_private* that, ml666_opaque_tag_name name){
  if(that->public.cb->tag_name_free)
    that->public.cb->tag_name_free(&that->public, name);
}

static bool ml666_parser_a_attribute_name_append(struct ml666__parser_private* that, ml666_opaque_attribute_name* name, struct ml666_buffer_ro data){
  return that->public.cb->attribute_name_append(&that->public, name, data);
}

static void ml666_parser_a_attribute_name_free(struct ml666__parser_private* that, ml666_opaque_attribute_name name){
  if(that->public.cb->attribute_name_free)
    that->public.cb->attribute_name_free(&that->public, name);
}

static bool ml666_parser_a_tag_push(struct ml666__parser_private* that, ml666_opaque_tag_name* name){
  return that->public.cb->tag_push(&that->public, name);
}

static bool ml666_parser_a_end_tag_check(struct ml666__parser_private* that, ml666_opaque_tag_name name){
  return that->public.cb->end_tag_check(&that->public, name);
}

static bool ml666_parser_a_tag_pop(struct ml666__parser_private* that){
  return that->public.cb->tag_pop(&that->public);
}

static bool ml666_parser_a_set_attribute(struct ml666__parser_private* that, ml666_opaque_attribute_name* name){
  if(that->public.cb->set_attribute)
    return that->public.cb->set_attribute(&that->public, name);
  return true;
}

static bool ml666_parser_a_data_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.cb->data_append)
    return that->public.cb->data_append(&that->public, data);
  return true;
}

static bool ml666_parser_a_value_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.cb->value_append)
    return that->public.cb->value_append(&that->public, data);
  return true;
}

static bool ml666_parser_a_comment_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.cb->comment_append)
    return that->public.cb->comment_append(&that->public, data);
  return true;
}

struct ml666_parser* ml666_parser_create_p(struct ml666_parser_create_args args){
  bool fail = false;
  if(args.fd < 0){
    fprintf(stderr, "ml666_parser_create_p: invalid fd arguent\n");
    fail = true;
  }
  if(!args.cb){
    fprintf(stderr, "ml666_parser_create_p: argument cb must be set\n");
    fail = true;
  }else{
    if(!args.cb->attribute_name_append){
      fprintf(stderr, "ml666_parser_create_p: callback attribute_name_append is mandatory\n");
      fail = true;
    }
    if(!args.cb->tag_name_append){
      fprintf(stderr, "ml666_parser_create_p: callback tag_name_append is mandatory\n");
      fail = true;
    }
    if(!args.cb->tag_push){
      fprintf(stderr, "ml666_parser_create_p: callback tag_push is mandatory\n");
      fail = true;
    }
    if(!args.cb->end_tag_check){
      fprintf(stderr, "ml666_parser_create_p: callback end_tag_check is mandatory\n");
      fail = true;
    }
    if(!args.cb->tag_pop){
      fprintf(stderr, "ml666_parser_create_p: callback tag_pop is mandatory\n");
      fail = true;
    }
  }
  if(fail)
    goto error;
  struct ml666__parser_private* parser = calloc(1, sizeof(*parser));
  if(!parser){
    fprintf(stderr, "ml666_parser_create_p: calloc failed (%d): %s\n", errno, strerror(errno));
    goto error;
  }
  *(const struct ml666_parser_cb**)&parser->public.cb = args.cb;
  parser->public.user_ptr = args.user_ptr;
  parser->tokenizer = ml666_tokenizer_create(args.fd);
  args.fd = -1;
  if(!parser->tokenizer)
    goto error_after_calloc;
  if(!ml666_parser_a_init(parser))
    goto error_after_tokenizer;
  return &parser->public;

error_after_tokenizer:
  ml666_tokenizer_destroy(parser->tokenizer);
error_after_calloc:
  free(parser);
error:
  if(args.fd >= 0)
    close(args.fd);
  return 0;
}

bool ml666_parser_next(struct ml666_parser* _parser){
  struct ml666__parser_private*restrict parser = (struct ml666__parser_private*)_parser;
  if(parser->tokenizer->token == ML666_EOF)
    return false;
  struct ml666_tokenizer* tokenizer = parser->tokenizer;
  bool done = !ml666_tokenizer_next(tokenizer);
  parser->public.error = tokenizer->error;
  if(tokenizer->match.length)
    parser->nonempty_token = true;
  switch(tokenizer->token){
    case ML666_NONE: break;
    case ML666_EOF: break;
    case ML666_TAG: {
      if(!ml666_parser_a_tag_name_append(parser, &parser->state.tag_name, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::tag_name_append failed";
        done = true;
        break;
      }
      if(tokenizer->complete){
        if(!ml666_parser_a_tag_push(parser, &parser->state.tag_name)){
          if(!parser->public.error)
            parser->public.error = "ml666_parser::tag_push failed";
          done = true;
          break;
        }
        if(parser->state.tag_name){
          ml666_parser_a_tag_name_free(parser, parser->state.tag_name);
          parser->state.tag_name = 0;
        }
      }
    } break;
    case ML666_END_TAG: {
      if(!ml666_parser_a_tag_name_append(parser, &parser->state.tag_name, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::tag_name_append failed";
        done = true;
        break;
      }
      if(tokenizer->complete){
        if(parser->nonempty_token){
          if(!ml666_parser_a_end_tag_check(parser, parser->state.tag_name)){
            if(!parser->public.error)
              parser->public.error = "ml666_parser::end_tag_check failed, did the opening / closing tags missmatch?";
            done = true;
            break;
          }
        }
        ml666_parser_a_tag_name_free(parser, parser->state.tag_name);
        parser->state.tag_name = 0;
        if(!ml666_parser_a_tag_pop(parser)){
          if(!parser->public.error)
            parser->public.error = "ml666_parser::tag_pop failed";
          done = true;
          break;
        }
      }
    } break;
    case ML666_ATTRIBUTE: {
      if(!ml666_parser_a_attribute_name_append(parser, &parser->state.attribute_name, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::attribute_name_append failed";
        done = true;
        break;
      }
      if(tokenizer->complete){
        if(!ml666_parser_a_set_attribute(parser, &parser->state.attribute_name)){
          if(!parser->public.error)
            parser->public.error = "ml666_parser::set_attribute failed";
          done = true;
          break;
        }
        if(parser->state.attribute_name){
          ml666_parser_a_attribute_name_free(parser, parser->state.attribute_name);
          parser->state.attribute_name = 0;
        }
      }
    } break;
    case ML666_ATTRIBUTE_VALUE: {
      if(!ml666_parser_a_value_append(parser, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::value_append failed";
        done = true;
        break;
      }
    } break;
    case ML666_TEXT: {
      if(!ml666_parser_a_data_append(parser, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::data_append failed";
        done = true;
        break;
      }
    } break;
    case ML666_COMMENT: {
      if(!ml666_parser_a_comment_append(parser, tokenizer->match)){
        if(!parser->public.error)
          parser->public.error = "ml666_parser::comment_append failed";
        done = true;
        break;
      }
    } break;
    case ML666_TOKEN_COUNT: abort();
  }
  if(tokenizer->complete)
    parser->nonempty_token = false;
  if(done){
    if(parser->state.tag_name)
      ml666_parser_a_tag_name_free(parser, parser->state.tag_name);
    if(parser->state.attribute_name)
      ml666_parser_a_attribute_name_free(parser, parser->state.attribute_name);
    parser->public.done = true;
    ml666_parser_a_done(parser);
    return false;
  }else{
    return true;
  }
}

void ml666_parser_destroy(struct ml666_parser* _parser){
  struct ml666__parser_private* parser = (struct ml666__parser_private*)_parser;
  ml666_parser_a_cleanup(parser);
  free(parser);
}
