/**
 * CS 240 Pointer Potions command parsing library.
 * Sample solution by Ben Wood, Fall 2015.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "command.h"

char** command_parse(char* line, int* foreground) {
  assert(line);

  // Count the words in the line.
  unsigned numwords = 0;
  {
    char* cursor;
    // Keeps track of whether cursor is in a word or a delimiter
    int inword = 0;
    for (cursor = line; *cursor != '\0' && *cursor != '&'; cursor++) {
      if (inword) {
        if (*cursor == ' ') {
          // word -> space transition
          inword = 0;
        }
      } else {
        if (*cursor != ' ') {
          // space -> word transition
          inword = 1;
          numwords++;
        }
      }
    }
    assert(*cursor == '&' || *cursor == '\0');
    const int ampersand = (*cursor == '&');
    if (ampersand) {
      // Check end of string.
      for (cursor++; *cursor != '\0'; cursor++) {
        // Only spaces allowed after &.
        if (*cursor != ' ') return NULL;
      }
    }
    // Foreground process unless we ended with &.
    *foreground = !ampersand;
  }

  // Create an array of char*'s with one slot for each word,
  // plus one slot for a NULL terminator.
  char** words = (char**)malloc(sizeof(char*) * (numwords + 1));
  if (!words) {
    perror("malloc");
    exit(-1);
  }








  // For each word in line.
  char* cursor = line;
  char** word = words;
  // Fill the corresponding array slot...
  for (int i = 0; i < numwords; i++, word++) {
    // Blast through any number of contiguous spaces.
    while (*cursor == ' ') cursor++;
    // Word starts at first non-space character.
    char* start = cursor;
    // Count length of word.
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '&') cursor++;
    // Allocate space for word (plus null terminator)
    // and store pointer to it in words array.
    *word = (char*)malloc(sizeof(char) * (cursor - start + 1));
    if (!*word) {
      perror("malloc");
      exit(-1);
    }
    // Copy word into this space...
    char* src;
    char* dest;
    for (src = start, dest = *word; src < cursor; src++, dest++) {
      *dest = *src;
    }
    // ... and null-terminate the word.
    *dest = '\0';
  }
  // ... and NULL-terminate the words array.
  *word = NULL;

  return words;
}

/*
  Free a command created by parse.
    - free each string
    - free the top-level array
*/
void command_free(char** command) {
  for (char** cursor = command; *cursor; cursor++) {
    free(*cursor);
  }
  free(command);
}

/*
  Print a command created by parse in a form matching what a user
  might type at the command line.  (You may include or omit a newline
  at the end as you wish -- later, it may be convenient to omit it.)
  command_show is similar.
*/
void command_print(char** command) {
  for (char** cursor = command; *cursor; cursor++) {
    printf("%s ", *cursor);
  }
}
