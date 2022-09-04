#include <ml666/simple-tree.h>
#include <ml666/simple-tree-parser.h>
#include <ml666/simple-tree-builder.h>
#include <ml666/simple-tree-serializer.h>
#include <stdio.h>

int main(){
  // Instanciating the tree builder
  struct ml666_st_builder* stb = ml666_st_builder_create(0);
  if(!stb){
    fprintf(stderr, "ml666_st_builder_create failed\n");
    return 0;
  }

  // Parsing the document
  struct ml666_st_document* document = ml666_st_parse(0, stb);
  if(!document){
    ml666_st_builder_destroy(stb);
    return 1;
  }

  // Serializing the document
  ml666_st_serialize(1, stb, ML666_ST_NODE(document));

  // Freeing all nodes
  // This recursively detaches all nodes from their parent, releasing their only reference
  ml666_st_subtree_disintegrate(stb, ML666_ST_CHILDREN(stb, document));
  // We still have a reference on the document, so let's drop it
  ml666_st_node_put(stb, ML666_ST_NODE(document));

  // Freeing the tree builder
  ml666_st_builder_destroy(stb);
  return 0;
}
