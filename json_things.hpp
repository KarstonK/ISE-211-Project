// Common
#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <stdexcept>

// Extra for sensor logger
#include <chrono>
#include <vector>
#include <optional>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace std;

class JsonFileBase {
public:
    explicit JsonFileBase(string path) : path_(move(path)) {}
    virtual ~JsonFileBase() = default;

    // Serialize current in-memory state to disk.
    void save() const { writeJson(toJson()); }

    // Load state from disk (throws if the file is missing / malformed).
    void load() { fromJson(readJson()); }

    bool exists() const {
        ifstream f(path_);
        return f.good();
    }

    const string& path() const { return path_; }

protected:
    // Derived classes describe their state via these two hooks.
    virtual nlohmann::json toJson() const = 0;
    virtual void fromJson(const nlohmann::json& j) = 0;

    nlohmann::json readJson() const {
        ifstream f(path_);
        if (!f.is_open())
            throw runtime_error("JsonFileBase: cannot open '" + path_ + "' for reading");
        try {
            nlohmann::json j;
            f >> j;
            return j;
        } catch (const nlohmann::json::parse_error& e) {
            throw runtime_error("JsonFileBase: parse error in '" + path_ +
                                     "': " + e.what());
        }
    }

    void writeJson(const nlohmann::json& j) const {
        ofstream f(path_);
        if (!f.is_open())
            throw runtime_error("JsonFileBase: cannot open '" + path_ + "' for writing");
        f << j.dump(2) << '\n';   // 2-space indent for human inspection
    }

private:
    string path_;
};

class ConfigReader : public JsonFileBase {
public:
    struct Config {
        string port             = "\\\\.\\COM5";
        int    baud             = 115200;
        float  alarmTempLow     =  0.0f;
        float  alarmTempHigh    = 35.0f;
        float  alarmHumidityLow = 20.0f;
        float  alarmHumidityHigh= 80.0f;
    };

    explicit ConfigReader(string path) : JsonFileBase(move(path)) {}

    // Call once at startup. If the file doesn't exist, we write
    // defaults out so the user has something to edit next run.
    void loadConfigOrCreateDefaults() {
        if (!exists()) { save(); return; }
        load();
    }

    const Config& config() const { return cfg_; }

protected:
    nlohmann::json toJson() const override {
        return {
            {"port",              cfg_.port},
            {"baud",              cfg_.baud},
            {"alarmTempLow",      cfg_.alarmTempLow},
            {"alarmTempHigh",     cfg_.alarmTempHigh},
            {"alarmHumidityLow",  cfg_.alarmHumidityLow},
            {"alarmHumidityHigh", cfg_.alarmHumidityHigh}
        };
    }

    void fromJson(const nlohmann::json& j) override {
        cfg_.port               = j.value("port",               cfg_.port);
        cfg_.baud               = j.value("baud",               cfg_.baud);
        cfg_.alarmTempLow       = j.value("alarmTempLow",       cfg_.alarmTempLow);
        cfg_.alarmTempHigh      = j.value("alarmTempHigh",      cfg_.alarmTempHigh);
        cfg_.alarmHumidityLow   = j.value("alarmHumidityLow",   cfg_.alarmHumidityLow);
        cfg_.alarmHumidityHigh  = j.value("alarmHumidityHigh",  cfg_.alarmHumidityHigh);
    }

private:
    Config cfg_;
};

using TimePoint = chrono::system_clock::time_point;

struct SensorReading {
    TimePoint timestamp;
    float     temperature;
    float     humidity;
};

class SensorDataLogger : public JsonFileBase {
public:
    struct Average {
        float       temperature;
        float       humidity;
        size_t sampleCount;
    };

    explicit SensorDataLogger(string path) : JsonFileBase(move(path)) {}

    // Load the historical log at startup (no-op if file doesn't exist yet).
    void loadHistory() { if (exists()) load(); }

    // Receive a new live reading (does not touch the file).
    void addReading(float temperature, float humidity,
                    TimePoint ts = chrono::system_clock::now()) {
        readings_.push_back({ts, temperature, humidity});
    }

    // Receive a new live reading and persist immediately.
    void addReadingAndSave(float temperature, float humidity,
                           TimePoint ts = chrono::system_clock::now()) {
        addReading(temperature, humidity, ts);
        save();
    }

    // Average over [from, to). Returns nullopt if there's no data.
    optional<Average> average(TimePoint from, TimePoint to) const {
        double sumT = 0.0, sumH = 0.0;
        size_t n = 0;
        for (const auto& r : readings_) {
            if (r.timestamp >= from && r.timestamp < to) {
                sumT += r.temperature;
                sumH += r.humidity;
                ++n;
            }
        }
        if (n == 0) return nullopt;
        return Average{ static_cast<float>(sumT / n),
                        static_cast<float>(sumH / n), n };
    }

    // Convenience: average over the last `window` before `end`.
    optional<Average> averageLast(
        chrono::seconds window,
        TimePoint end = chrono::system_clock::now()) const {
        return average(end - window, end);
    }

    size_t size() const { return readings_.size(); }

protected:
    nlohmann::json toJson() const override {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& r : readings_) {
            arr.push_back({
                {"timestamp",   toIso8601(r.timestamp)},
                {"temperature", r.temperature},
                {"humidity",    r.humidity}
            });
        }
        return {{"readings", arr}};
    }

    void fromJson(const nlohmann::json& j) override {
        readings_.clear();
        if (!j.contains("readings")) return;
        for (const auto& item : j.at("readings")) {
            SensorReading r;
            r.timestamp   = fromIso8601(item.at("timestamp").get<string>());
            r.temperature = item.value("temperature", 0.0f);
            r.humidity    = item.value("humidity",    0.0f);
            readings_.push_back(r);
        }
    }

private:
    static string toIso8601(TimePoint tp) {
        const time_t t  = chrono::system_clock::to_time_t(tp);
        const auto        ms = chrono::duration_cast<chrono::milliseconds>(
                                   tp.time_since_epoch()).count() % 1000;
        tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        ostringstream os;
        os << put_time(&tm, "%Y-%m-%dT%H:%M:%S")
           << '.' << setfill('0') << setw(3) << ms << 'Z';
        return os.str();
    }

    static TimePoint fromIso8601(const string& s) {
        tm tm{};
        istringstream is(s);
        is >> get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        int ms = 0;
        if (is.peek() == '.') { is.ignore(); is >> ms; }
#ifdef _WIN32
        time_t t = _mkgmtime(&tm);
#else
        time_t t = timegm(&tm);
#endif
        return chrono::system_clock::from_time_t(t)
             + chrono::milliseconds(ms);
    }

    vector<SensorReading> readings_;
};
