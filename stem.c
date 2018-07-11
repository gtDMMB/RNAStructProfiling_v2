//
// Created by hwilco on 5/26/18.
//
// stems, or groups of helices which almost always occur together and are encapsulated (ie. [5[4[1]]] )
//

#include "stem.h"
#include "array_list.h"
#include "Set.h"
#include "helix_class.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a DataNode of type node_type with data data
 *
 * @param node_type the type of data stored in node->data (Stem or FuncSimilarStemGroup)
 * @param data the data to store in the node
 * @return a pointer to the new node, NULL if memory allocation fails
 */
DataNode* create_data_node(NodeType node_type, void *data) {
    DataNode* node = (DataNode*) malloc(sizeof(DataNode));
    if (node == NULL) {
        return NULL;
    }
    node->node_type = node_type;
    node->data = data;
    return node;
}

/**
 * Creates a Stem and places it into a new DataNode
 *
 * @param intitial_helix the first helix to add to the stem
 * @return a pointer to the new node, Null if memory allocation fails
 */
DataNode* create_stem_node(HC* initial_helix) {
    DataNode* node = (DataNode*) malloc(sizeof(DataNode));
    if (node == NULL) {
        return NULL;
    }
    node->node_type = stem_type;
    node->data = create_stem(initial_helix);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

/**
 * Creates a FSStemGroup and places it into a new DataNode
 *
 * @param stem_group the stem_group to be stored by the DataNode
 * @return a pointer to the new node, Null if memory allocation fails
 */
DataNode* create_fs_stem_group_node() {
    DataNode* node = (DataNode*) malloc(sizeof(DataNode));
    if (node == NULL) {
        return NULL;
    }
    node->node_type = fs_stem_group_type;
    node->data = create_fs_stem_group();
    if (node->data == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

/**
 * Frees a DataNode and the data field of the node
 *
 * @param node the node to free
 * @return 0 if successful, non-zero otherwise
 */
int free_data_node(DataNode *node) {
    if (node == NULL) {
        return 1;
    } else if (node->data == NULL) {
        free(node);
        return 0;
    } else if (node->node_type == NULL) {
        return 1;
    }
    if (node->node_type == stem_type) {
        if (free_stem((Stem*) node->data) != 0) {
            return 1;
        }
    } else if (node->node_type == fs_stem_group_type) {
        if (free_fs_stem_group((FSStemGroup*)node->data) != 0) {
            return 1;
        }
    } else if (node->node_type == hc_type) {
        if (free_hc((HC*)node->data) != 0) {
            return 1;
        }
    }
    free(node);
    return 0;
}

// creates a stem with one helix, initial_helix, and returns a pointer to it. Returns NULL if memory allocation fails
Stem* create_stem(HC* initial_helix) {
    Stem* stem = (Stem*) malloc(sizeof(Stem));
    if (stem == NULL) {
        return NULL;
    }
    stem->num_helices = 1;
    stem->helices = create_array_list();
    if (stem->helices == NULL) {
        free(stem);
        return NULL;
    }
    add_to_array_list(stem->helices, 0, initial_helix);
    stem->int_max_quad[0] = initial_helix->int_max_quad[0];
    stem->int_max_quad[1] = initial_helix->int_max_quad[1];
    stem->int_max_quad[2] = initial_helix->int_max_quad[2];
    stem->int_max_quad[3] = initial_helix->int_max_quad[3];
    stem->max_quad = (char*) malloc(sizeof(char) * (1 + strlen(initial_helix->max_quad)));
    strcpy(stem->max_quad, initial_helix->max_quad);
    stem->freq = initial_helix->freq;
    char* endptr;
    stem->id = (int) strtol(initial_helix->id, &endptr, 10);
    return stem;
}

void stem_reset_id(Stem* stem) {
    char* endptr;
    stem->id = (int) strtol(((HC*)stem->helices->entries[0])->id, &endptr, 10);
}

// destroys a Stem, freeing allocated memory. Returns 0 if successful, non-zero otherwise
int free_stem(Stem* stem) {
    // TODO: fix to use free_hc
    destroy_array_list(stem->helices, &free);
    free(stem->max_quad);
    free(stem);
    return 0;
}

FSStemGroup* create_fs_stem_group() {
    FSStemGroup* stem_group = (FSStemGroup*) malloc(sizeof(FSStemGroup));
    if (stem_group == NULL) {
        return NULL;
    }
    stem_group->helices = create_array_list();
    if (stem_group->helices == NULL) {
        free(stem_group);
        return NULL;
    }
    stem_group->stems = create_array_list();
    // TODO: use proper free func
    if (stem_group->stems == NULL) {
        destroy_array_list(stem_group->helices, &free);
        free(stem_group);
        return NULL;
    }
    stem_group->num_helices = 0;
    stem_group->num_stems = 0;
    stem_group->combined_freq = 0;
    return stem_group;
}

/**
 * add a stem to a functionally similar stem group. frequency must be calculated outside of method
 *
 * @param stem_group the group to add to
 * @param stem the stem to add
 * @return 0 if successful, non-zero otherwise
 */
int add_to_fs_stem_group(FSStemGroup *stem_group, Stem *stem) {
    int added;
    added = add_to_array_list(stem_group->stems, stem_group->num_stems, stem);
    if (added != 0) {
        return 1;
    }
    stem_group->num_stems++;
    for (int i = 0; i < stem->num_helices; i++) {
        add_to_array_list(stem_group->helices, stem_group->num_helices, stem->helices->entries[i]);
        stem_group->num_helices++;
    }
    return 0;
}

// TODO: use proper free funcs
int free_fs_stem_group(FSStemGroup *stem_group) {
    destroy_array_list(stem_group->stems, &free);
    destroy_array_list(stem_group->helices, &free);
    free(stem_group);
    return 0;
}
