#include <math.h>
#include <string.h>

#include "quiet.h"

#include <sndfile.h>

float freq2rad(float freq) { return freq * 2 * M_PI; }

const int sample_rate = 44100;

float normalize_freq(float freq, float sample_rate) {
    return freq2rad(freq / (float)(sample_rate));
}

static SNDFILE *open_wav_for_write(const char *fname, float sample_rate) {
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = sample_rate;
    sfinfo.channels = 1;
    sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_FLOAT);

    return sf_open(fname, SFM_WRITE, &sfinfo);
}

static size_t write_wav_samples(SNDFILE *wav, const quiet_sample_t *samples, size_t sample_len) {
    return sf_write_float(wav, samples, sample_len);
}

static void close_wav(SNDFILE *wav) { sf_close(wav); }

int encode_to_wav(FILE *payload, const char *out_fname,
                  const quiet_encoder_options *opt) {
    SNDFILE *wav = open_wav_for_write(out_fname, sample_rate);

    if (wav == NULL) {
        fprintf(stderr, "failed to open wav file for writing: %s\n",
                sf_strerror(NULL));
        return 1;
    }

    quiet_encoder *e = quiet_encoder_create(opt, sample_rate);

    size_t block_len = 16384;
    uint8_t *readbuf = malloc(block_len * sizeof(uint8_t));
    size_t samplebuf_len = 16384;
    quiet_sample_t *samplebuf = malloc(samplebuf_len * sizeof(quiet_sample_t));
    bool done = false;
    if (readbuf == NULL) {
        return 1;
    }
    if (samplebuf == NULL) {
        return 1;
    }

    while (!done) {
        size_t nread = fread(readbuf, sizeof(uint8_t), block_len, payload);
        if (nread == 0) {
            break;
        } else if (nread < block_len) {
            done = true;
        }

        size_t frame_len = quiet_encoder_get_frame_len(e);
        for (size_t i = 0; i < nread; i += frame_len) {
            frame_len = (frame_len > (nread - i)) ? (nread - i) : frame_len;
            quiet_encoder_send(e, readbuf + i, frame_len);
        }
    }

    quiet_encoder_close(e);

    ssize_t written = samplebuf_len;
    while (written == samplebuf_len) {
        written = quiet_encoder_emit(e, samplebuf, samplebuf_len);
        if (written > 0) {
            write_wav_samples(wav, samplebuf, written);
        }
    }

    quiet_encoder_destroy(e);
    free(readbuf);
    free(samplebuf);
    close_wav(wav);
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

    quiet_encoder_options *encodeopt =
        quiet_encoder_profile_filename(conf_path, profilename);

    if (!encodeopt) {
        printf("failed to read profile %s\n", profilename);
        exit(1);
    }

    encode_to_wav(input, "encoded.wav", encodeopt);

    fclose(input);
    free(encodeopt);

    return 0;
}
