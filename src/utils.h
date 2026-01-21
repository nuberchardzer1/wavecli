#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define DEBUG 

#ifdef DEBUG
  #include <stdio.h>
  #include <time.h>

  #define DEBUG_PRINTF(fmt, ...) do {                                      \
      time_t now = time(NULL);                                             \
      fprintf(stderr, "\n### %s:%d %s() @ %s",                              \
              __FILE__, __LINE__, __func__, ctime(&now));                  \
      fprintf(stderr, (fmt), ##__VA_ARGS__);                               \
  } while (0)
#else
  #define DEBUG_PRINTF(...) do { } while (0)
#endif

void die(const char *fmt, ...);
int parse_int(const char *s, int *out);
int parse_float(const char *s, float *out);
char **split(char *s, size_t *n);
void split_free(char **v, size_t n);
int stdin_readline(const char *prompt, void *line);
void write_u32_le(unsigned char *out, uint32_t v);
void write_u16_le(unsigned char *out, uint32_t v);
#endif

