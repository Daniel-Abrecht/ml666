#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct ml666_testcase {
  struct ml666_testcase* next;
  const char* name;
  int(*run)(void);
};

static struct ml666_testcase* ml666_testcase_list;

#define ML666_CONCAT_EVAL(A,B) A ## B
#define ML666_CONCAT(A,B) ML666_CONCAT_EVAL(A, B)

#define ML666_TEST(NAME)  \
  static int ML666_CONCAT(ml666_test_f_, __LINE__)(void); \
  __attribute__((constructor,used)) \
  static void ML666_CONCAT(ml666_test_c_, __LINE__)(void){ \
    static struct ml666_testcase entry = { \
      .name = NAME, \
      .run = ML666_CONCAT(ml666_test_f_, __LINE__), \
    }; \
    entry.next = ml666_testcase_list; \
    ml666_testcase_list = &entry; \
  } \
  static int ML666_CONCAT(ml666_test_f_, __LINE__)(void)

static size_t ml666__alloc_counter = 0;

void* ml666__d__malloc(void* that, size_t size){
  (void)that;
  ml666__alloc_counter += 1;
  return malloc(size);
}

void* ml666__d__realloc(void* that, void* data, size_t size){
  (void)that;
  if(data && !size){
    if(ml666__alloc_counter){
      ml666__alloc_counter -= 1;
    }else{
      assert(!"More calls to free than to malloc");
    }
  }else if(!data && size){
    ml666__alloc_counter += 1;
  }
  return realloc(data, size);
}

void ml666__d__free(void* that, void* data){
  (void)that;
  if(!data)
    return;
  free(data);
  if(ml666__alloc_counter){
    ml666__alloc_counter -= 1;
  }else{
    assert(!"More calls to free than to malloc");
  }
}

__attribute__((weak)) void test_setup(void);
__attribute__((weak)) void test_teardown(void);

int main(int argc, const char* argv[]){
  if(argc == 1){
    for(struct ml666_testcase* it=ml666_testcase_list; it; it=it->next)
      puts(it->name);
  }else if(argc == 2){
    for(struct ml666_testcase* it=ml666_testcase_list; it; it=it->next){
      if(strcmp(it->name, argv[1]))
        continue;
      if(test_setup)
        test_setup();
      int res = it->run();
      if(test_teardown)
        test_teardown();
      assert(!ml666__alloc_counter);
      return res;
    }
  }
  return 10;
}
