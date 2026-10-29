using namespace std;

class SensorTester {
private:
    string name;
    int low;
    int high;
public:
    SensorTester(string name, const int low, const int high) : name(name), low(low), high(high) {
        if (low >= high) {
            throw invalid_argument("Low value must be less than high value");
        }
    };
    void test(float value) {
        if (value < low) {
            printf("[ALERT] %s is below %d!\n", name.c_str(), low);
        } else if (value > high) {
            printf("[ALERT] %s is above %d!\n", name.c_str(), high);
        }
    };
};