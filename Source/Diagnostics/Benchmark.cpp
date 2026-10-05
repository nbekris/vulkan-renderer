#include "Diagnostics/Benchmark.h"
#include <iostream>
#include <iomanip>

namespace VulkanRenderer {
void Benchmark::Add(const RenderStats &stats, std::optional<double> gpuMilliseconds) {
	++_frames;
	_cpu += stats.recordingMilliseconds;
	_draws += stats.draws;
	_culled += stats.culled;
	_triangles += static_cast<double>(stats.triangles);
	if (gpuMilliseconds) {
		++_samples;
		_gpu += *gpuMilliseconds;
	}
}

void Benchmark::Print() const {
	if (!_frames) {
		return;
	}
	std::cout << std::fixed << std::setprecision(4) << "Benchmark: " << _frames << " frames; CPU recording "
			  << _cpu / _frames << " ms; draws " << _draws / _frames << "; culled " << _culled / _frames
			  << "; triangles " << _triangles / _frames;
	if (_samples) {
		std::cout << "; GPU rendering " << _gpu / _samples << " ms (" << _samples << " samples)";
	}
	if (!_samples) {
		std::cout << "; GPU timestamps unavailable";
	}
	std::cout << '\n';
}
} // namespace VulkanRenderer
