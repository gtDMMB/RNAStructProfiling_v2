#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtbl.h"
#include "Set.h"
#include "Options.h"
#include "memoryDFS.h"
#include "boltzmann_main.h"
#include "helix_class.h"
#include "stem.h"
#include "consolidated_graph.h"

using namespace std;

/*input first the fasta file, then optionally the sample_1000.out file run on the fasta, then options*/
int main(int argc, char *argv[]) {
    int i,input = 0, gtargs = 9;
    char **args = NULL, *name;
    HASHTBL *deleteHash;
    FILE *fp;
    Set *set;
    Options *opt;

    if (argc < 2 || !strcmp(argv[1],"--help")) {
/*print out list of options
    fprintf(stderr,"Not enough arguments\n");
    exit(EXIT_FAILURE);*/
        puts("Error: Missing input file.");
        puts("Usage: RNAprofile [OPTIONS] ... FILE");
        puts("\tFILE is an RNA sequence file containing only the sequence or in FASTA format.\n");
        print_options();
        puts("\nEXAMPLES");
        puts("1. Profile 1,000 structures sampled with gtboltzmann, default options");
        puts("\t./RNAprofile <seq_file>");
        puts("2. Profile input structures obtained from gtboltzmann with verbose output");
        puts("\t./RNAprofile -e <output samples file> -v <seq_file>\n");
        exit(EXIT_FAILURE);
    }
    set = make_Set("output.samples");
    /* set = make_Set(argv[2]); */
    opt = set->opt;
    args = (char**)malloc(sizeof(char*)*16);
    /* Set default options for gtboltzmann */
    args[1] = (char*)"--paramdir";
    args[2] = (char*)"./data/GTparams/";
    //args[2] = (char*)"../Desktop/share/gtfold/Turner99/";
    args[3] = (char*)"-o";
    args[4] = (char*)"output";
    args[5] = (char*)"-s";
    args[6] = (char*)"1000";
    args[7] = (char*)"--scale";
    args[8] = (char*)"0.0";
    for (i = 1; i < argc-1; i++) {
        //printf("argv[%d] is %s\n",i,argv[i]);
        if (!strcmp(argv[i],"-e")) {
            if (i + 1 <= argc - 2) {
                char* temp = (char*) malloc(sizeof(char) * (strlen(argv[i+1]) + 1));
                strcpy(temp, argv[i+1]);
                set->structfile = temp;
                i++;
                input = 1;
            }
        }
        if (!strcmp(argv[i],"-sfold")) {
            if (i + 1 <= argc - 2) {
                char* temp = (char*) malloc(sizeof(char) * (strlen(argv[i+1]) + 1));
                strcpy(temp, argv[i+1]);
                set->structfile = temp;
                i++;
                input = 1;
                opt->SFOLD = 1;
            }
        }
        else if (!strcmp(argv[i],"-h")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->HC_FREQ))) {
                opt->HC_FREQ = atof(argv[i+1]);
                if (opt->HC_FREQ < 0 || opt->HC_FREQ > 100) {
                    fprintf(stderr,"Error: invalid input %f for hc frequency threshold\n",opt->HC_FREQ);
                    opt->HC_FREQ = -1;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-p")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->PROF_FREQ))) {
                opt->PROF_FREQ = atof(argv[i+1]);
                if (opt->PROF_FREQ < 0 || opt->PROF_FREQ > 100) {
                    fprintf(stderr,"Error: invalid input %lf for frequency threshold\n",opt->PROF_FREQ);
                    opt->PROF_FREQ = -1;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-c")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->COVERAGE))) {
                opt->COVERAGE = atof(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-f")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->NUM_FHC))) {
                opt->NUM_FHC = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-s")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->NUM_SPROF))) {
                opt->NUM_SPROF = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-l")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->MIN_HEL_LEN))) {
                opt->MIN_HEL_LEN = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-u")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->NUMSTRUCTS))) {
                opt->NUMSTRUCTS = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-m")) {
            if (i + 1 <= argc - 2) {
                opt->PNOISE = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-o")) {
            if (i + 1 <= argc - 2) {
                strcpy(opt->OUTPUT,argv[i+1]);
                int len = (int) strlen(opt->OUTPUT);
                if (len < 5 || strcmp(&(opt->OUTPUT[len-4]), ".dot") != 0) {
                    strcat(opt->OUTPUT, ".dot");
                }
                name = (char*) malloc(sizeof(char) * (strlen(argv[i+1]) + 8 + 1));
                strcpy(name, argv[i+1]);
                strcat(name,".samples");
                set->structfile = name;
                args[3] = argv[i];
                args[4] = argv[i+1];
                i++;
            }
        }
        else if (!strcmp(argv[i],"-i")) {
            if (i + 1 <= argc - 2) {
                opt->INPUT = argv[i+1];
                i++;
            }
        }
        else if (!strcmp(argv[i],"-n")) {
            if (i + 1 <= argc - 2) {
                opt->NATIVE = argv[i+1];
                i++;
            }
        } else if (!strcmp(argv[i],"-gc")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->GRAPH_SIZE_CAP))) {
                opt->GRAPH_SIZE_CAP = atoi(argv[i+1]);
                if (opt->GRAPH_SIZE_CAP < -1) {
                    fprintf(stderr,"Error: invalid input %d for graph size cap\n",opt->STEM_NUM_CUTOFF);
                    opt->GRAPH_SIZE_CAP = DEF_GRAPH_SIZE_CAP;
                }
                i++;
            }
        }
        // Consolidation options
        else if (!strcmp(argv[i],"-sc")) {
            opt->CONSOLIDATE = 0;
        }
        else if (!strcmp(argv[i],"-sd")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->STEM_END_DELTA))) {
                opt->STEM_END_DELTA = atoi(argv[i+1]);
                if (opt->STEM_END_DELTA < 0 || opt->STEM_END_DELTA > 15) {
                    fprintf(stderr,"Error: invalid input %d for stem end delta\n",opt->STEM_END_DELTA);
                    opt->STEM_END_DELTA = 3;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-se")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->STEM_VALID_PERCENT_ERROR))) {
                opt->STEM_VALID_PERCENT_ERROR = atof(argv[i+1]);
                if (opt->STEM_VALID_PERCENT_ERROR < 0 || opt->STEM_VALID_PERCENT_ERROR > 100) {
                    fprintf(stderr,"Error: invalid input %f for stem valid percent error\n",opt->STEM_VALID_PERCENT_ERROR);
                    opt->STEM_VALID_PERCENT_ERROR = 5;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-smh")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->STEM_NUM_CUTOFF))) {
                opt->STEM_NUM_CUTOFF = atoi(argv[i+1]);
                if (opt->STEM_NUM_CUTOFF < 0) {
                    fprintf(stderr,"Error: invalid input %d for number of helices to consider for stems\n",opt->STEM_NUM_CUTOFF);
                    opt->STEM_NUM_CUTOFF = 25;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-fsd")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->FUNC_SIMILAR_END_DELTA))) {
                opt->FUNC_SIMILAR_END_DELTA = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-fse")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->FUNC_SIMILAR_PERCENT_ERROR))) {
                opt->FUNC_SIMILAR_PERCENT_ERROR = atof(argv[i+1]);
                if (opt->FUNC_SIMILAR_PERCENT_ERROR < 0 || opt->FUNC_SIMILAR_PERCENT_ERROR > 100) {
                    fprintf(stderr,"Error: invalid input %f for functionally similar stem group percent error\n",opt->FUNC_SIMILAR_PERCENT_ERROR);
                    opt->FUNC_SIMILAR_PERCENT_ERROR = 1;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-sh")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->STEM_FREQ))) {
                opt->STEM_FREQ = atof(argv[i+1]);
                if (opt->STEM_FREQ < 0 || opt->STEM_FREQ > 100) {
                    fprintf(stderr,"Error: invalid input %f for hc frequency threshold\n",opt->STEM_FREQ);
                    opt->STEM_FREQ = -1;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-sf")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->NUM_FSTEMS))) {
                opt->NUM_FSTEMS = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-sp")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%lf",&(opt->STEM_PROF_FREQ))) {
                opt->STEM_PROF_FREQ = atof(argv[i+1]);
                if (opt->STEM_PROF_FREQ < 0 || opt->STEM_PROF_FREQ > 100) {
                    fprintf(stderr,"Error: invalid input %lf for frequency threshold\n",opt->STEM_PROF_FREQ);
                    opt->STEM_PROF_FREQ = -1;
                }
                i++;
            }
        }
        else if (!strcmp(argv[i],"-ss")) {
            if ((i + 1 <= argc - 2) && sscanf(argv[i+1],"%d",&(opt->NUM_S_STEM_PROF))) {
                opt->NUM_S_STEM_PROF = atoi(argv[i+1]);
                i++;
            }
        }
        else if (!strcmp(argv[i],"-sg")) {
            opt->STEM_GRAPH = 0;
        }

/*
    else if (!strcmp(argv[i],"-k")) {
      if (i + 1 <= argc - 2) {
	opt->CYCLES = argv[i+1];
	i++;
      }
    }
*/

/*
GTBOLTZMANN OPTIONS

    else if (!strcmp(argv[i],"-d" || !strcmp(argv[i],"--dangle"))) {
      if (i + 1 <= argc - 2) {
	if (atoi(argv[i+1]) == 0 || atoi(argv[i+1]) == 2) {
	  args[gtargs] = argv[i];
	  args[gtargs+1] = atoi(argv[i+1]);
	  gtargs += 2;
	} else {
	  fprintf(STDERR,"Wrong arguments to -d/--dangle option: ignoring option\n");
	}
	i++;
      }
    }
*/
        else if (!strcmp(argv[i],"--paramdir")) {
            if (i + 1 <= argc - 2) {
                args[1] = argv[i];
                args[2] = argv[i+1];
                i++;
            }
        }
        else if (!strcmp(argv[i],"--limitcd")) {
            if (i+1 <= argc-2) {
                args[gtargs++] = argv[i++];
                args[gtargs++] = argv[i];
            }
        }
        else if (!strcmp(argv[i],"--useSHAPE")) {
            if (i+1 <= argc-2) {
                args[gtargs++] = argv[i++];
                args[gtargs++] = argv[i];
            }
        }
        else if (!strcmp(argv[i],"--sample")) {
            if (i+1 <= argc-2) {
                args[5] = argv[i++];
                args[6] = argv[i++];
            }
        }
        else if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--workdir")) {
            if (i+1 <= argc-2) {
                args[gtargs++] = argv[i++];
                args[gtargs++] = argv[i];
            }
        }
        else if (!strcmp(argv[i],"-v"))
            opt->VERBOSE = 1;
        else if (!strcmp(argv[i],"-g"))
            opt->GRAPH = 0;
        else if (!strcmp(argv[i],"-r"))
            opt->REP_STRUCT = 1;
        else if (!strcmp(argv[i],"-t"))
            opt->TOPDOWN = 1;
        else if (!strcmp(argv[i],"-a"))
            opt->ALTTHRESH = 0;
    }
    if (!input) {
        args[0] = (char*)"gtboltzmann";
        args[gtargs] = argv[argc-1];
        boltzmann_main(gtargs+1,args);
    }
    strncpy(set->opt->CONSOLIDATED_OUTPUT, set->opt->OUTPUT, strlen(set->opt->OUTPUT) - 4);
    set->opt->CONSOLIDATED_OUTPUT[strlen(set->opt->OUTPUT) - 4] = '\0';
    strcat(set->opt->CONSOLIDATED_OUTPUT, "_consolidated.dot");

    input_seq(set,argv[argc-1]);
    if (opt->SFOLD)
        process_structs_sfold(set);
    else
        process_structs(set);
    reorder_helices(set);
    print_all_helices(set);
    printf("Total number of helix classes: %d\n",set->hc_num);

    if (set->opt->NUM_FHC)
        set->opt->HC_FREQ = set_num_fhc(set);
    else if (set->opt->HC_FREQ==-1)
        set->opt->HC_FREQ = set_threshold_entropy(set);

    if (set->opt->VERBOSE) {
        printf("Threshold to find frequent helices: %.1lf%%\n",set->opt->HC_FREQ);
        printf("Number of structures processed: %d\n",set->opt->NUMSTRUCTS);
    }

    //find_bools(set);
    find_freq(set);

    printf("Total number of featured helix classes: %d\n",set->num_fhc);
    if (opt->SFOLD)
        make_profiles_sfold(set);
    else
        make_profiles(set);
    printf("Total number of profiles: %d\n",set->prof_num);
    //print_meta(set);
    print_profiles(set);


    if (set->opt->NUM_SPROF)
        set->opt->PROF_FREQ = set_num_sprof(set);
    else if (set->opt->PROF_FREQ == -1) {
        //set->opt->PROF_FREQ = set_p_threshold(set,P_START);
        set->opt->PROF_FREQ = set_p_threshold_entropy(set);
    }
    if (set->opt->VERBOSE) {
        if (set->opt->PROF_FREQ == -2) {
            printf("No threshold set; every profile has frequency of 1\n");
        } else
            printf("setting p to %.1f\n",set->opt->PROF_FREQ);
    }
    select_profiles(set);
    printf("Total number of selected profiles: %d\n",set->num_sprof);

    if (set->opt->INPUT)
        process_one_input(set);
    if (set->opt->REP_STRUCT) {
        find_consensus(set);
        print_consensus(set);
    }

    if (set->opt->GRAPH) {
        fp = fopen(set->opt->OUTPUT,"w");
        init_graph(fp,set);
        i = initialize(set);
        if (set->opt->INPUT)
            print_input(fp,set);
        find_LCAs(fp,set,i);
        if (set->opt->GRAPH) {
            calc_gfreq(fp,set);
            //printGraph();
            deleteHash = MemoryDFS(set->graph, GRAPHSIZE);
            removeEdges(deleteHash);
            //start_trans_reductn(set->graph);
            //printGraph();
            print_edges(fp,set);
            fputs("}",fp);
            free_hashtbl(deleteHash);
        }
        fclose(fp);
        if (! set->opt->GRAPH) {
            remove(set->opt->OUTPUT);
        }
        //free_node(graph);
    }

    if (set->opt->CONSOLIDATE) {
        printf("Consolidating...\n");
        set->structures = (array_list_t**) malloc(sizeof(array_list_t*) * set->opt->NUMSTRUCTS);
        set->stem_structures = (array_list_t**) malloc(sizeof(array_list_t*) * set->opt->NUMSTRUCTS);
        add_structures_to_set(set);
        //simplify set using stems
        add_initial_stems(set);
        bool combined_stems = true;
        // loop and combine stems until no more combinations can be made
        while(combined_stems) {
            combined_stems = combine_stems(set);
            // find frequencies of all combined stems
            update_freq_stems(set);
        }
        find_func_similar_stems(set);
        // TODO: determine if this should also be in a loop
        combine_stems_using_func_similar(set);
        update_freq_stems(set);
        reindex_stems(set);
        add_hc_stems(set);
        update_max_quads(set);
        update_ave_quads(set);
        if(set->opt->VERBOSE) {
            print_stems(set);
        }
        generate_stem_key(set, argv[argc-1]);
        if (set->opt->NUM_FSTEMS)
            set->opt->STEM_FREQ = set_num_fstems(set);
        else if (set->opt->STEM_FREQ==-1)
            set->opt->STEM_FREQ = set_threshold_entropy_stems(set);
        if (set->opt->VERBOSE) {
            printf("Threshold to find featured stems: %.1lf%%\n",set->opt->STEM_FREQ);
            printf("Number of structures processed: %d\n",set->opt->NUMSTRUCTS);
        }
        find_featured_stems(set);
        printf("Total number of featured stems: %d\n",set->num_fstems);
        make_stem_profiles(set);
        printf("Total number of stem profiles: %d\n",set->stem_prof_num);
        print_stem_profiles(set);
        stem_profiles_make_bracket(set);

        if (set->opt->NUM_S_STEM_PROF)
            set->opt->STEM_PROF_FREQ = set_num_s_stem_prof(set);
        else if (set->opt->STEM_PROF_FREQ == -1) {
            //set->opt->PROF_FREQ = set_p_threshold(set,P_START);
            set->opt->STEM_PROF_FREQ = set_stem_p_threshold_entropy(set);
        }
        if (set->opt->VERBOSE) {
            if (set->opt->STEM_PROF_FREQ == -2) {
                printf("No threshold set; every stem profile has frequency of 1\n");
            } else
                printf("setting p to %.1f\n",set->opt->STEM_PROF_FREQ);
        }
        select_stem_profiles(set);
        printf("Total number of selected stem profiles: %d\n",set->num_s_stem_prof);
        if (set->opt->STEM_GRAPH) {
            fp = fopen(set->opt->CONSOLIDATED_OUTPUT,"w");
            consolidated_init_graph(fp,set);
            i = consolidated_initialize(set);
            consolidated_find_LCAs(fp,set,i);
            if (set->opt->STEM_GRAPH) {
                consolidated_calc_gfreq(fp, set);
                deleteHash = MemoryDFS(set->consolidated_graph, CONSOLIDATED_GRAPHSIZE);
                consolidated_removeEdges(deleteHash);
                consolidated_print_edges(fp, set);
                fputs("}", fp);

                free_hashtbl(deleteHash);
            }
            fclose(fp);
            if (! set->opt->STEM_GRAPH) {
                remove(set->opt->CONSOLIDATED_OUTPUT);
            }
            //free_node(consolidated_graph);
        }
    }
    free_Set(set);
    exit(EXIT_SUCCESS);
}
