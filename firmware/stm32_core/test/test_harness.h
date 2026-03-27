#pragma once

#include <cmath>
#include <cstdio>

struct TestContext {
  int failures = 0;

  void Check(bool condition, const char* message) {
    if (!condition) {
      ++failures;
      std::printf("FAIL: %s\n", message);
    }
  }

  void CheckFloatEq(float expected, float actual, const char* message) {
    const float delta = std::fabs(expected - actual);
    if (delta > 0.0001F) {
      ++failures;
      std::printf("FAIL: %s (expected %.4f, got %.4f)\n", message, expected, actual);
    }
  }
};
