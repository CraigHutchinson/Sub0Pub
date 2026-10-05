#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

// Test structs with known member counts
struct Empty {};

struct OneField {
    int a;
};

struct TwoFields {
    int a;
    float b;
};

struct ThreeFields {
    int a;
    float b;
    double c;
};

struct Packed {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
};

struct WithPadding {
    uint8_t a;
    // padding
    uint32_t b;
};

// Simulates a "different version" of the same struct
struct TwoFieldsV2 {
    int a;
    float b;
    double c; // added field
};

} // namespace

TEST_CASE("Arity detection: member count") {
    CHECK(sub0::utility::memberCount<Empty> == 0);
    CHECK(sub0::utility::memberCount<OneField> == 1);
    CHECK(sub0::utility::memberCount<TwoFields> == 2);
    CHECK(sub0::utility::memberCount<ThreeFields> == 3);
    CHECK(sub0::utility::memberCount<Packed> == 4);
    CHECK(sub0::utility::memberCount<WithPadding> == 2);
}

TEST_CASE("Arity detection: primitive types") {
    // Primitives are brace-initializable with 1 value, so arity = 1
    CHECK(sub0::utility::memberCount<int> == 1);
    CHECK(sub0::utility::memberCount<float> == 1);
    CHECK(sub0::utility::memberCount<double> == 1);
}

// Exercise both sides of the search midpoint and its maximum supported arity.
namespace {
struct Fields15
{
    int f0, f1, f2, f3, f4, f5, f6, f7;
    int f8, f9, f10, f11, f12, f13, f14;
};
struct Fields16
{
    int f0, f1, f2, f3, f4, f5, f6, f7;
    int f8, f9, f10, f11, f12, f13, f14, f15;
};
struct Fields17
{
    int f0, f1, f2, f3, f4, f5, f6, f7;
    int f8, f9, f10, f11, f12, f13, f14, f15;
    int f16;
};
struct Fields31
{
    int f0, f1, f2, f3, f4, f5, f6, f7;
    int f8, f9, f10, f11, f12, f13, f14, f15;
    int f16, f17, f18, f19, f20, f21, f22, f23;
    int f24, f25, f26, f27, f28, f29, f30;
};
struct Fields32
{
    int f0, f1, f2, f3, f4, f5, f6, f7;
    int f8, f9, f10, f11, f12, f13, f14, f15;
    int f16, f17, f18, f19, f20, f21, f22, f23;
    int f24, f25, f26, f27, f28, f29, f30, f31;
};
}

TEST_CASE("Arity detection: search boundaries") {
    static_assert(sub0::utility::memberCount<Fields15> == 15);
    static_assert(sub0::utility::memberCount<Fields16> == 16);
    static_assert(sub0::utility::memberCount<Fields17> == 17);
    static_assert(sub0::utility::memberCount<Fields31> == 31);
    static_assert(sub0::utility::memberCount<Fields32> == 32);
    static_assert(sub0::utility::makeFingerprint<Fields32[2]>().arity == 32);
}

TEST_CASE("TypeFingerprint: basic properties") {
    constexpr auto fp = sub0::utility::makeFingerprint<TwoFields>();
    CHECK(fp.size == sizeof(TwoFields));
    CHECK(fp.alignment == alignof(TwoFields));
    CHECK(fp.arity == 2);
}

TEST_CASE("TypeFingerprint: detects struct version mismatch") {
    constexpr auto v1 = sub0::utility::makeFingerprint<TwoFields>();
    constexpr auto v2 = sub0::utility::makeFingerprint<TwoFieldsV2>();

    // Different size, different arity — fingerprints must differ
    CHECK(v1 != v2);
    CHECK(v1.size != v2.size);
    CHECK(v1.arity != v2.arity);
}

TEST_CASE("TypeFingerprint: same struct matches itself") {
    constexpr auto a = sub0::utility::makeFingerprint<ThreeFields>();
    constexpr auto b = sub0::utility::makeFingerprint<ThreeFields>();
    CHECK(a == b);
}

TEST_CASE("TypeFingerprint: padding affects size but not arity") {
    constexpr auto packed = sub0::utility::makeFingerprint<Packed>();
    constexpr auto padded = sub0::utility::makeFingerprint<WithPadding>();

    // Both have different sizes due to padding
    CHECK(packed.size != padded.size);
    // Different arity
    CHECK(packed.arity != padded.arity);
}

// --- Automatic layout tests (no macro, no member names) ---

struct SensorData {
    float temperature;
    uint32_t timestamp;
    uint8_t flags;
};

// Same members reordered — different offsets
struct SensorDataReordered {
    uint8_t flags;
    float temperature;
    uint32_t timestamp;
};

// Same names, added field
struct SensorDataV2 {
    float temperature;
    uint32_t timestamp;
    uint8_t flags;
    uint16_t quality;
};

TEST_CASE("makeLayout: automatic layout capture") {
    auto layout = sub0::utility::makeLayout<SensorData>();

    CHECK(layout.fingerprint.size == sizeof(SensorData));
    CHECK(layout.fingerprint.alignment == alignof(SensorData));
    CHECK(layout.fingerprint.arity == 3);
#if !defined(_MSC_VER) // MSVC falls back to fingerprint-only (no per-member decomposition)
    CHECK(layout.layoutHash != 0);
#endif
}

