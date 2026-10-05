#pragma once
#include "Rendering/MeshRenderer.h"
#include <optional>

namespace VulkanRenderer {
/** Aggregates recording and GPU durations, excluding uploads and presentation waits. */
class Benchmark {
public:
	Benchmark() = default;
	virtual ~Benchmark() = default;
	Benchmark(const Benchmark &) = delete;
	Benchmark &operator=(const Benchmark &) = delete;
	void Add(const RenderStats &stats, std::optional<double> gpuMilliseconds);
	void Print() const;

private:
	uint64_t _frames = 0;
	uint64_t _samples = 0;
	double _cpu = 0, _gpu = 0, _draws = 0, _culled = 0, _triangles = 0;
};
} // namespace VulkanRenderer
