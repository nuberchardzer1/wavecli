#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include "app.h"
#include "utils.h"
#include "command.h"
#include "effect.h"
#include "audio_io.h"
#include "audio_types.h"
#include "portaudio.h"

const struct option long_options[] = {
    { "help",     no_argument,       NULL, 'h'},
    { "gain",     required_argument, NULL, 'g'},
    { "rate",     required_argument, NULL, 'r'},
    { "channels", required_argument, NULL, 'c'},
    { 0, 0, 0, 0 }
};

void print_help(){
    printf("\n");
    printf("WAVECLI — minimal real-time audio DSP monitor / capture tool\n");
    printf("\n"
           "Usage: %s [--gain X] [--rate N] [--channels 1|2] [--help]\n"
           "\n"
           "  --gain     X        gain multiplier (def: 1.0)\n"
           "  --rate     N        sample rate (def: 48000)\n"
           "  --channels 1|2      mono/stereo (def: 2)\n"
           "  --help              this help\n"
           "\n",
           progname);


    printf("Notes:\n");
    printf("  • Invalid values fall back to defaults with warning\n");
    printf("  • Real-time processing uses PortAudio (input → DSP → output)\n");
    printf("  • Add more options later (volume, effects, recording, device selection)\n\n");
}

void parse_opts(int argc, char *argv[]){
    int ch;
    while ((ch = getopt_long(argc, argv, "t:a:", long_options, NULL)) != -1){
        // check to see if a single character or long option came through
        float volume_val;
        float gain_val; 
        int channels_val; 

        switch (ch){
            // short option 't'
            case 'h':
                print_help();
                exit(0);
            // short option 'a'
            case 'g':
                if (parse_float(optarg, &gain_val)){
                    fprintf(stderr, "wrong val, used default\n");
                    break;
                }
                audio_cb_ctx->audio_params.gain = gain_val;
                break;
            case 'v': 
                if (parse_float(optarg, &volume_val)){
                    fprintf(stderr, "wrong val, used default\n");
                    break;
                }
                audio_cb_ctx->audio_params.volume = volume_val;
                break;
            case 'c':
                if (parse_int(optarg, &channels_val)){
                    fprintf(stderr, "wrong val, used default\n");
                    break;
                }
                audio_cb_ctx->audio_params.channels = channels_val;
                break;
            case 'w':
                audio_cb_ctx->flags |= FLAG_RECORD;
                break;
            default:
                die("error: unknown option");
        }
    }
}

int set_gain_cmd(int argc, const char** argv){
    if (argv == NULL || argc < 1){
        DEBUG_PRINTF("handle set gain command. no args\n");
        return -1;
    }
    float gain;
    parse_float(argv[0], &gain);
    DEBUG_PRINTF("handle set gain command. args: %0.2f\n", gain);
    audio_params_t *p = &audio_cb_ctx->audio_params;
    p->gain = gain;
    return 0;
}

int set_volume_cmd(int argc, const char** args){
    audio_params_t *p = &audio_cb_ctx->audio_params;
    float arg = *(float *)args;
    p->volume = arg;
    return 0;
}

static int print_effects(void) {
    printf("Available effects:\n");
    printf("---------------------------------------------------------------\n");
    printf("%-4s %-15s %s\n", "ID", "NAME", "DESCRIPTION");
    printf("---------------------------------------------------------------\n");

    for (size_t i = 0; i < effects_count; ++i) {
        const char *name = effects[i].name ? effects[i].name : "(null)";
        const char *desc = effects[i].description ? effects[i].description : "(no description)";
        printf("%-4d %-15s %s\n", i, name, desc);
    }

    printf("---------------------------------------------------------------\n");
    return 0;
}

int set_effect_cmd(int argc, const char** argv){
    (void)argv;

    print_effects();
    const int maxsize = 50;
    char line[maxsize];
    stdin_readline("Write effect number: ", line);

    int effect_index;
    if (parse_int(line, &effect_index) < 0)
        return -1;
    
    if (effect_index >= effects_count)
        return -1;

    audio_cb_ctx->dsp_process = effects[effect_index].func;
    return 0;
}

