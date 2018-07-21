#include <stdlib.h>
#include "Profile.h"

Profile* create_profile(char *profile) {
  Profile *prof = (Profile*) malloc(sizeof(Profile));
  prof->freq = 1;
  prof->genfreq = 0;
  prof->selected = false;
  prof->profile = profile;
  prof->bracket = NULL;
  //prof->repstruct = NULL;
  //prof->children = NULL;
  return prof;
}

void free_profile(void* ptr) {
  if(ptr == NULL) {
    return;
  }
  Profile* prof = (Profile*) ptr;
  free(prof->bracket);
  prof->bracket = NULL;
  free(prof->profile);
  prof->profile = NULL;
  free(prof);
  prof = NULL;
}