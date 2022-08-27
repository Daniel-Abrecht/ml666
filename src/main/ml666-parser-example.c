#define _DEFAULT_SOURCE
#include <ml666/parser.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const struct ml666_parser_cb callbacks = {
  .tag_name_append       = ml666_parser__d_mal__tag_name_append,
  .tag_name_free         = ml666_parser__d_mal__tag_name_free,
  .attribute_name_append = ml666_parser__d_mal__attribute_name_append,
  .attribute_name_free   = ml666_parser__d_mal__attribute_name_free,
};

int main(){
  ml666_parser_create( .fd = 0, .cb = &callbacks );
  // TODO
  return 0;
}
