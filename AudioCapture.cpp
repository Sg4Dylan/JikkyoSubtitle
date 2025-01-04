#include "AudioCapture.h"
#include <Functiondiscoverykeys.h>
#include <Mmdeviceapi.h>// For IMMDeviceEnumerator and related
#include <combaseapi.h>
#include <iostream>
#include <mfapi.h>
#include <stdexcept>
#include <string>

// Helper function to throw HRESULT errors as exceptions
void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("HRESULT failed: " + std::to_string(hr));
    }
}

AudioCapture::AudioCapture() {
    m_SampleReadyEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (m_SampleReadyEvent == INVALID_HANDLE_VALUE) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

AudioCapture::~AudioCapture() {
    if (m_SampleReadyEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_SampleReadyEvent);
        m_SampleReadyEvent = INVALID_HANDLE_VALUE;
    }
    CoTaskMemFree(m_MixFormat);
}

HRESULT AudioCapture::Initialize() {
    if (m_IsInitialized) {
        return S_OK;
    }
    HRESULT hr = S_OK;

    // Get the default audio render device
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> deviceEnumerator;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          &deviceEnumerator);
    ThrowIfFailed(hr);

    Microsoft::WRL::ComPtr<IMMDevice> audioDevice;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, static_cast<ERole>(eConsole), &audioDevice);
    if (FAILED(hr)) {
        // Try getting the default communication device
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, static_cast<ERole>(eCommunications), &audioDevice);
        if (FAILED(hr)) {
            // If still faild use all
            hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, static_cast<ERole>(eAll), &audioDevice);
            if (FAILED(hr)) {
                throw std::runtime_error("Failed to get default audio render device");
            }
        }
    }

    ThrowIfFailed(hr);

    // Activate the audio client
    hr = audioDevice->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr, &m_AudioClient);
    ThrowIfFailed(hr);

    // Get the mix format
    hr = m_AudioClient->GetMixFormat(&m_MixFormat);
    ThrowIfFailed(hr);

    // convert from Float to 16-bit PCM
    switch (m_MixFormat->wFormatTag) {
        case WAVE_FORMAT_PCM:
            // nothing to do
            break;

        case WAVE_FORMAT_IEEE_FLOAT:
            m_MixFormat->wFormatTag = WAVE_FORMAT_PCM;
            m_MixFormat->wBitsPerSample = 16;
            m_MixFormat->nBlockAlign = m_MixFormat->nChannels * m_MixFormat->wBitsPerSample / 8;
            m_MixFormat->nAvgBytesPerSec = m_MixFormat->nSamplesPerSec * m_MixFormat->nBlockAlign;
            break;

        case WAVE_FORMAT_EXTENSIBLE: {
            WAVEFORMATEXTENSIBLE *pWaveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(m_MixFormat);
            if (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
                // nothing to do
            } else if (pWaveFormatExtensible->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
                pWaveFormatExtensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
                pWaveFormatExtensible->Format.wBitsPerSample = 16;
                pWaveFormatExtensible->Format.nBlockAlign =
                        pWaveFormatExtensible->Format.nChannels *
                        pWaveFormatExtensible->Format.wBitsPerSample /
                        8;
                pWaveFormatExtensible->Format.nAvgBytesPerSec =
                        pWaveFormatExtensible->Format.nSamplesPerSec *
                        pWaveFormatExtensible->Format.nBlockAlign;
                pWaveFormatExtensible->Samples.wValidBitsPerSample =
                        pWaveFormatExtensible->Format.wBitsPerSample;

                // leave the channel mask as-is
            } else {
                // we can only handle float or PCM
                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                ThrowIfFailed(hr);
            }
            break;
        }

        default:
            // we can only handle float or PCM
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            ThrowIfFailed(hr);
            break;
    }

    // Get engine period
    hr = m_AudioClient->GetSharedModeEnginePeriod(m_MixFormat, &m_DefaultPeriodInFrames, &m_FundamentalPeriodInFrames,
                                                  &m_MinPeriodInFrames, &m_MaxPeriodInFrames);
    ThrowIfFailed(hr);

    // Initialize the AudioClient in Shared Mode with the user specified buffer
    if (!m_isLowLatency) {
        hr = m_AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                       200000,
                                       0,
                                       m_MixFormat,
                                       nullptr);
    } else {
        hr = m_AudioClient->InitializeSharedAudioStream(
            AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            m_MinPeriodInFrames,
            m_MixFormat,
            nullptr);
    }

    ThrowIfFailed(hr);

    // Get the maximum size of the AudioClient Buffer
    hr = m_AudioClient->GetBufferSize(&m_BufferFrames);
    ThrowIfFailed(hr);

    // Get the capture client
    hr = m_AudioClient->GetService(__uuidof(IAudioCaptureClient), (void **) &m_AudioCaptureClient);
    ThrowIfFailed(hr);

    hr = m_AudioClient->SetEventHandle(m_SampleReadyEvent);
    ThrowIfFailed(hr);

    m_IsInitialized = true;
    return hr;
}


