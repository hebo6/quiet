#include <math.h>
#include <string.h>
#include <unistd.h>

#include "quiet-portaudio.h"

int encode_to_soundcard(FILE *input, quiet_encoder_options *opt) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("failed to initialize port audio, %s\n", Pa_GetErrorText(err));
        return 1;
    }

    PaDeviceIndex device = Pa_GetDefaultOutputDevice();
    if (device == paNoDevice) {
        printf("no default output device found\n");
        return 1;
    }
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(device);
    if (!deviceInfo) {
        printf("failed to get device info\n");
        return 1;
    }
    double sample_rate = deviceInfo->defaultSampleRate;
    PaTime latency = deviceInfo->defaultLowOutputLatency;

    size_t sample_buffer_size = 16384;
    quiet_portaudio_encoder *e = quiet_portaudio_encoder_create(opt, device, latency, sample_rate, sample_buffer_size);
    if (!e) {
        return 1;
    }

    size_t read_buffer_size = 16384;
    uint8_t *read_buffer = malloc(read_buffer_size*sizeof(uint8_t));
    bool done = false;

    while (!done) {
        size_t nread = fread(read_buffer, sizeof(uint8_t), read_buffer_size, input);
        if (nread == 0) {
            break;
        } else if (nread < read_buffer_size) {
            done = true;
        }

        size_t frame_len = quiet_portaudio_encoder_get_frame_len(e);
        for (size_t i = 0; i < nread; i += frame_len) {
            frame_len = (frame_len > (nread - i)) ? (nread - i) : frame_len;
            quiet_portaudio_encoder_send(e, read_buffer + i, frame_len);
        }
    }

    quiet_portaudio_encoder_close(e);

    while (quiet_portaudio_encoder_emit(e) > 0) {
    }

    free(read_buffer);

    quiet_portaudio_encoder_destroy(e);

    return 0;
}

int main(int argc, char **argv) {
    const char *profilename = NULL;
    const char *input_source = NULL;
    const char *conf_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--conf") == 0) {
            if (i + 1 >= argc) {
                printf("--conf requires an argument\n");
                exit(1);
            }
            conf_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s <profilename> [<input_source>] [--conf <path>]\n", argv[0]);
            exit(0);
        } else if (!profilename) {
            profilename = argv[i];
        } else if (!input_source) {
            input_source = argv[i];
        }
    }

    if (!profilename) {
        printf("usage: %s <profilename> [<input_source>] [--conf <path>]\n", argv[0]);
        exit(1);
    }

    quiet_encoder_options *encodeopt =
        quiet_encoder_profile_filename(conf_path, profilename);

    if (!encodeopt) {
        printf("failed to read profile %s\n", profilename);
        exit(1);
    }

    FILE *input;
    if (!input_source || strncmp(input_source, "-", 2) == 0) {
        input = stdin;
    } else {
        input = fopen(input_source, "rb");
        if (!input) {
            fprintf(stderr, "failed to open %s: ", input_source);
            perror(NULL);
            exit(1);
        }
    }

    int code = encode_to_soundcard(input, encodeopt);

    fclose(input);
    free(encodeopt);

    Pa_Terminate();

    return code;
}
