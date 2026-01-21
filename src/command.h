#ifndef COMMAND_H
#define COMMAND_H

#include <getopt.h>    // for option
#include "audio_io.h"  // for device_index

typedef int (*command_fn)(int argc, const char** args);

typedef struct command_t{
    char *longname;
    char *shortname;
    command_fn func;
    char *help;
} command_t;

extern const struct option long_options[];

void parse_opts(int argc, char *argv[]);
int handle_command(const char *cmd, int argc, const char **argv);

int select_input_device_cmd(int argc, const char** argv);
int select_output_device_cmd(int argc, const char** argv);
int stop_recording_cmd(int argc, const char** args);

void print_help();

#endif