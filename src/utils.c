#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

void die(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr); 
    exit(1);
}

int parse_int(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') 
        return -1; 
    *out = (int)v;
    return 0;
}


int parse_float(const char *s, float *out) {
    char *end;
    long v = strtof(s, &end);
    if (end == s || *end != '\0') 
        return -1; 
    *out = (int)v;
    return 0;
}

#define MAXLINESIZE (50)

void skip_lead_spaces(char **ps) {
    if (!ps || !*ps) 
        return;
    char *s = *ps;
    while (*s == ' ') 
        s++;
    *ps = s;
}


int stdin_readline(const char *prompt, void *line){
    printf("%s", prompt);
    char *p_line = line;
    int c; 
    int cnt = 0;
    while ((c = getc(stdin)) != '\n' && c != EOF){
        if (cnt > MAXLINESIZE)
            return -1;

        *p_line++ = c;
        cnt++;
    }

    *p_line = '\0';
    return 0;
} 


int count_split_params(const char *s) {
    int cnt = 0;
    bool in_word = false;

    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if (isspace(*p)) {
            in_word = false;
        } else if (!in_word) {
            cnt++;
            in_word = true;
        }
    }
    return cnt;
}

char *get_until_ws(char **ps) {
    if (!ps || !*ps)
        return NULL;

    char *s = *ps;

    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) {               
        *ps = s;
        return NULL;
    }

    char *p = strchr(s, ' ');
    size_t len;

    if (!p) {
        len = strlen(s);
        *ps = s + len;       
    } else {
        len = (size_t)(p - s);
        *ps = p + 1;           
    }

    char *out = malloc(len);
    if (!out)
        return NULL;

    memcpy(out, s, len);
    out[len] = '\0';

    return out;
}

char **split(char *s, size_t *n){
    if (n) *n = 0;
    if (!s || !n) 
        return NULL;

    skip_lead_spaces(&s);

    int cap = count_split_params(s);
    char **out = malloc(sizeof(char *) * cap);

    for (int i = 0; i < cap; i++){
        out[i] = get_until_ws(&s);
    }
    
    *n = cap;
    return out;
}

void split_free(char **v, size_t n) {
    if (!v) 
        return;
    for (size_t i = 0; i < n; ++i) 
        free(v[i]);
    free(v);
}

//out >= 4
void write_u32_le(unsigned char *out, uint32_t v){
    out[0] = (unsigned char)( v        & 0xFFu);
    out[1] = (unsigned char)((v >>  8) & 0xFFu);
    out[2] = (unsigned char)((v >> 16) & 0xFFu);
    out[3] = (unsigned char)((v >> 24) & 0xFFu);
}

//out >= 2
void write_u16_le(unsigned char *out, uint16_t v){
    out[0] = (unsigned char)( v        & 0xFFu);
    out[1] = (unsigned char)((v >>  8) & 0xFFu);
}