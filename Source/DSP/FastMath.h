#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace voidworm::fastmath
{
/* The same answer as std::isfinite -- false for NaN and both infinities --
   from the exponent bits alone. MSVC compiles std::isfinite on a float to a
   library call (measured 2.5 ns against 0.75 ns for this), and the DSP asks
   it per sample, per filter stage, in every validity check. */
/* x modulo 4, in [0, 4), for the reactors' triangle folds. std::fmod is a
   library call under MSVC and ran per sample; truncation gives the same
   value to within rounding for any input the folds see (they are bounded
   well inside the int range). */
inline float wrap4 (float x) noexcept
{
    const auto scaled = x * 0.25f;
    auto whole = static_cast<float> (static_cast<int> (scaled));
    if (whole > scaled)
        whole -= 1.0f;
    return x - 4.0f * whole;
}

inline bool isFinite (float x) noexcept
{
    std::uint32_t bits;
    std::memcpy (&bits, &x, sizeof bits);
    return (bits & 0x7f800000u) != 0x7f800000u;
}

inline bool isFinite (double x) noexcept
{
    std::uint64_t bits;
    std::memcpy (&bits, &x, sizeof bits);
    return (bits & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

/* log2 and 2^x for gain computers, which run per sample at the oversampled
   rate in every reactor. A gain needs a ten-thousandth of a dB, not full
   precision: log2 is within 1.5e-5 over the whole positive float range and
   exp2 within 3.6e-6 relative (both fitted on Chebyshev nodes and checked
   against the library functions), about 0.0001 dB of gain. Positive,
   normal inputs only for log2; exponents between -126 and 127 for exp2. */
inline float log2 (float x) noexcept
{
    std::uint32_t bits;
    std::memcpy (&bits, &x, sizeof bits);
    const auto exponent = static_cast<float> (static_cast<int> ((bits >> 23) & 0xffu) - 127);
    bits = (bits & 0x007fffffu) | 0x3f800000u;
    float m;
    std::memcpy (&m, &bits, sizeof m);             // mantissa in [1, 2)
    const auto p = ((((0.043929100f * m - 0.409479927f) * m + 1.610192859f) * m
                     - 3.520244911f) * m + 5.069777846f) * m - 2.794160595f;
    return exponent + p;
}

inline float exp2 (float y) noexcept
{
    const auto whole = std::floor (y);
    const auto f = y - whole;                      // [0, 1)
    const auto p = (((0.013683990f * f + 0.051717781f) * f + 0.241621243f) * f
                    + 0.692969586f) * f + 1.000003594f;
    const auto scale = static_cast<std::uint32_t> (static_cast<int> (whole) + 127) << 23;
    float power;
    std::memcpy (&power, &scale, sizeof power);
    return p * power;
}
}
