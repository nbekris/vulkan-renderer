#include "Application.h"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char **argv) {
	try {
		int frameLimit = 0;
		if (argc != 1) {
			if (argc != 3 || std::string(argv[1]) != "--frames") {
				throw std::invalid_argument("usage: VulkanProj.exe [--frames positive-count]");
			}
			size_t parsed = 0;
			frameLimit = std::stoi(argv[2], &parsed);
			if (frameLimit <= 0 || parsed != std::string(argv[2]).size()) {
				throw std::invalid_argument("frame count must be a positive integer");
			}
		}
		VulkanRenderer::Application application;
		application.Run(frameLimit);
	} catch (const std::exception &exception) {
		std::cerr << exception.what() << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
