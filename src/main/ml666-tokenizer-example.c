#define _DEFAULT_SOURCE
#include <ml666/tokenizer.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bool ml666__print_buf_escaped(int fd, const struct ml666_buffer_ro ro);

int main(){
  struct ml666_tokenizer* tokenizer = ml666_tokenizer_create(0);
  if(!tokenizer){
    fprintf(stderr, "ml666_tokenizer_create failed\n");
    return 1;
  }
  while(ml666_tokenizer_next(tokenizer)){
    if(tokenizer->token){
      printf(
        "%4zu:%4zu: %c %s [%zu]: ",
        tokenizer->line, tokenizer->column,
        tokenizer->complete ? 'c' : 'p',
        ml666__token_name[tokenizer->token],
        tokenizer->match.length
      );
      fflush(stdout);
      ml666__print_buf_escaped(1, tokenizer->match);
      printf("\n");
    }
  }
  if(tokenizer->error){
    fprintf(stderr, "ml666_tokenizer_next failed: At %zu,%zu: %s\n", tokenizer->line, tokenizer->column, tokenizer->error);
    ml666_tokenizer_destroy(tokenizer);
    return 1;
  }
  ml666_tokenizer_destroy(tokenizer);
  return 0;
}


// Stuff for printing escaped strings follows

static const char ml666__hex[] = "0123456789ABCDEF";

static char* ml666__escape_p(char ret[5], unsigned char x){
  if(x < ' ' || (char)x == '"' || (char)x == '\\' || x >= 0x7F){
    ret[0] = '\\';
    switch((char)x){
      case '"'   : ret[1] = '"'; break;
      case '\\'  : ret[1] = '\\'; break;
      case '\a'  : ret[1] = 'a'; break;
      case '\b'  : ret[1] = 'b'; break;
      case '\x1B': ret[1] = 'e'; break;
      case '\f'  : ret[1] = 'f'; break;
      case '\n'  : ret[1] = 'n'; break;
      case '\t'  : ret[1] = 't'; break;
      case '\v'  : ret[1] = 'v'; break;
      default: {
        ret[1] = 'x';
        ret[2] = ml666__hex[x/16];
        ret[3] = ml666__hex[x%16];
      } break;
    }
  }else{
    ret[0] = x;
  }
  return ret;
}
#define ml666__escape(X) ml666__escape_p((char[5]){0},(X))

static bool ml666__print_buf_escaped(int fd, const struct ml666_buffer_ro ro){
  if(dprintf(fd, "\"") < 0)
    return false;
  for(size_t i=0; i<ro.length; i++)
    if(dprintf(fd, "%s", ml666__escape(ro.data[i])) < 0)
      return false;
  if(dprintf(fd, "\"") < 0)
    return false;
  return true;
}
