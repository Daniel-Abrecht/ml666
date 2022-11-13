#include <-ml666/test.x>
#include <ml666/simple-tree-builder.h>

ML666_TEST("set"){
  struct ml666_st_builder* stb = ml666_st_builder_create(0);
  // TODO
  ml666_st_builder_destroy(stb);
  return 0;
}
