#define MINIAUDIO_IMPLEMENTATION
#include "input.hpp"

static constexpr int SAMPLE_RATE = 48000;
static constexpr int CHANNELS = 1;
static constexpr int FRAME_SAMPLES = 480;
static constexpr int FRAME_BYTES = FRAME_SAMPLES * sizeof(int16_t);

static ma_device device;

static mutex mtx;
static condition_variable cv;

static vector<uint8_t> capturedFrame;
static bool frameReady = false;
static bool initialized = false;

static void captureCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    (void)pDevice;
    (void)pOutput;

    if (pInput == nullptr || frameCount == 0) return;

    unique_lock<mutex> lock(mtx);

    capturedFrame.resize(frameCount * sizeof(int16_t) * CHANNELS);

    memcpy(capturedFrame.data(), pInput, capturedFrame.size());

    frameReady = true;

    lock.unlock();
    cv.notify_one();
}

static bool initInput()
{
    ma_device_config config;

    config = ma_device_config_init(ma_device_type_capture);

    config.capture.format = ma_format_s16;
    config.capture.channels = CHANNELS;

    config.sampleRate = SAMPLE_RATE;

    config.dataCallback = captureCallback;

    config.periodSizeInFrames = FRAME_SAMPLES;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)return false;

    if (ma_device_start(&device) != MA_SUCCESS){
        ma_device_uninit(&device);
        return false;
    }

    initialized = true;

    return true;
}

bool takeInput(uint8_t* outbuf, int& outlen){
    if (!initialized){
        if (!initInput()){
            return false;
        }
    }

    unique_lock<mutex> lock(mtx);

    cv.wait(lock, [](){
        return frameReady;
    });

    if (capturedFrame.empty())
        return false;

    memcpy(outbuf, capturedFrame.data(), capturedFrame.size());

    outlen = capturedFrame.size();

    frameReady = false;

    return true;
}

void startMicStream(int sockfd, const sockaddr_in& serveraddr, const string& username, atomic<bool>& running){
    thread micThread([=, &running](){
        while(running){
            uint8_t micBuffer[960];
            int micSize = 0;

            if(!takeInput(micBuffer, micSize)){
                continue;
            }

            vector<uint8_t> sendBuffer;
            sendBuffer.push_back(0);
            sendBuffer.push_back((uint8_t)username.size());
            sendBuffer.insert(sendBuffer.end(), username.begin(), username.end());
            sendBuffer.insert(sendBuffer.end(), micBuffer, micBuffer + micSize);

            sendto(
                sockfd,
                sendBuffer.data(),
                sendBuffer.size(),
                0,
                (sockaddr*)&serveraddr,
                sizeof(serveraddr)
            );
        }
    });

    micThread.detach();
}
