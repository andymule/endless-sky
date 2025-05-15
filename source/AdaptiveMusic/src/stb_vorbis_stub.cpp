#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Define implementation
#define STB_VORBIS_NO_STDIO 1
#define STB_VORBIS_NO_PUSHDATA_API 1
#include "../../../extern/soloud/src/audiosource/wav/stb_vorbis.c"

#define MAX_CHANNELS 2

// Dummy implementation to satisfy the linker
typedef struct {
    int channels;
    int sample_rate;
    int setup_memory_required;
    int setup_temp_memory_required;
    int temp_memory_required;
} stb_vorbis_info;

typedef struct {
    unsigned int sample_rate;
    int channels;
    unsigned int setup_memory_required;
    unsigned int setup_temp_memory_required;
    unsigned int temp_memory_required;
    unsigned int max_frame_size;
} stb_vorbis;

// Function stubs to satisfy the linker
void stb_vorbis_close(stb_vorbis* v) {
    if (v) {
        free(v);
    }
}

stb_vorbis* stb_vorbis_open_memory(const unsigned char* data, int len, int* error, void* alloc_buffer) {
    stb_vorbis* result = (stb_vorbis*)malloc(sizeof(stb_vorbis));
    if (!result) {
        if (error) *error = -1;
        return NULL;
    }
    
    result->sample_rate = 44100;
    result->channels = 2;
    
    return result;
}

stb_vorbis* stb_vorbis_open_file(FILE* f, int close_handle_on_close, int* error, const void* alloc_buffer) {
    return stb_vorbis_open_memory(NULL, 0, error, NULL);
}

stb_vorbis_info stb_vorbis_get_info(stb_vorbis* v) {
    stb_vorbis_info info;
    info.channels = v ? v->channels : 2;
    info.sample_rate = v ? v->sample_rate : 44100;
    info.setup_memory_required = 0;
    info.setup_temp_memory_required = 0;
    info.temp_memory_required = 0;
    return info;
}

int stb_vorbis_stream_length_in_samples(stb_vorbis* v) {
    return 0;
}

int stb_vorbis_get_frame_float(stb_vorbis* v, int* channels, float*** output) {
    static float buffer[MAX_CHANNELS][1024];
    static float* buffers[MAX_CHANNELS];
    
    if (!v) return 0;
    
    for (int i = 0; i < v->channels; ++i) {
        buffers[i] = buffer[i];
        for (int j = 0; j < 1024; ++j) {
            buffer[i][j] = 0.0f;
        }
    }
    
    if (channels) *channels = v->channels;
    if (output) *output = buffers;
    
    return 0;
}

int stb_vorbis_seek(stb_vorbis* v, unsigned int sample_position) {
    return 0;
}

int stb_vorbis_seek_start(stb_vorbis* v) {
    return 0;
}

unsigned int stb_vorbis_get_sample_offset(stb_vorbis* v) {
    return 0;
} 