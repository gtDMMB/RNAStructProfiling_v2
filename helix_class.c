#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "helix_class.h"
#include "Set.h"

HC* create_HC(int id, char* max) {
    HC *hc = (HC*) malloc(sizeof(HC));
    if (hc == NULL) {
      return NULL;
    }
    char *key = (char*) malloc(sizeof(char)*ARRAYSIZE);
    if (key == NULL) {
      free(hc);
      return NULL;
    }
    sprintf(key,"%d",id);
    hc->id = key;
    hc->maxtrip = max;
    hc->avetrip = NULL;

    //create quadruplets from triplets
    int i, j, k;
    sscanf(max, "%d %d %d", &i, &j, &k);
    hc->int_max_quad[0] = i;
    hc->int_max_quad[1] = i + k - 1;
    hc->int_max_quad[2] = j - k +1;
    hc->int_max_quad[3] = j;
    hc->max_quad = (char*) malloc(sizeof(*hc->max_quad) * STRING_BUFFER);
    if (hc->max_quad == NULL) {
      free(key);
      free(hc);
      return NULL;
    }
    sprintf(hc->max_quad, "%d %d %d %d", hc->int_max_quad[0], hc->int_max_quad[1], hc->int_max_quad[2],
          hc->int_max_quad[3]);

    hc->freq = 1;
    hc->isfreq = 0;
    hc->binary = 0;
    return hc;
}

int free_HC(HC* hc) {
    free(hc->max_quad);
    free(hc);
    return 0;
}


