#ifndef ML666_COMMON_H
#define ML666_COMMON_H

// This must match struct ml666_buffer
struct ml666_buffer_ro {
  const char* data;
  size_t length;
};

struct ml666_buffer {
  union {
    struct ml666_buffer_ro ro;
    struct {
      char* data;
      size_t length;
    };
  };
};

#endif
