#include "graph.h"
#include "hashtbl.h"
#include "Set.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int CONSOLIDATED_GRAPHSIZE;
extern node** consolidated_graph;

void consolidated_init_graph(FILE *fp, Set *set) {
    int i;

    fputs("digraph G {\n",fp);
    //fprintf(fp,"\tlabel = \"%s\";\n",set->opt->OUTPUT);
    fprintf(fp,"\tpad = 0.5;\n");
    fprintf(fp,"\tnodesep = 0.5;\n");
    fprintf(fp,"\"legend\" [label = < <table border=\"0\" cellborder=\"1\" cellspacing=\"0\"><tr><td>Stem</td><td>Quadruplet</td><td>Frequency</td></tr>\n");
    for (i = 0; i < set->num_fstems; i++) {
        Stem* stem = (Stem*) set->stems->entries[i];
        fprintf(fp,"<tr><td>%s</td><td>%s</td><td>%d</td></tr>\n",stem->id,stem->max_quad,stem->freq);
    }
    fprintf(fp,"</table>>, shape = plaintext, fontsize=11];\n");

    for (i = 0; i < set->num_s_stem_prof; i++) {
        fprintf(fp, "\"%s\" [shape = box];\n", set->stem_profiles[i]->profile);
    }
}

int consolidated_initialize(Set *set) {
    int i,j,k,size=1;
    node *root;
    char **diff,*rt;
    Stem** stem_list = (Stem**) set->stems->entries;

    for (j = 0; j < set->stems->size; j++) {
        if ((stem_list[j])->freq < set->opt->NUMSTRUCTS)
            break;
    }
    if (j>0) {
    //assuming <100 always stems
        rt = (char*)malloc(sizeof(char)*j*3);
        strcpy(rt, stem_list[0]->id);
        strcat(rt, " ");
        for (k=1; k<j; k++) {
            strcat(rt,stem_list[k]->id);
            strcat(rt, " ");
        }
        root = createNode(rt);
        root->sum = stem_binary_rep(set,root->label);
    } else {
        root = createNode((char *) "");
    }
    for (i = 0; i < set->stem_prof_num; i++) {
        if (!strcmp(root->label, set->stem_profiles[i]->profile))
            root->sfreq = set->stem_profiles[i]->freq;
    }
    while (set->num_s_stem_prof +1 > ARRAYSIZE*size) {
        size++;
    }
    node **neighbors = (node**)malloc(sizeof(node*)*ARRAYSIZE*size);
    diff = (char**)malloc(sizeof(char*)*ARRAYSIZE*size);
    for (i = 0; i < set->num_s_stem_prof; i++) {
        neighbors[i] = createNode(set->stem_profiles[i]->profile);
        neighbors[i]->sum = stem_binary_rep(set,neighbors[i]->label);
        if (j>0) {
            diff[i] = rm_root(j,neighbors[i]->label);
        } else
            diff[i] = neighbors[i]->label;
    }
    root->numNeighbors = set->num_s_stem_prof;

    root->neighbors = neighbors;
    root->nsize = size;
    root->diff = diff;
    set->consolidated_graph = root;
    return j;
}

//converts binary rep to string of stems (stem profile)
char* consolidated_convert_binary(Set* set, unsigned long binary) {
    int k,size = 1,num=0;
    char stem_id[ARRAYSIZE],*stem_profile;

    stem_profile = (char*) malloc(sizeof(char)*ARRAYSIZE);
    stem_profile[0] = '\0';
    for (k = 0; binary > 0; binary >>= 1, k++) {
        if ((binary & 1) == 1) {
            strcpy(stem_id, ((Stem*)(set->stems->entries[k]))->id);
            if (strlen(stem_profile)+strlen(stem_id) > (unsigned int)ARRAYSIZE*size-2) {
                stem_profile = (char*) realloc(stem_profile,sizeof(char)*ARRAYSIZE*++size);
            }
            strcat(stem_profile,stem_id);
            strcat(stem_profile," ");
            num++;
        }
    }
    return stem_profile;
}



/*make edge
update parents' general freq in graph[] with child's spec. freq if exists
helix set difference = edge label
child and parent are indices
returns 1 if inserted into edge hash, 0 otherwise
*/
void consolidated_found_edge(Set* set, node *child,node *parent) {
    int i;
    unsigned long xr;
    char *diff;

    //printf("child is %s and parent is %s\n",childprof,parentprof);
    for (i = 0; i < parent->numNeighbors; i++)
        if (!strcmp(parent->neighbors[i]->label,child->label))
            return;
    if (parent->numNeighbors >= ARRAYSIZE*parent->nsize) {
        parent->nsize++;
        parent->neighbors = (node**) realloc(parent->neighbors,sizeof(node*)*ARRAYSIZE*parent->nsize);
        parent->diff = (char**) realloc(parent->diff,sizeof(char*)*ARRAYSIZE*parent->nsize);
    } else if (parent->numNeighbors == 0) {
        parent->neighbors = (node**) malloc(sizeof(node*)*ARRAYSIZE*parent->nsize);
        parent->diff = (char**) malloc(sizeof(char*)*ARRAYSIZE*parent->nsize);
    }
    xr = child->sum ^ parent->sum;
    diff = consolidated_convert_binary(set, xr);
    parent->neighbors[parent->numNeighbors] = child;
    parent->diff[parent->numNeighbors] = diff;
    parent->numNeighbors++;
}




