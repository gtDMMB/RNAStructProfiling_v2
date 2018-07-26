//
// Created by Harrison on 5/26/18.
//
// Header for stems, or groups of helices which almost always occur together and are encapsulated (ie. [5[4[1]]] )
//

#include "helix_class.h"
#include "array_list.h"

#ifndef STEM_H
#define STEM_H

typedef enum {
    hc_type,
    stem_type,
    fs_stem_group_type
} NodeType;

typedef struct {
    NodeType node_type;
    void* data;
    int freq;
} DataNode;

typedef struct {
    // stem id
    char* id;
    // stem defined in terms of its component helices and functionally similar stem groups
    char * hc_str;
    // number of helices in the stem
    int num_helices;
    // quadruplet, defined by the start and end nucleotides of each end of the stem (and thus of the outermost helix)
    int int_max_quad[4];
    // a string with the above info
    char* max_quad;
    // quadruplet, using average values of external HCs instead of max
    double double_ave_quad[4];
    // a string with the above info
    char* ave_quad;
    // an array list of the component helices
    array_list_t* helices;
    // an ordered array list of the components (helices or functionally similar stem groups), ordered such that the
    // outermost helix is first, then the one inside, et cetera
    array_list_t* components;
    // the number of times every component helix occurs in the same structure (in the case of functionally equivalent,
    // either but not both of the feq helices)
    int freq;
    bool is_featured;
    unsigned long binary;
} Stem;

typedef struct {
    char* id;
    int num_stems;
    int num_helices;
    int freq;
    // quadruplet, defined by the start and end nucleotides of each of the outermost ends of the component stems
    int int_max_quad[4];
    // string with the above info
    char* max_quad;
    array_list_t* helices;
    array_list_t* stems;
} FSStemGroup;

DataNode* create_data_node(NodeType node_type, void *data);
DataNode* create_stem_node(HC* initial_helix);
DataNode* create_fs_stem_group_node();
void free_data_node(void* ptr);

Stem* create_stem();
// creates a stem with one helix, initial_helix, and returns a pointer to it. Returns NULL if memory allocation fails
Stem* create_stem_from_HC(HC *initial_helix);
// destroys a Stem, freeing allocated memory. Returns 0 if successful, non-zero otherwise
void free_stem(void* ptr);

FSStemGroup* create_fs_stem_group();
int add_to_fs_stem_group(FSStemGroup *stem_group, Stem *stem);
void free_fs_stem_group(void* ptr);

int max(int a, int b);
int min(int a, int b);

#endif //STEM_H
