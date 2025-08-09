#include "stdafx.h"
#include "BluetoothHeartRate.h"
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <cstring>

#pragma comment(lib, "winhttp.lib")

BluetoothHeartRate::BluetoothHeartRate() : m_latestHeartRate(-1), m_running(false) {}
BluetoothHeartRate::~BluetoothHeartRate() { Stop(); }

bool BluetoothHeartRate::Start()
{
    if (m_running) return false;
    m_running = true;
    m_worker = std::thread(&BluetoothHeartRate::PollHeartRate, this);
    return true;
}

void BluetoothHeartRate::Stop()
{
    m_running = false;
    if (m_worker.joinable())
        m_worker.join();
}

int BluetoothHeartRate::GetLatestHeartRate() const
{
    return m_latestHeartRate.load();
}

// 通过 HTTP 轮询获取心率数据
void BluetoothHeartRate::PollHeartRate()
{
    while (m_running)
    {
        int heartRate = -1;
        HINTERNET hSession = WinHttpOpen(L"HeartRateClient/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0);
        if (hSession)
        {
            HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 3030, 0);
            if (hConnect)
            {
                HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/heartrate", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
                if (hRequest)
                {
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                        WinHttpReceiveResponse(hRequest, NULL))
                    {
                        DWORD dwSize = 0;
                        if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0)
                        {
                            char* buffer = new char[dwSize + 1];
                            DWORD dwDownloaded = 0;
                            if (WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded))
                            {
                                buffer[dwDownloaded] = 0;
                                try {
                                    heartRate = std::stoi(buffer);
                                } catch (...) {
                                    heartRate = -1;
                                }
                            }
                            delete[] buffer;
                        }
                    }
                    WinHttpCloseHandle(hRequest);
                }
                WinHttpCloseHandle(hConnect);
            }
            WinHttpCloseHandle(hSession);
        }

        if (heartRate > 0)
        {
            m_latestHeartRate = heartRate;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 刷新间隔
    }
}