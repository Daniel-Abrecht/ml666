#ifndef ML666_TEST_H
#define ML666_TEST_H

// This is an internal header

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/**
 * \addtogroup ml666-utils Utils
 * @{
 */

/**
 * \addtogroup ml666-test Test Utils
 * @{
 */

/**
 * Send a test result to the `testsuite` server program.
 * \param fd Testsuite server fd, set it to -1 to make it use the `TESTSUITE_FD` environment variable.
 * \param name The testcase name, or 0 if none (may be desirable in case of a single testcase).
 * \param result Set this to `"success"` if it was ok, `skipped` if it was skipped.
 *   All other values count as failure, the values `"failed"`, `"unavailable"`, `"unknown"` are
 *   used by the `testsuite` program itself, but any value can be used.
 * \returns true on success, false if sending the result failed
 */
static inline bool ml666_testcase_result(int fd, const char* name, const char* result){
  if(fd < 0){
    static int sfd = -1;
    if(sfd == -1){
      const char* tfd = getenv("TESTSUITE_FD");
      if(!tfd) return false;
      int x = atoi(tfd);
      if(x < 0) return false;
      sfd = x;
    }
    fd = sfd;
  }
  const size_t name_length = (name && *name) ? strlen(name)+1 : 0;
  const size_t result_length = strlen(result);
  const size_t length = 2+name_length+1+result_length;
  if(length > 0x1000)
    return false;
  char mem[length];
  mem[0] = length >> 8;
  mem[1] = length;
  if(name_length)
    memcpy(mem+2, name, name_length);
  mem[2+name_length] = 0;
  memcpy(mem+2+name_length+1, result, result_length);
  // This has to be a single write (because this is a SOCK_SEQPACKET socket)
  if(write(fd, mem, length) != (ssize_t)length){
    perror("write");
    return false;
  }
  return true;
}


/** @} */
/** @} */

#endif
