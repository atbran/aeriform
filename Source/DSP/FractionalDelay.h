#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace aeriform::dsp
{
/**
    Circular delay line with fractional read using 4-point (3rd-order) Lagrange
    interpolation. Storage is allocated once in prepare(); reads and writes
    never allocate.
*/
class FractionalDelay
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 16;
        while (size < maxDelaySamples + 8) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    int getMaxDelay() const noexcept { return (int) buffer.size() - 6; }

    inline void push (float x) noexcept
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    /** Sample written delaySamples ago (delay >= 1). 4-point Lagrange around the integer part. */
    inline float readLagrange (float delaySamples) const noexcept
    {
        // Minimum 2 so that the newer neighbour (delay di-1) is a valid, already-written sample.
        const float d = std::clamp (delaySamples, 2.0f, (float) getMaxDelay());
        const int   di = (int) d;
        const float f  = d - (float) di;

        const int base = writePos - di;
        const float y0 = buffer[(size_t) ((base + 1) & mask)];   // one sample newer
        const float y1 = buffer[(size_t) ((base) & mask)];       // integer delay
        const float y2 = buffer[(size_t) ((base - 1) & mask)];   // one sample older
        const float y3 = buffer[(size_t) ((base - 2) & mask)];

        // Lagrange 3rd order for fractional position f in [0,1) between y1 and y2
        const float c0 = -f * (f - 1.0f) * (f - 2.0f) / 6.0f;
        const float c1 = (f + 1.0f) * (f - 1.0f) * (f - 2.0f) / 2.0f;
        const float c2 = -(f + 1.0f) * f * (f - 2.0f) / 2.0f;
        const float c3 = (f + 1.0f) * f * (f - 1.0f) / 6.0f;
        return c0 * y0 + c1 * y1 + c2 * y2 + c3 * y3;
    }

    /** Cheaper linear read for secondary taps. */
    inline float readLinear (float delaySamples) const noexcept
    {
        const float d = std::clamp (delaySamples, 1.0f, (float) getMaxDelay());
        const int   di = (int) d;
        const float f  = d - (float) di;
        const int base = writePos - di;
        const float y1 = buffer[(size_t) (base & mask)];
        const float y2 = buffer[(size_t) ((base - 1) & mask)];
        return y1 + f * (y2 - y1);
    }

    inline float readInteger (int delaySamples) const noexcept
    {
        const int d = std::clamp (delaySamples, 0, getMaxDelay());
        return buffer[(size_t) ((writePos - d) & mask)];
    }

private:
    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
};
} // namespace aeriform::dsp
