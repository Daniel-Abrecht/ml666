#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree.h>
#include <stdio.h>

int main(){
  setvbuf(stdout, NULL, _IOLBF, 0);
  struct ml666_simple_tree_builder* stb = ml666_simple_tree_builder_create( .fd = 0, .cb = &ml666_st_default_api );
  if(!stb){
    fprintf(stderr, "ml666_parser_create failed");
    return 1;
  }
  while(ml666_simple_tree_builder_next(stb));
  const char* error = ml666_simple_tree_builder_get_error(stb);
  if(error){
    fprintf(stderr, "ml666_parser_next failed: %s\n", error);
    return 1;
  }
  ml666_simple_tree_builder_destroy(stb);
  return 0;
}
