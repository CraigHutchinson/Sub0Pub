/** Temperature displays — runtime publish/subscribe
 *
 * Use when: objects should discover messages by type and subscribe or unsubscribe through their lifetime.
 * Demonstrates: Publish<float>, Subscribe<float>, publish(), and subscribing and unsubscribing through construction and destruction.
 * Story: one sensor sends temperatures to LCD and LOG displays. A temporary display subscribes for
 * 26 degrees, then leaves its scope; the next reading reaches only LCD and LOG. No publisher
 * stores a list of displays or calls them individually.
 * Keep in mind: this uses the default unlocked broker. Subscribers subscribe during construction;
 * locked subscribers need the explicit lifecycle shown in ../thread_safe_lifetime.cpp instead.
 * Run: Sub0Pub_BasicPubSub prints each delivery; TMP appears only for the 26-degree reading.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>

// A sensor that publishes temperature readings
class TemperatureSensor : public sub0::Publish<float> {
public:
    void sample(float celsius) {
        sub0::publish(this, celsius);
    }
};

// A display that subscribes to temperature readings
class TemperatureDisplay : public sub0::Subscribe<float> {
    const char* label_;
public:
    TemperatureDisplay(const char* label) : label_(label) {}

    void receive(const float& celsius) noexcept {
        std::printf("  [%s] Temperature: %.1f C\n", label_, celsius);
    }
};

int main()
{
    std::printf("=== Basic Pub/Sub ===\n");

    TemperatureSensor sensor;

    // Create two displays — both auto-subscribe to float
    TemperatureDisplay lcd("LCD");
    TemperatureDisplay log("LOG");

    std::printf("Publishing 23.5:\n");
    sensor.sample(23.5f);

    std::printf("Publishing 24.1:\n");
    sensor.sample(24.1f);

    // Add a temporary subscriber; LCD and LOG stay alive throughout.
    {
        TemperatureDisplay temporary("TMP");
        std::printf("TMP added, publishing 26.0:\n");
        sensor.sample(26.0f);
    }
    // TMP destroyed
    std::printf("TMP removed, publishing 27.0:\n");
    sensor.sample(27.0f);

    return 0;
}
