#ifndef SET
#define SET

#include "helix_class.h"
#include "Profile.h"
#include "hashtbl.h"
//#include "graph.h"
#include "Options.h"
#include "Profnode.h"
#include "stem.h"
#include "array_list.h"

//default value that looking for h dropoff starts at (in percent)
#define H_START 10
//default value that looking for p dropoff starts at (in percent)
#define P_START 5
#define HASHSIZE 31
#define ARRAYSIZE 20
#define INIT_SIZE 2
#define STRING_BUFFER 256
#define BASE_PROF_NUM 300

// Max length of a line in the structure file TODO: make into an option
#define MAX_STRUCT_FILE_LINE_LEN 512

typedef struct node
{
    char *label;
    struct node **neighbors;
    int numNeighbors;
    int nsize;
    int sfreq;
    int gfreq;
    char *bracket;
    char **diff;
    int DFS;
    unsigned long sum;
} node;

typedef struct {
    char *seq;
    char *structfile;
    int hc_size;
    int hc_num;
    int helsum;
    int num_fhc;
    HC **helices;
    int **joint;
    int prof_size;
    int prof_num;
    int num_sprof;
    Profile **profiles;
    Profnode ***proftree;
    int *treeindex;
    int treesize;
    //Profile **freqprof;  //?
    double h_cutoff;     //? in options already  set->inputprof = NULL;

    double p_cutoff;     //? in options already
    Options *opt;
    node *inputnode;
    node *graph;
    array_list_t* original_hc_stems;
    array_list_t* stems;
    array_list_t* featured_stems;

    int num_fstems;
    int num_stem_sprof;
    int stem_prof_num;
    Profile** stem_profiles;
    array_list_t** structures;
    array_list_t** stem_structures;
} Set;

node* createNode(char *name);
Set* make_Set(char *name);
void input_seq(Set *set,char *seqfile);
void process_structs(Set *set);
void process_structs_sfold(Set *set);
char* longest_possible(int id,int i,int j,int k,char *seq);
int match(int i,int j,char *seq);
void addHC(Set *set, HC *hc, int idcount);
void reorder_helices(Set *set);
int HC_freq_compare(const void *v1, const void *v2);
double set_threshold_entropy(Set *set);
void init_joint(Set *set);
double set_threshold(Set *set, int start);
int compare(const void *v1, const void *v2);
int print_all_helices(Set *set);
double set_num_fhc(Set *set);
void find_freq(Set *set);
int top_down_h(Set *set,int minh);
int find_kink(double *opt, int j);
void translate(Profile *prof);
double split(Set *set, int index);
int nodecompare(const void *v1, const void *v2);
int split_one(Set *set,Profnode *node,int index);
int check_hc(char *prof, int index);
char* convert(int *array,int length);
int top_down_p(Set *set,int h);
int find_kink_p(Profnode **profs,int start, int stop);
void print_topdown_prof(Set *set, int h, int p);

void make_profiles(Set *set);
void make_profiles_sfold(Set *set);
void calc_joint(Set *set, int *prof, int num);
char* process_profile(HASHTBL *halfbrac,int *profile,int numhelix,Set *set);
void make_bracket_rep(HASHTBL *brac,Profile *prof);
void make_brackets(HASHTBL *brac, int i, int j, int id);
void make_rep_struct(HASHTBL *consensus,char *profile, char* trips);
void print_meta(Set *set);
void print_profiles(Set *set);
int profsort(const void *v1, const void *v2);
double set_num_sprof(Set *set);
double set_p_threshold_entropy(Set *set);
void find_general_freq(Set *set);
int subset(Set *set,char *one, char *two);
unsigned long binary_rep(Set *set,char *profile);
void select_profiles(Set *set);
void process_one_input(Set *set);
int* process_native(Set *set,int i, int j, int k);
void find_consensus(Set *set);
int print_consensus(Set *set);
void free_Set(Set *set);

void add_structures_to_set(Set* ste);

// Concerning stems
void add_stems_to_set(Set* set);
bool combine_stems(Set* set);
void merge_stems(Stem* stem1, Stem* stem2);
bool validate_stems(Stem* stem1, Stem* stem2);
bool check_ends_to_combine_stems(Stem* stem1, Stem* stem2, int end_delta, int* outer_stem);

// Concerning functionally similar stems
void find_func_similar_stems(Set* set);
bool check_func_similar_stems(Set*set, Stem* stem1, Stem* stem2, int* freq);
bool validate_func_similar_stems(Set* set, Stem* stem1, Stem* stem2, int* freq);

bool combine_stems_using_func_similar(Set* set);
bool validate_stem_and_func_similar(FSStemGroup* stem_group, Stem* stem);
void merge_stem_and_fs_stem_group(Stem* stem, FSStemGroup* stem_group, int outer_stem);

bool stem_in_structure(Stem *stem, array_list_t* structure);
bool component_in_structure(DataNode* component, array_list_t* structure);
void update_freq_stems(Set* set);

void reindex_stems(Set *set);
int stem_freq_compare(const void* s1, const void* s2);
size_t int2size_t(int val);
void get_alpha_id(int int_id, char* alpha_id);
void generate_stem_key(Set* set, char* seqfile);
void print_stem_to_file(FILE* fp, Stem* stem);

double set_threshold_entropy_stems(Set *set);
double set_num_fstems(Set *set);
void find_featured_stems(Set* set);

void make_stem_profiles(Set* set);
bool stem_prof_exists(Set *set, char *stem_prof_str);
char* stem_profile_from_stem_structure(Set* set, array_list_t* structure);
char* strinsrt(char* sentence, char* word, int index);
char* strcut(char** str, int index, int n);
int component_start_i(DataNode *component, array_list_t *structure);
int component_end_i(DataNode *component, array_list_t *structure);
int stem_start_i(Stem *stem, array_list_t *structure);
int stem_end_i(Stem *stem, array_list_t *structure);
void join(char* dest, char** src, const char* delim, int n);

bool stem_is_hc(Stem* stem);

#endif
