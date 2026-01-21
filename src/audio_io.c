#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <math.h>

#include "utils.h"
#include "audio_io.h"
#include "effect.h"
#include "portaudio.h"
#include "audio_types.h"
#include "wav.h"

#ifdef VISUALIZE_EFFECTS

#define MAXFILES (25)

#define MAX_VIS_FILENAME (50)

typedef struct effect_visual_t{
    FILE *file;
    effect_t effect;
} effect_visual_t;

effect_visual_t *effect_visuals[MAXFILES];
static int effect_visuals_count = 0;

static void open_vis_effects_files(){
    if (effects_count >= MAXFILES){
        fprintf(stderr, "too many files to visualize it");
        exit(1);
    }

    for (int i = 0; i < effects_count; i++){
        effect_visual_t *visual = malloc(sizeof(effect_visual_t));
        if (!visual){
            perror("malloc");
            exit(1);
        }

        visual->effect = effects[i];

        char filename[MAX_VIS_FILENAME];
        sprintf(filename, "%s.raw", visual->effect.name);

        visual->file = fopen(filename, "wb");
        if (!visual->file){
            fprintf(stderr, "error: can`t open visualization file");
            exit(1);
        }

        effect_visuals[effect_visuals_count++] = visual;
    }
}

static void free_visuals(){
    for (int i = 0; i < effect_visuals_count; i++){
        fclose(effect_visuals[i]->file);
        free(effect_visuals[i]);
    }
    effect_visuals_count = 0;
}

#endif

typedef struct {
    PaStream *stream;
    PaStreamParameters in_params;
    PaStreamParameters out_params;
    double sample_rate;
    unsigned long frames_per_buffer;
} audio_engine_t;

audio_engine_t audio_engine;
audio_cb_ctx_t *audio_cb_ctx;

