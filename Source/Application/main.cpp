#include "Application/Application.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char **argv) {
	try {
		int frameLimit = 0;
		unsigned framesInFlight = 2;
		std::string modelPath;
		bool benchmark = false, culling = true, autoFrame = true;
		for (int i = 1; i < argc; i += 2) {
			if (i + 1 >= argc) {
				throw std::invalid_argument("each option requires a value");
			}
			const std::string OPTION = argv[i], ARGUMENT = argv[i + 1];
			if (OPTION == "--model") {
				modelPath = ARGUMENT;
				continue;
			}
			if (OPTION == "--culling" || OPTION == "--auto-frame") {
				if (ARGUMENT != "on" && ARGUMENT != "off") {
					throw std::invalid_argument("expected on or off");
				}
				(OPTION == "--culling" ? culling : autoFrame) = ARGUMENT == "on";
				continue;
			}
			if (OPTION != "--frames" && OPTION != "--benchmark" && OPTION != "--frames-in-flight") {
				throw std::invalid_argument("options: --model path --frames count --benchmark count --frames-in-flight "
											"2|3 --culling on|off --auto-frame on|off");
			}
			size_t parsed = 0;
			const int VALUE = std::stoi(ARGUMENT, &parsed);
			if (VALUE <= 0 || parsed != ARGUMENT.size()) {
				throw std::invalid_argument("invalid positive integer");
			}
			if (OPTION == "--frames-in-flight") {
				if (VALUE != 2 && VALUE != 3) {
					throw std::invalid_argument("frames in flight must be 2 or 3");
				}
				framesInFlight = static_cast<unsigned>(VALUE);
				continue;
			}
			frameLimit = VALUE;
			if (OPTION == "--benchmark") {
				benchmark = true;
			}
		}
		VulkanRenderer::Application application;
		application.Run(frameLimit, framesInFlight, modelPath, benchmark, culling, autoFrame);
	} catch (const std::exception &exception) {
		std::cerr << exception.what() << '\n';
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
