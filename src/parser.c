#include <ml666/parser.h>
#include <ml666/tokenizer.h>
#include <ml666/utils.h>
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

  ml666__cb__malloc* malloc;
  ml666__cb__realloc* realloc;
  ml666__cb__free* free;
};
static_assert(offsetof(struct ml666__parser_private, public) == 0);

static bool ml666_parser_a_init(struct ml666__parser_private* that){
  if(that->public.api->init)
    return that->public.api->init(&that->public);
  return true;
}

static void ml666_parser_a_done(struct ml666__parser_private* that){
  if(that->public.api->done)
    that->public.api->done(&that->public);
  ml666_tokenizer_destroy(that->tokenizer);
  that->tokenizer = 0;
}

static void ml666_parser_a_cleanup(struct ml666__parser_private* that){
  if(that->public.api->cleanup)
    that->public.api->cleanup(&that->public);
  if(that->tokenizer)
    ml666_tokenizer_destroy(that->tokenizer);
}

bool ml666_parser__d_mal__tag_name_append(struct ml666_parser* _that, ml666_opaque_tag_name* name, struct ml666_buffer_ro data){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!*name)
    *name = &that->default_hander_state.tag_name;
  return ml666_buffer__append(&(*name)->buffer, data, that->public.user_ptr, that->realloc);
}

bool ml666_parser__d_mal__attribute_name_append(struct ml666_parser* _that, ml666_opaque_attribute_name* name, struct ml666_buffer_ro data){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!*name)
    *name = &that->default_hander_state.attribute_name;
  return ml666_buffer__append(&(*name)->buffer, data, that->public.user_ptr, that->realloc);
}

void ml666_parser__d_mal__tag_name_free(struct ml666_parser* _that, ml666_opaque_tag_name name){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!name)
    return;
  if(name->buffer.data){
    that->realloc(that->public.user_ptr, name->buffer.data, 0);
    name->buffer.data = 0;
    name->buffer.length = 0;
  }
}

void ml666_parser__d_mal__attribute_name_free(struct ml666_parser* _that, ml666_opaque_attribute_name name){
  struct ml666__parser_private* that = (struct ml666__parser_private*)_that;
  if(!name)
    return;
  if(name->buffer.data){
    that->realloc(that->public.user_ptr, name->buffer.data, 0);
    name->buffer.data = 0;
    name->buffer.length = 0;
  }
}

static bool ml666_parser_a_tag_name_append(struct ml666__parser_private* that, ml666_opaque_tag_name* name, struct ml666_buffer_ro data){
  return that->public.api->tag_name_append(&that->public, name, data);
}

static void ml666_parser_a_tag_name_free(struct ml666__parser_private* that, ml666_opaque_tag_name name){
  if(that->public.api->tag_name_free)
    that->public.api->tag_name_free(&that->public, name);
}

static bool ml666_parser_a_attribute_name_append(struct ml666__parser_private* that, ml666_opaque_attribute_name* name, struct ml666_buffer_ro data){
  return that->public.api->attribute_name_append(&that->public, name, data);
}

static void ml666_parser_a_attribute_name_free(struct ml666__parser_private* that, ml666_opaque_attribute_name name){
  if(that->public.api->attribute_name_free)
    that->public.api->attribute_name_free(&that->public, name);
}

static bool ml666_parser_a_tag_push(struct ml666__parser_private* that, ml666_opaque_tag_name* name){
  return that->public.api->tag_push(&that->public, name);
}

static bool ml666_parser_a_end_tag_check(struct ml666__parser_private* that, ml666_opaque_tag_name name){
  return that->public.api->end_tag_check(&that->public, name);
}

static bool ml666_parser_a_tag_pop(struct ml666__parser_private* that){
  return that->public.api->tag_pop(&that->public);
}

static bool ml666_parser_a_set_attribute(struct ml666__parser_private* that, ml666_opaque_attribute_name* name){
  if(that->public.api->set_attribute)
    return that->public.api->set_attribute(&that->public, name);
  return true;
}

static bool ml666_parser_a_data_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.api->data_append)
    return that->public.api->data_append(&that->public, data);
  return true;
}

