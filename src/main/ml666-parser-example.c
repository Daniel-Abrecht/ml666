#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree.h>
#include <stdio.h>

int main(){
  setvbuf(stdout, NULL, _IOLBF, 0);
  struct ml666_simple_tree_parser* stp = ml666_simple_tree_parser_create( .fd = 0, .cb = &ml666_default_st_api );
  if(!stp){
    fprintf(stderr, "ml666_parser_create failed");
    return 1;
  }
  while(ml666_simple_tree_parser_next(stp));
  const char* error = ml666_simple_tree_parser_get_error(stp);
  if(error){
    fprintf(stderr, "ml666_parser_next failed: %s\n", error);
    return 1;
  }
  ml666_simple_tree_parser_destroy(stp);
  return 0;
}
