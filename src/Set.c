#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Set.h"
#include "hashtbl.h"
#include "Options.h"
#include "helix_class.h"
#include "Profile.h"
#include "Profnode.h"
#include "stem.h"
#include <math.h>
#include <float.h>
#include <regex.h>

static HASHTBL *bp;
static HASHTBL *translate_hc;
static HASHTBL *consensus;

node* createNode(char *name)
{
    node* newNode;
    newNode = (node*)malloc(sizeof(node));
    newNode->label = name;
    newNode->neighbors = NULL;
    //newNode->neighbors = (node**)malloc(sizeof(node*)*GRAPHSIZE);
    newNode->nsize = 1;
    newNode->numNeighbors = 0;
    newNode->sfreq = 0;
    newNode->gfreq = 0;
    newNode->bracket = NULL;
    newNode->diff = NULL;
    newNode->DFS = 0;
    newNode->sum = 0;
    return newNode;
}

// TODO: impement free_node properly
void free_node(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    node* graph_node = (node*) ptr;
    free(graph_node->label);
    free(graph_node->bracket);
    for (int i = 0; i < graph_node->numNeighbors; i++) {
        free_node(graph_node->neighbors[i]);
    }
    free(graph_node);
}

Set* make_Set(char *name) {
    Set *set = (Set*) malloc(sizeof(Set));
    set->seq = NULL;
    set->structfile = (char *) malloc((strlen(name) + 1) * sizeof(char));
    strcpy(set->structfile, name);
    fprintf(stderr, "make_Set: Stored name = \"%s\"\n", set->structfile);
    set->hc_size = 5;
    set->hc_num = 0;
    set->helsum = 0;
    set->num_fhc = 0;
    set->helices = (HC**) malloc(sizeof(HC*)*ARRAYSIZE*10);
    set->prof_size = 5;
    set->prof_num = 0;
    set->num_sprof = 0;
    set->profiles = (Profile**) malloc(sizeof(Profile*)*BASE_PROF_NUM);
    set->treeindex = (int*) malloc(sizeof(int)*ARRAYSIZE*2);
    set->treesize = 2;
    set->h_cutoff = 0;
    set->p_cutoff = 0;
    set->opt = make_options();
    set->inputnode = NULL;
    set->graph = NULL;

    set->consolidated_graph = NULL;

    create_array_list();
    set->stems = create_array_list();
    set->featured_stem_ids = create_array_list();

    set->num_fstems = 0;
    set->stem_prof_num = 0;
    set->stem_prof_cap = BASE_PROF_NUM;
    set->num_s_stem_prof = 0;
    set->stem_profiles = (Profile**) malloc(sizeof(Profile*) * BASE_PROF_NUM);
    return set;
}

void free_Set(Set* set) {
    free(set->seq);
    free(set->structfile);
    if (set->helices != NULL) {
        for (int i = 0; i < set->hc_num; i++) {
            free_hc(set->helices[i]);
            set->helices[i] = NULL;
        }
        free(set->helices);
    }
    if (set->profiles != NULL) {
        for (int i = 0; i < set->prof_num; i++) {
            free_profile(set->profiles[i]);
        }
        free(set->profiles);
    }
    free(set->treeindex);
    // TODO: make free_node work
    //free_node(set->inputnode);
    //free_node(set->graph);
    //free_node(set->consolidated_graph);
    // TODO: fix memory leak caused by use of wrong free function. Using proper free function resulted in double free, likely due to the same stems/helices being used across multiple stems somewhere
    free_array_list(set->stems, &free);
    free_array_list(set->featured_stem_ids, &free);
    for (int i = 0; i < set->opt->NUMSTRUCTS; i++) {
        if (set->structures != NULL) {
            free_array_list(set->structures[i], &free);
        }
        if (set->stem_structures != NULL) {
            free_array_list(set->stem_structures[i], &free);
        }
    }
    if (set->stem_profiles != NULL) {
        for (int i = 0; i < set->stem_prof_num; i++) {
            free_profile(set->stem_profiles[i]);
        }
        free(set->stem_profiles);
    }
    free_options(set->opt);
    free(set->structures);
    free(set->stem_structures);
    free(set);
}

void input_seq(Set *set,char *seqfile) {
    FILE * fp;
    int size = 5,fasta = 0;
    char temp[100],*blank = (char*)" \n",*part,*final;

    fp = fopen(seqfile,(char*)"r");
    if (fp == NULL) {
        fprintf(stderr, "can't open %s\n",seqfile);
    }
    final = (char*) malloc(sizeof(char)*ARRAYSIZE*size);
    final[0] = '\0';
    while (fgets(temp,100,fp)) {
        //put error handling in case first line has more than 100 chars
        //skipping first line if fasta header and not seq
        if (temp[0] == '>' || fasta) {
            //printf("found fasta header %s",temp);
            if (strlen(temp) < 99 || (strlen(temp) == 99 && temp[98] == '\n')) {
                fasta = 0;
                continue;
            }
            else
                fasta = 1;
            continue;
        }
        for (part = strtok(temp,blank); part; part = strtok(NULL,blank)) {
            if (strlen(final)+strlen(part) > (unsigned int)ARRAYSIZE*size-1) {
                while ((unsigned int)(++size*ARRAYSIZE - 1) < strlen(final)+strlen(part)) ;
                final = (char*) realloc(final,sizeof(char)*ARRAYSIZE*size);
            }
            final = strcat(final,part);
        }
    }
    if (set->opt->VERBOSE)
        printf("seq in %s is %s with length %d\n",seqfile,final,(signed int)strlen(final));
    //printf("final char is %c\n",final[strlen(final)-1]);
    set->seq = final;
}

