#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <signal.h>

#include "app.h"
#include "audio_io.h"
#include "effect.h"
#include "audio_types.h"
#include "utils.h"
#include "command.h"
#include "effect.h"
#include "audio_io.h"

#define BUFSIZE (8192)

char *progname;
char *lineprompt;

static volatile sig_atomic_t g_sigint = 0;

void free_app(){
    free(audio_cb_ctx);
}

static float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

// void print_record_meter(const meter_t *m) {
//     const int width = 50;
//     const float gain = 30.0f;

//     float peak = atomic_load(&m->peak);
//     peak = clamp01(peak * gain);

//     int filled = (int)lroundf(peak * width);
//     if (filled < 0) filled = 0;
//     if (filled > width) filled = width;

//     char bar[width + 1];
//     for (int i = 0; i < width; ++i)
//         bar[i] = (i < filled) ? '#' : ' ';
//     bar[width] = '\0';

//     printf("\033[5A");

//     printf("REC peak=%0.2f\n", peak);
//     printf("|%s|\n", bar);
//     printf("|%s|\n", bar);
//     printf("|%s|\n", bar);
//     printf("|%s|\n", bar);
//     fflush(stdout);
// }

void print_record_meter(const meter_t *m) {
    const int width = 50;
    const float gain = 30.0f;

    float peak = atomic_load(&m->peak);
    peak = clamp01(peak * gain);

    int filled = (int)lroundf(peak * width);
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;

    char bar[width + 1];
    for (int i = 0; i < width; ++i)
        bar[i] = (i < filled) ? '#' : ' ';
    bar[width] = '\0';

    printf("\rREC peak=%0.2f |%s| cntrl + c to stop", peak, bar);
    fflush(stdout);
}

static void signal_handler(int signum){
    if (signum == SIGINT)
        g_sigint = 1;
}


void init_signals(struct sigaction *sigact){
    sigact->sa_handler = signal_handler;
    sigemptyset(&sigact->sa_mask);
    sigact->sa_flags = 0;
    sigaction(SIGINT, sigact, NULL);
}

// cc -g *.c $(pkg-config --cflags --libs portaudio-2.0) && ./a.out --gain=0
int main(int argc, char *argv[]){
    progname = argv[0];

    lineprompt = malloc(MAXLINESIZE);
    if (!lineprompt){
        perror("malloc");
        return 1;
    }

    //initialize prompt in stdout
    sprintf(lineprompt, "%s> ", progname);
    
    const int default_channels = 1;
    if (init_audio_io(default_channels) < 0)
        die("audio initialization failed\n");

    parse_opts(argc, argv);

    int channels = audio_cb_ctx->audio_params.channels;

    //starting choice 
    if (select_input_device_cmd(0, NULL) < 0)
        return 1;
    
    char line[MAXLINESIZE];
    int rc;
    char *cmd;
    size_t n;

    struct sigaction sigact;
    init_signals(&sigact);

    for (;;){
        if (g_sigint) {
            g_sigint = 0;
            if (is_record()) { // stop recording 
                stop_recording_cmd(0, NULL);
                printf("\n");
                continue;
            }
            goto cleanup;
        }
        //when record
        if (is_record()) {
            print_record_meter(&audio_cb_ctx->metrics);
            usleep(50 * 1000);
            continue;
        }

        memset(line, 0, sizeof line); 
        n = 0;
        cmd = NULL;

        if (stdin_readline(lineprompt, line) < 0)
            goto cleanup;
        
        char **arr = split(line, &n);
        if (!arr) 
            continue;

        if (n == 0) {
            split_free(arr, n);
            continue;
        }
        
        cmd = arr[0];
        int cmd_argc = (n > 0) ? (int)(n - 1) : 0;
        const char **cmd_argv = (const char **)(arr + 1);

        int rc = handle_command(cmd, cmd_argc, cmd_argv); 
        if (rc < 0){
            print_help();
        }

        split_free(arr, cmd_argc);
    }

cleanup:
    printf("\n");
    terminate_audio_io();
}