int file_exists(const char* filepath){
    FILE *f = fopen(filepath, "r");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

char *find_new_filename(){
    const int max_files_cnt = 128;
    const int max_filepath_size = 128;
    char* filepath = malloc(max_filepath_size);
    for (int i = 1; i < max_files_cnt; i++){
        sprintf(filepath, "./%s_%d.wav", "out", i);
        if (!file_exists(filepath))
            return filepath;
    }

    free(filepath);
    return NULL;   
}

int audio_io_open_record_file(const char* filepath){
    printf("filepath: %s\n", filepath);
    if (!filepath)
        filepath = find_new_filename(); 
    
    printf("filepath: %s\n", filepath);
    audio_cb_ctx->ww = wav_open(filepath, WAVE_FORMAT_IEEE_FLOAT, SAMPLE_RATE, 
        audio_cb_ctx->audio_params.channels, sizeof(SAMPLE) * 8);
}

int audio_io_write_to_record_file(const void* data, size_t bytes){
    return wav_write(audio_cb_ctx->ww, data, bytes);
}

int audio_io_close_record_file(){
    int rc = 0;
    set_no_record_flag();
    if (wav_close(audio_cb_ctx->ww) < 0)
        rc = -1;

    audio_cb_ctx->ww = NULL;
    return rc;
}

int is_record(void){
    return (audio_cb_ctx->flags & FLAG_RECORD) != 0;
}

void set_record_flag(void){
    audio_cb_ctx->flags |= FLAG_RECORD;
}

void set_no_record_flag(void){
    audio_cb_ctx->flags &= ~FLAG_RECORD;
}

int stop_recording(){
    set_no_record_flag();
}

static inline void meter_update(meter_t *m, const SAMPLE *x, unsigned long n) {
    float peak = 0.0f;

    for (unsigned long i = 0; i < n; ++i) {
        float v = fabsf((float)x[i]);
        if (v > peak) peak = v;
    }

    atomic_store(&m->peak, peak);
}

static int audio_cb(const void *input, void *output,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    (void)timeInfo; (void)statusFlags; (void)userData;

    audio_cb_ctx_t *audio_cb_ctx = (audio_cb_ctx_t *)userData; 
    audio_params_t *audio_params = &audio_cb_ctx->audio_params; 
    SAMPLE *in = (SAMPLE *)input;
    SAMPLE *out = (SAMPLE*)output;

    #ifdef VISUALIZE_EFFECTS   
    for (int i = 0; i < effect_visuals_count; i++){
        SAMPLE in_copy[sizeof(SAMPLE) * frameCount * audio_params->channels];
        memcpy(in_copy, in, sizeof(SAMPLE) * frameCount * audio_params->channels);
        
        effect_visual_t *ev = effect_visuals[i];
        ev->effect.func(in_copy, frameCount, audio_params);
        fwrite(in_copy, sizeof(SAMPLE), 
            frameCount * audio_params->channels, 
            ev->file);
    }
    #endif

    //dsp
    if (audio_cb_ctx->dsp_process)
        audio_cb_ctx->dsp_process(in, frameCount, audio_params);

    memcpy(out, in, sizeof(SAMPLE) * frameCount * audio_params->channels);
    
    if (audio_cb_ctx->flags & FLAG_RECORD && audio_cb_ctx->ww){
        int n = audio_io_write_to_record_file(in, 
            sizeof(SAMPLE) * frameCount * audio_params->channels);
        
        meter_update(&audio_cb_ctx->metrics, in, frameCount);
    }

    return paContinue;  
}

int init_audio_cb_ctx(){
    audio_cb_ctx = calloc(1, sizeof(audio_cb_ctx_t));
    if (!audio_cb_ctx) { 
        perror("malloc"); 
        exit(1); 
    }
    // defaults
    audio_params_t *ap = &audio_cb_ctx->audio_params;
    ap->channels = 1;
    ap->gain = 10.0f;
    ap->volume = 0.2f;
    audio_cb_ctx->dsp_process = NULL;
    return 0;
}

int start_audio_io(){
    PaError err = Pa_OpenStream(
        &audio_engine.stream, 
        &audio_engine.in_params, 
        &audio_engine.out_params,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        0,
        audio_cb,
        audio_cb_ctx
    );
    
    if (err){
        fprintf(stderr, "error: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    
    err = Pa_StartStream(audio_engine.stream);
    if (err != paNoError) 
        return -1;
    
    return 0;
}

int init_audio_io(int channels){   
    #ifdef VISUALIZE_EFFECTS
    open_vis_effects_files();
    #endif
    
    if (init_audio_cb_ctx() < 0)
        return -1;

    if (Pa_Initialize() < 0)
        return -1;
    
    PaError err = paNoError;
    
    PaDeviceIndex input_device = Pa_GetDefaultInputDevice();
    PaDeviceIndex output_device = Pa_GetDefaultOutputDevice();

    if (input_device == paNoDevice || output_device == paNoDevice){
        fprintf(stderr, "error: %s\n", Pa_GetErrorText(paNoDevice));
        return -1;
    }

    audio_engine.in_params.device = (PaDeviceIndex)input_device;
    audio_engine.in_params.channelCount = channels;
    audio_engine.in_params.sampleFormat = paFloat32;
    audio_engine.in_params.suggestedLatency = Pa_GetDeviceInfo( 
        audio_engine.in_params.device )->defaultLowOutputLatency;
    
    audio_engine.out_params.device = (PaDeviceIndex)output_device;
    audio_engine.out_params.channelCount = channels;
    audio_engine.out_params.sampleFormat = paFloat32;
    audio_engine.out_params.suggestedLatency = Pa_GetDeviceInfo( 
        audio_engine.out_params.device )->defaultLowOutputLatency;  
    
    if (start_audio_io() < 0)
        return -1;
}

int terminate_audio_io_stream(){
    if (audio_engine.stream == NULL)
        return 0;
    
    PaError err; 
    if ((err = Pa_StopStream(audio_engine.stream)) < 0){
        if (err != paStreamIsStopped){
            fprintf(stderr, "terminate_audio_io_stream error: %s\n", Pa_GetErrorText(err));
            return -1;
        }
    }

    if ((err = Pa_CloseStream(audio_engine.stream)) < 0){
        fprintf(stderr, "Pa_CloseStream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    audio_engine.stream = NULL;
    return 0;
}

int terminate_audio_io(){
    PaError err;
    if ((err = terminate_audio_io_stream())){
        fprintf(stderr, "error: ", Pa_GetErrorText(err));
        return -1;
    }
    if ((err = Pa_Terminate())){
        fprintf(stderr, "error: ", Pa_GetErrorText(err));
        return -1;
    }

    #ifdef VISUALIZE_EFFECTS
    free_visuals(); 
    #endif
};

int restart_audio_io(){
    if (terminate_audio_io_stream() < 0)
        return -1;
    
    if (start_audio_io() < 0)
        return -1;
    
    return 0;
}

/* envelope for Pa_IsFormatSupported. return: like Pa_IsFormatSupported*/
PaError validate_input_device(PaDeviceIndex device, double srate){
    PaError err;
    const PaDeviceInfo *device_info = Pa_GetDeviceInfo(device);
    if (device_info->maxInputChannels < 1)
        return paInvalidChannelCount;

    PaStreamParameters in_params = {0};
    in_params.device = device;
    in_params.sampleFormat = paFloat32;
    in_params.suggestedLatency = Pa_GetDeviceInfo( in_params.device )->defaultLowOutputLatency;
    in_params.channelCount = 1;
    return Pa_IsFormatSupported(&in_params, NULL, srate);
}

int fprint_devices(FILE *file){
    fprintf(file, "Choose your audio device:\n");
    fprintf(file, "---------------------------------------------------------------\n");
    fprintf(file, "%-9s %s\n", "NUMBER", "NAME");
    fprintf(file, "---------------------------------------------------------------\n");

    int num_devices = Pa_GetDeviceCount();
    if (num_devices < 0) {
        DEBUG_PRINTF("Pa_GetDeviceCount error: %s\n", Pa_GetErrorText(num_devices));
        return -1;
    }

    for (int i = 0; i < num_devices; ++i) {
        const PaDeviceInfo *di = Pa_GetDeviceInfo((PaDeviceIndex)i);
        const char *name = (di && di->name) ? di->name : "(null)";
        fprintf(file, "%-9d %s\n", i, name);
    }
    fprintf(file, "---------------------------------------------------------------\n");
    return 0;
}

int set_in_dev_audio_io(device_index idx, int channels){
    if (terminate_audio_io_stream() < 0)
        return -1;

    audio_engine.in_params.device = (PaDeviceIndex)idx;
    audio_engine.in_params.channelCount = channels;
    audio_engine.in_params.sampleFormat = paFloat32;
    
    const PaDeviceInfo *di = Pa_GetDeviceInfo(audio_engine.in_params.device);
    if (!di) { return -1; }
    audio_engine.in_params.suggestedLatency = di->defaultLowOutputLatency;
    
    if (restart_audio_io() < 0)
        return -1;

    return 0;
}

int set_out_dev_audio_io(device_index idx, int channels){
    if (terminate_audio_io_stream() < 0)
        return -1;
    
    audio_engine.out_params.device = (PaDeviceIndex)idx;
    audio_engine.out_params.channelCount = channels;
    audio_engine.out_params.sampleFormat = paFloat32;
    audio_engine.out_params.suggestedLatency = Pa_GetDeviceInfo( 
        audio_engine.out_params.device )->defaultLowOutputLatency;
    
    if (restart_audio_io() < 0)
        return -1;

    return 0;
}

