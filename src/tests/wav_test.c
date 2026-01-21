#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../wav.h"

static const char filepath[] = "./test.wav";
static const char test_pcm_filepath[] = "./test_mono_44100_f32le.pcm";

#define SAMPLE_RATE  (44100)
#define BUFFERSIZE   (1024)

int main(void) {
    FILE *in = fopen(test_pcm_filepath, "rb");
    if (!in) {
        perror("fopen test.pcm");
        return 1;
    }

    wav_writer *w = wav_open(filepath, WAVE_FORMAT_IEEE_FLOAT, SAMPLE_RATE, 1, 32);
    if (!w) {
        fprintf(stderr, "wav_open failed\n");
        fclose(in);
        return 1;
    }

    unsigned char buf[BUFFERSIZE];
    size_t readn;

    while ((readn = fread(buf, 1, sizeof buf, in)) > 0) {
        size_t written = wav_write(w, buf, readn);
        if (written != readn) {
            fprintf(stderr, "wav_write error: wrote %zu, expected %zu\n", written, readn);
            fclose(in);
            wav_close(w);
            return 1;
        }
    }

    if (ferror(in)) {
        perror("fread");
        fclose(in);
        wav_close(w);
        return 1;
    }

    fclose(in);

    if (wav_close(w) != 0) {
        fprintf(stderr, "wav_close failed\n");
        return 1;
    }

    return 0;
}