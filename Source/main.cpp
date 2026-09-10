#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr std::array<const char*, 1> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanupWindow();
    }

private:
    void initWindow() {
        if (glfwInit() != GLFW_TRUE) throw std::runtime_error("failed to initialize GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("failed to create GLFW window");
        }
    }

    void initVulkan() {
        createInstance(); createSurface(); pickPhysicalDevice(); createLogicalDevice();
        createSwapChain(); createImageViews(); createGraphicsPipeline(); createCommandPool();
        createCommandBuffers(); createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) { glfwPollEvents(); drawFrame(); }
        device.waitIdle();
    }

    void cleanupWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance() {
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName   = "Vulkan Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "No Engine",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = vk::ApiVersion14,
        };
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) throw std::runtime_error("failed to get GLFW Vulkan extensions");
        const std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        const auto supportedExtensions = context.enumerateInstanceExtensionProperties();
        for (const char* required : extensions) {
            const bool supported = std::any_of(supportedExtensions.begin(), supportedExtensions.end(), [required](const vk::ExtensionProperties& available) {
                return std::string_view(available.extensionName.data()) == required;
            });
            if (!supported) throw std::runtime_error("required GLFW extension not supported: " + std::string(required));
        }
        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo        = &appInfo,
            .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };
        instance = vk::raii::Instance(context, createInfo);
    }

    void createSurface() {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(static_cast<VkInstance>(static_cast<vk::Instance>(instance)), window, nullptr, &rawSurface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface");
        surface = vk::raii::SurfaceKHR(instance, rawSurface);
    }

    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& candidate) const {
        QueueFamilyIndices indices;
        const auto families = candidate.getQueueFamilyProperties();
        for (uint32_t i = 0; i < families.size(); ++i) {
            if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) indices.graphicsFamily = i;
            if (candidate.getSurfaceSupportKHR(i, *surface)) indices.presentFamily = i;
            if (indices.isComplete()) break;
        }
        return indices;
    }

    bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& candidate) const {
        std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : candidate.enumerateDeviceExtensionProperties()) required.erase(extension.extensionName.data());
        return required.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& candidate) const {
        return { candidate.getSurfaceCapabilitiesKHR(*surface), candidate.getSurfaceFormatsKHR(*surface), candidate.getSurfacePresentModesKHR(*surface) };
    }

    bool supportsDynamicRendering(const vk::raii::PhysicalDevice& candidate) const {
        const auto features = candidate.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDynamicRenderingFeatures>();
        return features.template get<vk::PhysicalDeviceDynamicRenderingFeatures>().dynamicRendering;
    }

    bool isDeviceSuitable(const vk::raii::PhysicalDevice& candidate) const {
        if (!findQueueFamilies(candidate).isComplete() || !checkDeviceExtensionSupport(candidate) || !supportsDynamicRendering(candidate)) return false;
        const auto support = querySwapChainSupport(candidate);
        return !support.formats.empty() && !support.presentModes.empty();
    }

    void pickPhysicalDevice() {
        const auto devices = instance.enumeratePhysicalDevices();
        if (devices.empty()) throw std::runtime_error("failed to find a Vulkan-capable GPU");
        for (const auto& candidate : devices) if (isDeviceSuitable(candidate)) { physicalDevice = candidate; return; }
        throw std::runtime_error("failed to find a GPU with swap-chain and dynamic-rendering support");
    }

    void createLogicalDevice() {
        const auto indices = findQueueFamilies(physicalDevice);
        const std::set<uint32_t> uniqueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        constexpr float queuePriority = 1.0f;
        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        for (uint32_t family : uniqueFamilies) {
            queueInfos.push_back(vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = family,
                .queueCount       = 1,
                .pQueuePriorities = &queuePriority,
            });
        }
        vk::PhysicalDeviceDynamicRenderingFeatures dynamicRendering{
            .dynamicRendering = VK_TRUE,
        };
        vk::DeviceCreateInfo createInfo{
            .pNext                  = &dynamicRendering,
            .queueCreateInfoCount   = static_cast<uint32_t>(queueInfos.size()),
            .pQueueCreateInfos      = queueInfos.data(),
            .enabledExtensionCount  = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
        };
        device = vk::raii::Device(physicalDevice, createInfo);
        graphicsQueue = vk::raii::Queue(device, indices.graphicsFamily.value(), 0);
        presentQueue = vk::raii::Queue(device, indices.presentFamily.value(), 0);
    }

    vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) const {
        for (const auto& format : formats)
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) return format;
        return formats.front();
    }
    vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& modes) const {
        for (const auto mode : modes) if (mode == vk::PresentModeKHR::eMailbox) return mode;
        return vk::PresentModeKHR::eFifo;
    }
    vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
        int width = 0, height = 0; glfwGetFramebufferSize(window, &width, &height);
        return { std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                 std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
    }

    void createSwapChain() {
        const auto support = querySwapChainSupport(physicalDevice);
        const auto format = chooseSurfaceFormat(support.formats);
        const auto presentMode = choosePresentMode(support.presentModes);
        const auto extent = chooseExtent(support.capabilities);
        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) imageCount = support.capabilities.maxImageCount;
        const auto indices = findQueueFamilies(physicalDevice);
        const std::array<uint32_t, 2> families = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        const bool separateQueueFamilies = indices.graphicsFamily != indices.presentFamily;
        vk::SwapchainCreateInfoKHR info{
            .surface               = *surface,
            .minImageCount         = imageCount,
            .imageFormat           = format.format,
            .imageColorSpace       = format.colorSpace,
            .imageExtent           = extent,
            .imageArrayLayers      = 1,
            .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode      = separateQueueFamilies ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = separateQueueFamilies ? static_cast<uint32_t>(families.size()) : 0,
            .pQueueFamilyIndices   = separateQueueFamilies ? families.data() : nullptr,
            .preTransform          = support.capabilities.currentTransform,
            .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode           = presentMode,
            .clipped               = VK_TRUE,
        };
        swapChain = vk::raii::SwapchainKHR(device, info);
        swapChainImages = swapChain.getImages(); swapChainImageFormat = format.format; swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.clear(); swapChainImageViews.reserve(swapChainImages.size());
        for (const auto image : swapChainImages) {
            vk::ImageViewCreateInfo info{
                .image            = image,
                .viewType         = vk::ImageViewType::e2D,
                .format           = swapChainImageFormat,
                .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1},
            };
            swapChainImageViews.emplace_back(device, info);
        }
    }

    static std::vector<char> readFile(const std::string& name) {
        std::ifstream file(name, std::ios::ate | std::ios::binary);
        if (!file) throw std::runtime_error("failed to open shader: " + name);
        const size_t size = static_cast<size_t>(file.tellg()); std::vector<char> buffer(size);
        file.seekg(0); file.read(buffer.data(), static_cast<std::streamsize>(size)); return buffer;
    }
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo info{
            .codeSize = code.size(),
            .pCode    = reinterpret_cast<const uint32_t*>(code.data()),
        };
        return vk::raii::ShaderModule(device, info);
    }

    void createGraphicsPipeline() {
        const auto vertModule = createShaderModule(readFile("Shaders/vert.spv"));
        const auto fragModule = createShaderModule(readFile("Shaders/frag.spv"));
        const std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
            vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = *vertModule, .pName = "main"},
            vk::PipelineShaderStageCreateInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = *fragModule, .pName = "main"},
        };
        vk::PipelineVertexInputStateCreateInfo vertexInput{};
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
        vk::Viewport viewport{.width = static_cast<float>(swapChainExtent.width), .height = static_cast<float>(swapChainExtent.height), .maxDepth = 1.0f};
        vk::Rect2D scissor{.extent = swapChainExtent};
        vk::PipelineViewportStateCreateInfo viewportState{
            .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor,
        };
        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack, .frontFace = vk::FrontFace::eClockwise, .lineWidth = 1.0f,
        };
        vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1};
        vk::PipelineColorBlendAttachmentState blendAttachment{
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };
        vk::PipelineColorBlendStateCreateInfo blending{.attachmentCount = 1, .pAttachments = &blendAttachment};
        pipelineLayout = vk::raii::PipelineLayout(device, vk::PipelineLayoutCreateInfo{});
        vk::PipelineRenderingCreateInfo renderingInfo{
            .colorAttachmentCount    = 1,
            .pColorAttachmentFormats = &swapChainImageFormat,
        };
        vk::GraphicsPipelineCreateInfo info{
            .pNext                  = &renderingInfo,
            .stageCount             = static_cast<uint32_t>(stages.size()),
            .pStages                = stages.data(),
            .pVertexInputState      = &vertexInput,
            .pInputAssemblyState    = &inputAssembly,
            .pViewportState         = &viewportState,
            .pRasterizationState    = &rasterizer,
            .pMultisampleState      = &multisampling,
            .pColorBlendState       = &blending,
            .layout                 = *pipelineLayout,
            .renderPass             = nullptr,
        };
        graphicsPipeline = vk::raii::Pipeline(device, nullptr, info);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo info{
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = findQueueFamilies(physicalDevice).graphicsFamily.value(),
        };
        commandPool = vk::raii::CommandPool(device, info);
    }
    void createCommandBuffers() {
        vk::CommandBufferAllocateInfo info{
            .commandPool        = *commandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        };
        commandBuffers = vk::raii::CommandBuffers(device, info);
    }

    void recordCommandBuffer(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) const {
        commandBuffer.begin(vk::CommandBufferBeginInfo{});
        vk::ImageMemoryBarrier toColor{
            .dstAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
            .oldLayout       = vk::ImageLayout::eUndefined,
            .newLayout       = vk::ImageLayout::eColorAttachmentOptimal,
            .image           = swapChainImages[imageIndex],
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1},
        };
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, nullptr, nullptr, toColor);
        vk::ClearValue clear{};
        clear.color.float32[0] = 0.02f;
        clear.color.float32[1] = 0.02f;
        clear.color.float32[2] = 0.04f;
        clear.color.float32[3] = 1.0f;
        vk::RenderingAttachmentInfo attachment{
            .imageView   = *swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp      = vk::AttachmentLoadOp::eClear,
            .storeOp     = vk::AttachmentStoreOp::eStore,
            .clearValue  = clear,
        };
        vk::RenderingInfo renderingInfo{
            .renderArea             = {.extent = swapChainExtent},
            .layerCount             = 1,
            .colorAttachmentCount   = 1,
            .pColorAttachments      = &attachment,
        };
        commandBuffer.beginRendering(renderingInfo); commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline); commandBuffer.draw(3, 1, 0, 0); commandBuffer.endRendering();
        vk::ImageMemoryBarrier toPresent{
            .srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite,
            .oldLayout       = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout       = vk::ImageLayout::ePresentSrcKHR,
            .image           = swapChainImages[imageIndex],
            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1},
        };
        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, nullptr, nullptr, toPresent);
        commandBuffer.end();
    }

    void createSyncObjects() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
            renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
            vk::FenceCreateInfo fenceInfo{.flags = vk::FenceCreateFlagBits::eSignaled};
            inFlightFences.emplace_back(device, fenceInfo);
        }
    }
    void drawFrame() {
        const auto& fence = inFlightFences[currentFrame];
        if (device.waitForFences(*fence, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess)
            throw std::runtime_error("failed while waiting for the frame fence");
        const auto acquired = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphores[currentFrame]);
        if (acquired.result != vk::Result::eSuccess && acquired.result != vk::Result::eSuboptimalKHR) throw std::runtime_error("failed to acquire swap-chain image");
        const uint32_t imageIndex = acquired.value;
        device.resetFences(*fence); commandBuffers[currentFrame].reset(); recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
        const vk::Semaphore waitSemaphore = *imageAvailableSemaphores[currentFrame];
        const vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const vk::CommandBuffer commandBuffer = *commandBuffers[currentFrame];
        const vk::Semaphore signalSemaphore = *renderFinishedSemaphores[currentFrame];
        vk::SubmitInfo submit{
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = &waitSemaphore,
            .pWaitDstStageMask    = &waitStage,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &signalSemaphore,
        };
        graphicsQueue.submit(submit, *fence);
        const vk::SwapchainKHR swapchainHandle = *swapChain;
        vk::PresentInfoKHR present{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &signalSemaphore,
            .swapchainCount     = 1,
            .pSwapchains        = &swapchainHandle,
            .pImageIndices      = &imageIndex,
        };
        const auto presented = presentQueue.presentKHR(present);
        if (presented != vk::Result::eSuccess && presented != vk::Result::eSuboptimalKHR) throw std::runtime_error("failed to present swap-chain image");
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    GLFWwindow* window = nullptr;
    vk::raii::Context context;
    vk::raii::Instance instance{ nullptr };
    vk::raii::SurfaceKHR surface{ nullptr };
    vk::raii::PhysicalDevice physicalDevice{ nullptr };
    vk::raii::Device device{ nullptr };
    vk::raii::Queue graphicsQueue{ nullptr };
    vk::raii::Queue presentQueue{ nullptr };
    vk::raii::SwapchainKHR swapChain{ nullptr };
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    vk::Extent2D swapChainExtent{};
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::raii::PipelineLayout pipelineLayout{ nullptr };
    vk::raii::Pipeline graphicsPipeline{ nullptr };
    vk::raii::CommandPool commandPool{ nullptr };
    vk::raii::CommandBuffers commandBuffers{ nullptr };
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores, renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    size_t currentFrame = 0;
};

int main() {
    try { HelloTriangleApplication app; app.run(); }
    catch (const std::exception& exception) { std::cerr << exception.what() << '\n'; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
