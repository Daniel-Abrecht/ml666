#include <ml666/simple-tree.h>
#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree-ml666-serializer.h>
#include <stdio.h>

int main(){
  // Instanciating the tree builder
  struct ml666_st_builder* stb = ml666_st_builder_create(0);
  if(!stb){
    fprintf(stderr, "ml666_st_builder_create failed\n");
    return 1;
  }

  // Parsing the document
  struct ml666_st_document* document = ml666_st_parse(0, stb);
  if(!document){
    ml666_st_builder_destroy(stb);
    return 1;
  }

  // Serializing the document
  struct ml666_st_serializer* serializer = ml666_st_ml666_serializer_create(1, stb, ML666_ST_NODE(document));
  if(!serializer){
    fprintf(stderr, "ml666_st_ml666_serializer_create failed");
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
  return 0;
}
