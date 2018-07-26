#ifndef Helix_class
#define Helix_class

typedef struct helix_class {
    char *id;
    char *maxtrip;
    char *avetrip;
    // changes maxtrip into a quadruplet, defined by the start and end nucleotides of each end of the helix
    int int_max_quad[4];
    // string with above info
    char* max_quad;
    //TODO: implement ave_quad
    /*
    // quadruplet, using avetrip instead of maxtrip
    int double_ave_quad[4];
    // a string with the above info
    char* ave_quad;
     */
    int freq;
    int isfreq;
    unsigned long binary;
} HC;

HC* create_HC(int id,char *max);
void free_hc(void *ptr);

#endif
