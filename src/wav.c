#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "wav.h"
#include "utils.h"

static const char CHUNK_ID[] = { 'R','I','F','F' };
static const char FORMAT[]   = { 'W','A','V','E' };
static const char SUBCHUNK1_ID[] = { 'f','m','t',' ' };
static const uint32_t SUBCHUNK1_SIZE = 16;
static const char SUBCHUNK2_ID[] = { 'd','a','t','a' };

const char MODES[] = "wb";

wav_writer *wav_open(
    const char *path, 
    int audio_format,
    int sample_rate, 
    int channels, 
    int bits_per_sample
)
{   
    wav_writer *w = calloc(1, sizeof(wav_writer));
    if (!w)
        goto criterro;

    w->bits_per_sample = bits_per_sample; 
    w->sample_rate = sample_rate;
    w->num_channels = channels;
    w->file = fopen(path, MODES);
    if (!w->file)
        goto criterro;

    char header[WAV_HEADER_SIZE];
    memset(header, 0, WAV_HEADER_SIZE);    
    char *header_p = header; 

    unsigned char b16[2];
    unsigned char b32[4];

    //ChunkID: RIFF
    memcpy(header_p, CHUNK_ID, sizeof CHUNK_ID);
    header_p += sizeof CHUNK_ID;

    //skip chunk size
    header_p += 4;

    //Format: WAVE
    memcpy(header_p, FORMAT, sizeof FORMAT);
    header_p += sizeof FORMAT;

    //Subchunk1ID: "fmt "
    memcpy(header_p, SUBCHUNK1_ID, sizeof SUBCHUNK1_ID);
    header_p += sizeof SUBCHUNK1_ID;

    //subchunk1 size
    write_u32_le(b32, SUBCHUNK1_SIZE);
    memcpy(header_p, b32, 4);
    header_p += 4;

    //Audio format
    write_u16_le(b16, audio_format);
    memcpy(header_p, b16, 2);
    header_p += 2;

    //Num channels
    write_u16_le(b16, channels);
    memcpy(header_p, b16, 2);
    header_p += 2;

    //Sample rate
    write_u32_le(b32, sample_rate);
    memcpy(header_p, b32, 4);
    header_p += 4;

    // Byte rate
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    write_u32_le(b32, byte_rate);
    memcpy(header_p, b32, 4);
    header_p += 4;
    
    // Block align
    uint16_t block_align = channels * bits_per_sample / 8;
    write_u16_le(b16, block_align);
    memcpy(header_p, b16, 2);
    header_p += 2;

    //Bits per sample
    write_u16_le(b16, bits_per_sample);
    memcpy(header_p, b16, 2);
    header_p += 2;

    //Subchunk2ID: "data"
    memcpy(header_p, SUBCHUNK2_ID, sizeof SUBCHUNK2_ID);
    header_p += sizeof SUBCHUNK2_ID;


    if (fwrite(header, WAV_HEADER_SIZE, 1, w->file) != 1)
        goto criterro;

    return w;

criterro:
    perror("wav_open");
    if (w) {
        if (w->file) fclose(w->file);
        free(w);
    }
    return NULL;
}

size_t wav_write(wav_writer *w, const void *data, size_t bytes) {
    size_t written = fwrite(data, 1, bytes, w->file);
    size_t bytes_per_sample = (size_t)w->bits_per_sample / 8;

    if (bytes_per_sample && w->num_channels > 0)
        w->num_samples += written / (bytes_per_sample * (size_t)w->num_channels);

    return written;
}

int wav_close(wav_writer *w){
    unsigned char b[4];

    //Subchunk2Size: 32 LE. OFF 49
    uint32_t subchunk2size = (uint32_t)(w->num_samples * (size_t)w->num_channels * (size_t)w->bits_per_sample / 8u);
    write_u32_le(b, subchunk2size);

    if (fseek(w->file, 40, SEEK_SET) != 0) 
        return -1;
    if (fwrite(b, 1, 4, w->file) != 4) 
        return -1;
    
    //chunksize: 32 LE. OFF 4
    uint32_t chunksize = 36u + subchunk2size;
    write_u32_le(b, chunksize);

    if (fseek(w->file, 4, SEEK_SET) != 0) 
        return -1;
    if (fwrite(b, 1, 4, w->file) != 4) 
        return -1;

    if (fclose(w->file) != 0) 
        return -1;

    free(w);
    return 0;
}
