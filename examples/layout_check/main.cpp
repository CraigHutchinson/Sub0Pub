/** Compare message representations before exchanging bytes — layout inspection
 *
 * Use when: peers plan to serialize raw structures and need a compatibility check first.
 * Demonstrates: makeFingerprint<T>(), makeLayout<T>(), layout comparison, nested types and arrays.
 * Story: two versions of SensorData differ by a flags member. Their layouts are printed and
 * compared; a same-version comparison follows, then a nested particle structure is inspected.
 * Keep in mind: peers and connection rejection are simulated; this does not perform a handshake
 * or byte conversion. Matching fingerprints are not proof of semantic compatibility. MSVC lacks
 * the per-member layout hash used on GCC/Clang, so its comparison is less detailed.
 * Run: Sub0Pub_LayoutCheck prints fingerprints, a version mismatch and a same-build match.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>

// --- Simulated "v1" and "v2" of the same message type ---

namespace v1 {
    struct SensorData {
        float temperature;
        uint32_t timestamp;
    };
}

namespace v2 {
    struct SensorData {
        float temperature;
        uint32_t timestamp;
        uint8_t flags;      // added in v2
    };
}

// --- A struct with nested types and arrays ---

struct Vec3 { float x, y, z; };
struct Particle { Vec3 position; Vec3 velocity; float mass; };
struct ParticleSystem { Particle particles[16]; uint32_t count; };

void printFingerprint(const char* name, const sub0::utility::TypeFingerprint& fp)
{
    std::printf("  %-20s  size=%2u  align=%u  arity=%u  extent=%u  elemHash=0x%08X\n",
                name, fp.size, fp.alignment, fp.arity, fp.extent, fp.elementHash);
}

void printLayout(const char* name, const sub0::utility::TypeLayout& layout)
{
    std::printf("  %-20s  size=%2u  align=%u  arity=%u  layoutHash=0x%08X\n",
                name, layout.fingerprint.size, layout.fingerprint.alignment,
                layout.fingerprint.arity, layout.layoutHash);
}

int main()
{
    std::printf("=== Layout Fingerprinting ===\n\n");

    // --- Basic fingerprints ---
    std::printf("TypeFingerprint (automatic, compile-time):\n");
    printFingerprint("v1::SensorData", sub0::utility::makeFingerprint<v1::SensorData>());
    printFingerprint("v2::SensorData", sub0::utility::makeFingerprint<v2::SensorData>());
    printFingerprint("Vec3", sub0::utility::makeFingerprint<Vec3>());
    printFingerprint("Particle", sub0::utility::makeFingerprint<Particle>());
    printFingerprint("float[4]", sub0::utility::makeFingerprint<float[4]>());
    printFingerprint("float[4][4]", sub0::utility::makeFingerprint<float[4][4]>());

    // --- Full layout with per-member hash ---
    std::printf("\nTypeLayout (automatic, per-member offset+size hash):\n");
    auto layoutV1 = sub0::utility::makeLayout<v1::SensorData>();
    auto layoutV2 = sub0::utility::makeLayout<v2::SensorData>();
    printLayout("v1::SensorData", layoutV1);
    printLayout("v2::SensorData", layoutV2);

    // --- Version mismatch detection ---
    std::printf("\nVersion check:\n");
    if (layoutV1 == layoutV2)
        std::printf("  v1 and v2 are compatible\n");
    else
        std::printf("  v1 and v2 are INCOMPATIBLE (would reject connection)\n");

    // --- Same-version check ---
    auto layoutV1b = sub0::utility::makeLayout<v1::SensorData>();
    if (layoutV1 == layoutV1b)
        std::printf("  v1 matches v1 (same build) - compatible\n");

    // --- Nested struct fingerprint ---
    std::printf("\nNested type fingerprint:\n");
    printLayout("ParticleSystem", sub0::utility::makeLayout<ParticleSystem>());

    return 0;
}
