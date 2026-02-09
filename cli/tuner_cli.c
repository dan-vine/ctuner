//
//  tuner_cli.c
//  Command-line pitch detector for comparison testing
//
//  Uses the same FFT + phase vocoder algorithm as the macOS GUI app
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <Accelerate/Accelerate.h>

// Audio processing constants (matching Audio.h)
#define kSampleRate     11025
#define kOversample     16
#define kSamples        16384
#define kLog2Samples    14
#define kSamples2       (kSamples / 2)
#define kMaxima         8
#define kRange          (kSamples * 7 / 16)
#define kStep           (kSamples / kOversample)

// Tuner reference values
#define kA5Reference    440.0
#define kC5Offset       57
#define kAOffset        9
#define kOctave         12
#define kEqual          8

#define kMin            0.5
#define kScale          2048.0

// Note names
const char *noteNames[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

// Equal temperament ratios
double equalTemperament[12] = {
    1.000000000, 1.059463094, 1.122462048, 1.189207115,
    1.259921050, 1.334839854, 1.414213562, 1.498307077,
    1.587401052, 1.681792831, 1.781797436, 1.887748625
};

// Maximum structure
typedef struct {
    double f;
    double fr;
    int n;
    double cents;
    char note_name[4];
    int octave;
} Maximum;

// Result structure
typedef struct {
    bool valid;
    double frequency;
    double ref_frequency;
    double cents;
    int note;
    int octave;
    char note_name[4];
    double confidence;
    Maximum maxima[kMaxima];
    int maxima_count;
} PitchResult;

// WAV file header
typedef struct {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} WavHeader;

// Global state for pitch detection
typedef struct {
    double *buffer;
    double *window;
    double *xp;  // Previous phase
    FFTSetupD fft_setup;
    double reference;
    int temper;
    int key;
    double sample_rate;
    double fps;
    double expect;
} PitchDetector;

// Initialize pitch detector
PitchDetector *pitch_detector_create(double sample_rate, double reference) {
    PitchDetector *pd = malloc(sizeof(PitchDetector));

    pd->buffer = calloc(kSamples, sizeof(double));
    pd->window = malloc(kSamples * sizeof(double));
    pd->xp = calloc(kRange, sizeof(double));

    pd->reference = reference;
    pd->temper = kEqual;
    pd->key = 0;
    pd->sample_rate = sample_rate;

    pd->fps = sample_rate / (double)kSamples;
    pd->expect = 2.0 * M_PI * (double)kStep / (double)kSamples;

    // Init Hamming window
    vDSP_hamm_windowD(pd->window, kSamples, 0);

    // Init FFT
    pd->fft_setup = vDSP_create_fftsetupD(kLog2Samples, kFFTRadix2);

    return pd;
}

// Free pitch detector
void pitch_detector_free(PitchDetector *pd) {
    free(pd->buffer);
    free(pd->window);
    free(pd->xp);
    vDSP_destroy_fftsetupD(pd->fft_setup);
    free(pd);
}

// Process audio samples and detect pitch
PitchResult pitch_detector_process(PitchDetector *pd, double *samples, int num_samples) {
    PitchResult result = {0};

    // Shift buffer and add new samples
    int shift = (num_samples < kSamples) ? num_samples : kSamples;
    memmove(pd->buffer, pd->buffer + shift, (kSamples - shift) * sizeof(double));

    int copy_start = (num_samples > kSamples) ? num_samples - kSamples : 0;
    int copy_len = (num_samples > kSamples) ? kSamples : num_samples;
    memcpy(pd->buffer + kSamples - copy_len, samples + copy_start, copy_len * sizeof(double));

    // Arrays for processing
    double xa[kRange];
    double xq[kRange];
    double xf[kRange];
    double dxa[kRange];
    double dxp[kRange];

    double input[kSamples];
    double re[kSamples2];
    double im[kSamples2];
    DSPDoubleSplitComplex x = {re, im};

    Maximum maxima[kMaxima];

    // Maximum data value
    double dmax;
    vDSP_maxmgvD(pd->buffer, 1, &dmax, kSamples);

    if (dmax < 0.125)
        dmax = 0.125;

    // Normalize
    double norm = dmax;
    vDSP_vsdivD(pd->buffer, 1, &norm, input, 1, kSamples);

    // Apply window
    vDSP_vmulD(input, 1, pd->window, 1, input, 1, kSamples);

    // Copy to split complex
    vDSP_ctozD((DSPDoubleComplex *)input, 2, &x, 1, kSamples2);

    // FFT
    vDSP_fft_zripD(pd->fft_setup, &x, 1, kLog2Samples, kFFTDirection_Forward);

    // Zero DC
    x.realp[0] = 0.0;
    x.imagp[0] = 0.0;

    // Scale
    double scale = kScale;
    vDSP_vsdivD(x.realp, 1, &scale, x.realp, 1, kSamples2);
    vDSP_vsdivD(x.imagp, 1, &scale, x.imagp, 1, kSamples2);

    // Magnitude
    vDSP_vdistD(x.realp, 1, x.imagp, 1, xa, 1, kRange);

    // Phase
    vDSP_zvphasD(&x, 1, xq, 1, kRange);

    // Phase difference
    vDSP_vsubD(pd->xp, 1, xq, 1, dxp, 1, kRange);

    // Calculate frequencies using phase vocoder
    for (int i = 1; i < kRange; i++) {
        double dp = dxp[i];
        dp -= (double)i * pd->expect;

        int qpd = dp / M_PI;
        if (qpd >= 0)
            qpd += qpd & 1;
        else
            qpd -= qpd & 1;

        dp -= M_PI * (double)qpd;

        double df = kOversample * dp / (2.0 * M_PI);
        xf[i] = i * pd->fps + df * pd->fps;

        dxa[i] = xa[i] - xa[i - 1];
    }

    // Save phase for next iteration
    memcpy(pd->xp, xq, kRange * sizeof(double));

    // Find maximum
    double max;
    vDSP_Length imax;
    vDSP_maxmgviD(xa, 1, &max, &imax, kRange);

    double f = xf[imax];

    int count = 0;
    int limit = kRange - 1;

    // Find maxima
    for (int i = 1; i < limit && count < kMaxima; i++) {
        double cf = -12.0 * log2(pd->reference / xf[i]);
        int note = round(cf) + kC5Offset;

        if (note < 0)
            continue;

        if (xa[i] > kMin && xa[i] > (max / 4) &&
            dxa[i] > 0.0 && dxa[i + 1] < 0.0) {

            maxima[count].f = xf[i];
            maxima[count].n = note;

            int n = (note - pd->key + kOctave) % kOctave;
            int a = (kAOffset - pd->key + kOctave) % kOctave;

            double temperRatio = equalTemperament[n] / equalTemperament[a];
            double equalRatio = equalTemperament[n] / equalTemperament[a];
            double temperAdjust = temperRatio / equalRatio;

            maxima[count].fr = pd->reference * pow(2.0, round(cf) / 12.0) * temperAdjust;

            // Calculate cents for this maximum
            maxima[count].cents = -12.0 * log2(maxima[count].fr / maxima[count].f) * 100.0;
            maxima[count].octave = note / kOctave;
            strncpy(maxima[count].note_name, noteNames[note % kOctave], 3);

            if (limit > i * 2)
                limit = i * 2 - 1;

            count++;
        }
    }

    // Copy maxima to result
    result.maxima_count = count;
    for (int i = 0; i < count; i++) {
        result.maxima[i] = maxima[i];
    }

    // Calculate final result
    if (max > kMin && count > 0) {
        f = maxima[0].f;

        double cf = -12.0 * log2(pd->reference / f);

        if (!isnan(cf)) {
            int note = round(cf) + kC5Offset;

            if (note >= 0) {
                int n = (note - pd->key + kOctave) % kOctave;
                int a = (kAOffset - pd->key + kOctave) % kOctave;

                double temperRatio = equalTemperament[n] / equalTemperament[a];
                double equalRatio = equalTemperament[n] / equalTemperament[a];
                double temperAdjust = temperRatio / equalRatio;

                double fr = pd->reference * pow(2.0, round(cf) / 12.0) * temperAdjust;

                // Find nearest maximum to reference
                double df_min = 1000.0;
                for (int i = 0; i < count; i++) {
                    if (fabs(maxima[i].f - fr) < df_min) {
                        df_min = fabs(maxima[i].f - fr);
                        f = maxima[i].f;
                    }
                }

                // Cents relative to reference
                double c = -12.0 * log2(fr / f);

                if (!isnan(c) && fabs(c) <= 0.6) {
                    result.valid = true;
                    result.frequency = f;
                    result.ref_frequency = fr;
                    result.cents = c * 100.0;  // Convert to cents
                    result.note = note;
                    result.octave = note / kOctave;
                    strncpy(result.note_name, noteNames[note % kOctave], 3);
                    result.confidence = max;
                }
            }
        }
    }

    return result;
}

// Read WAV file
double *read_wav(const char *filename, int *num_samples, int *sample_rate) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return NULL;
    }

    WavHeader header;

    // Read RIFF header
    fread(header.riff, 4, 1, fp);
    fread(&header.file_size, 4, 1, fp);
    fread(header.wave, 4, 1, fp);

    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
        fprintf(stderr, "Error: Not a valid WAV file\n");
        fclose(fp);
        return NULL;
    }

    // Find fmt chunk
    char chunk_id[4];
    uint32_t chunk_size;

    while (fread(chunk_id, 4, 1, fp) == 1) {
        fread(&chunk_size, 4, 1, fp);

        if (strncmp(chunk_id, "fmt ", 4) == 0) {
            fread(&header.audio_format, 2, 1, fp);
            fread(&header.num_channels, 2, 1, fp);
            fread(&header.sample_rate, 4, 1, fp);
            fread(&header.byte_rate, 4, 1, fp);
            fread(&header.block_align, 2, 1, fp);
            fread(&header.bits_per_sample, 2, 1, fp);

            // Skip any extra format bytes
            if (chunk_size > 16) {
                fseek(fp, chunk_size - 16, SEEK_CUR);
            }
        } else if (strncmp(chunk_id, "data", 4) == 0) {
            header.data_size = chunk_size;
            break;
        } else {
            fseek(fp, chunk_size, SEEK_CUR);
        }
    }

    *sample_rate = header.sample_rate;
    *num_samples = header.data_size / (header.bits_per_sample / 8) / header.num_channels;

    double *samples = malloc(*num_samples * sizeof(double));

    if (header.bits_per_sample == 16) {
        int16_t *raw = malloc(header.data_size);
        fread(raw, header.data_size, 1, fp);

        for (int i = 0; i < *num_samples; i++) {
            // Take first channel if stereo
            samples[i] = raw[i * header.num_channels] / 32768.0;
        }
        free(raw);
    } else if (header.bits_per_sample == 32 && header.audio_format == 3) {
        // 32-bit float
        float *raw = malloc(header.data_size);
        fread(raw, header.data_size, 1, fp);

        for (int i = 0; i < *num_samples; i++) {
            samples[i] = raw[i * header.num_channels];
        }
        free(raw);
    } else {
        fprintf(stderr, "Error: Unsupported WAV format (bits=%d, format=%d)\n",
                header.bits_per_sample, header.audio_format);
        free(samples);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return samples;
}

