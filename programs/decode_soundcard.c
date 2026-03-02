#include <string.h>
#include <signal.h>

#include "quiet-portaudio.h"

static quiet_portaudio_decoder *decoder = NULL;
static void sig_handler(int signal) {
    if (decoder) {
        quiet_portaudio_decoder_close(decoder);
    }
}

int decode_from_soundcard(FILE *output, quiet_decoder_options *opt) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("failed to initialize port audio, %s\n", Pa_GetErrorText(err));
        return 1;
    }

    PaDeviceIndex device = Pa_GetDefaultInputDevice();
    if (device == paNoDevice) {
        printf("no default input device found\n");
        return 1;
    }
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(device);
    if (!deviceInfo) {
        printf("failed to get device info\n");
        return 1;
    }
    double sample_rate = deviceInfo->defaultSampleRate;
    PaTime latency = deviceInfo->defaultLowInputLatency;

    decoder = quiet_portaudio_decoder_create(opt, device, latency, sample_rate);
    if (!decoder) {
        return 1;
    }
    quiet_portaudio_decoder_set_blocking(decoder, 0, 0);

    size_t write_buffer_size = 16384;
    uint8_t *write_buffer = malloc(write_buffer_size*sizeof(uint8_t));

    while (true) {
        ssize_t read = quiet_portaudio_decoder_recv(decoder, write_buffer, write_buffer_size);
        if (read <= 0) {
            break;
        }
        fwrite(write_buffer, 1, read, output);
    }

    free(write_buffer);
    quiet_portaudio_decoder_destroy(decoder);

    return 0;
}

int main(int argc, char **argv) {
    const char *profilename = NULL;
    const char *output_dest = NULL;
    const char *conf_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--conf") == 0) {
            if (i + 1 >= argc) {
                printf("--conf requires an argument\n");
                exit(1);
            }
            conf_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: %s <profilename> [<output_destination>] [--conf <path>]\n", argv[0]);
            exit(0);
        } else if (!profilename) {
            profilename = argv[i];
        } else if (!output_dest) {
            output_dest = argv[i];
        }
    }

    if (!profilename) {
        printf("usage: %s <profilename> [<output_destination>] [--conf <path>]\n", argv[0]);
        exit(1);
    }

    quiet_decoder_options *decodeopt =
        quiet_decoder_profile_filename(conf_path, profilename);

    if (!decodeopt) {
        printf("failed to read profile %s\n", profilename);
        exit(1);
    }

    FILE *output;
    if (!output_dest || strncmp(output_dest, "-", 2) == 0) {
        output = stdout;
        setvbuf(stdout, NULL, _IONBF, 0);  // in order to get interactive let's make stdout unbuffered
    } else {
        output = fopen(output_dest, "wb");
        if (!output) {
            fprintf(stderr, "failed to open %s: ", output_dest);
            perror(NULL);
            exit(1);
        }
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int code = decode_from_soundcard(output, decodeopt);

    fclose(output);
    free(decodeopt);

    Pa_Terminate();

    return code;
}