int start_recording_cmd(int argc, const char** argv){
    DEBUG_PRINTF("handle start record command\n");
    set_record_flag();

    printf("argc: %d\n", argc);
    for (int i = 0; i<argc; i++)    
        printf("arg %d: %s\n", i, argv[i]);

    const char *filepath = NULL;
    if (argc >= 1)
        filepath = argv[0];

    printf("filepath: %s\n", filepath);
    audio_io_open_record_file(filepath);
    return 0;
}

int stop_recording_cmd(int argc, const char** args){
    (void)args;
    DEBUG_PRINTF("handle stop record command\n");
    set_no_record_flag();
    return audio_io_close_record_file();
}

int help_cmd(int argc, const char **argv) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("WAVECLI TUI Audio Tool\n");
    printf("───────────────────\n\n");

    printf("%-7s %-5s %s\n", "Command", "Alias", "Description");
    printf("─────── ───── ────────────────────────────────────────────────\n");
    printf("gain    g     Set gain multiplier           <value>\n");
    printf("effect  e     Select & apply audio effect      \n");
    printf("record  r     Start recording to file       optional[filename]\n");
    printf("input   di    Select input device               \n");
    printf("output  do    Select output device              \n");
    printf("help    h     Show this help\n");
    printf("\n");

    printf("Examples:\n");
    printf("  gain 1.5          or   g 1.5       → ≈ +3.5 dB gain\n");
    printf("  record            or   r           → record to default filename\n");
    printf("  record test.wav                    → record to \"test.wav\"\n");
    printf("  effect            or   e           → show list and prompt for number\n");
    printf("  input             or   di          → interactive device selection\n\n");

    printf("Note:\n");
    printf("  • Commands without arguments usually enter interactive mode\n");
    printf("  • Short aliases (g, e, r, di, do, h) work everywhere\n\n");

    return 0;
}

int select_input_device_cmd(int argc, const char** argv){
    (void)argc;
    (void)argv;

    if (fprint_devices(stdout) < 0){
        fprintf(stderr, "choose_input_device: failed to print devices\n");
        return -1;
    }
    
    const int maxsize = 50;
    char line[maxsize];

    if (stdin_readline("Write device number: ", line) < 0) {
        fprintf(stderr, "choose_input_device: failed to read line\n");
        return -1;
    }

    device_index idx;
    if (parse_int(line, &idx) < 0) {
        fprintf(stderr, "choose_input_device: not a number: \"%s\"\n", line);
        return -1;
    }
    const int channels = audio_cb_ctx->audio_params.channels;
    if (set_in_dev_audio_io(idx, channels) < 0) {
        fprintf(stderr, "choose_input_device: failed to set input device=%d channels=%d\n",
                (int)idx, channels);
        return -1;
    }
    return 0;
}

int select_output_device_cmd(int argc, const char** argv){
    (void)argc;
    (void)argv;

    fprint_devices(stdin);

    const int maxsize = 50;
    char line[maxsize];

    if (stdin_readline(lineprompt, line) < 0) {
        fprintf(stderr, "choose_output_device: failed to read line\n");
        return -1;
    }

    device_index idx;
    if (parse_int(line, &idx) < 0) {
        fprintf(stderr, "choose_output_device: not a number: \"%s\"\n", line);
        return -1;
    }

    const int channels = audio_cb_ctx->audio_params.channels;
    if (set_in_dev_audio_io(idx, channels) < 0) {
        fprintf(stderr, "choose_output_device: failed to set output device idx=%d channels=%d\n",
                (int)idx, channels);
        return -1;
    }

    return 0;
}


static const command_t commands[] = {
    { "gain",    "g",   set_gain_cmd       },
    { "effect",  "e",   set_effect_cmd         },
    { "record",  "r",   start_recording_cmd    },
    { "help",    "h",   help_cmd          },     
    { "input",   "di",  select_input_device_cmd  },
    { "output",  "do",  select_output_device_cmd }
};

static const size_t commands_count = sizeof(commands) / sizeof((commands[0]));

//drop prog if error


int handle_command(const char *cmd, int argc, const char **argv){
    for (int i = 0; i < commands_count; i++){
        if (strcmp(cmd, commands[i].longname) == 0 || strcmp(cmd, commands[i].shortname) == 0){
            commands[i].func(argc, argv);
        }
    }
}