// Process WAV file and output JSON
void process_file(const char *filename, double reference) {
    int num_samples, file_sample_rate;
    double *samples = read_wav(filename, &num_samples, &file_sample_rate);

    if (!samples) {
        printf("{\"valid\": false, \"error\": \"Failed to read file\"}\n");
        return;
    }

    // Resample if needed (simple decimation for now)
    double *resampled = samples;
    int resampled_count = num_samples;
    int target_rate = kSampleRate;

    if (file_sample_rate != target_rate) {
        int ratio = file_sample_rate / target_rate;
        if (ratio > 0 && file_sample_rate == ratio * target_rate) {
            resampled_count = num_samples / ratio;
            resampled = malloc(resampled_count * sizeof(double));
            for (int i = 0; i < resampled_count; i++) {
                resampled[i] = samples[i * ratio];
            }
        } else {
            // Simple resampling for non-integer ratios
            double ratio_d = (double)file_sample_rate / target_rate;
            resampled_count = (int)(num_samples / ratio_d);
            resampled = malloc(resampled_count * sizeof(double));
            for (int i = 0; i < resampled_count; i++) {
                int src_idx = (int)(i * ratio_d);
                if (src_idx < num_samples)
                    resampled[i] = samples[src_idx];
            }
        }
    }

    PitchDetector *pd = pitch_detector_create(target_rate, reference);

    // Process in chunks and collect results
    int chunk_size = kStep;
    int num_chunks = resampled_count / chunk_size;

    // Track frequencies by cluster (not by note) to detect detuned reeds
    // Use frequency bins of ~1 Hz resolution for clustering
    #define MAX_FREQ_CLUSTERS 64
    #define CLUSTER_THRESHOLD 1.5  // Hz - frequencies within this are same cluster

    typedef struct {
        double freq_sum;
        double cents_sum;
        int count;
        int note;
        char note_name[4];
        int octave;
    } FreqCluster;

    FreqCluster clusters[MAX_FREQ_CLUSTERS] = {0};
    int num_clusters = 0;

    int valid_frames = 0;
    PitchResult last_result = {0};

    for (int i = 0; i < num_chunks; i++) {
        PitchResult r = pitch_detector_process(pd, resampled + i * chunk_size, chunk_size);

        if (r.valid) {
            valid_frames++;
            last_result = r;

            // Add each detected frequency to appropriate cluster
            for (int m = 0; m < r.maxima_count; m++) {
                double freq = r.maxima[m].f;
                int found_cluster = -1;

                // Find existing cluster within threshold
                for (int c = 0; c < num_clusters; c++) {
                    double cluster_freq = clusters[c].freq_sum / clusters[c].count;
                    if (fabs(freq - cluster_freq) < CLUSTER_THRESHOLD) {
                        found_cluster = c;
                        break;
                    }
                }

                if (found_cluster >= 0) {
                    // Add to existing cluster
                    clusters[found_cluster].freq_sum += freq;
                    clusters[found_cluster].cents_sum += r.maxima[m].cents;
                    clusters[found_cluster].count++;
                } else if (num_clusters < MAX_FREQ_CLUSTERS) {
                    // Create new cluster
                    clusters[num_clusters].freq_sum = freq;
                    clusters[num_clusters].cents_sum = r.maxima[m].cents;
                    clusters[num_clusters].count = 1;
                    clusters[num_clusters].note = r.maxima[m].n;
                    clusters[num_clusters].octave = r.maxima[m].octave;
                    strncpy(clusters[num_clusters].note_name, r.maxima[m].note_name, 3);
                    num_clusters++;
                }
            }
        }
    }

    // Collect clusters that were detected in multiple frames
    Maximum detected_notes[kMaxima];
    int detected_count = 0;

    for (int c = 0; c < num_clusters && detected_count < kMaxima; c++) {
        if (clusters[c].count >= valid_frames / 4) {  // Detected in at least 1/4 of frames
            double avg_freq = clusters[c].freq_sum / clusters[c].count;

            // Recalculate note info from average frequency
            double cf = -12.0 * log2(reference / avg_freq);
            int note = (int)round(cf) + kC5Offset;

            detected_notes[detected_count].f = avg_freq;
            detected_notes[detected_count].cents = clusters[c].cents_sum / clusters[c].count;
            detected_notes[detected_count].n = note;
            detected_notes[detected_count].octave = note / kOctave;
            strncpy(detected_notes[detected_count].note_name, noteNames[note % kOctave], 3);
            detected_count++;
        }
    }

    // Sort by frequency
    for (int i = 0; i < detected_count - 1; i++) {
        for (int j = i + 1; j < detected_count; j++) {
            if (detected_notes[j].f < detected_notes[i].f) {
                Maximum tmp = detected_notes[i];
                detected_notes[i] = detected_notes[j];
                detected_notes[j] = tmp;
            }
        }
    }

    // Filter out octave harmonics (frequencies that are ~2x a lower frequency)
    Maximum filtered_notes[kMaxima];
    int filtered_count = 0;
    for (int i = 0; i < detected_count; i++) {
        bool is_harmonic = false;
        for (int j = 0; j < filtered_count; j++) {
            double ratio = detected_notes[i].f / filtered_notes[j].f;
            // Check if ratio is close to 2 (octave) or 3 (fifth+octave)
            if (fabs(ratio - 2.0) < 0.05 || fabs(ratio - 3.0) < 0.05) {
                is_harmonic = true;
                break;
            }
        }
        if (!is_harmonic && filtered_count < kMaxima) {
            filtered_notes[filtered_count++] = detected_notes[i];
        }
    }
    // Use filtered results
    detected_count = filtered_count;
    for (int i = 0; i < detected_count; i++) {
        detected_notes[i] = filtered_notes[i];
    }

    // Output JSON
    if (detected_count > 0) {
        printf("{\n");
        printf("  \"valid\": true,\n");
        printf("  \"num_notes\": %d,\n", detected_count);
        printf("  \"notes\": [\n");

        for (int i = 0; i < detected_count; i++) {
            printf("    {\n");
            printf("      \"note_name\": \"%s\",\n", detected_notes[i].note_name);
            printf("      \"octave\": %d,\n", detected_notes[i].octave);
            printf("      \"frequency\": %.2f,\n", detected_notes[i].f);
            printf("      \"cents\": %.2f\n", detected_notes[i].cents);
            printf("    }%s\n", (i < detected_count - 1) ? "," : "");
        }

        printf("  ],\n");
        printf("  \"primary_note\": \"%s\",\n", detected_notes[0].note_name);
        printf("  \"primary_octave\": %d,\n", detected_notes[0].octave);
        printf("  \"primary_frequency\": %.2f,\n", detected_notes[0].f);
        printf("  \"primary_cents\": %.2f,\n", detected_notes[0].cents);
        printf("  \"num_valid_frames\": %d\n", valid_frames);
        printf("}\n");
    } else {
        printf("{\"valid\": false, \"error\": \"No pitch detected\"}\n");
    }
    pitch_detector_free(pd);

    if (resampled != samples)
        free(resampled);
    free(samples);
}