/* finished profiles = [0 start-1]
   present LCA to be intersected = [start oldk]
   newly generated LCA = [oldk k]
   returns k, the number of vertices in graph
 */
void consolidated_find_LCAs(FILE *fp,Set *set, int i) {
    int newk, oldk, go, start, size, k, cycles=0;
    unsigned long num;
    char *stem_profile,**diff;
    node **vertices = set->consolidated_graph->neighbors;

    k = set->consolidated_graph->numNeighbors;
    size = set->consolidated_graph->nsize;
    diff = set->consolidated_graph->diff;
    start = 0;
    for (oldk = k; start != k; oldk = k) {
        for (newk = start; newk != oldk; newk++) {
            for (go = advance(newk,oldk); go != start; go = advance(go,oldk)) {
                //printf("start is %d oldk is %d and go is %d\n",start,oldk,go);
                num = vertices[newk]->sum & vertices[go]->sum;
                //printf("num is %u of s[%d] = %u and s[%d] = %u\n",num,new,sums[new],go,sums[go]);
                if (not_in_sums(num,k,vertices)) {
                    //printf("found new profile for %s and %s\n",modprofileID[new],modprofileID[go]);

                    stem_profile = consolidated_convert_binary(set, num);
                    fprintf(fp,"\"%s\" [style = dashed];\n",stem_profile);
                    if (k >= ARRAYSIZE*(size)) {
                        vertices = (node**) realloc(vertices,sizeof(node*)*ARRAYSIZE*++size);
                        diff = (char**) realloc(diff,sizeof(char*)*ARRAYSIZE*size);
                    }
                    vertices[k] = createNode(stem_profile);
                    vertices[k]->sum = num;
                    if (i>0) {
                        diff[k] = rm_root(i,stem_profile);
                    } else {
                        diff[k] = stem_profile;
                    }
                    consolidated_found_edge(set, vertices[newk],vertices[k]);
                    consolidated_found_edge(set, vertices[go],vertices[k]);
                    k++;
                } else if (num == vertices[newk]->sum) {
                    consolidated_found_edge(set, vertices[go],vertices[newk]);
                } else if (num == vertices[go]->sum) {
                    consolidated_found_edge(set, vertices[newk], vertices[go]);
                }
            }
            //printf("comparings against %d with end %d, vertices %d\n",new,oldk,k);
        }
        start = oldk;
        if (++cycles == set->opt->CYCLES) {
            break;
        }
    }
    set->consolidated_graph->neighbors = vertices;
    set->consolidated_graph->diff = diff;
    set->consolidated_graph->numNeighbors = k;
    if (set->consolidated_graph->sfreq == 0) {
        k++;
    }
    printf("Total number of vertices: %d\n",k);
    set->consolidated_graph->nsize = size;
}

void consolidated_make_oval_bracket(node *vert) {
    int i = 0,j=0,k,h=0,m=0,count = 0,*skip;
    char *pbrac, *cbrac,*diff,*val,**df,**stems;
    node *child;

    if (vert->bracket) return;
    child = (node*) malloc(sizeof(node));
    diff = find_child_bracket(vert,child);

    df = (char**) malloc(sizeof(char*)*(strlen(diff)/2 + 1));
    for (int index = 0; int2size_t(index) < (strlen(diff)/2 + 1); index++) {
        df[index] = (char*) malloc(sizeof(char) * 10);
    }
    for (val = strtok(mystrdup(diff)," "); val; val = strtok(NULL," ")) {
        strcpy(df[i++],val);
    }
    cbrac = mystrdup(child->bracket);
    pbrac = (char*) malloc(sizeof(char)*strlen(cbrac));
    skip = (int*) malloc(sizeof(int)*i);
    stems = (char**) malloc(sizeof(char*)*(strlen(cbrac)/3 + 1));
    for (int index = 0; int2size_t(index) < (strlen(cbrac)/3 + 1); index++) {
        stems[index] = (char*) malloc(sizeof(char) * 10);
    }
    for (val = strtok(cbrac,"[]"); val; val = strtok(NULL,"[]")) {
        strcpy(stems[j++], val);
    }
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    cbrac = child->bracket;
    pbrac[0] = '\0';
    for (j = 0; (unsigned int) j < strlen(cbrac); j++) {
        if (cbrac[j] == '[') {
            for (k=0; k < i; k++)
                if (strcmp(df[k], stems[h]) == 0)
                    break;
            if (k == i) {
                sprintf(val,"[%s",stems[h]);
                pbrac = strcat(pbrac,val);
            } else
                skip[m++] = count;
            h++;
            count++;
        }

        else if (cbrac[j] == ']') {
            count--;
            if (m == 0 || count != skip[m-1])
                pbrac = strcat(pbrac,"]");
            else
                m--;
        }
    }
    //printf("bracket based on %s for %s is %s\n",child->bracket,vert->label,pbrac);
    vert->bracket = pbrac;
    free(df);
    free(stems);
    free(skip);
    free(val);
    free(child);
}

