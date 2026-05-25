#include <doctest/doctest.h>
#include "ghostManager.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TEST_CASE("Testing GhostManager Serialization & Deserialization") {
    GhostManager gm;
    
    // 1. Record test frames
    gm.startRecording();
    gm.recordFrame(100, 10.0f, 20.0f, 1.5);
    gm.recordFrame(200, 15.0f, 25.0f, 1.8);
    gm.saveBestLap("Test_Track");
    
    CHECK(gm.hasBestLap() == true);
    CHECK(gm.getBestLapTime() == doctest::Approx(0.2f)); // 200ms = 0.2s
    
    // 2. Serialize to bytes buffer
    std::vector<uint8_t> buffer = gm.getSerializedBestLap();
    CHECK(buffer.size() > 0);
    
    // 3. Load from buffer to a new manager
    GhostManager gm2;
    gm2.loadFromBuffer(buffer);
    
    CHECK(gm2.hasBestLap() == true);
    CHECK(gm2.getBestLapTime() == doctest::Approx(0.2f));
    
    // 4. Test state interpolation logic
    float gx = 0, gy = 0;
    double ga = 0;
    // Exactly at midpoint of recorded frames (150ms)
    bool hasState = gm2.getGhostState(150, gx, gy, ga);
    CHECK(hasState == true);
    CHECK(gx == doctest::Approx(12.5f)); // (10.0 + 15.0) / 2
    CHECK(gy == doctest::Approx(22.5f)); // (20.0 + 25.0) / 2
    CHECK(ga == doctest::Approx(1.65)); // (1.5 + 1.8) / 2
}