static bool ml666_parser_a_value_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.api->value_append)
    return that->public.api->value_append(&that->public, data);
  return true;
}

static bool ml666_parser_a_comment_append(struct ml666__parser_private* that, struct ml666_buffer_ro data){
  if(that->public.api->comment_append)
    return that->public.api->comment_append(&that->public, data);
  return true;
}

static ml666_parser_cb_next ml666_parser_d_next;
static ml666_parser_cb_destroy ml666_parser_d_destroy;

static const struct ml666_parser_cb parser_cb = {
  .next = ml666_parser_d_next,
  .destroy = ml666_parser_d_destroy,
};

struct ml666_parser* ml666_parser_create_p(struct ml666_parser_create_args args){
  bool fail = false;
  if(args.fd < 0){
    fprintf(stderr, "ml666_parser_create_p: invalid fd arguent\n");
    fail = true;
  }
  if(!args.api){
    fprintf(stderr, "ml666_parser_create_p: argument cb must be set\n");
    fail = true;
  }else{
    if(!args.api->attribute_name_append){
      fprintf(stderr, "ml666_parser_create_p: callback attribute_name_append is mandatory\n");
      fail = true;
    }
    if(!args.api->tag_name_append){
      fprintf(stderr, "ml666_parser_create_p: callback tag_name_append is mandatory\n");
      fail = true;
    }
    if(!args.api->tag_push){
      fprintf(stderr, "ml666_parser_create_p: callback tag_push is mandatory\n");
      fail = true;
    }
    if(!args.api->end_tag_check){
      fprintf(stderr, "ml666_parser_create_p: callback end_tag_check is mandatory\n");
      fail = true;
    }
    if(!args.api->tag_pop){
      fprintf(stderr, "ml666_parser_create_p: callback tag_pop is mandatory\n");
      fail = true;
    }
  }
  if(fail)
    goto error;
  if(!args.malloc)
    args.malloc = ml666__d__malloc;
  if(!args.realloc)
    args.realloc = ml666__d__realloc;
  if(!args.free)
    args.free = ml666__d__free;
  struct ml666__parser_private* parser = args.malloc(args.user_ptr, sizeof(*parser));
  if(!parser){
    fprintf(stderr, "ml666_parser_create_p: *malloc failed (%d): %s\n", errno, strerror(errno));
    goto error;
  }
  memset(parser, 0, sizeof(*parser));
  *(const struct ml666_parser_cb**)&parser->public.cb = &parser_cb;
  *(const struct ml666_parser_api**)&parser->public.api = args.api;
  parser->public.user_ptr = args.user_ptr;
  parser->malloc = args.malloc;
  parser->realloc = args.realloc;
  parser->free = args.free;
  if(!args.tokenizer){
    args.tokenizer = ml666_tokenizer_create(args.fd, .user_ptr=0, .malloc=args.malloc, .free=args.free);
    if(!args.tokenizer)
      goto error_after_calloc;
  }
  parser->tokenizer = args.tokenizer;
  args.fd = -1;
  if(!ml666_parser_a_init(parser))
    goto error_after_tokenizer;
  return &parser->public;

error_after_tokenizer:
  ml666_tokenizer_destroy(parser->tokenizer);
error_after_calloc:
  args.free(args.user_ptr, parser);
error:
  if(args.fd >= 0)
    close(args.fd);
  if(args.tokenizer)
    ml666_tokenizer_destroy(args.tokenizer);
  return 0;
}

static bool ml666_parser_d_next(struct ml666_parser* _parser){
  struct ml666__parser_private*restrict parser = (struct ml666__parser_private*)_parser;
  if(!parser->tokenizer || parser->tokenizer->token == ML666_EOF)
    return false;
  struct ml666_tokenizer* tokenizer = parser->tokenizer;
  bool done = !ml666_tokenizer_next(tokenizer);
  parser->public.line = tokenizer->line;
  parser->public.column = tokenizer->column;
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

static void ml666_parser_d_destroy(struct ml666_parser* _parser){
  struct ml666__parser_private* parser = (struct ml666__parser_private*)_parser;
  ml666_parser_a_cleanup(parser);
  parser->free(parser->public.user_ptr, parser);
}
