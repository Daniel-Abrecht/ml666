#include <ml666/utils.h>
#include <ml666/simple-tree.h>
#include <ml666/json-token-emmiter.h>
#include <ml666/binary-token-emmiter.h>
#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree-ml666-serializer.h>
#include <ml666/simple-tree-json-serializer.h>
#include <ml666/simple-tree-binary-serializer.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum format {
  F_ML666,
  F_JSON,
  F_BINARY,
};

struct arguments {
  enum format output_format;
  enum format input_format;
  struct ml666_hashed_buffer attribute;
  bool recursive;
  bool lf;
};

bool parse_args(struct arguments* args, int* argc, char* argv[]){
  int j=1;
  for(int i=1,n=*argc; i<n; i++){
    if(i > j)
      argv[j] = argv[i];
    if(argv[i][0] != '-'){
      j += 1;
      continue;
    }
    if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--recursive")){
      args->recursive = true;
    }else if(!strcmp(argv[i], "--lf")){
      args->lf = true;
    }else if(!strcmp(argv[i], "--input-format")){
      if(++i >= n)
        return false;
      if(!strcmp(argv[i], "ml666")){
        args->input_format = F_ML666;
      }else if(!strcmp(argv[i], "json")){
        args->input_format = F_JSON;
      }else if(!strcmp(argv[i], "binary")){
        args->input_format = F_BINARY;
      }else return false;
    }else if(!strcmp(argv[i], "--output-format")){
      if(++i >= n)
        return false;
      if(!strcmp(argv[i], "ml666")){
        args->output_format = F_ML666;
      }else if(!strcmp(argv[i], "json")){
        args->output_format = F_JSON;
      }else if(!strcmp(argv[i], "binary")){
        args->output_format = F_BINARY;
      }else return false;
    }else if(!strcmp(argv[i], "--attribute")){
      if(++i >= n)
        return false;
      struct ml666_buffer buf = {
        .data = argv[i],
        .length = strlen(argv[i]),
      };
      args->attribute = ml666_hashed_buffer__create(buf.ro);
    }else return false;
  }
  if(args->attribute.buffer.length && args->output_format != F_BINARY)
    return false;
  *argc = j;
  if(j > 1)
    return false;
  return true;
}

int main(int argc, char* argv[]){
  struct arguments args = {0};
  if(!parse_args(&args, &argc, argv)){
    fprintf(stderr, "usage: %s [-r] [--lf] [--input-format ml666|json|binary] [--output-format ml666|json|binary]\n", *argv);
    return 1;
  }

  const int fd_out = dup(1);
  if(fd_out == -1){
    perror("dup failed");
    return 1;
  }

  // Instanciating the tree builder
  struct ml666_st_builder* stb = ml666_st_builder_create(0);
  if(!stb){
    fprintf(stderr, "ml666_st_builder_create failed\n");
    return 1;
  }

  struct ml666_tokenizer* tokenizer = 0;
  switch(args.input_format){
    case F_ML666: break;
    case F_JSON : tokenizer = ml666_json_token_emmiter_create(0); break;
    case F_BINARY: tokenizer = ml666_binary_token_emmiter_create(0); break;
  }

  // Parsing the document
  struct ml666_st_document* document = ml666_st_parse(stb, 0, .tokenizer=tokenizer);
  if(!document){
    ml666_st_builder_destroy(stb);
    return 1;
  }

  // Serializing the document
  struct ml666_st_serializer* serializer = 0;
  switch(args.output_format){
    case F_ML666 : serializer = ml666_st_ml666_serializer_create(fd_out, stb, ML666_ST_NODE(document)); break;
    case F_JSON  : serializer = ml666_st_json_serializer_create (fd_out, stb, ML666_ST_NODE(document)); break;
    case F_BINARY: {
      serializer = ml666_st_binary_serializer_create(
        fd_out, stb, ML666_ST_NODE(document),
        .recursive=args.recursive,
        .attribute=args.attribute.buffer.length ? &args.attribute : 0,
      );
    } break;
  }
  if(!serializer){
    fprintf(stderr, "error: couldn't create serializer\n");
    return 1;
  }
  ml666_st_serialize(serializer);
  ml666_st_serializer_destroy(serializer);

  // Freeing all nodes
  // This recursively detaches all nodes from their parent, releasing their only reference
  ml666_st_subtree_disintegrate(stb, ML666_ST_CHILDREN(stb, document));
  // We still have a reference on the document, so let's drop it
  ml666_st_node_put(stb, ML666_ST_NODE(document));

  // Freeing the tree builder
  ml666_st_builder_destroy(stb);

  if(args.lf)
    while(write(1, "\n", 1) == -1 && errno == EINTR);

  return 0;
}
