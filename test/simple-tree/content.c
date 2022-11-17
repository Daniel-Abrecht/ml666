#include <-ml666/test.x>
#include <ml666/utils.h>
#include <ml666/simple-tree-builder.h>

#define ML666_BUFFER_STR(...) (struct ml666_buffer_ro){sizeof(__VA_ARGS__), (__VA_ARGS__)}

struct ml666_st_builder* stb;
struct ml666_st_content* content;

void test_setup(void){
  stb = ml666_st_builder_create(0);
  content = ml666_st_content_create(stb);
}

void test_teardown(void){
  ml666_st_node_put(stb, ML666_ST_NODE(content));
  ml666_st_builder_destroy(stb);
}

ML666_TEST("nothing"){
  return 0;
}

ML666_TEST("set"){
  struct ml666_buffer dest;
  ml666_buffer__dup(&dest, ML666_BUFFER_STR("Hello World!"));
  ml666_st_content_set(stb, content, dest);
  struct ml666_buffer_ro current = ml666_st_content_get(stb, content);
  if(!ml666_buffer__equal(current, dest.ro))
    return 1;
  return 0;
}

ML666_TEST("free-after-set"){
  struct ml666_buffer dest;
  ml666_buffer__dup(&dest, ML666_BUFFER_STR("Hello World!"));
  ml666_st_content_set(stb, content, dest);
  ml666_buffer__dup(&dest, ML666_BUFFER_STR("Hello World!"));
  ml666_st_content_set(stb, content, dest);
  return 0;
}

ML666_TEST("take"){
  struct ml666_buffer dest;
  ml666_buffer__dup(&dest, ML666_BUFFER_STR("Hello World!"));
  ml666_st_content_set(stb, content, dest);
  struct ml666_buffer current = ml666_st_content_take(stb, content);
  if(!ml666_buffer__equal(current.ro, dest.ro))
    return 1;
  if(ml666_st_content_get(stb, content).length)
    return 2;
  ml666_buffer__clear(&current);
  return 0;
}
