#pragma once

#include <Windows.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
#include <fstream>
#include <functional>

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();

    HRESULT Initialize();
    HRESULT StartCapture(const std::function<void(short *, uint32_t, uint32_t)> &handler);
    HRESULT StopCapture();
    void SetIsLowLatency(bool isLowLatency);

private:
    HRESULT OnAudioSampleReady(const std::function<void(short *, uint32_t,uint32_t)> &handler);
    bool m_IsInitialized = false;

    WAVEFORMATEX *m_MixFormat = nullptr;
    Microsoft::WRL::ComPtr<IAudioClient3> m_AudioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_AudioCaptureClient;
    HANDLE m_SampleReadyEvent = INVALID_HANDLE_VALUE;
    UINT32 m_BufferFrames = 0;
    UINT32 m_DefaultPeriodInFrames = 0;
    UINT32 m_FundamentalPeriodInFrames = 0;
    UINT32 m_MinPeriodInFrames = 0;
    UINT32 m_MaxPeriodInFrames = 0;
    DWORD m_cbHeaderSize = 0;
    DWORD m_cbDataSize = 0;
    bool m_isLowLatency = false;
};