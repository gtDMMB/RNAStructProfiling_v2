#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Options.h"

Options* make_options() {
    Options *opt = (Options*) malloc(sizeof(Options));
    opt->OUTPUT = (char*) malloc(sizeof(char) * NAME_STRING_BUFFER);
    strcpy(opt->OUTPUT, DEF_OUTPUT);
    opt->INPUT = NULL;
    opt->NATIVE = NULL;
    opt->MIN_HEL_LEN = DEF_MIN_HEL_LEN;
    opt->NUM_FHC = 0;
    opt->NUM_SPROF = 0;
    opt->HC_FREQ = -1;
    opt->VERBOSE = 0;
    opt->SFOLD = 0;
    opt->PROF_FREQ = -1;
    opt->COVERAGE = 0.5;
    opt->NUMSTRUCTS = 0;
    opt->PNOISE = 5;
    opt->CYCLES = 10;
    opt->GRAPH = 1;
    opt->REP_STRUCT = 0;
    opt->TOPDOWN = 0;
    opt->ALTTHRESH = 1;
    opt->GRAPH_SIZE_CAP = DEF_GRAPH_SIZE_CAP;

    opt->CONSOLIDATE = 1;
    opt->CONSOLIDATED_OUTPUT = (char*) malloc(sizeof(char) * NAME_STRING_BUFFER);
    opt->STEM_END_DELTA = 3;
    opt->STEM_VALID_PERCENT_ERROR = 5;
    opt->STEM_NUM_CUTOFF = 25;
    opt->FUNC_SIMILAR_PERCENT_ERROR = 1;
    opt->FUNC_SIMILAR_END_DELTA = 3;
    opt->NUM_FSTEMS = 0;
    opt->NUM_S_STEM_PROF = 0;
    opt->STEM_FREQ = -1;
    opt->STEM_PROF_FREQ = -1;
    opt->STEM_GRAPH = 1;
  return opt;
}

void print_options() { 
    puts("OPTIONS");
    puts("-e [FILE]     External structure as input, following gtboltzmann format (dot bracket, energy, set of triplets on same line)");
    puts("-sfold [FILE] External structure as input, following Sfold format ('Structure xx' followed by a triplet per line");
    puts("-gc INT       Set graph size cutoff. Graph will not be generated if it has > [gc] vertices. -1 for no limit");
    puts("-h DBL        Set frequency threshold for hc features(in percentage, e.g. 10.5 for 10.5% threshold)");
    puts("-p DBL        Set frequency threshold for selected profiles (in percentage, e.g. 10.5 for 10.5% threshold)");
    puts("-c DBL        Set minimum coverage requirement for featured helix classes and selected profiles (in percentage)");
    puts("-f INT        Set number of featured helix classes");
    puts("-s INT        Set number of selected profiles");
    puts("-l INT        Set minimum helix length");
    puts("-u INT        Set number of structures to profile");
    /*puts("-m INT        Set PNOISE");*/
    puts("-o NAME       Prefix of output files (generates [NAME].dot and [NAME]_consolidated.dot)");
    puts("-i FILE       File containing external structure to be inserted into summary profile graph");
    puts("-n FILE       File containing native structure to be inserted into summary profile graph");
    puts("-v            Run in verbose mode");
    puts("-g            Run without generating summary profile graph");

    puts("\nConsolidation options (stems and functionally similar stem groups):");
    puts("-sc      Run without using consolidation of helix classes into stems");
    puts("-sd  INT Set the max distance between two stems' ends for them to be merged");
    puts("-se  DBL Set the maximum percent error in frequency for two stems to be merged");
    // TODO: better way to determine # helices to consider
    puts("-snh INT Set the max number of helices to consider for initial stem creation");
    puts("-fsd INT Set the max distance between two stem's ends for them to be considered functionally similar");
    puts("-fse DBL Set the maximum percent error in frequency for two stems to be considered functionally similar (how often the can occur in the same structure)");
    puts("-sh  DBL Set frequency threshold for stem features(in percentage, e.g. 10.5 for 10.5% threshold)");
    puts("-sf  INT Set number of featured stems");
    puts("-sp  DBL Set frequency threshold for selected stem profiles (in percentage, e.g. 10.5 for 10.5% threshold)");
    puts("-ss  INT Set number of selected stem profiles");
    puts("-sg      Run without generating summary stem profile graph");

    //puts("-t         Run with top-down alternate algorithm");
    // puts("-a         Run with alternate threshold");
    puts("\ngtboltzmann options (passed to gtboltzmann):");
    //puts("-d, --dangle INT Restricts treatment of dangling energies (INT=0,2)");
    puts("--limitcd INT     Set a maximum base pair contact distance to INT. If no limit is given, base pairs can be over any distance");
    puts("--paramdir DIR    Path to directory from which parameters are to be read");
    puts("--useSHAPE FILE   Use SHAPE constraints from FILE");
    puts("-w, --workdir DIR Path of directory where output files will be written");
    puts("--sample INT      Number of structures to sample");
}

void free_options(Options* opt) {
    free(opt);
}