// Print usage
void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <wav_file> [wav_file2 ...]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -r <freq>    Reference frequency (default: 440.0)\n");
    fprintf(stderr, "  -a           Process all test files and output combined JSON\n");
    fprintf(stderr, "  -h           Show this help\n");
    fprintf(stderr, "\nOutput:\n");
    fprintf(stderr, "  JSON with detected pitch information\n");
}

int main(int argc, char *argv[]) {
    double reference = 440.0;
    bool all_mode = false;
    int file_start = 1;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            reference = atof(argv[++i]);
            file_start = i + 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            all_mode = true;
            file_start = i + 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            file_start = i;
            break;
        }
    }

    if (file_start >= argc) {
        usage(argv[0]);
        return 1;
    }

    if (all_mode && argc - file_start > 1) {
        // Process multiple files, output as JSON object
        printf("{\n");
        for (int i = file_start; i < argc; i++) {
            // Extract base name without path and extension
            char *base = strrchr(argv[i], '/');
            base = base ? base + 1 : argv[i];
            char name[256];
            strncpy(name, base, 255);
            char *dot = strrchr(name, '.');
            if (dot) *dot = '\0';

            printf("  \"%s\": ", name);

            // Capture output (redirect to string would be cleaner but this works)
            process_file(argv[i], reference);

            if (i < argc - 1)
                printf(",");
            printf("\n");
        }
        printf("}\n");
    } else {
        // Single file mode
        for (int i = file_start; i < argc; i++) {
            if (argc - file_start > 1) {
                printf("=== %s ===\n", argv[i]);
            }
            process_file(argv[i], reference);
        }
    }

    return 0;
}