void consolidated_calc_gfreq(FILE *fp,Set *set) {
    int i,j;
    unsigned long *sum;
    node *vert;

    CONSOLIDATED_GRAPHSIZE = set->consolidated_graph->numNeighbors + 1;
    consolidated_graph = (node**)malloc(sizeof(*consolidated_graph) * CONSOLIDATED_GRAPHSIZE);
    sum = (long unsigned int*) malloc(sizeof(unsigned long)*set->stem_prof_num);
    for (i = 0; i < set->stem_prof_num; i++) {
        sum[i] = stem_binary_rep(set,set->stem_profiles[i]->profile);
        if (!strcmp(set->stem_profiles[i]->profile," "))
            set->consolidated_graph->sfreq = set->stem_profiles[i]->freq;
    }

    for (i = 0; i < set->consolidated_graph->numNeighbors; i++) {
        vert = set->consolidated_graph->neighbors[i];
        consolidated_graph[i] = vert;
        for (j = 0; j < set->stem_prof_num; j++) {
            if (sum[j] == vert->sum) {
                vert->sfreq = set->stem_profiles[j]->freq;
                vert->bracket = set->stem_profiles[j]->bracket;
            }
            if ((sum[j] & vert->sum) == vert->sum)
                vert->gfreq += set->stem_profiles[j]->freq;
        }
        if (vert->sfreq == 0)
            consolidated_make_oval_bracket(vert);
        fprintf(fp,"\"%s\" [label = \"%s\\n%d/%d\"];\n",vert->label,vert->bracket,vert->sfreq,vert->gfreq);
    }
    if (!strcmp(set->consolidated_graph->label,""))
        set->consolidated_graph->bracket = (char*)"[]";
    else
        consolidated_make_oval_bracket(set->consolidated_graph);
    fprintf(fp,"\"%s\" [label = \"%s\\n%d/%d\"];\n",set->consolidated_graph->label,set->consolidated_graph->bracket,set->consolidated_graph->sfreq,set->opt->NUMSTRUCTS);
    consolidated_graph[i] = set->consolidated_graph;
}

void consolidated_removeEdge(int i, int j) {
    node *root = consolidated_graph[i];

    //  printf("removing %s -> %s\n",root->label,root->neighbors[j]->label);
    if (j < root->numNeighbors-1) {
        int probe;
        probe = j;
        while(probe < root->numNeighbors - 1) {
            root->neighbors[probe] = root->neighbors[probe+1];
            root->diff[probe] = root->diff[probe+1];
            probe++;
        }
    }
    root->numNeighbors--;
}

void consolidated_removeEdges(HASHTBL *deleteHash) {
    int i,j;
    KEY *parent,*child;

    for (parent = hashtbl_getkeys(deleteHash); parent; parent = parent->next) {
        for (i = 0; i < CONSOLIDATED_GRAPHSIZE; i++) {
            if (!strcmp(consolidated_graph[i]->label, parent->data)) {
                break;
            }
        }
        if (i == CONSOLIDATED_GRAPHSIZE) {
            fprintf(stderr,"didn't find %s in consolidated_removeEdges\n",parent->data);
        }
        for (child = (KEY*) hashtbl_get(deleteHash,parent->data); child; child = child->next) {
            for (j = 0; j < consolidated_graph[i]->numNeighbors; j++) {
                if (!strcmp(consolidated_graph[i]->neighbors[j]->label,child->data)) {
                break;
                }
            }
            if (j == consolidated_graph[i]->numNeighbors) {
                fprintf(stderr,"didn't find %s in neighbors of %s in consolidated_removeEdges()\n",child->data,parent->data);
            }
            consolidated_removeEdge(i,j);
        }
    }
}


void consolidated_print_edges(FILE *fp,Set *set) {
    int i, j;
    char *diff=NULL;

    for (i = 0; i < CONSOLIDATED_GRAPHSIZE; i++) {
        if (consolidated_graph[i]->label != NULL) {
            if (set->opt->VERBOSE)
                printf("node: '%s'\n", consolidated_graph[i]->label);
            for (j = 0; j < consolidated_graph[i]->numNeighbors; j++) {
                diff = consolidated_graph[i]->diff[j];
                if (diff) {
                    fprintf(fp,"\"%s\" -> \"%s\" [label = \"%s\", arrowhead = vee];\n",consolidated_graph[i]->label,consolidated_graph[i]->neighbors[j]->label,diff);
                } else {
                    fprintf(stderr, "no diff for %s and %s in print_edges()\n",consolidated_graph[i]->label,consolidated_graph[i]->neighbors[j]->label);
                }
                //printf("'%s', ", graph[i].neighbors[j]->label);
            }
        }
    }
}