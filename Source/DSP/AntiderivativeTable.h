#pragma once

#include <cmath>
#include <vector>

namespace voidworm
{
/* A clip's antiderivative, tabulated once so the anti-aliased clips cost a
   few multiply-adds per sample instead of a log and an arctangent.

   Nodes every 1/32 over +-128 hold the exact integral F and the exact clip f
   (its derivative), and a cubic Hermite through them reproduces F to about
   1e-9: the interpolation error goes as step^4 times F's fourth derivative,
   which is small for these smooth rational clips. Outside the table the
   exact function is called; nothing the reactors normally produce reaches
   it. Built outside the audio thread (in prepare), never resized after. */
class AntiderivativeTable
{
public:
    static constexpr double range = 128.0;
    static constexpr double step = 1.0 / 32.0;

    using Function = double (*) (double) noexcept;

    AntiderivativeTable (Function exactIntegral, Function exactClip)
        : integral (exactIntegral)
    {
        const auto count = static_cast<size_t> (2.0 * range / step) + 1;
        values.resize (count);
        slopes.resize (count);
        for (size_t i = 0; i < count; ++i)
        {
            const auto x = -range + static_cast<double> (i) * step;
            values[i] = exactIntegral (x);
            slopes[i] = exactClip (x);
        }
    }

    double evaluate (double x) const noexcept
    {
        if (! (x > -range && x < range - step))
            return integral (x);
        const auto position = (x + range) / step;
        const auto index = static_cast<size_t> (position);
        const auto u = position - static_cast<double> (index);
        const auto u2 = u * u, u3 = u2 * u;
        return (2.0 * u3 - 3.0 * u2 + 1.0) * values[index]
             + (u3 - 2.0 * u2 + u) * step * slopes[index]
             + (-2.0 * u3 + 3.0 * u2) * values[index + 1]
             + (u3 - u2) * step * slopes[index + 1];
    }

private:
    Function integral;
    std::vector<double> values, slopes;
};
}
