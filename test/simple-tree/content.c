#include <-ml666/test.x>
#include <ml666/utils.h>
#include <ml666/simple-tree-builder.h>

#define ML666_BUFFER_STR(...) (struct ml666_buffer_ro){sizeof(__VA_ARGS__), (__VA_ARGS__)}

ML666_TEST("set"){
  struct ml666_st_builder* stb = ml666_st_builder_create(0);
  struct ml666_st_content* content = ml666_st_content_create(stb);
  struct ml666_buffer dest;
  ml666_buffer__dup(&dest, ML666_BUFFER_STR("Hello World!"));
  ml666_st_content_set(stb, content, dest);
  struct ml666_buffer_ro current = ml666_st_content_get(stb, content);
  ml666_st_content_set(stb, content, (struct ml666_buffer){0});
  ml666_st_builder_destroy(stb);
  return 0;
}
