#ifndef PROFILE_H
#define PROFILE_H

#include <stdio.h>
#include <stdbool.h>

typedef struct profile {
  int freq;
  int genfreq;
  bool selected;
  char *profile;
  char *bracket;
  //char *repstruct;
  //struct profile **children;
} Profile;

Profile* create_profile(char *profile);
void free_profile(void* ptr);

#endif
