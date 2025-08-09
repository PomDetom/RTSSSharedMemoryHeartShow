#pragma once
#include <atomic>
#include <thread>
#include <string>

class BluetoothHeartRate
{
public:
    BluetoothHeartRate();
    ~BluetoothHeartRate();
    bool Start();
    void Stop();
    int GetLatestHeartRate() const;

private:
    void PollHeartRate();
    std::atomic<int> m_latestHeartRate;
    std::thread m_worker;
    bool m_running;
};