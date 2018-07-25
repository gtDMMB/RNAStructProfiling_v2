#ifndef CONSOLIDATED_GRAPH_H
#define CONSOLIDATED_GRAPH_H

#define SIZE 4

#include "Set.h"

int CONSOLIDATED_GRAPHSIZE;
node** consolidated_graph;

void consolidated_init_graph(FILE *fp, Set *set);
int consolidated_initialize(Set *set);
char* consolidated_convert_binary(Set* set, unsigned long binary);
void consolidated_found_edge(Set* set, node *child,node *parent) ;
void consolidated_find_LCAs(FILE *fp,Set *set,int i);
void consolidated_make_oval_bracket(node *vert);
void consolidated_calc_gfreq(FILE *fp,Set *set);
void consolidated_removeEdge(int i, int j);
void consolidated_removeEdges(HASHTBL *deleteHash);
void consolidated_print_edges(FILE *fp,Set *set);

#endif
