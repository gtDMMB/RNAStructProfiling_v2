//
// Created by Harrison on 5/26/18.
//
// Header for stems, or groups of helices which almost always occur together and are encapsulated (ie. [5[4[1]]] )
//

#include "helix_class.h"
#include "array_list.h"

#ifndef STEM_H
#define STEM_H

// TODO: make these all options
// how close the ends of two stems must be for them to be considered 1 stem
#define STEM_END_DELTA 3
// max allowable % diff between frequencies of two stems to be merged
#define STEM_VALID_PERCENT_ERROR 5
// TODO: better way to determine # helices to consider
// max number of helices to consider for stems
#define STEM_NUM_CUTOFF 15
// max allowable % of structures containing two stems for them to be considered functionally similar
#define FUNC_SIMILAR_PERCENT_ERROR 1
// max difference between ends of two functionally similar stems
#define FUNC_SIMILAR_END_DELTA 3


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
    // TODO: decide on id notation
    // stem id, using first helix id
    int id;
    // number of helices in the stem
    int num_helices;
    // quadruplet, defined by the start and end nucleotides of each end of the stem (and thus of the outermost helix)
    int int_max_quad[4];
    // TODO: update max quad properly when stems/helices are added
    // a string with the above info
    char* max_quad;
    // TODO: implement ave_quad
    /*
    // quadruplet, using avetrip instead of maxtrip
    int ave_quad[4];
    // a string with the above info
    char* ave_quad;
     */
    // TODO: ensure this is ordered correctly when stems are merged
    // an array list of the component helices
    array_list_t* helices;
    // an ordered array list of the components (helices or functionally similar stem groups), ordered such that the
    // outermost helix is first, then the one inside, et cetera
    array_list_t* components;
    // the number of times every component helix occurs in the same structure (in the case of functionally equivalent,
    // either but not both of the feq helices)
    int freq;
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
int free_data_node(DataNode *node);

// creates a stem with one helix, initial_helix, and returns a pointer to it. Returns NULL if memory allocation fails
Stem* create_stem(HC* initial_helix);
// recreate the id for a stem using its helix set
void stem_reset_id(Stem* stem);
// destroys a Stem, freeing allocated memory. Returns 0 if successful, non-zero otherwise
int free_stem(Stem* stem);

FSStemGroup* create_fs_stem_group();
int add_to_fs_stem_group(FSStemGroup *stem_group, Stem *stem);
int free_fs_stem_group(FSStemGroup *stem_group);

int max(int a, int b);
int min(int a, int b);

#endif //STEM_H
