#ifndef ML666_COMMON_H
#define ML666_COMMON_H

// This must match struct ml666_buffer
struct ml666_buffer_ro {
  size_t length;
  const char* data;
};

struct ml666_buffer {
  union {
    struct ml666_buffer_ro ro;
    struct {
      size_t length;
      char* data;
    };
  };
};

#endif
