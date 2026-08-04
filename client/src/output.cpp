#include "output.hpp"

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CHANNELS = 1;
static constexpr int FRAME_SAMPLES = 480;
static constexpr int FRAME_BYTES = FRAME_SAMPLES * sizeof(int16_t);

static ma_device device;

static mutex queueMutex;

static queue<vector<uint8_t>> audioQueue;

static bool initialized = false;

static void playbackCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    (void)pDevice;
    (void)pInput;

    int16_t* out = (int16_t*)pOutput;

    memset(out, 0, frameCount * sizeof(int16_t) * CHANNELS);

    lock_guard<mutex> lock(queueMutex);

    if (audioQueue.empty()) return;

    vector<uint8_t> frame = move(audioQueue.front());

    audioQueue.pop();

    int samplesToCopy = min((int)(frame.size() / sizeof(int16_t)), (int)frameCount);

    memcpy(out, frame.data(), samplesToCopy * sizeof(int16_t));
}
static bool initOutput()
{
    ma_device_config config;
    config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_s16;
    config.playback.channels = CHANNELS;
    config.sampleRate = SAMPLE_RATE;
    config.dataCallback = playbackCallback;
    config.periodSizeInFrames = FRAME_SAMPLES;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)return false;

    if (ma_device_start(&device) != MA_SUCCESS){
        ma_device_uninit(&device);
        return false;
    }

    initialized = true;

    return true;

}

void processOutput(const string& username, const uint8_t* data, int datalen){
    (void)username;

    if (!initialized){
        if (!initOutput()){
            return;
        }
    }

    if (datalen <= 0) return;

    vector<uint8_t> frame(datalen);

    memcpy(frame.data(),data,datalen);

    lock_guard<mutex> lock(queueMutex);

    audioQueue.push(move(frame));
}