HRESULT AudioCapture::StartCapture(const std::function<void(short *, uint32_t, uint32_t)> &handler) {
    if (!m_IsInitialized) {
        return E_FAIL;
    }
    HRESULT hr = m_AudioClient->Start();
    if (FAILED(hr)) {
        return hr;
    }

    // Main capture loop
    while (true) {
        WaitForSingleObject(m_SampleReadyEvent, INFINITE);
        hr = OnAudioSampleReady(handler);
        if (FAILED(hr)) {
            break;
        }
    }

    return hr;
}

HRESULT AudioCapture::StopCapture() {
    HRESULT hr = S_OK;
    if (!m_IsInitialized) {
        return E_FAIL;
    }

    hr = m_AudioClient->Stop();
    if (FAILED(hr)) {
        return hr;
    }
    m_IsInitialized = false;

    return hr;
}

HRESULT AudioCapture::OnAudioSampleReady(const std::function<void(short *, uint32_t, uint32_t)> &handler) {
    HRESULT hr = S_OK;
    UINT32 framesAvailable = 0;
    BYTE *data = nullptr;
    DWORD captureFlags;
    UINT64 devicePosition = 0;
    UINT64 qpcPosition = 0;
    DWORD cbBytesToCapture = 0;

    // GetNextPacketSize in loop
    for (
        hr = m_AudioCaptureClient->GetNextPacketSize(&framesAvailable);
        SUCCEEDED(hr) && framesAvailable > 0;
        hr = m_AudioCaptureClient->GetNextPacketSize(&framesAvailable)) {
        cbBytesToCapture = framesAvailable * m_MixFormat->nBlockAlign;
        if ((m_cbDataSize + cbBytesToCapture) < m_cbDataSize) {
            hr = StopCapture();
            if (FAILED(hr)) {
                return hr;
            }
            break;
        }

        hr = m_AudioCaptureClient->GetBuffer(&data, &framesAvailable, &captureFlags, &devicePosition, &qpcPosition);


        if (FAILED(hr)) {
            return hr;
        }

        // if (captureFlags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
        //     std::cout << std::endl;
        // }

        if ((captureFlags & AUDCLNT_BUFFERFLAGS_SILENT) != 0) {
            memset(data, 0, framesAvailable * m_MixFormat->nBlockAlign);
        }

        if (framesAvailable) {
            handler(reinterpret_cast<short *>(data), framesAvailable, m_MixFormat->nSamplesPerSec);
        }

        m_cbDataSize += cbBytesToCapture;
        m_AudioCaptureClient->ReleaseBuffer(framesAvailable);
    }
    return hr;
}

void AudioCapture::SetIsLowLatency(bool isLowLatency) {
    m_isLowLatency = isLowLatency;
}
