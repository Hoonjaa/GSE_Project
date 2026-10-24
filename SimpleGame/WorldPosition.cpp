#include "stdafx.h"
#include "WorldPosition.h"
#include <cmath>
#include <limits>

void WorldPosition::Move(double dx, double dz)
{
    // Only small local steps are accepted; normalization supports negative chunks.
    auto axis = [](std::int64_t& chunk, double& local, double delta)
    {
        const double next = local + delta;
        if (!std::isfinite(next) || std::abs(delta) > ChunkSize * 1000)
        {
            return;
        }
        const auto shift = static_cast<std::int64_t>(std::floor(next / ChunkSize));
        const auto limit = (std::numeric_limits<std::int64_t>::max)() - 1024;
        if ((shift > 0 && chunk > limit - shift) || (shift < 0 && chunk < -limit - shift))
        {
            return;
        }
        chunk += shift;
        local = next - static_cast<double>(shift) * ChunkSize;
    };
    axis(chunkX, x, dx);
    axis(chunkZ, z, dz);
}

Vec3 WorldPosition::RelativeTo(const WorldPosition& origin) const
{
    // Subtract integers first for nearby chunks; never convert global positions to float.
    auto relative = [](std::int64_t a, std::int64_t b, double local)
    {
        // Unsigned subtraction also handles endpoints without signed overflow.
        const std::uint64_t distance =
            a >= b ? std::uint64_t(a) - std::uint64_t(b) : std::uint64_t(b) - std::uint64_t(a);
        if (distance > 1000)
        {
            return a >= b ? 100000.f : -100000.f;
        }
        return static_cast<float>((a >= b ? 1.0 : -1.0) * double(distance) * ChunkSize + local);
    };
    return {relative(chunkX, origin.chunkX, x - origin.x), 0, relative(chunkZ, origin.chunkZ, z - origin.z)};
}
