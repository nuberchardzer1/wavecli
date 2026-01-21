#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include <stdint.h>
#include <stdatomic.h>
#include <portaudio.h>

#include "audio_types.h"
#include "wav.h"

// #define VISUALIZE_EFFECTS

typedef PaStreamCallback audio_io_cb_t;
typedef PaDeviceIndex device_index;

typedef uint32_t flags_t;

enum flags {
    FLAG_DEBUG = 1u << 0,
	FLAG_RECORD = 1u << 1,
};

typedef struct {
    _Atomic SAMPLE peak; // float [-1...1] 
    _Atomic unsigned long frames;
} meter_t;

typedef struct audio_cb_ctx_t{
    wav_writer* ww;
    flags_t flags;
    audio_params_t audio_params;
    meter_t metrics;
    audio_process_fn dsp_process;
} audio_cb_ctx_t;

extern audio_cb_ctx_t *audio_cb_ctx;

int init_audio_io(int channels);
int terminate_audio_io();

int fprint_devices(FILE *file); 

int set_in_dev_audio_io(device_index idx, int channels);
int set_out_dev_audio_io(device_index idx, int channels);

int is_record(void);
void set_record_flag(void);
void set_no_record_flag(void);

int audio_io_open_record_file(const char* filepath);
int audio_io_write_to_record_file(const void* data, size_t bytes);
int audio_io_close_record_file();

#endif