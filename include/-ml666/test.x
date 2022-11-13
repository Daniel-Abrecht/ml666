#include <stdio.h>
#include <string.h>

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

int main(int argc, const char* argv[]){
  if(argc == 1){
    for(struct ml666_testcase* it=ml666_testcase_list; it; it=it->next)
      puts(it->name);
  }else if(argc == 2){
    for(struct ml666_testcase* it=ml666_testcase_list; it; it=it->next){
      if(strcmp(it->name, argv[1]))
        continue;
      return it->run();
    }
  }
  return 10;
}
