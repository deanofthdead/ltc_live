#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <portaudio.h>
#include <ltc.h>

// Global settings
float gain = 1.0f;
int show_rms = 0;
int auto_gain = 0;

typedef struct {
    LTCDecoder *decoder;
} paUserData;

static int paCallback(const void *inputBuffer, void *outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void *userData)
{
    paUserData *data = (paUserData *)userData;
    const short *in = (const short*)inputBuffer;
    char buf[64];
    LTCFrameExt frame;
    SMPTETimecode stime;
    double sum = 0.0;

    if (!inputBuffer) return paContinue;

    // Process samples and calculate RMS
    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        double sample = (in[i] / 32768.0) * gain;  // apply gain
        sum += sample * sample;

        ltcsnd_sample_t ltcSample = (ltcsnd_sample_t)((in[i] >> 8) & 0xFF);
        ltc_decoder_write(data->decoder, &ltcSample, 1, 0);
    }

    double rms = sqrt(sum / framesPerBuffer);

    // Auto-gain adjustment
    if (auto_gain) {
        if (rms < 0.2) gain *= 1.05;       // boost slightly
        else if (rms > 0.4) gain *= 0.95;  // reduce slightly

        if (gain > 100) gain = 100;        // clamp gain
        if (gain < 0.01) gain = 0.01;
    }

    if (show_rms) {
        printf("[RMS: %.3f | Gain: %.2f] ", rms, gain);
        fflush(stdout);
    }

    while (ltc_decoder_read(data->decoder, &frame)) {
        ltc_frame_to_time(&stime, &frame.ltc, 1);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d",
                 stime.hours, stime.mins, stime.secs, stime.frame);
        printf("LTC: %s\n", buf);
        fflush(stdout);
    }

    return paContinue;
}

static void listDevices() {
    Pa_Initialize();
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        printf("ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices);
        return;
    }
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        printf("Device %d: %s\n", i, info->name);
    }
    Pa_Terminate();
}

int main(int argc, char *argv[]) {
    PaError err;
    PaStream *stream;
    PaStreamParameters inputParameters;
    paUserData data;
    int deviceIndex = -1;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) {
            listDevices();
            return 0;
        }
        else if (strcmp(argv[i], "--device") == 0 && i + 1 < argc) {
            deviceIndex = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--gain") == 0 && i + 1 < argc) {
            gain = atof(argv[++i]);
            if (gain <= 0) gain = 1.0f;
        }
        else if (strcmp(argv[i], "--auto-gain") == 0) {
            auto_gain = 1;
        }
        else if (strcmp(argv[i], "--show-rms") == 0) {
            show_rms = 1;
        }
        else {
            printf("Usage: %s [--list] [--device N] [--gain X] [--auto-gain] [--show-rms]\n", argv[0]);
            return 1;
        }
    }

    if (deviceIndex < 0) {
        printf("No device specified. Use --list to see available devices.\n");
        return 1;
    }

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    if (!deviceInfo) {
        printf("Invalid device index: %d\n", deviceIndex);
        goto error;
    }
    printf("Using device %d: %s\n", deviceIndex, deviceInfo->name);

    inputParameters.device = deviceIndex;
    inputParameters.channelCount = 1;         // Mono
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    // Create LTC Decoder
    data.decoder = ltc_decoder_create(1920, 32);
    if (!data.decoder) {
        printf("Failed to create LTC decoder\n");
        goto error;
    }

    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,                  // no output
        48000,
        1920,
        paClipOff,
        paCallback,
        &data
    );
    if (err != paNoError) goto error;

    printf("Listening for LTC (gain=%.2f, auto-gain=%s, RMS=%s)...\n", gain,
           auto_gain ? "ON" : "OFF",
           show_rms ? "ON" : "OFF");

    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;

    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(100);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    ltc_decoder_free(data.decoder);
    return 0;

error:
    fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
    if (data.decoder) ltc_decoder_free(data.decoder);
    Pa_Terminate();
    return 1;
}
