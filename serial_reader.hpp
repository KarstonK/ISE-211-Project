#pragma once

#include <windows.h>

#include <cstdio>
#include <cstdint>
#include <expected>
#include <string>
#include <utility>

using namespace std;

class Reader {
public:
    virtual ~Reader() = default;
    virtual expected<pair<float, float>, string> read() = 0;
};

class FakeReader : public Reader {
public:
    expected<pair<float, float>, string> read() override {
        Sleep(500);
        static float t = -5.0f, h = 15.0f;
        t += 2.0f;
        h += 2.0f;
        if (t > 40.0f) t = -5.0f;
        if (h > 85.0f) h = 15.0f;
        return make_pair(t, h);
    }
};

class SerialReader : public Reader {
private:
    string port_;
    DWORD  baud_;
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    string accumulator_;

public:
    SerialReader(string portName, DWORD baudRate) {
        port_ = move(portName);
        baud_ = baudRate;
    }

    ~SerialReader() {
        if (handle_ == INVALID_HANDLE_VALUE) {
            return;
        }
        CloseHandle(handle_);
    }

    bool reconnect();
    expected<pair<float, float>, string> read() override;
};

bool SerialReader::reconnect() {
    if (handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
    }

    HANDLE h = CreateFileA(port_.c_str(), GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) { CloseHandle(h); return false; }

    dcb.BaudRate    = baud_;
    dcb.ByteSize    = 8;
    dcb.Parity      = NOPARITY;
    dcb.StopBits    = ONESTOPBIT;
    dcb.fBinary     = TRUE;
    dcb.fParity     = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(h, &dcb)) { CloseHandle(h); return false; }

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout        = 50;
    to.ReadTotalTimeoutMultiplier = 10;
    to.ReadTotalTimeoutConstant   = 100;
    if (!SetCommTimeouts(h, &to)) { CloseHandle(h); return false; }

    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    handle_ = h;
    accumulator_.clear();
    return true;
}

expected<pair<float, float>, string> SerialReader::read() {
    while (true) {
        // (Re)connect on demand.
        if (handle_ == INVALID_HANDLE_VALUE) {
            if (!reconnect()) {
                Sleep(1000);
                return unexpected("Not connected to " + port_);
            }
            printf("Connected to %s @ %lu\n", port_.c_str(), (unsigned long)baud_);
        }

        char buf[256];
        DWORD got = 0;
        BOOL ok = ReadFile(handle_, buf, sizeof(buf), &got, nullptr);

        if (!ok) {
            // USB-serial vanished
            // Leave the handle invalid so the next read() attempt reconnects
            CloseHandle(handle_);
            handle_ = INVALID_HANDLE_VALUE;
            return unexpected("Port lost");
        }
        if (got == 0) continue;   // No data yet, keep waiting

        for (DWORD i = 0; i < got; ++i) {
            char c = buf[i];
            if (c != '\n') {
                accumulator_.push_back(c);
                if (accumulator_.size() > 1024) accumulator_.clear();
                continue;
            }

            while (!accumulator_.empty() && accumulator_.back() == '\r')
                accumulator_.pop_back();

            string line;
            line.swap(accumulator_);
            if (line.empty()) continue;

            if (line.starts_with("Data: ")) {
                float temperature = 0.0f, humidity = 0.0f;
                if (sscanf(line.c_str() + 6, "%f , %f",
                           &temperature, &humidity) == 2)
                    return make_pair(temperature, humidity);
                return unexpected("Failed to parse: " + line);
            } else if (line.starts_with("Info: ")) {
                printf("[INFO] %s\n", line.c_str() + 6);
            } else if (line.starts_with("Error: ")) {
                printf("[ERROR] %s\n", line.c_str() + 7);
            } else {
                printf("[????] %s\n", line.c_str());
            }
        }
    }
}