void process_structs(Set *set) {
    FILE *fp;
    int i,j,k,*helixid,idcount=1,*lg,last = 0, toosmall = 0,numhelix=0,*profile=NULL,size=1,seqsize;
    double *trip;
    char *tmp,*key,dbl[ARRAYSIZE],*max, *delim = (char*)" ,\t\n", *val;
    HASHTBL *halfbrac,*extra,*avetrip;
    HC *hc;

    fp = fopen(set->structfile,(char*)"r");
    if (fp == NULL) {
        fprintf(stderr, "can't open %s\n",set->structfile);
    }
    if (!(bp = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for bp failed");
        exit(EXIT_FAILURE);
    }
    if (!(avetrip = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for avetrip failed");
        exit(EXIT_FAILURE);
    }
    if (!(extra = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for extra failed");
        exit(EXIT_FAILURE);
    }
    if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for halfbrac failed");
        exit(EXIT_FAILURE);
    }
    profile = (int*) malloc(sizeof(int)*ARRAYSIZE);
    key = (char*) malloc(sizeof(char)*ARRAYSIZE);
    seqsize = strlen(set->seq)*10;
    tmp = (char*) malloc(sizeof(char)*seqsize);
    while (fgets(tmp,seqsize,fp)) {
        numhelix=0;
        val = strtok(tmp,delim);
        val = strtok(NULL,delim);
        while ((val = strtok(NULL,delim))) {
            i = atoi(val);
            if ((val = strtok(NULL,delim)))
                j = atoi(val);
            else
                fprintf(stderr, "Error in file input format: struct %d\n",set->opt->NUMSTRUCTS);
            if ((val = strtok(NULL,delim)))
                k = atoi(val);
            else
                fprintf(stderr, "Error in file input format: struct %d\n",set->opt->NUMSTRUCTS);
            if (k < set->opt->MIN_HEL_LEN) {
                toosmall++;
                //printf("too small %d %d %d\n",i,j,k);
                continue;
            }
            sprintf(dbl,"%d %d",i,j);
            helixid = (int*)hashtbl_get(bp,dbl);
            numhelix++;
            //printf("%d %d %d\n",i,j,k);
            if (!helixid) {
                max = longest_possible(idcount,i,j,k,set->seq);
                hc = create_HC(idcount,max);
                addHC(set,hc,idcount);
                //triplet stats
                trip = (double*) malloc(sizeof(double)*3);
                trip[0] = i;
                trip[1] = j;
                trip[2] = k;
                sprintf(key,"%d",idcount);
                hashtbl_insert(avetrip,key,trip);
                last = idcount++;
            }
            else {
                //printf("Found %d %d with id %d\n",i,j,*helixid);
                sprintf(key,"%d",*helixid);
                if (last != *helixid) {
                    hc = set->helices[*helixid-1];
                    hc->freq++;
                } else {
                    if ((lg = (int*) hashtbl_get(extra,key)))
                        ++*lg;
                    else {
                        lg = (int*) malloc(sizeof(int));
                        *lg = 1;
                        hashtbl_insert(extra,key,lg);
                    }
                    //if (VERBOSE)
                    //printf("Found repeat id %d:%s\n",last,hashtbl_get(idhash,key));
                }
                //average stats
                trip = (double*) hashtbl_get(avetrip,key);
                trip[0] += i;
                trip[1] += j;
                trip[2] += k;

                last = *helixid;
            }
            if (numhelix >= ARRAYSIZE*size)
                profile = (int*) realloc(profile,sizeof(int)*ARRAYSIZE*++size);
            profile[numhelix-1] = last;
            //calc_joint(set,prof,numhelix);
        }
        set->opt->NUMSTRUCTS++;
    }
    for (i = 1; i < idcount; i++) {
        j = set->helices[i-1]->freq;
        sprintf(key,"%d",i);
        if ((lg = (int*) hashtbl_get(extra,key)))
            k = j + *lg;
        else
            k = j;
        trip = (double*) hashtbl_get(avetrip,key);
        sprintf(key,"%.1f %.1f %.1f", trip[0]/k,trip[1]/k,trip[2]/k);
        hc = set->helices[i-1];
        hc->avetrip = mystrdup(key);
    }
    if (fclose(fp))
        fprintf(stderr, "File %s not closed successfully\n",set->structfile);
    free_hashtbl(extra);
    free_hashtbl(avetrip);
}

/*calculates joint for last value in prof vs everything else

void calc_joint(Set *set, int *prof, int num) {
  int k, *freq;
  char val[ARRAYSIZE];
  HASHTBL *hash;

  if (!set->joint[prof[num-1]-1])
    set->joint[prof[num-1]-1] = create_hashtbl(HASHSIZE,NULL);
  if (num < 2)
    return;
  for (k = 0; k < num-1; k++) {
    if (prof[k] < prof[num-1]) {
      sprintf(val,"%d",prof[num-1]);
      hash = set->joint[prof[k]-1];
      if (!hash) fprintf(stderr,"hash doesn't exist in calc_joint\n");
      freq = (int*) hashtbl_get(hash,val);
      if (freq)
	*freq++;
      else {
	freq = (int*) malloc(sizeof(int));
	*freq = 1;
	hashtbl_insert(hash,val,freq);
      }
    }
    else {
      sprintf(val,"%d",prof[k]);
      hash = set->joint[prof[num-1]-1];
      if (!hash) fprintf(stderr,"hash doesn't exist in calc_joint\n");
      freq = (int*) hashtbl_get(hash,val);
      if (freq)
	*freq++;
      else {
	freq = (int*) malloc(sizeof(int));
	*freq = 1;
	hashtbl_insert(hash,val,freq);
      }
    }
  }
  return;
}
*/
void process_structs_sfold(Set *set) {
    FILE *fp;
    int i,j,k,*helixid,idcount=1,*lg,last = 0, toosmall = 0,numhelix=0,*profile=NULL,size=1;
    double *trip;
    char tmp[100],*key,dbl[ARRAYSIZE],*max;
    HASHTBL *halfbrac,*extra,*avetrip;
    HC *hc;

    fp = fopen(set->structfile,"r");
    if (fp == NULL) {
        fprintf(stderr, "can't open %s\n",set->structfile);
    }
    if (!(bp = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for bp failed");
        exit(EXIT_FAILURE);
    }
    if (!(avetrip = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for avetrip failed");
        exit(EXIT_FAILURE);
    }
    if (!(extra = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for extra failed");
        exit(EXIT_FAILURE);
    }
    if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ER"
                        "ROR: create_hashtbl() for halfbrac failed");
        exit(EXIT_FAILURE);
    }
    key = (char*) malloc(sizeof(char)*ARRAYSIZE);
    while (fgets(tmp,100,fp) != NULL) {
        if (sscanf(tmp,"%d %d %d",&i,&j,&k) == 3) {
            if (i == 0) continue;
            if (k < set->opt->MIN_HEL_LEN) {
                toosmall++;
                //printf("too small %d %d %d\n",i,j,k);
                continue;
            }
            sprintf(dbl,"%d %d",i,j);
            helixid = (int*) hashtbl_get(bp,dbl);
            //printf("%d %d %d\n",i,j,k);
            if (!helixid) {
                max = longest_possible(idcount,i,j,k,set->seq);
                hc = create_HC(idcount,max);
                addHC(set,hc,idcount);
                //triplet stats
                trip = (double*)malloc(sizeof(double)*3);
                trip[0] = i;
                trip[1] = j;
                trip[2] = k;
                sprintf(key,"%d",idcount);
                hashtbl_insert(avetrip,key,trip);
                if (set->opt->TOPDOWN) {
                    if (numhelix >= ARRAYSIZE*size)
                        profile = (int*) realloc(profile,sizeof(int)*ARRAYSIZE*++size);
                    profile[numhelix++]=idcount;
                    make_brackets(halfbrac,i,j,idcount);
                }
                last = idcount++;
            }
            else {
                //printf("Found %d %d with id %d\n",i,j,*helixid);
                sprintf(key,"%d",*helixid);
                if (last != *helixid) {
                    hc = set->helices[*helixid-1];
                    hc->freq++;
                    if (set->opt->TOPDOWN) {
                        if (numhelix >= ARRAYSIZE*size)
                            profile = (int*)realloc(profile,sizeof(int)*ARRAYSIZE*++size);
                        profile[numhelix++]=*helixid;
                        make_brackets(halfbrac,i,j,*helixid);
                    }
                } else {
                    if ((lg = (int*) hashtbl_get(extra,key)))
                        ++*lg;
                    else {
                        lg = (int*) malloc(sizeof(int));
                        *lg = 1;
                        hashtbl_insert(extra,key,lg);
                    }
                    //if (VERBOSE)
                    //printf("Found repeat id %d:%s\n",last,hashtbl_get(idhash,key));
                }
                //average stats
                trip = (double*) hashtbl_get(avetrip,key);
                trip[0] += i;
                trip[1] += j;
                trip[2] += k;

                last = *helixid;
            }
        }
        else if (sscanf(tmp,"Structure %d",&i) == 1) {
            if (set->opt->TOPDOWN) {
                if (profile) {
                    process_profile(halfbrac,profile,numhelix,set);
                    if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
                        fprintf(stderr, "ERROR: create_hashtbl() for halfbrac failed");
                        exit(EXIT_FAILURE);
                    }
                } else
                    profile = (int*) malloc(sizeof(int)*ARRAYSIZE);
                numhelix = 0;
            }
            set->opt->NUMSTRUCTS++;
        }
    }
    if (set->opt->TOPDOWN)
        process_profile(halfbrac,profile,numhelix,set);
    for (i = 1; i < idcount; i++) {
        j = set->helices[i-1]->freq;
        sprintf(key,"%d",i);
        if ((lg = (int*) hashtbl_get(extra,key)))
            k = j + *lg;
        else
            k = j;
        trip = (double*) hashtbl_get(avetrip,key);
        sprintf(key,"%.1f %.1f %.1f", trip[0]/k,trip[1]/k,trip[2]/k);
        hc = set->helices[i-1];
        hc->avetrip = mystrdup(key);
    }
    if (fclose(fp))
        fprintf(stderr, "File %s not closed successfully\n",set->structfile);
    free_hashtbl(extra);
    free_hashtbl(avetrip);
}

char* longest_possible(int id,int i,int j,int k,char *seq) {
    int m = 1,*check,*num,diff;
    char *val;

    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    for (diff = j-i-2*(k+1); diff >= 2 && match(i+k,j-k,seq); diff = j-i-2*(k+1)) k++;
    //if (diff < 2 && match(i+k,j-k)) printf("found overlap for %d %d %d\n",i,j,k+1);
    while (match(i-m,j+m,seq)) m++;
    m--;
    i -= m;
    j += m;
    k+= m;
    sprintf(val,"%d %d %d",i,j,k);

    num = (int*) malloc(sizeof(int));
    *num = id;
    for (m = 0; m < k; m++) {
        sprintf(val,"%d %d",i+m,j-m);
        if ((check = (int*) hashtbl_get(bp,val)))
            printf("%s (id %d) already has id %d\n",val,id,*check);
        hashtbl_insert(bp,val,num);
    }
    sprintf(val,"%d %d %d",i,j,k);
    return val;
}

//finds longest possible helix based on i,j
//populates all bp in longest possible with id in hash
int match(int i,int j,char *seq) {
    char l,r;
    if (i >= j) return 0;
    if (i < 1) return 0;
    if ((unsigned int) j > strlen(seq)) return 0;
    l = seq[i-1];
    r = seq[j-1];
    //printf("l(%d) is %c and r(%d) is %c\n",i,l,j,r);
    if ((l == 'a' || l == 'A') && (r == 'u' || r == 'U' || r == 't' || r == 'T'))
        return 1;
    else if ((l == 'u' || l == 'U' || l == 't' || l == 'T') && (r == 'a' || r == 'A' || r == 'g' || r == 'G'))
        return 1;
    else if ((l == 'c' || l == 'C') && (r == 'g' || r == 'G'))
        return 1;
    else if ((l == 'g' || l == 'G') && (r == 'c' || r == 'C' || r == 'u' || r == 'U' || r == 't' || r == 'T' ))
        return 1;
    else
        return 0;
}

void addHC(Set *set, HC *hc, int idcount) {
    int k;

    if (idcount > ARRAYSIZE*set->hc_size) {
        set->hc_size++;
        k = set->hc_size;
        set->helices = (HC**) realloc(set->helices, sizeof(HC*)*ARRAYSIZE*k);
        /*set->joint = (int**) realloc(set->joint, sizeof(int*)*ARRAYSIZE*k);
        for (i=ARRAYSIZE*(k - 1); i < ARRAYSIZE*k; i++) {
          set->joint[i] = (int*) malloc(sizeof(int)*(k-1-i));
        }
        */
    }

    set->helices[idcount-1] = hc;
    set->hc_num = idcount;
}

void reorder_helices(Set *set) {
    int i,*nw, total, sum=0;
    char *old;
    HC **helices = set->helices;

    total = set->hc_num;
    if (!(translate_hc = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for translate_hc failed");
        exit(EXIT_FAILURE);
    }

    qsort(helices, total, sizeof(HC *), HC_freq_compare);
    for (i = 0; i < total; i++) {
        old = helices[i]->id;
        nw = (int*) malloc(sizeof(int)*ARRAYSIZE);
        *nw = i+1;
        hashtbl_insert(translate_hc,old,nw);
        sprintf(old,"%d",i+1);
        sum += helices[i]->freq;
    }
    set->helsum = sum;
}

//will sort to have descending freq
int HC_freq_compare(const void *v1, const void *v2) {
    HC *h1,*h2;
    h1 = *((HC**)v1);
    int i1 = h1->freq;
    h2 = *((HC**)v2);
    int i2 = h2->freq;
    return (i2 - i1);
}

double set_threshold_entropy(Set *set) {
    int i=0;
    double ent =0,frac,last=0,ave,norm;
    HC **list = set->helices;

    norm = (double)list[i]->freq;
    for (i=0; i < set->hc_num; i++) {
        frac = (double)list[i]->freq/norm;
        ent -= frac*log(frac);
        if (frac != 1)
            ent -= (1-frac)*log(1-frac);
        ave = ent/(double)(i+1);
        if ((ave > last) || (fabs(ave-last) < FLT_EPSILON*2)) {
            last = ave;
        }
        else {
            //printf("%f is lower than %f\n",ent/(i+1), last);
            set->num_fhc = i;
            //init_joint(set);
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
    }
    return 0;
}

/*LU array
void init_joint(Set *set) {
  int i,j,k=set->num_fhc;

  set->joint = (int**) malloc(sizeof(int*)*(k-1));
  for (i = 0; i < k-1; i++) {
    set->joint[i] = (int*) malloc(sizeof(int)*(k-1-i));
    for (j = 0; j < k-1-i; j++) {
      set->joint[i][j] = 0;
    }
  }
}
*/
int compare(const void *v1, const void *v2) {
    return (*(int*)v1 - *(int*)v2);
}

//looks up and prints actual helices for all id's
int print_all_helices(Set *set) {
    HC **helices = set->helices;
    char *val,*trip;
    int i,k,m,total = set->hc_num,target=0,cov=0;
    FILE* helix_file = fopen("helices.txt", "w");

    target = set->opt->COVERAGE*set->helsum;
    for (i = 0; i < total; i++) {
        val = helices[i]->maxtrip;
        m = helices[i]->freq;
        trip = helices[i]->avetrip;
        fprintf(helix_file, "Helix %d is %s (%s) with freq %d\n", i + 1, val, trip, m);
        if (set->opt->VERBOSE) {
            if (val != NULL) {
                printf("Helix %d is %s (%s) with freq %d\n", i + 1, val, trip, m);
            } else
                printf("No entry for %d\n",i);
        }
        if (cov < target) {
            cov += helices[i]->freq;
            k = i;
        }
    }
    fclose(helix_file);
    printf("%.2f coverage of helices after first %d HC\n",set->opt->COVERAGE,k+1);
    return k+1;
}

double set_num_fhc(Set *set) {
    int marg;
    double percent;

    if (set->opt->NUM_FHC > set->hc_num) {
        printf("Number of requested fhc %d greater than total number of hc %d\n",set->opt->NUM_FHC, set->hc_num);
        set->opt->NUM_FHC = set->hc_num;
    }
    if (set->opt->NUM_FHC > 63)
        printf("Warning: requested num fhc %d exceeds limit of 63\n",set->opt->NUM_FHC);

    marg = set->helices[set->opt->NUM_FHC-1]->freq;
    percent = ((double) marg)*100.0/((double)set->opt->NUMSTRUCTS);
    return percent;
}
/*
void find_bools(Set *set) {
  int i,j,k,joint;
  double cond1,cond2,freqthresh=0.1,thresh=0.1;

  //find 10% thresh
  for (k = set->num_fhc; k < set->hc_num && set->helices[k]->freq > NUMSTRUCTS*freqthresh; k++) ;
  //i only ranges in entropy features, j to 10%
  for (i=0; i < set->num_fhc; i++) {
    for (j=i+1; j < k; j++) {
      joint = set->joint[i];
      cond1 = (double)set->joint[i][j]/(double)set->helices[i+j+1]->freq;
      cond2 = (double)set->joint[i][j]/(double)set->helices[i]->freq;
      if (set->opt->VERBOSE) {
	printf("p(%d|%d) = %.3f\n",i+1,i+j+2,cond1);
	printf("p(%d|%d) = %.3f\n",i+j+2,i+1,cond2);
      }
      if (cond1 > 1-thresh && cond2 > 1-thresh) {
	printf("%d AND %d\n", i+1,i+j+2);
      } else if (cond1 < thresh && cond2 < thresh)
	printf("%d XOR %d\n",i+1,i+j+2);
    }
  }
}
*/
void print_meta(Set *set) {
    int i,j,k=set->num_fhc-1;
    double cond1,cond2,thresh=0.1;

    for (i=0; i < k; i++) {
        for (j=0; j < k-i; j++) {
            cond1 = (double)set->joint[i][j]/(double)set->helices[i+j+1]->freq;
            cond2 = (double)set->joint[i][j]/(double)set->helices[i]->freq;
            if (set->opt->VERBOSE) {
                printf("p(%d|%d) = %.3f\n",i+1,i+j+2,cond1);
                printf("p(%d|%d) = %.3f\n",i+j+2,i+1,cond2);
            }
            if (cond1 > 1-thresh && cond2 > 1-thresh) {
                printf("%d AND %d\n", i+1,i+j+2);
            } else if (cond1 < thresh && cond2 < thresh)
                printf("%d XOR %d\n",i+1,i+j+2);
        }
    }
}

/*assumes threshold found, now setting actual helices
  calculates conditionals, augments features with appropriate
*/
void find_freq(Set *set) {
    int marg,i,total;
    double percent,cov=0;
    HC *hc;

    total = set->hc_num;
    set->num_fhc = set->hc_num;
    for (i = 0; i < total; i++) {
        hc = set->helices[i];
        marg = hc->freq;
        percent = ((double) marg*100.0)/((double)set->opt->NUMSTRUCTS);
        if (percent >= set->opt->HC_FREQ) {
            if (set->opt->VERBOSE)
                printf("Featured helix %d: %s with freq %d\n",i+1,hc->maxtrip,marg);
            hc->isfreq = 1;
            hc->binary = 1<<i;
            cov += (double)marg/(double)set->helsum;;
        }
        else {
            set->num_fhc = i;
            i = total;
        }
    }
    //TODO: fix cov in this case (when featured helices are capped)
    if (set->num_fhc > 63) {
        //fprintf(stderr,"number of helices greater than allowed in find_freq()\n");
        set->num_fhc = 63;
        marg = set->helices[62]->freq;
        printf("Capping at 63 fhc with freq %d\n",marg);
        set->opt->HC_FREQ = ((double) marg)*100.0/((double)set->opt->NUMSTRUCTS);
    }
    printf("Coverage by featured helix classes: %.3f\n",cov);
}

void make_profiles(Set *set) {
    FILE *fp,*file;
    int num=1,*id = 0,i,j,k,last = -1, lastfreq = -1,*profile,totalhc=0,allhelix=0;
    int numhelix = 0,size=1,tripsize = INIT_SIZE,allbp=0,fbp=0,seqsize;
    double coverage=0,bpcov=0;
    char *temp,val[ARRAYSIZE],*trips,*name,*prof,*sub,*delim = (char*)" ,\t\n";
    HASHTBL *halfbrac;

    name = set->structfile;
    profile = (int*) malloc(sizeof(int)*ARRAYSIZE);

    if (!(halfbrac = create_hashtbl(HASHSIZE,NULL))) {
        fprintf(stderr, "ERROR: hashtbl_create() for halfbrac failed");
        exit(EXIT_FAILURE);
    }
    if (set->opt->REP_STRUCT) {
        if (!(consensus = create_hashtbl(HASHSIZE,NULL))) {
            fprintf(stderr, "ERROR: hashtbl_create() for consensus failed");
            exit(EXIT_FAILURE);
        }
        trips = (char*) malloc(sizeof(char)*tripsize*ARRAYSIZE);
        trips[0] = '\0';
    }
    fp = fopen(name,"r");
    if (fp == NULL) {
        fprintf(stderr, "can't open %s\n",name);
        return;
    }
    file = fopen("structure.out","w");
    if (file == NULL)
        fprintf(stderr,"Error: can't open structure.out\n");
    fprintf(file,"Processing %s\n",name);
    seqsize = strlen(set->seq)*10;
    temp = (char*) malloc(sizeof(char)*seqsize);
    while (fgets(temp,seqsize,fp)) {
        fprintf(file,"Structure %d: ",num++);
        sub = strtok(temp,delim);
        sub = strtok(NULL,delim);
        while ((sub = strtok(NULL,delim))) {
            i = atoi(sub);
            if ((sub = strtok(NULL,delim)))
                j = atoi(sub);
            else
                fprintf(stderr, "Error in file input format, processing %s\n",sub);
            if ((sub = strtok(NULL,delim)))
                k = atoi(sub);
            else
                fprintf(stderr, "Error in file input format, processing %s\n",sub);
            if (k < set->opt->MIN_HEL_LEN)
                continue;
            sprintf(val,"%d %d",i,j);
            id = (int*) hashtbl_get(bp,val);
            sprintf(val,"%d",*id);
            totalhc++;
            allhelix++;
            allbp+=k;
            id = (int*) hashtbl_get(translate_hc,val);
            if (!id) fprintf(stderr,"no valid translation hc exists for %s\n",val);
            if (*id != -1 && *id != last) {
                fprintf(file,"%d ",*id);
                if (*id <= set->num_fhc && *id != lastfreq) {
                    numhelix++;
                    fbp+=k;
                    if (numhelix >= ARRAYSIZE*size)
                        profile = (int*) realloc(profile,sizeof(int)*ARRAYSIZE*++size);
                    profile[numhelix-1] = *id;
                    //calc_joint(set,profile,numhelix);
                    make_brackets(halfbrac,i,j,*id);
                    lastfreq = *id;
                }
                last = *id;
                //printf("assigning %d to %d %d\n",*id,i,j);
            }
            if (set->opt->REP_STRUCT) {
                sprintf(val,"%d %d %d ",i,j,k);
                while (strlen(trips)+strlen(val) > (unsigned int)(ARRAYSIZE*tripsize-1))
                    trips = (char*) realloc(trips,sizeof(char)*++tripsize*ARRAYSIZE);
                strcat(trips,val);
            }
        }
        prof = process_profile(halfbrac,profile,numhelix,set);
        //printf("processing %d with profile %s\n",num,prof);
        fprintf(file,"\n\t-> %s\n",prof);

        if (set->opt->REP_STRUCT) {
            make_rep_struct(consensus,prof,trips);
        }

        if (!(halfbrac = create_hashtbl(HASHSIZE,NULL))) {
            fprintf(stderr, "ERROR: hashtbl_create() for halfbrac failed");
            exit(EXIT_FAILURE);
        }
        last = 0;
        lastfreq = 0;
        coverage += ((double)numhelix/(double)allhelix);
        bpcov += ((double)fbp/(double)allbp);
        numhelix = 0;
        allhelix = 0;
        if (set->opt->REP_STRUCT)
            trips[0] = '\0';
    }
    if (set->opt->VERBOSE) {
        printf("Ave number of HC per structure: %.1f\n",(double)totalhc/(double) set->opt->NUMSTRUCTS);
        printf("Ave structure coverage by fhc: %.2f\n",coverage/(double)set->opt->NUMSTRUCTS);
        printf("Ave structure coverage by fbp: %.2f\n",bpcov/(double)set->opt->NUMSTRUCTS);
    }
    free(profile);
    fclose(fp);
    fclose(file);
}

/*calculates joint for last value in prof vs everything else
Moved up to process_structs

void calc_joint(Set *set, int *prof, int num) {
  int k;

  if (num < 2)
    return;
  for (k = 0; k < num-1; k++) {
    if (prof[k] < prof[num-1])
      set->joint[prof[k]-1][prof[num-1]-prof[k]-1]++;
    else
      set->joint[prof[num-1]-1][prof[k]-prof[num-1]-1]++;
    //if (j)
  }
  return;
}
*/
void make_profiles_sfold(Set *set) {
    FILE *fp,*file;
    int num=0,*id = 0,i,j,k,last = -1, lastfreq = -1,*profile,totalhc=0,allhelix=0;
    int numhelix = 0,size=1,tripsize = INIT_SIZE,allbp=0,fbp=0;
    double coverage=0,bpcov=0;
    char temp[100],val[ARRAYSIZE],*trips,*name,*prof;
    HASHTBL *halfbrac;

    name = set->structfile;
    profile = (int*) malloc(sizeof(int)*ARRAYSIZE);

    if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for halfbrac failed");
        exit(EXIT_FAILURE);
    }
    if (set->opt->REP_STRUCT) {
        if (!(consensus = create_hashtbl(HASHSIZE, NULL))) {
            fprintf(stderr, "ERROR: create_hashtbl() for consensus failed");
            exit(EXIT_FAILURE);
        }
        trips = (char*) malloc(sizeof(char)*tripsize*ARRAYSIZE);
        trips[0] = '\0';
    }
    fp = fopen(name,"r");
    if (fp == NULL) {
        fprintf(stderr, "can't open %s\n",name);
        return;
    }
    file = fopen("structure.out","w");
    if (file == NULL)
        fprintf(stderr,"Error: can't open structure.out\n");
    fprintf(file,"Processing %s\n",name);
    while (fgets(temp,100,fp) != NULL) {

        if (sscanf(temp,"Structure %d",&num) == 1) {
            if (last == -1) {
                fprintf(file,"Structure %d: ",num);
                continue;
            }
            prof = process_profile(halfbrac,profile,numhelix,set);
            //printf("processing %d with profile %s\n",num,prof);
            fprintf(file,"\n\t-> %s\nStructure %d: ",prof,num);

            if (set->opt->REP_STRUCT) {
                make_rep_struct(consensus,prof,trips);
            }

            if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
                fprintf(stderr, "ERROR: create_hashtbl() for halfbrac failed");
                exit(EXIT_FAILURE);
            }
            last = 0;
            lastfreq = 0;
            coverage += ((double)numhelix/(double)allhelix);
            bpcov += ((double)fbp/(double)allbp);
            numhelix = 0;
            allhelix = 0;
            if (set->opt->REP_STRUCT)
                trips[0] = '\0';
        }
        else if (sscanf(temp,"%d %d %d",&i,&j,&k) == 3) {
            if (k < set->opt->MIN_HEL_LEN)
                continue;
            sprintf(val,"%d %d",i,j);
            id = (int*) hashtbl_get(bp,val);
            sprintf(val,"%d",*id);
            totalhc++;
            allhelix++;
            allbp+=k;
            id = (int*) hashtbl_get(translate_hc,val);
            if (!id) fprintf(stderr,"no valid translation hc exists for %s\n",val);
            if (*id != -1 && *id != last) {
                fprintf(file,"%d ",*id);
                if (*id <= set->num_fhc && *id != lastfreq) {
                    numhelix++;
                    fbp+=k;
                    if (numhelix >= ARRAYSIZE*size)
                        profile = (int*) realloc(profile,sizeof(int)*ARRAYSIZE*++size);
                    profile[numhelix-1] = *id;
                    make_brackets(halfbrac,i,j,*id);
                    lastfreq = *id;
                }
                last = *id;
                //printf("assigning %d to %d %d\n",*id,i,j);
            }
            if (set->opt->REP_STRUCT) {
                sprintf(val,"%d %d %d ",i,j,k);
                while (strlen(trips)+strlen(val) > (unsigned int)(ARRAYSIZE*tripsize-1))
                    trips = (char*) realloc(trips,sizeof(char)*++tripsize*ARRAYSIZE);
                strcat(trips,val);
            }
        }
    }
    fprintf(file,"\n\t-> %s ",prof);
    process_profile(halfbrac,profile,numhelix,set);
    //fprintf(file,"Structure %d: %s\n",num,profile);
    printf("Ave number of HC per structure: %.1f\n",(double)totalhc/(double) set->opt->NUMSTRUCTS);
    printf("Ave structure coverage by fhc: %.2f\n",coverage/(double)set->opt->NUMSTRUCTS);
    printf("Ave structure coverage by fbp: %.2f\n",bpcov/(double)set->opt->NUMSTRUCTS);
    free(profile);
    profile = NULL;
    fclose(fp);
    fclose(file);
}

char* process_profile(HASHTBL *halfbrac,int *profile,int numhelix,Set *set) {
    int i,size=INIT_SIZE;
    char val[ARRAYSIZE],*dup;
    Profile **profiles;

    profiles = set->profiles;
    qsort(profile,numhelix,sizeof(int),compare);

    dup = (char*) malloc(sizeof(char)*ARRAYSIZE*size);
    dup[0] = '\0';
    for (i = 0; i < numhelix; i++) {
        sprintf(val,"%d ",profile[i]);
        if (strlen(dup) + strlen(val) >= (unsigned int) ARRAYSIZE*size-1) {
            dup = (char*) realloc(dup,sizeof(char)*ARRAYSIZE*++size);
        }
        dup = strcat(dup,val);
    }
    for (i = 0; i < set->prof_num; i++) {
        if (!strcmp(profiles[i]->profile,dup)) {
            profiles[i]->freq++;
            break;
        }
    }
    if (i == set->prof_num) {
        if (i >= ARRAYSIZE*set->prof_size) {
            set->prof_size++;
            profiles = (Profile**) realloc(profiles,sizeof(Profile*)*ARRAYSIZE*set->prof_size);
            set->profiles = profiles;
        }
        profiles[i] = create_profile(dup);
        //printf("adding profile[%d] %s\n",i,profiles[i]->profile);
        set->prof_num++;
        make_bracket_rep(halfbrac,profiles[i]);
    }
    free_hashtbl(halfbrac);
    return dup;
}

//makes the bracket representation of dup, using values in hashtbl brac
//dup is a (mod) profile in graph
//called by process_profile()
void make_bracket_rep(HASHTBL *brac,Profile *prof) {
    int num,*array,k=0,size = INIT_SIZE,total;
    char *profile,*val;
    KEY *node = NULL;

    num = hashtbl_numkeys(brac);
    array = (int*) malloc(sizeof(int)*num);
    for (node = hashtbl_getkeys(brac); node; node=node->next)
        array[k++] = atoi(node->data);
    //sort by i,j position
    qsort(array,num,sizeof(int),compare);
    profile = (char*) malloc(sizeof(char)*ARRAYSIZE*size);
    profile[0] = '\0';
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    for (k = 0; k < num; k++) {
        sprintf(val,"%d",array[k]);
        val = (char*) hashtbl_get(brac,val);
        if ((total = strlen(profile)+strlen(val)) > ARRAYSIZE*size-1)
            profile = (char*) realloc(profile,sizeof(char)*ARRAYSIZE*++size);
        strcat(profile,val);
    }
    prof->bracket = profile;
    //free(val);
    free(array);
}

//inserts bracket representation for i,j into a hash
void make_brackets(HASHTBL *brac, int i, int j, int id) {
    char key[ARRAYSIZE],*val;

    sprintf(key,"%d",i);
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    sprintf(val,"[%d",id);
    //  printf("making bracket %s for %d\n",val,i);
    hashtbl_insert(brac,key,val);
    sprintf(key,"%d",j);
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    val[0] = ']';
    val[1] = '\0';
    hashtbl_insert(brac,key,val);
}

void make_rep_struct(HASHTBL *consensus,char *profile, char* trips) {
    int *bpfreq,i,j,k;
    char *val,*blank = (char*)" ",bpair[ARRAYSIZE];
    HASHTBL *ij;

    ij = (HASHTBL*) hashtbl_get(consensus,profile);
    if (!ij) {
        if (!(ij = create_hashtbl(HASHSIZE, NULL))) {
            fprintf(stderr, "ERROR: create_hashtbl() for ij failed");
            exit(EXIT_FAILURE);
        }
        hashtbl_insert(consensus,profile,ij);
    }
    for (val = strtok(trips,blank); val; val = strtok(NULL,blank)) {
        i = atoi(val);
        j = atoi(strtok(NULL,blank));
        k = atoi(strtok(NULL,blank));
        for (k--; k >= 0; k--) {
            sprintf(bpair,"%d %d",i+k,j-k);
            bpfreq = (int*)hashtbl_get(ij,bpair);
            if (bpfreq)
                (*bpfreq)++;
            else {
                bpfreq = (int*) malloc(sizeof(int));
                *bpfreq = 1;
                hashtbl_insert(ij,bpair,bpfreq);
            }
            //printf("in rep struct for %s, inserting %d %d\n",profile,i+k,j-k);
        }
    }
}

void print_profiles(Set *set) {
    int i;
    Profile **profiles = set->profiles;

    find_general_freq(set);
    qsort(profiles,set->prof_num,sizeof(Profile*),profsort);
    for (i = 0; i < set->prof_num; i++)
        if (set->opt->VERBOSE)
            printf("Profile %s with freq %d (%d)\n",profiles[i]->profile,profiles[i]->freq,profiles[i]->genfreq);
}

int profsort(const void *v1, const void *v2) {
    Profile *p1,*p2;
    p1 = *((Profile**)v1);
    p2 = *((Profile**)v2);
    //return (p2->genfreq + p2->freq - p1->genfreq - p1->freq);
    //return (p2->genfreq - p1->genfreq);
    return (p2->freq - p1->freq);
}

double set_num_sprof(Set *set) {
    int marg;

    if (set->opt->NUM_SPROF > set->prof_num) {
        printf("Number of requested sprof %d greater than total number of hc %d\n",set->opt->NUM_SPROF, set->prof_num);
        set->opt->NUM_SPROF = set->prof_num;
    }
    /*
    for (k = set->opt->NUM_SPROF; set->profiles[set->opt->NUM_SPROF-1]->freq == set->profiles[k]->freq; k++)
      if (k == set->prof_num -1)
        break;
    if (k != set->opt->NUM_SPROF) {
      if (set->opt->VERBOSE)
        printf("Increasing number of sprof to %d\n",k);
      set->opt->NUM_SPROF = k;
    }
    */
    set->num_sprof = set->opt->NUM_SPROF;
    marg = set->profiles[set->opt->NUM_SPROF-1]->freq;
    return (100*(double) marg/(double) set->opt->NUMSTRUCTS);
}

/*If all profiles have frequency of 1, we default to displaying first 10
 */
double set_p_threshold_entropy(Set *set) {
    int i=0;
    double ent =0,frac,last=0,norm,ave;
    Profile **list = set->profiles;

    norm = (double)list[0]->freq;
    //  find_general_freq(set);
    if (norm == 1) {
        set->num_sprof = 10;
        return -2;
    }
    for (i=0; i < set->prof_num; i++) {
        if (list[i]->freq == 1) {
            set->num_sprof = i;
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
        frac = (double)list[i]->freq/norm;
        ent -= frac*log(frac);
        if (frac != 1)
            ent -= (1-frac)*log(1-frac);
        ave = ent/(double)(i+1);
        if ((ave > last) || (fabs(ave-last) < FLT_EPSILON*2)) {
            last = ave;
        }
        else {
            set->num_sprof = i;
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
    }
    set->num_sprof = i;
    return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
}

void find_general_freq(Set *set) {
    int i,j,val;

    //  genfreq = (int*)malloc(sizeof(int)*set->prof_num);
    for (i = 0; i < set->prof_num-1; i++) {
        set->profiles[i]->genfreq += set->profiles[i]->freq;
        for (j = i+1; j < set->prof_num; j++) {
            val = subset(set,set->profiles[i]->profile,set->profiles[j]->profile);
            if (val == 1)
                set->profiles[i]->genfreq += set->profiles[j]->freq;
            else if (val == 2)
                set->profiles[j]->genfreq += set->profiles[i]->freq;
        }
    }
}

/*tests whether one profile is a subset of profile one
returns 1 if first profile is a subset, 2 if second is ,0 if none are*/
int subset(Set *set,char *one, char *two) {
    unsigned long rep1,rep2;

    rep1 = binary_rep(set,one);
    rep2 = binary_rep(set,two);
    if ((rep1 & rep2) == rep1)
        return 1;
    else if ((rep1 & rep2) == rep2)
        return 2;

    return 0;
}

unsigned long binary_rep(Set *set,char *profile) {
    int i;
    unsigned long sum = 0;
    char *copy = mystrdup(profile),*helix;

    for (helix = strtok(copy," "); helix; helix = strtok(NULL," ")) {
        for (i = 0; i < set->num_fhc; i++)
            if (!strcmp(set->helices[i]->id,helix))
                break;
        sum += set->helices[i]->binary;
    }
    free(copy);
    copy = NULL;
    return sum;
}

void select_profiles(Set *set) {
    int i,coverage=0,cov=0,target;
    //double percent;
    Profile *prof;

    if (set->num_sprof == 0)
        set->num_sprof = set->prof_num;
    target = set->opt->COVERAGE*set->opt->NUMSTRUCTS;
    for (i = 0; i < set->num_sprof; i++) {
        prof = set->profiles[i];
        //percent = ((double) prof->freq)*100.0 / ((double)set->opt->NUMSTRUCTS);
        //printf("%s has percent %.1f\n",node->data,percent);
        if (((double)prof->freq*100.0/((double)set->opt->NUMSTRUCTS)) < set->opt->PROF_FREQ) {
            set->num_sprof = i;
            break;
        }
        prof->selected = true;
        coverage += prof->freq;
        if (coverage < target)
            cov = i+1;
        if (set->opt->VERBOSE)
            printf("Selected profile %swith freq %d (%d)\n",prof->profile,prof->freq,prof->genfreq);
    }
    printf("Coverage by selected profiles: %.3f\n",(double)coverage/(double)set->opt->NUMSTRUCTS);
    if (coverage < target) {
        while (coverage < target) {
            coverage += set->profiles[i++]->freq;
            if (i >= set->prof_num)
                fprintf(stderr,"in select profiles() exceeding number of profiles\n");
        }
        cov = i;
    }
    printf("Number of profiles needed to get %.2f coverage: %d\n",set->opt->COVERAGE, cov+1);
    //printf("Number of structures represented by top %d profiles: %d/%d\n",stop,cov15,set->opt->NUMSTRUCTS);
}

void process_one_input(Set *set) {
    HASHTBL *halfbrac;
    FILE *file;
    char temp[100],tmp[ARRAYSIZE],*profile=NULL,*fullprofile=NULL,*diff=NULL,*native,*diffn=NULL;
    int i,j,k,*id,last=0,*prof,hc_num;
    int numhelix = 0,fullnum = 0,natnum = 0;
    int size = INIT_SIZE;
    Profile *natprof;
    node *inode;

    if (!(halfbrac = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for halfbrac failed");
        exit(EXIT_FAILURE);
    }
    prof = (int*) malloc(sizeof(int)*ARRAYSIZE*size);
    if (!(file = fopen(set->opt->INPUT,"r")))
        fprintf(stderr,"Cannot open %s\n",set->opt->INPUT);
    hc_num = set->hc_num;
    while (fgets(temp,100,file)) {
        //    if (sscanf(temp,"Structure %d (%d)",&i,&prob) == 2)
        if (sscanf(temp,"%d %d %d",&i,&j,&k) == 3) {
            sprintf(tmp,"%d %d",i,j);
            id = (int*) hashtbl_get(bp,tmp);
            if (!id) {
                id = process_native(set,i,j,k);
            } else {
                sprintf(tmp,"%d",*id);
                id = (int*) hashtbl_get(translate_hc,tmp);
            }
            printf("found %d for %d %d %d\n",*id,i,j,k);
            if (*id != last) {
                if (natnum >= ARRAYSIZE*size)
                    prof = (int*) realloc(prof,sizeof(int)*ARRAYSIZE*++size);
                prof[natnum++] = *id;
                last = *id;
                make_brackets(halfbrac,i,j,*id);
            }
            else if (set->opt->VERBOSE)
                printf("helix %d %d %d is duplicate with id %d: process_input()\n",i,j,k,*id);
        }
    }
    set->hc_num = hc_num;
    qsort(prof,natnum,sizeof(int),compare);

    native = (char*) malloc(sizeof(char)*ARRAYSIZE*++size);
    native[0] = '\0';
    numhelix = natnum;
    fullnum = natnum;
    for (i = 0; i<natnum; i++) {
        if (prof[i] > set->num_fhc && !profile) {
            profile = mystrdup(native);
            numhelix = i;
            native[0] = '\0';
        }
        if (prof[i] > set->hc_num && !diffn) {
            diffn = mystrdup(native);
            fullnum = i;
            native[0]='\0';
        }
        sprintf(tmp,"%d ",prof[i]);
        if (strlen(tmp)+strlen(native)+1 > (unsigned int) ARRAYSIZE*size)
            native = (char*) realloc(native,sizeof(char)*ARRAYSIZE*++size);
        native = strcat(native,tmp);
    }
    diff = mystrdup(native);
    if (!profile) {
        profile = (char*) malloc(sizeof(char));
        profile = (char*)"";
    }
    if (!diffn) {
        diffn = diff;
        diff = (char*)"";
    }

    //printf("native %s%s%s(%d), fullprofile %s%s(%d), profile %s(%d)\n",profile,diffn,diff,natnum,profile,diffn,fullnum,profile,numhelix);

    /*making data structure*/
    while (strlen(profile)+strlen(diffn)+strlen(diff)+1 > (unsigned int) ARRAYSIZE*size)
        native = (char*) realloc(native,sizeof(char)*ARRAYSIZE*++size);
    sprintf(native,"%s%s%s",profile,diffn,diff);
    fullprofile = (char*) malloc(strlen(native));
    sprintf(fullprofile,"%s%s",profile,diffn);

    //printf("now native %s, full profile %s\n",native,fullprofile);
    natprof = create_profile(native);
    make_bracket_rep(halfbrac,natprof);
    free_hashtbl(halfbrac);

    set->inputnode = createNode(native);
    set->inputnode->bracket = natprof->bracket;

    if (fullnum != natnum) {
        //printf("creating input node %s with diff %s\n",fullprofile,diff);
        inode = createNode(fullprofile);
        inode->neighbors = (node**) malloc(sizeof(node*)*ARRAYSIZE);
        inode->neighbors[0] = set->inputnode;
        inode->diff = (char**) malloc(sizeof(char*)*ARRAYSIZE);
        inode->diff[0] = diff;
        inode->numNeighbors++;
        set->inputnode = inode;
    }
    if (numhelix != fullnum) {
        //printf("creating input node %s with diffn %s\n",profile,diffn);
        inode = createNode(profile);
        inode->neighbors = (node**) malloc(sizeof(node*)*ARRAYSIZE);
        inode->neighbors[0] = set->inputnode;
        inode->diff = (char**) malloc(sizeof(char*)*ARRAYSIZE);
        inode->diff[0] = diffn;
        inode->numNeighbors++;
        set->inputnode = inode;
    }
}

//processes native helices if necessary; called by process_input
//returns id for i,j,k helix
int* process_native(Set *set,int i, int j, int k) {
    int *id = NULL,l;
    char *tmp,key[ARRAYSIZE];

    tmp = (char*) malloc(sizeof(char)*ARRAYSIZE);
    for (l=1; l < k; l++) {
        sprintf(tmp,"%d %d",i+l,j-l);
        id = (int*) hashtbl_get(bp,tmp);
        if (id) {
            sprintf(tmp,"%d",*id);
            id = (int*) hashtbl_get(translate_hc,tmp);
            for (l-- ; l >= 0; l--) {
                sprintf(tmp,"%d %d",i+l,j+l);
                hashtbl_insert(bp,tmp,id);
            }
            return id;
        }
    }

    id = (int*) malloc(sizeof(int));
    *id = ++(set->hc_num);
    sprintf(key,"%d %d",i,j);
    hashtbl_insert(bp,key,id);
    return id;
}

void find_consensus(Set *set) {
    int freq,*bpfreq,i;
    KEY *node,*bpnode;
    HASHTBL *ij,*final;

    if (!(final = create_hashtbl(HASHSIZE, NULL))) {
        fprintf(stderr, "ERROR: create_hashtbl() for final failed");
        exit(EXIT_FAILURE);
    }

    //cycle thru all profiles, either calc consensus for selected or destroying
    for (node = hashtbl_getkeys(consensus); node; node = node->next) {
        freq = 0;
        for (i = 0; i < set->num_sprof; i++)
            if (!strcmp(set->profiles[i]->profile,node->data))
                freq = set->profiles[i]->freq;
        ij = (HASHTBL*) hashtbl_get(consensus,node->data);
        if (freq) {
            if (!ij)
                fprintf(stderr, "ij not found in find_consensus()\n");
            for (bpnode = hashtbl_getkeys(ij); bpnode; bpnode = bpnode->next) {
                bpfreq = (int*) hashtbl_get(ij,bpnode->data);
                if (*bpfreq*100/freq <= 50)
                    hashtbl_remove(ij,bpnode->data);
            }
        } else {
            free_hashtbl(ij);
            //insert dummy pointer so remove won't seg fault
            hashtbl_insert(consensus,node->data, malloc(sizeof(char)));
            hashtbl_remove(consensus,node->data);
        }
    }
}

//in ct format
int print_consensus(Set *set) {
    int l,k=0,m,seqlen;
    char outfile[ARRAYSIZE],key[ARRAYSIZE],*pair,*i,*j,*blank = (char*)" ";
    KEY *bpnode;
    HASHTBL *bpairs,*temp;
    FILE *fp;

    seqlen = strlen(set->seq);

    //foreach profile
    //for (node = hashtbl_getkeys(consensus); node; node = node->next) {
    for (l = 0; l < set->num_sprof; l++) {
        if (!(temp = create_hashtbl(HASHSIZE, NULL))) {
            fprintf(stderr, "ERROR: create_hashtbl() for temp failed");
            exit(EXIT_FAILURE);
        }
        sprintf(outfile,"Structure_%d.ct",++k);
        fp = fopen(outfile,"w");

        fprintf(fp,"Profile: %s\n",set->profiles[l]->profile);
        fprintf(fp,"Freq: %d\n",set->profiles[l]->freq);
        fprintf(fp,"%d dG = n/a\n",seqlen);
        bpairs = (HASHTBL*) hashtbl_get(consensus,set->profiles[l]->profile);
        if (!bpairs)
            fprintf(stderr,"no bpairs found\n");
        for (bpnode = hashtbl_getkeys(bpairs); bpnode; bpnode = bpnode->next) {
            pair = mystrdup(bpnode->data);
            //printf("processing pair %s\n",pair);
            i = strtok(pair,blank);
            j = strtok(NULL,blank);
            hashtbl_insert(temp,i,j);
            hashtbl_insert(temp,j,i);

        }
        for (m = 0; m < seqlen; m++) {
            sprintf(key,"%d",m+1);
            if ((j = (char*) hashtbl_get(temp,key)) )
                fprintf(fp,"\t%d %c\t%d   %d   %s   %d\n",m+1,set->seq[m],m,m+2,j,m+1);
            else
                fprintf(fp,"\t%d %c\t%d   %d   0   %d\n",m+1,set->seq[m],m,m+2,m+1);
        }
        free_hashtbl(temp);
        fclose(fp);
    }
    free_hashtbl(consensus);
    return 0;
}

/**
 * Store the structures defined in structure.out into memory
 *
 * @param set the set to store strucures in
 */
void add_structures_to_set(Set* set) {
    FILE* struct_file = fopen("structure.out", "r");
    if (struct_file == NULL) {
        fprintf(stderr,"Error: could not open structure file");
        exit(EXIT_FAILURE);
    }
    char *token = (char*) malloc(sizeof(char) * STRING_BUFFER);
    const char delim[2] = " ";
    char line[MAX_STRUCT_FILE_LINE_LEN];
    int struct_i = 0;
    while(fgets(line, MAX_STRUCT_FILE_LINE_LEN, struct_file) != NULL) {
        if (line[0] == 'S') {
            set->structures[struct_i] = create_array_list();

            token = strtok(line, delim);
            int token_i = 0;
            int helix_i = 0;

            while( token != NULL ) {
                if(token_i >= 2) {
                    if (strcmp(token, "\n") == 0) {
                        token = strtok(NULL, delim);
                        token_i++;
                        continue;
                    }
                    char* temp = (char*) malloc(sizeof(char) * (strlen(token) + 1));
                    strcpy(temp, token);
                    add_to_array_list(set->structures[struct_i], helix_i, temp);
                    helix_i++;
                }
                token = strtok(NULL, delim);
                token_i++;
            }
            struct_i++;
        }
    }
    free(token);
    token = NULL;
    fclose(struct_file);
}

/**
 * Find first n HCs in the set and add them to set->stems, where n is determined by -snh option
 *
 * @param set the set to add stems to
 */
void add_initial_stems(Set *set) {
    // populate array_list of single helix stems
    for (int i_hc = 0; i_hc < min(set->opt->STEM_NUM_CUTOFF, set->hc_num); i_hc++) {
        add_to_array_list(set->stems, i_hc, create_stem_from_HC(set->helices[i_hc]));
    }
}

/**
 * Iterate through all stems pairwise and see if any stems can be combined
 *
 * @param set the set to combine stems in
 * @return true if any stems were combined, false otherwise
 */
bool combine_stems(Set* set) {
    bool combined = false;
    bool combining;
    array_list_t* stems = set->stems;
    Stem** stem_list = (Stem**) stems->entries;
    int size = stems->size;
    for (int i = 0; i < size; i++) {
        for (int j = i + 1; j < size; j++) {
            Stem* stem1 = stem_list[i];
            Stem* stem2 = stem_list[j];
            // check that the stems occur at similar enough frequencies to be considered
            if (!validate_stems(stem1, stem2, set->opt->STEM_VALID_PERCENT_ERROR)) {
                continue;
            }
            int* outer_stem = (int*) malloc(sizeof(int));
            combining = check_ends_to_combine_stems(stem1, stem2, set->opt->STEM_END_DELTA, outer_stem);
            if (combining) {
                void **old_stem = (void**) malloc(sizeof(void**));
                combined = true;
                if (*outer_stem == 1) {
                    if (set->opt->VERBOSE) {
                        if (stem1->num_helices == 1 && stem2->num_helices == 1) {
                            printf("Joining helices %s and %s\n", ((HC *) stem1->helices->entries[0])->id,
                                   ((HC *) stem2->helices->entries[0])->id);
                        } else {
                            printf("Joining stems %s and %s\n", ((HC *) stem1->helices->entries[0])->id,
                                   ((HC *) stem2->helices->entries[0])->id);
                        }
                    }
                    merge_stems(stem1, stem2);
                    remove_from_array_list(stems, j, old_stem);
                } else if (*outer_stem == 2) {
                    if (set->opt->VERBOSE) {
                        if (stem1->num_helices == 1 && stem2->num_helices == 1) {
                            printf("Joining helices %s and %s\n", ((HC *) stem2->helices->entries[0])->id,
                                   ((HC *) stem1->helices->entries[0])->id);
                        } else {
                            printf("Joining stems %s and %s\n", ((HC *) stem2->helices->entries[0])->id,
                                   ((HC *) stem1->helices->entries[0])->id);
                        }
                    }
                    merge_stems(stem2, stem1);
                    remove_from_array_list(stems, i, old_stem);
                }
                //free_stem(*(Stem**) old_stem);
                size--;
            }
        }
    }
    return combined;
}

/**
 * Merges two stems. Assumes they have been validated. Does NOT remove stem2 from set -- must be done outside of method
 *
 * @param stem1 the "outer" stem (encapsulates stem2)
 * @param stem2  the "innter" stem (encapsulated by stem1)
 */
void merge_stems(Stem* stem1, Stem* stem2) {
    int insert_index = stem1->num_helices;
    stem1->num_helices += stem2->num_helices;
    // get min frequency. This will later be updated to actual occurences of all component helices in
    // get_frequency_of_stem
    stem1->freq = (stem1->freq >= stem2->freq)? stem2->freq : stem1->freq;
    stem1->int_max_quad[1] = stem2->int_max_quad[1];
    stem1->int_max_quad[2] = stem2->int_max_quad[2];
    sprintf(stem1->max_quad, "%d %d %d %d", stem1->int_max_quad[0], stem1->int_max_quad[1], stem1->int_max_quad[2],
            stem1->int_max_quad[3]);
    for (int i = 0; i < stem2->num_helices; i++) {
        add_to_array_list(stem1->helices, insert_index + i, stem2->helices->entries[i]);
    }
    for (int i = 0; i < stem2->components->size; i++) {
        add_to_array_list(stem1->components, stem1->components->size, stem2->components->entries[i]);
    }
    //stem_reset_id(stem1);
}

/**
 * Determine if two stems should be considered one based on how different their frequencies are
 *
 * @param stem1 first stem to validate
 * @param stem2 second stem to validate
 * @return true if they occur within % error bound, false otherwise
 */
bool validate_stems(Stem* stem1, Stem* stem2, double valid_percent_error) {
    double f1 = stem1->freq;
    double f2 = stem2->freq;
    // TODO: check if we should divide by min or max
    double max = (f1 >= f2)? f1 : f2;
    double error = fabs((f2 - f1) / max) * 100;
    return error <= valid_percent_error;
}

// TODO: cleanup this function. This may involve moving some actions into add_to_fs_stem_group
/**
 * Find which stems are functionally similar and can be interchanged
 *
 * @param set the set to find functionally similar stems in
 */
void find_func_similar_stems(Set* set) {
    DataNode* fs_stem_group_node;
    int* freq = (int*) malloc(sizeof(int));
    Stem** stem_list = (Stem**) set->stems->entries;
    // only used for removals from array lists
    void** data_out = (void**) malloc(sizeof(void**));

    for (int i = 0; i < set->stems->size; i++) {
        for (int j = i + 1; j < set->stems->size; j++) {
            Stem* stem1 = stem_list[i];
            Stem* stem2 = stem_list[j];
            if (check_func_similar_stems(set, stem1, stem2, freq)) {
                if (stem1->components->size == 1
                    && ((DataNode*)(stem1->components->entries[0]))->node_type == fs_stem_group_type) {
                    FSStemGroup* stem_group = (FSStemGroup*) ((DataNode*)(stem1->components->entries[0]))->data;
                    add_to_fs_stem_group(stem_group, stem2);
                    stem_group->freq = *freq;
                    stem1->freq = *freq;
                    ((DataNode*)stem1->components->entries[0])->freq = *freq;
                    memcpy(stem1->int_max_quad, stem_group->int_max_quad,
                           sizeof(stem1->int_max_quad));
                    stem1->num_helices = stem_group->num_helices;
                    strcpy(stem1->max_quad, stem_group->max_quad);
                    memcpy(stem1->int_max_quad, stem_group->int_max_quad,
                           sizeof(stem1->int_max_quad));
                    for (int k = 0; k < stem_group->helices->size; k++) {
                        add_to_array_list(stem1->helices, k, stem_group->helices->entries[k]);
                    }
                    strcpy(stem1->id, stem_group->id);
                    remove_from_array_list(set->stems, j, data_out);
                    j--;
                } else {
                    // TODO: cleanup this section, examine better method
                    fs_stem_group_node = create_fs_stem_group_node();
                    add_to_fs_stem_group((FSStemGroup*)fs_stem_group_node->data, stem1);
                    add_to_fs_stem_group((FSStemGroup*)fs_stem_group_node->data, stem2);
                    FSStemGroup* stem_group = (FSStemGroup*) fs_stem_group_node->data;
                    stem_group->freq = *freq;
                    fs_stem_group_node->freq = *freq;
                    remove_from_array_list(set->stems, i, data_out);
                    j--;
                    remove_from_array_list(set->stems, j, data_out);

                    // Create and empty Stem DataNode to hold the FSStemGroup
                    DataNode* node = create_data_node(stem_type, create_stem());
                    Stem* stem = (Stem*) node->data;
                    add_to_array_list(stem->components, 0, fs_stem_group_node);
                    stem->freq = *freq;
                    node->freq = *freq;
                    memcpy(stem->int_max_quad, stem_group->int_max_quad,
                           sizeof(stem->int_max_quad));
                    stem->num_helices = stem_group->num_helices;
                    strcpy(stem->max_quad, stem_group->max_quad);
                    memcpy(stem->int_max_quad, stem_group->int_max_quad,
                           sizeof(stem->int_max_quad));
                    for (int k = 0; k < stem_group->helices->size; k++) {
                        add_to_array_list(stem->helices, k, stem_group->helices->entries[k]);
                    }
                    strcpy(stem->id, stem_group->id);
                    add_to_array_list(set->stems, i, stem);
                }
            }
        }
    }
    free(data_out);
    free(freq);
}

/**
 * Check if two stems are functionally similar
 *
 * @param stem1 the first stem to check
 * @param stem2 the second stem to check
 * @param freq pointer to the frequency of the fs stem group (if the stems should be merged). This times stem1 occurs
 * plus times stem2 occurs minus times they occur together
 * @return true if the two are functionally similar, false otherwise
 */
bool check_func_similar_stems(Set* set, Stem* stem1, Stem* stem2, int* freq) {
    // Check overlap (or close to, determined by FUCN_SIMILAR_END_DELTA) of the front portions of a stem
    int front1 = stem1->int_max_quad[0];
    int back1  = stem1->int_max_quad[1];
    int front2 = stem2->int_max_quad[0];
    int back2 = stem2->int_max_quad[1];
    if (abs(front1 - front2) > set->opt->FUNC_SIMILAR_END_DELTA || abs(back1-back2) > set->opt->FUNC_SIMILAR_END_DELTA) {
        return false;
    }
    // check for back portions
    front1 = stem1->int_max_quad[2];
    back1  = stem1->int_max_quad[3];
    front2 = stem2->int_max_quad[2];
    back2  = stem2->int_max_quad[3];
    if (abs(front1 - front2) > set->opt->FUNC_SIMILAR_END_DELTA || abs(back1-back2) > set->opt->FUNC_SIMILAR_END_DELTA) {
        return false;
    }
    if (set->opt->VERBOSE) {
        printf("Do stem %s and stem %s occur infrequently enough to be functionally similar: ", stem1->id,
               stem2->id);
    }
    bool func_similar = validate_func_similar_stems(set, stem1, stem2, freq);
    if (set->opt->VERBOSE) {
        if (func_similar) {
            printf("Yes\n");
        } else {
            printf("No\n");

        }
    }
    return func_similar;
}

/**
 * Determine if two stems should be considered functionally similar
 *
 * @param stem1 the first stem to check
 * @param stem2 the second stem to check
 * @param freq pointer to the frequency of the fs stem group (if the stems should be merged). This is defined as the
 * number structures stem1 or stem2 occur in
 * @return true if the two occur
 */
bool validate_func_similar_stems(Set* set, Stem* stem1, Stem* stem2, int* freq) {
    int stem1_count = 0;
    int stem2_count  = 0;
    int both_count = 0;
    bool stem1_found;
    bool stem2_found;
    *freq = 0;
    char* hc_id = (char*) malloc(sizeof(char) * STRING_BUFFER);
    for (int i = 0; i < set->opt->NUMSTRUCTS; i++) {
        array_list_t* structure = set->structures[i];
        stem1_found = stem_in_structure(stem1, structure);
        stem2_found = stem_in_structure(stem2, structure);
        if (stem1_found || stem2_found) {
            (*freq)++;
        }
        if (stem1_found) {
            stem1_count++;
        }
        if (stem2_found) {
            stem2_count++;
        }
        if (stem1_found && stem2_found) {
            both_count++;
        }
    }
    free(hc_id);
    int min_count = (stem1_count <= stem2_count)? stem1_count : stem2_count;
    return (100 * ((float) both_count / min_count)) <= set->opt->FUNC_SIMILAR_PERCENT_ERROR;
}

bool combine_stems_using_func_similar(Set* set) {
    bool combined = false;
    bool combining = false;
    void** data_out = (void**) malloc(sizeof(void*));
    Stem** stem_list = (Stem**)set->stems->entries;
    int* outer_stem = (int*) malloc(sizeof(int));
    for (int i = 0; i < set->stems->size; i++) {
        if (((Stem*)set->stems->entries[i])->components->size == 1
            && ((DataNode*)(stem_list[i])->components->entries[0])->node_type == fs_stem_group_type) {
            FSStemGroup* stem_group = (FSStemGroup*) ((DataNode*) (stem_list[i])->components->entries[0])->data;
            for (int j = 0; j < set->stems->size; j++) {
                if (i == j) {
                    continue;
                }
                Stem* stem = stem_list[j];
                if (!validate_stem_and_func_similar(stem_group, stem, set->opt->STEM_VALID_PERCENT_ERROR)) {
                    continue;
                }
                for (int k = 0; k < stem_group->stems->size; k++) {
                    Stem* stem1 = stem;
                    Stem* stem2 = (Stem*) stem_group->stems->entries[k];
                    combining = check_ends_to_combine_stems(stem1, stem2, set->opt->FUNC_SIMILAR_END_DELTA, outer_stem);
                    if (!combining) {
                        break;
                    }
                }
                // combine if all stems in functionally similar group were within set->opt->FUNC_SIMILAR_END_DELTA to the stem
                if (combining) {
                    if (set->opt->VERBOSE) {
                        printf("Joining stem %s and functionally similar group %s\n", stem_list[j]->id, stem_group->id);
                    }
                    combined = true;
                    merge_stem_and_fs_stem_group(stem, stem_group, *outer_stem);
                    remove_from_array_list(set->stems, i, data_out);
                    i--;
                }
            }
        }
    }
    free(data_out);
    free(outer_stem);
    return combined;
}

/**
 *
 * Merges a stem and a functionally similar stem group, placing the outer one first in the stem's list of components.
 * Does not update frequency
 *
 * @param stem the stem to merge
 * @param stem_group the functionally similar stem group to merge
 * @param outer_stem 1 if stem encompasses stem_group, 2 if stem_group encompasses stem
 */
void merge_stem_and_fs_stem_group(Stem* stem, FSStemGroup* stem_group, int outer_stem) {
    if (outer_stem != 1 && outer_stem != 2) {
        printf("Error in merging stems and functionally similar stems. No valid outer_stem number");
        exit(EXIT_FAILURE);
    }
    stem->int_max_quad[0] = min(stem_group->int_max_quad[0], stem->int_max_quad[0]);
    stem->int_max_quad[1] = max(stem_group->int_max_quad[1], stem->int_max_quad[1]);
    stem->int_max_quad[2] = min(stem_group->int_max_quad[2], stem->int_max_quad[2]);
    stem->int_max_quad[3] = max(stem_group->int_max_quad[3], stem->int_max_quad[3]);
    sprintf(stem->max_quad, "%d %d %d %d", stem->int_max_quad[0], stem->int_max_quad[1],
            stem->int_max_quad[2], stem->int_max_quad[3]);
    // if stem encompasses stem_group
    if(outer_stem == 1) {
        add_to_array_list(stem->components, stem->components->size, create_data_node(fs_stem_group_type, stem_group));
        for (int i = 0; i < stem_group->helices->size; i++) {
            add_to_array_list(stem->helices, stem->helices->size + i, stem_group->helices->entries[i]);
        }
    } else if (outer_stem == 2) {
        add_to_array_list(stem->components, 0, create_data_node(fs_stem_group_type, stem_group));
        for (int i = 0; i < stem_group->helices->size; i++) {
            add_to_array_list(stem->helices, i, stem_group->helices->entries[i]);
        }
    }
    stem->num_helices += stem_group->num_helices;
}


/**
 * Check that two stems' ends are close enough to be consolidated into one extended stem
 *
 * @param stem1 first stem to check
 * @param stem2 second stem to check
 * @param end_delta the amount of difference between ends allowable, in nucleotides
 * @param outer_stem int pointer that is modified to 1 if stem1 is the outer stem (encapsulates stem2), 2 if stem2 is
 * @return true if close enough, false otherwise
 */
bool check_ends_to_combine_stems(Stem* stem1, Stem* stem2, int end_delta, int* outer_stem) {
    int diff_front = stem2->int_max_quad[0] - stem1->int_max_quad[1];
    int diff_back = stem1->int_max_quad[2] - stem2->int_max_quad[3];
    if (diff_front >= 0 && diff_back >=0 && diff_front <= end_delta && diff_back <= end_delta) {
        *outer_stem = 1;
        return true;
    } else {
        diff_front = stem1->int_max_quad[0] - stem2->int_max_quad[1];
        diff_back = stem2->int_max_quad[2] - stem1->int_max_quad[3];
        if (diff_front >= 0 && diff_back >=0 && diff_front <= end_delta && diff_back <= end_delta) {
            *outer_stem = 2;
            return true;
        }
    }
    *outer_stem = 0;
    return false;
}

/**
 * Determine if a stem and a functionally similar stem group (pair for now) should be considered one based on how
 * different their frequencies are
 *
 * @param stem_pair functionally similar group (pair for now) to validate
 * @param stem stem to validate with stem_pair
 * @return true if they occur within % error bound, false otherwise
 */
bool validate_stem_and_func_similar(FSStemGroup* stem_pair, Stem* stem, double valid_percent_error) {
    double f1 = stem_pair->freq;
    double f2 = stem->freq;
    // TODO: check if we should divide by min or max
    double max = (f1 >= f2)? f1 : f2;
    double error = fabs((f2 - f1) / max) * 100;
    return error <= valid_percent_error;
}

/**
 * Determines if a stem exists in a given structure
 *
 * @param stem the stem to search for
 * @param structure the structure to search in
 * @return true if stem is in structure, false otherwise
 */
bool stem_in_structure(Stem* stem, array_list_t* structure) {
    DataNode* component;
    for (int i = 0; i < stem->components->size; i++) {
        component = (DataNode*) stem->components->entries[i];
        if (!component_in_structure(component, structure)) {
            return false;
        }
    }
    return true;
}

/**
 * Determines if a component (held in a DataNode) exists in a given structure
 *
 * @param component the component to search for
 * @param structure the structure to search in
 * @return true if component is in structure, false otherwise
 */
bool component_in_structure(DataNode* component, array_list_t* structure) {
    if (component->node_type == stem_type){
        return stem_in_structure((Stem*) component, structure);
    } else if (component->node_type == fs_stem_group_type) {
        FSStemGroup* stem_group = (FSStemGroup*) component->data;
        for(int i = 0; i < stem_group->stems->size; i++) {
            Stem *stem = (Stem *) stem_group->stems->entries[i];
            if (stem_in_structure(stem, structure)) {
                return true;
            }
        }
        return false;
    } else if (component->node_type == hc_type){
        char* hc_id_string = (char*) malloc(sizeof(char) * STRING_BUFFER);
        sprintf(hc_id_string, "%s", ((HC*)(component->data))->id);
        for (int i = 0; i < structure->size; i++) {
            if (strcmp((char*) structure->entries[i], hc_id_string) == 0) {
                free(hc_id_string);
                return true;
            }
        }
        free(hc_id_string);
        return false;
    } else {
        printf("Error: non recognized DataNode* type");
        exit(EXIT_FAILURE);
    }
}

/**
 * Updates the frequency of all Stems in set->stems
 *
 * @param set the set to update Stems in
 */
void update_freq_stems(Set* set) {
    array_list_t* structure;
    Stem** stem_list = (Stem**) set->stems->entries;
    //Reset all stems with more than 1 helix to freq 0
    for (int i = 0; i < set->stems->size; i++) {
        Stem *stem = stem_list[i];
        if (stem_is_hc(stem)) {
            continue;
        }
        stem->freq = 0;
    }
    for (int i = 0; i < set->opt->NUMSTRUCTS; i++) {
        structure = set->structures[i];
        for (int j = 0; j < set->stems->size; j++) {
            Stem *stem = stem_list[j];
            if (stem_is_hc(stem)) {
                continue;
            }
            if (stem_in_structure(stem, structure)) {
                stem->freq++;
            }
        }
    }
}

/**
 * Reindexes set->stems to use alphabetical indices. Stems are sorted by frequency first.
 *
 * @param set the set to reindex Stems of
 */
void reindex_stems(Set *set) {
    // Sort based on frequency in descending order
    qsort(set->stems->entries, int2size_t(set->stems->size), sizeof(set->stems->entries[0]), stem_freq_compare);
    int count = 0;
    Stem* stem;
    for (int i = 0; i< set->stems->size; i++) {
        stem = (Stem*) set->stems->entries[i];
        if (stem_is_hc(stem)) {
            continue;
        } else {
            count++;
            get_alpha_id(count, stem->id);
        }
    }
}

/**
 * Add remaining helix classes to set->stems after consolidating chosen number of stems
 *
 * @param set the set to add hc stems in
 */
void add_hc_stems(Set* set) {
    for (int i_hc = min(set->opt->STEM_NUM_CUTOFF, set->hc_num); i_hc < set->hc_num; i_hc++) {
        add_to_array_list(set->stems, set->stems->size, create_stem_from_HC(set->helices[i_hc]));
    }
}


/**
 * Update the max quad string to use int_max_quad's current values
 *
 * @param set the set to update stems in
 */
void update_max_quads(Set* set) {
    for (int i = 0; i < set->stems->size; i++) {
        Stem* stem = (Stem*) set->stems->entries[i];
        sprintf(stem->max_quad, "%d %d %d %d", stem->int_max_quad[0], stem->int_max_quad[1], stem->int_max_quad[2],
                stem->int_max_quad[3]);
    }
}

/**
 * Find the ave quads for stems in set and then update ave quad strings for those stems
 *
 * @param set the set to update stems in
 */
void update_ave_quads(Set* set) {
    for (int i = 0; i < set->stems->size; i++) {
        Stem* stem = (Stem*) set->stems->entries[i];
        find_double_max_quad(stem);
        sprintf(stem->ave_quad, "%.1f %.1f %.1f %.1f", stem->double_ave_quad[0], stem->double_ave_quad[1],
                stem->double_ave_quad[2], stem->double_ave_quad[3]);
    }
}

/**
 * Find the int_max_quad of a stem
 * @param stem
 */
void find_double_max_quad(Stem* stem) {
    double* i = (double*) malloc(sizeof(double));
    double* j = (double*) malloc(sizeof(double));
    double* k = (double*) malloc(sizeof(double));
    double* l = (double*) malloc(sizeof(double));
    DataNode* outer_component = (DataNode*) stem->components->entries[0];
    DataNode* inner_component = (DataNode*) stem->components->entries[stem->components->size - 1];
    get_ave_i_l(outer_component, i, l);
    get_ave_j_k(inner_component, j, k);
    stem->double_ave_quad[0] = *i;
    stem->double_ave_quad[1] = *j;
    stem->double_ave_quad[2] = *k;
    stem->double_ave_quad[3] = *l;
    free(i);
    free(j);
    free(k);
    free(l);
}

/**
 * Get the average i amd l for a component, defined as the average i and average l if it is an HC, or weighted
 * average by freq of component stems if it is a FSStemGroup
 *
 * @param component the outermost component of the stem to find ave i and l for
 */
void get_ave_i_l(DataNode *component, double *i, double *l) {
    if (component->node_type == fs_stem_group_type) {
        FSStemGroup* stem_group = (FSStemGroup*) component->data;
        int total_freq = 0;
        double* temp_i = (double*) malloc(sizeof(double));
        double* temp_l = (double*) malloc(sizeof(double));
        *temp_i = 0;
        *temp_l = 0;
        for (int index = 0; index < stem_group->stems->size; index++) {
            Stem* stem = (Stem*) stem_group->stems->entries[index];
            int stem_freq = stem->freq;
            DataNode* stem_group_component = (DataNode*) stem->components->entries[0];
            get_ave_i_l(stem_group_component, temp_i, temp_l);
            *i += *temp_i * stem_freq;
            *l += *temp_l * stem_freq;
            total_freq += stem_freq;
        }
        free(temp_i);
        free(temp_l);
        *i /= total_freq;
        *l /= total_freq;
    } else if (component->node_type == hc_type) {
        HC* hc = (HC*) component->data;
        double k;
        sscanf(hc->avetrip, "%lf %lf %lf", i, l, &k);
        return;
    } else {
        printf("Error: component passed to get_ave_i_l() does not contain an HC or FSStemGroup");
        exit(EXIT_FAILURE);
    }
}

/**
 * Get the average j amd k for a component, defined as the (ave_i + ave_k - 1) and average (ave_j - ave_k + 1) if it is
 * an HC, or weighted average by freq of component stems if it is a FSStemGroup
 *
 * @param component the outermost component of the stem to find ave j and k for
 */
void get_ave_j_k(DataNode *component, double *j, double *k) {
    if (component->node_type == fs_stem_group_type) {
        FSStemGroup* stem_group = (FSStemGroup*) component->data;
        int total_freq = 0;
        double* temp_j = (double*) malloc(sizeof(double));
        double* temp_k = (double*) malloc(sizeof(double));
        *temp_j = 0;
        *temp_k = 0;
        for (int index = 0; index < stem_group->stems->size; index++) {
            Stem* stem = (Stem*) stem_group->stems->entries[index];
            int stem_freq = stem->freq;
            DataNode* stem_group_component = (DataNode*) stem->components->entries[stem->components->size - 1];
            get_ave_j_k(stem_group_component, temp_j, temp_k);
            *j += *temp_j * stem_freq;
            *k += *temp_k * stem_freq;
            total_freq += stem_freq;
        }
        free(temp_j);
        free(temp_k);
        *j /= total_freq;
        *k /= total_freq;
    } else if (component->node_type == hc_type) {
        HC* hc = (HC*) component->data;
        double len;
        sscanf(hc->avetrip, "%lf %lf %lf", j, k, &len);
        *j += len - 1;
        *k -= len + 1;
        return;
    } else {
        printf("Error: component passed to get_ave_i_l() does not contain an HC or FSStemGroup");
        exit(EXIT_FAILURE);
    }
}

/**
 * Print all stems to the command line output
 *
 * @param set the set to print stems of
 */
void print_stems(Set* set) {
    for (int i = 0; i < set->stems->size; i++) {
        Stem* stem = (Stem*) set->stems->entries[i];
        if (stem_is_hc(stem)) {
            //printf("Helix %s is %s (%s) with freq %d", stem->id, stem->max_quad, stem->ave_quad, stem->freq);
        } else {
            printf("Stem %s is %s (%s) with freq %d\n", stem->id, stem->max_quad, stem->ave_quad, stem->freq);
        }
    }
}

/**
 * Comparator function for qsort on Stems, sorting in descending order by frequency.
 *
 * @param s1 the first stem to compare
 * @param s2  the second stem to compare
 * @return 0 if two are equivalent in order, <0 if s1 goes before s2, >0 if s1 goes after s2
 */
int stem_freq_compare(const void* s1, const void* s2) {
    Stem *stem1 = *(Stem**) s1;
    Stem *stem2 = *(Stem**) s2;
    return (stem2->freq - stem1->freq);
}

/**
 * Safely convert an int to a size_t
 * source: https://stackoverflow.com/questions/27490762/how-can-i-convert-to-size-t-from-int-safely
 *
 * @param val int value to convert
 * @return converted value
 */
size_t int2size_t(int val) {
    return (val < 0) ? __SIZE_MAX__ : (size_t)((unsigned)val);
}

/**
 * Converts an integer (representing a helix class id)to its corresponding alphabetic id.
 * 1 -> A, 2 -> B, ..., 27 -> AA, 28 -> AB, ...
 *
 * @param int_id the int id to convert
 * @param alpha_id the new id field
 * @return the new alphabetical id for a stem
 */
void get_alpha_id(int int_id, char* alpha_id) {
    if (int_id <= 0) {
        printf("Error: Unknown ID encountered during Stem reindexing (id <= 0)");
        exit(EXIT_FAILURE);
    }
    alpha_id[0] = '\0';
    char lookup_table[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* temp = (char*) malloc(sizeof(char) * STRING_BUFFER);
    while(--int_id >= 0) {
        temp[0] = lookup_table[int_id%26];
        temp[1] = '\0';
        strcat(temp, alpha_id);
        strcpy(alpha_id, temp);
        temp[0] = '\0';
        int_id /= 26;
    }
    free(temp);
}

/**
 * Generates a file, key.txt, which indicates the helices that make up each stem
 *
 * @param set the set to make a key for
 */
void generate_stem_key(Set* set, char* seqfile) {
    FILE* key_file = fopen("key.txt", "w");
    fprintf(key_file, "seq is in %s\n", seqfile);
    for (int i = 0; i < set->stems->size; i++) {
        Stem* stem = (Stem*) set->stems->entries[i];
        if (stem_is_hc(stem)) {
            continue;
        }
        fprintf(key_file, "%s: ", stem->id);
        print_stem_to_file(key_file, stem);
        fprintf(key_file, "\n");
    }
    fclose(key_file);
}

void print_stem_to_file(FILE* fp, Stem* stem) {
    for (int i = 0; i < stem->components->size; i++) {
        DataNode* component = (DataNode*) stem->components->entries[i];
        if (component->node_type == hc_type) {
            fprintf(fp, "%s ", ((HC*) (component->data))->id);
        } else if (component->node_type == fs_stem_group_type) {
            FSStemGroup* stem_group = (FSStemGroup*) (component->data);
            for (int j = 0; j < stem_group->stems->size; j++) {
                if (j == 0){
                    fprintf(fp, "( ");
                } else if (j <= stem_group->stems->size - 1) {
                    fprintf(fp, "/ ");
                }
                print_stem_to_file(fp, (Stem*) stem_group->stems->entries[j]);
                if (j == stem_group->stems->size - 1) {
                    fprintf(fp, ") ");
                }
            }
        } else {
            printf("Error: component in stem %s does not contain an HC or FSStemGroup", stem->id);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Finds the threshold for featured stems using entropy method
 *
 * @param set the set to find threshold in
 * @return percentage of structs a stem must be found in to be featured
 */
double set_threshold_entropy_stems(Set *set) {
    int i=0;
    double ent =0,frac,last=0,ave,norm;
    Stem** list = (Stem**) set->stems->entries;

    norm = (double)list[i]->freq;
    for (i=0; i < set->stems->size; i++) {
        frac = (double)list[i]->freq/norm;
        ent -= frac*log(frac);
        if (frac != 1)
            ent -= (1-frac)*log(1-frac);
        ave = ent/(double)(i+1);
        if ((ave > last) || (fabs(ave-last) < FLT_EPSILON*2)) {
            last = ave;
        }
        else {
            //printf("%f is lower than %f\n",ent/(i+1), last);
            set->num_fstems = i;
            //init_joint(set);
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
    }
    printf("Error: No threshold entropy found for stems\n");
    return 0;
}

double set_num_fstems(Set *set) {
    int marg;
    double percent;

    if (set->opt->NUM_FSTEMS > set->stems->size) {
        printf("Number of requested featured stems %d greater than total number of stems %d\n",set->opt->NUM_FSTEMS, set->stems->size);
        set->opt->NUM_FSTEMS = set->stems->size;
    }
    if (set->opt->NUM_FSTEMS > 63)
        printf("Warning: requested num featured stems %d exceeds limit of 63\n",set->opt->NUM_FSTEMS);

    marg = ((Stem*) (set->stems->entries[set->opt->NUM_FSTEMS-1]))->freq;
    percent = ((double) marg)*100.0/((double)set->opt->NUMSTRUCTS);
    return percent;
}

/**
 * Finds which stems meet frequency threshold to be featured. Assumes threshold already found
 *
 * @param set the set to find featured stems in
 */
void find_featured_stems(Set* set) {
    int marg;
    double percent;
    //double cov=0;
    Stem* stem;

    int total = set->stems->size;
    set->num_fstems = set->stems->size;
    for (int i = 0; i < total; i++) {
        stem = (Stem*) set->stems->entries[i];
        marg = stem->freq;
        percent = ((double) marg*100.0)/((double)set->opt->NUMSTRUCTS);
        if (percent >= set->opt->STEM_FREQ) {
            if (set->opt->VERBOSE) {
                if (stem_is_hc(stem)) {
                    printf("Featured helix %s: %s with freq %d\n", stem->id, stem->max_quad, stem->freq);
                } else {
                    printf("Featured stem %s: %s with freq %d\n", stem->id, stem->max_quad, stem->freq);
                }
            }
            char* stem_id_str = (char*) malloc(sizeof(char) * (strlen(stem->id) + 1));
            strcpy(stem_id_str, stem->id);
            add_to_array_list(set->featured_stem_ids, i, stem_id_str);
            stem->is_featured = 1;
            stem->binary = 1u<<(unsigned long)i;
            // TODO: find equivalent of helsum for stems and implement coverage tracking for this function
            //cov += (double)marg/(double)set->helsum;;
        }
        else {
            set->num_fstems = i;
            i = total;
        }
    }
    //TODO: fix cov in this case (when featured stems are capped)
    if (set->num_fstems > 63) {
        //fprintf(stderr,"number of helices greater than allowed in find_freq()\n");
        set->num_fstems = 63;
        marg = ((Stem*)set->stems->entries[62])->freq;
        printf("Capping at 63 featured stems with freq %d\n",marg);
        set->opt->STEM_FREQ = ((double) marg)*100.0/((double)set->opt->NUMSTRUCTS);
    }
    // TODO: find equivalent of helsum for stems and implement coverage tracking for this function
    //printf("Coverage by featured helix classes: %.3f\n",cov);
}

/**
 * Determines profiles in terms of stems. Also creates stem_structure.out containing structures in terms of
 *
 * @param set the set to redefine structures in
 */
void make_stem_profiles(Set* set) {
    FILE* stem_struct_file = fopen("stem_structure.out", "w");
    fprintf(stem_struct_file, "Processing: %s\n", set->structfile);
    char* stem_structure_str = (char*) malloc(sizeof(char) * MAX_STRUCT_FILE_LINE_LEN);
    void** data_out = (void**) malloc(sizeof(void*));
    for (int i = 0; i < set->opt->NUMSTRUCTS; i++) {
        array_list_t* structure = set->structures[i];
        set->stem_structures[i] = create_array_list();
        array_list_t* stem_structure = set->stem_structures[i];
        // TODO: implement array_list_deep_copy and use in place of this loop to duplicate structure
        for (int j = 0; j < structure->size; j++) {
            char* hc_id = (char*) malloc(sizeof(char) * (strlen((char*) structure->entries[j]) + 1));
            strcpy(hc_id,(char*) structure->entries[j]);
            add_to_array_list(stem_structure, j, hc_id);
        }
        for (int stem_i = 0; stem_i < set->num_fstems; stem_i++) {
            Stem* stem = (Stem*) set->stems->entries[stem_i];
            if (stem_is_hc(stem) || !stem_in_structure(stem, stem_structure)) {
                continue;
            }
            // TODO: modify stem_in_structure to find start_i and end_i (or len) and use that instead
            int start_i = stem_start_i(stem, stem_structure);
            int end_i = stem_end_i(stem, stem_structure);
            int len = end_i - start_i + 1;
            for(int k = 0; k < len; k++) {
                remove_from_array_list(stem_structure, start_i, data_out);
            }
            char* stem_id_str = (char*) malloc(sizeof(char) * (strlen(stem->id) + 1));
            strcpy(stem_id_str, stem->id);
            add_to_array_list(stem_structure, start_i, stem_id_str);
        }
        join(stem_structure_str, (char**) stem_structure->entries, " ", stem_structure->size);
        fprintf(stem_struct_file, "Structure %d: %s\n", i+1, stem_structure_str);
        char* stem_prof_str = stem_profile_from_stem_structure(set, set->stem_structures[i]);
        fprintf(stem_struct_file, "\t-> %s\n", stem_prof_str);
        if (!stem_prof_exists(set, stem_prof_str)) {
            if (set->stem_prof_cap < set->stem_prof_num + 1) {
                set->stem_prof_cap += 300;
                set->stem_profiles = (Profile**) realloc(set->stem_profiles, sizeof(Profile*) * set->stem_prof_cap);
            }
            Profile* stem_prof = create_profile(stem_prof_str);
            set->stem_profiles[set->stem_prof_num] = stem_prof;
            set->stem_prof_num++;
        } else {
            free(stem_prof_str);
        }
    }
    free(stem_structure_str);
    free(data_out);
    fclose(stem_struct_file);
}

/**
 * Determine if a stem profile has already been added to set->stem_profiles. If found, the freq of the profile is
 * incremented
 *
 * @param set the set to search in
 * @param stem_prof_str the string representation of the profile to search for
 * @return true if found, false otherwise
 */
bool stem_prof_exists(Set *set, char *stem_prof_str) {
    for (int i = 0; i < set->stem_prof_num; i++) {
        Profile* stem_prof = set->stem_profiles[i];
        if (strcmp(stem_prof->profile, stem_prof_str) == 0) {
            stem_prof->freq++;
            return true;
        }
    }
    return false;
}

/**
 * Generates the string representation of a profile (featured stems in a structure) from a structure
 *
 * @param set the set to use for featured stems
 * @param structure the structure to make a profile from
 * @return the string representation of the profile
 */
char* stem_profile_from_stem_structure(Set* set, array_list_t* structure)   {
    char* stem_prof_str = (char*) malloc(sizeof(char) * MAX_STRUCT_FILE_LINE_LEN);
    stem_prof_str[0] = '\0';
    int count = 0;
    for (int i = 0; i < set->featured_stem_ids->size; i++) {
        char* featured_stem_id = (char*) set->featured_stem_ids->entries[i];
        for (int j = 0; j < structure->size; j++) {
            char* stem_id = (char*) structure->entries[j];
            if (strcmp(stem_id, featured_stem_id) == 0) {
                strcat(stem_prof_str, stem_id);
                strcat(stem_prof_str, " ");
                count++;
                break;
            }
        }
    }
    return stem_prof_str;
}

void print_stem_profiles(Set *set) {
    int i;
    Profile **stem_profiles = set->stem_profiles;

    find_stem_general_freq(set);
    qsort(stem_profiles,int2size_t(set->stem_prof_num),sizeof(Profile*),profsort);
    for (i = 0; i < set->stem_prof_num; i++)
        if (set->opt->VERBOSE)
            printf("Stem profile %s with freq %d (%d)\n",stem_profiles[i]->profile,stem_profiles[i]->freq,stem_profiles[i]->genfreq);
}

void find_stem_general_freq(Set *set) {
    int i,j,val;

    //  genfreq = (int*)malloc(sizeof(int)*set->prof_num);
    for (i = 0; i < set->stem_prof_num-1; i++) {
        set->stem_profiles[i]->genfreq += set->stem_profiles[i]->freq;
        for (j = i+1; j < set->stem_prof_num; j++) {
            val = stem_subset(set,set->stem_profiles[i]->profile,set->stem_profiles[j]->profile);
            if (val == 1)
                set->stem_profiles[i]->genfreq += set->stem_profiles[j]->freq;
            else if (val == 2)
                set->stem_profiles[j]->genfreq += set->stem_profiles[i]->freq;
        }
    }
}

/*tests whether one profile is a subset of profile one
returns 1 if first profile is a subset, 2 if second is ,0 if none are*/
int stem_subset(Set *set,char *one, char *two) {
    unsigned long rep1,rep2;

    rep1 = stem_binary_rep(set,one);
    rep2 = stem_binary_rep(set,two);
    if ((rep1 & rep2) == rep1)
        return 1;
    else if ((rep1 & rep2) == rep2)
        return 2;

    return 0;
}

unsigned long stem_binary_rep(Set *set,char *stem_profile) {
    int i;
    unsigned long sum = 0;
    char* copy = mystrdup(stem_profile);
    char* stem;

    for (stem = strtok(copy," "); stem; stem = strtok(NULL," ")) {
        for (i = 0; i < set->num_fstems; i++)
            if (!strcmp(((Stem*)set->stems->entries[i])->id,stem))
                break;
        sum += ((Stem*)set->stems->entries[i])->binary;
    }
    free(copy);
    return sum;
}

double set_num_s_stem_prof(Set *set) {
    int marg;

    if (set->opt->NUM_S_STEM_PROF > set->stem_prof_num) {
        printf("Number of requested s stem prof %d greater than total number of stem profiles %d\n",set->opt->NUM_S_STEM_PROF, set->stem_prof_num);
        set->opt->NUM_S_STEM_PROF = set->stem_prof_num;
    }
    set->num_s_stem_prof = set->opt->NUM_S_STEM_PROF;
    marg = set->stem_profiles[set->opt->NUM_S_STEM_PROF-1]->freq;
    return (100*(double) marg/(double) set->opt->NUMSTRUCTS);
}

/*If all profiles have frequency of 1, we default to displaying first 10
 */
double set_stem_p_threshold_entropy(Set *set) {
    int i=0;
    double ent =0,frac,last=0,norm,ave;
    Profile **list = set->stem_profiles;

    norm = (double)list[0]->freq;
    //  find_general_freq(set);
    if (norm == 1) {
        set->num_s_stem_prof = 10;
        return -2;
    }
    for (i=0; i < set->stem_prof_num; i++) {
        if (list[i]->freq == 1) {
            set->num_s_stem_prof = i;
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
        frac = (double)list[i]->freq/norm;
        ent -= frac*log(frac);
        if (frac != 1)
            ent -= (1-frac)*log(1-frac);
        ave = ent/(double)(i+1);
        if ((ave > last) || (fabs(ave-last) < FLT_EPSILON*2)) {
            last = ave;
        }
        else {
            set->num_s_stem_prof = i;
            return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
        }
    }
    set->num_s_stem_prof = i;
    return (100*(double) list[i-1]->freq/(double) set->opt->NUMSTRUCTS);
}

void select_stem_profiles(Set* set) {
    int i,coverage=0,cov=0,target;
    //double percent;
    Profile *stem_prof;

    if (set->num_s_stem_prof == 0)
        set->num_s_stem_prof = set->stem_prof_num;
    target = (int) (set->opt->COVERAGE*set->opt->NUMSTRUCTS);
    for (i = 0; i < set->num_s_stem_prof; i++) {
        stem_prof = set->stem_profiles[i];
        //percent = ((double) stem_prof->freq)*100.0 / ((double)set->opt->NUMSTRUCTS);
        //printf("%s has percent %.1f\n",node->data,percent);
        if (((double)stem_prof->freq*100.0/((double)set->opt->NUMSTRUCTS)) < set->opt->STEM_PROF_FREQ) {
            set->num_s_stem_prof = i;
            break;
        }
        stem_prof->selected = true;
        coverage += stem_prof->freq;
        if (coverage < target)
            cov = i+1;
        if (set->opt->VERBOSE)
            printf("Selected stem profile %swith freq %d (%d)\n",stem_prof->profile,stem_prof->freq,stem_prof->genfreq);
    }
    printf("Coverage by selected stem profiles: %.3f\n",(double)coverage/(double)set->opt->NUMSTRUCTS);
    if (coverage < target) {
        while (coverage < target) {
            coverage += set->stem_profiles[i++]->freq;
            if (i >= set->stem_prof_num)
                fprintf(stderr,"in select_stem_profiles() exceeding number of stem profiles\n");
        }
        cov = i;
    }
    printf("Number of stem profiles needed to get %.2f coverage: %d\n",set->opt->COVERAGE, cov+1);
    //printf("Number of structures represented by top %d stem profiles: %d/%d\n",stop,cov15,set->opt->NUMSTRUCTS);
}

/**
 * Create the bracket representation of the stem profiles in set
 * @param set containing stem profiles
 */
void stem_profiles_make_bracket(Set* set) {
    for (int prof_i = 0; prof_i < set->stem_prof_num; prof_i++) {
        Profile* stem_prof = set->stem_profiles[prof_i];
        HASHTBL* halfbrac = create_hashtbl(HASHSIZE, NULL);
        for (int stem_i = 0; stem_i < set->featured_stem_ids->size; stem_i++) {
            Stem* stem = (Stem*) set->stems->entries[stem_i];
            if (strstr(stem_prof->profile, stem->id) != NULL) {
                int i = stem->int_max_quad[0];
                int j = stem->int_max_quad[3];
                make_stem_brackets(halfbrac,i,j, stem->id);
            }
        }
        make_bracket_rep(halfbrac, stem_prof);
    }
}

//inserts bracket representation for i,j into a hash
void make_stem_brackets(HASHTBL *brac, int i, int j, char* id) {
    char key[ARRAYSIZE],*val;

    sprintf(key,"%d",i);
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    sprintf(val,"[%s",id);
    //  printf("making bracket %s for %d\n",val,i);
    hashtbl_insert(brac,key,val);
    sprintf(key,"%d",j);
    val = (char*) malloc(sizeof(char)*ARRAYSIZE);
    val[0] = ']';
    val[1] = '\0';
    hashtbl_insert(brac,key,val);
}

/**
 * Find the start of a component in a structure
 *
 * @param component the component to find
 * @param structure the structure to search in
 * @return the index of the components's first appearance in structure, -1 if it does not appear
 */
 int component_start_i(DataNode *component, array_list_t *structure) {
     while (component->node_type == stem_type){
         component = (DataNode*) ((Stem*) component)->components->entries[0];
     }
     int index = -1;
     if (component->node_type == fs_stem_group_type) {
         FSStemGroup* stem_group = (FSStemGroup*) component->data;
         int temp;
         for(int i = 0; i < stem_group->stems->size; i++) {
             temp = stem_start_i((Stem *) stem_group->stems->entries[i], structure);
             if (temp != -1) {
                 if (index == -1) {
                     index = temp;
                 } else {
                     index = min(index, temp);
                 }
             }
         }
         return index;
     } else if (component->node_type == hc_type){
         char* hc_id_string = (char*) malloc(sizeof(char) * STRING_BUFFER);
         sprintf(hc_id_string, "%s", ((HC*)(component->data))->id);
         for (int i = 0; i < structure->size; i++) {
             if (strcmp((char*) structure->entries[i], hc_id_string) == 0) {
                 free(hc_id_string);
                 return i;
             }
         }
         free(hc_id_string);
         return index;
     } else {
         printf("Error: non recognized DataNode* type");
         exit(EXIT_FAILURE);
     }
 }

/**
* Find the end of a component in a structure/

*
* @param component the component to find
* @param structure the structure to search in
* @return the index of the components's last appearance in structure, -1 if it does not appear
*/
int component_end_i(DataNode *component, array_list_t *structure) {
    while (component->node_type == stem_type){
        Stem* stem = (Stem*) component;
        component = (DataNode*) (stem->components->entries[stem->components->size - 1]);
    }
    int index = -1;
    if (component->node_type == fs_stem_group_type) {
        FSStemGroup* stem_group = (FSStemGroup*) component->data;
        int temp;
        for(int i = 0; i < stem_group->stems->size; i++) {
            temp = stem_end_i((Stem *) stem_group->stems->entries[i], structure);
            if (temp != -1) {
                if (index == -1) {
                    index = temp;
                } else {
                    index = max(index, temp);
                }
            }
        }
        return index;
    } else if (component->node_type == hc_type){
        char* hc_id_string = (char*) malloc(sizeof(char) * STRING_BUFFER);
        sprintf(hc_id_string, "%s", ((HC*)(component->data))->id);
        for (int i = 0; i < structure->size; i++) {
            if (strcmp((char*) structure->entries[i], hc_id_string) == 0) {
                free(hc_id_string);
                return i;
            }
        }
        free(hc_id_string);
        return index;
    } else {
        printf("Error: non recognized DataNode* type");
        exit(EXIT_FAILURE);
    }
}

/**
 * Find the start of a stem in a structure
 *
 * @param stem the stem to find
 * @param structure the structure to search in
 * @return the index of the stem's first appearance in structure -1 if it does not appear
 */
int stem_start_i(Stem *stem, array_list_t *structure) {
    DataNode* component = (DataNode*) stem->components->entries[0];
    return component_start_i(component, structure);
}

/**
 * Find the end of a stem in a structure
 *
 * @param stem the stem to find
 * @param structure the structure to search in
 * @return the index of the stem's last appearance in structure -1 if it does not appear
 */
int stem_end_i(Stem *stem, array_list_t *structure) {
    DataNode* component = (DataNode*) stem->components->entries[stem->components->size - 1];
    return component_end_i(component, structure);
}

/**
 * Joins an array of strings into one string, with each original string separated by delim
 *
 * @param dest the string to joing src into, NULL if join fails or n <= 0
 * @param src the array of strings to join
 * @param delim string used to seperate each value from src
 * @param n number of elements in src
 */
void join(char* dest, char** src, const char* delim, int n) {
    if (n <= 0 || delim == NULL || src == NULL || dest == NULL) {
        dest = NULL;
        return;
    }
    strcpy(dest, src[0]);
    for (int i = 1; i < n; i++) {
        strcat(dest, delim);
        strcat(dest, src[i]);
    }
}

/**
 * Determine if a stem is simply a wrapper for a single HC
 * @param stem the stem to check
 * @return true if stem contains a single HC, false otherwise
 */
bool stem_is_hc(Stem* stem) {
    return (stem->helices->size == 1 && stem->components->size == 1);
}