TEST_CASE("makeLayout: same struct matches itself") {
    auto a = sub0::utility::makeLayout<SensorData>();
    auto b = sub0::utility::makeLayout<SensorData>();
    CHECK(a == b);
}

#if !defined(_MSC_VER) // Per-member layout hash requires structured binding decomposition
TEST_CASE("makeLayout: detects member reordering") {
    auto original = sub0::utility::makeLayout<SensorData>();
    auto reordered = sub0::utility::makeLayout<SensorDataReordered>();
    CHECK(original.layoutHash != reordered.layoutHash);
}
#endif

TEST_CASE("makeLayout: detects added field") {
    auto v1 = sub0::utility::makeLayout<SensorData>();
    auto v2 = sub0::utility::makeLayout<SensorDataV2>();

    CHECK(v1 != v2);
    CHECK(v1.fingerprint.arity != v2.fingerprint.arity);
}

TEST_CASE("makeLayout: single member struct") {
    auto layout = sub0::utility::makeLayout<OneField>();
    CHECK(layout.fingerprint.arity == 1);
    CHECK(layout.fingerprint.size == sizeof(int));
}

// --- Recursive fingerprinting: arrays and nested structs ---

struct Vec3 { float x, y, z; };
struct Particle { Vec3 position; Vec3 velocity; float mass; };
struct ParticleSystem { Particle particles[64]; uint32_t count; };

struct Vec3_v2 { double x, y, z; };
struct Particle_v2 { Vec3_v2 position; Vec3_v2 velocity; float mass; };
struct ParticleSystem_v2 { Particle_v2 particles[64]; uint32_t count; };

struct WithArray { float data[4]; int tag; };
struct WithDifferentArray { float data[8]; int tag; };

TEST_CASE("TypeFingerprint: array type includes element info") {
    constexpr auto scalar = sub0::utility::makeFingerprint<float>();
    constexpr auto arr4 = sub0::utility::makeFingerprint<float[4]>();
    constexpr auto arr8 = sub0::utility::makeFingerprint<float[8]>();

    CHECK(scalar.extent == 0);
    CHECK(arr4.extent == 4);
    CHECK(arr8.extent == 8);
    CHECK(scalar.elementHash == 0);
    CHECK(arr4.elementHash != 0);
    CHECK(arr4.elementHash == arr8.elementHash);
    CHECK(arr4 != arr8);
}

TEST_CASE("TypeFingerprint: nested array (2D)") {
    constexpr auto flat = sub0::utility::makeFingerprint<float[16]>();
    constexpr auto mat = sub0::utility::makeFingerprint<float[4][4]>();

    CHECK(flat.size == mat.size);
    CHECK(flat.extent == 16);
    CHECK(mat.extent == 4);
    CHECK(flat.elementHash != mat.elementHash);
}

TEST_CASE("makeLayout: struct with array vs different array extent") {
    auto layout = sub0::utility::makeLayout<WithArray>();
    auto layout2 = sub0::utility::makeLayout<WithDifferentArray>();
    // Different sizes must produce different fingerprints
    CHECK(layout.fingerprint != layout2.fingerprint);
}

TEST_CASE("makeLayout: nested struct changes propagate via fingerprint") {
    auto v1 = sub0::utility::makeLayout<ParticleSystem>();
    auto v2 = sub0::utility::makeLayout<ParticleSystem_v2>();

    // Vec3 float->double changes sizeof which propagates through fingerprint
    CHECK(v1 != v2);
    CHECK(v1.fingerprint.size != v2.fingerprint.size);
}

// 32-member struct — automatic decomposition
struct Ridiculous {
    uint8_t  m00; uint8_t  m01; uint8_t  m02; uint8_t  m03;
    uint16_t m04; uint16_t m05; uint16_t m06; uint16_t m07;
    uint32_t m08; uint32_t m09; uint32_t m10; uint32_t m11;
    float    m12; float    m13; float    m14; float    m15;
    double   m16; double   m17; double   m18; double   m19;
    int8_t   m20; int8_t   m21; int8_t   m22; int8_t   m23;
    int16_t  m24; int16_t  m25; int16_t  m26; int16_t  m27;
    int32_t  m28; int32_t  m29; int64_t  m30; int64_t  m31;
};

TEST_CASE("makeLayout: 32 members automatic") {
    auto layout = sub0::utility::makeLayout<Ridiculous>();

    CHECK(layout.fingerprint.size == sizeof(Ridiculous));
    CHECK(layout.fingerprint.alignment == alignof(Ridiculous));
    CHECK(layout.fingerprint.arity == 32);

    auto layout2 = sub0::utility::makeLayout<Ridiculous>();
    CHECK(layout == layout2);
}

TEST_CASE("memberCount: array members count once (no brace elision)") {
    // Aggregate-init probing must match structured-binding arity used by makeLayout()
    CHECK(sub0::utility::memberCount<WithArray> == 2);
    CHECK(sub0::utility::memberCount<WithDifferentArray> == 2);
    CHECK(sub0::utility::memberCount<ParticleSystem> == 2);
}
