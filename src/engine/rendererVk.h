#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

class InstanceVk {
public:
	struct Buffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		operator VkBuffer() const noexcept { return buffer; }
	};

	struct Image {
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;

		operator VkImage() const noexcept { return image; }
	};

	struct Swizzle {
		uint8 r, g, b, a;
	};

	struct ViewFrame {
		VkImageView view = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
	};

	struct ViewVk : Renderer::View {
		static constexpr uint maxFrames = 2;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkExtent2D extent{};
		uptr<ViewFrame[]> frames;
		uint32 imageCount = 0;
		Image pp;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		Buffer uniformBuf;
	};

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkCreateInstance vkCreateInstance;
	PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
	PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;

	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkBindImageMemory vkBindImageMemory;
	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
	PFN_vkCmdDispatch vkCmdDispatch;
	PFN_vkCmdDraw vkCmdDraw;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
	PFN_vkCmdNextSubpass vkCmdNextSubpass;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkCmdPushConstants vkCmdPushConstants;
	PFN_vkCmdSetScissor vkCmdSetScissor;
	PFN_vkCmdSetViewport vkCmdSetViewport;
	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkCreateComputePipelines vkCreateComputePipelines;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	PFN_vkCreateImage vkCreateImage;
	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkCreateRenderPass vkCreateRenderPass;
	PFN_vkCreateSampler vkCreateSampler;
	PFN_vkCreateSemaphore vkCreateSemaphore;
	PFN_vkCreateShaderModule vkCreateShaderModule;
	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	PFN_vkDestroyBuffer vkDestroyBuffer;
	PFN_vkDestroyCommandPool vkDestroyCommandPool;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
	PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
	PFN_vkDestroyDevice vkDestroyDevice;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	PFN_vkDestroyImage vkDestroyImage;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkDestroyPipeline vkDestroyPipeline;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
	PFN_vkDestroyRenderPass vkDestroyRenderPass;
	PFN_vkDestroySampler vkDestroySampler;
	PFN_vkDestroySemaphore vkDestroySemaphore;
	PFN_vkDestroyShaderModule vkDestroyShaderModule;
	PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkFreeMemory vkFreeMemory;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	PFN_vkGetDeviceQueue vkGetDeviceQueue;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
	PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkQueuePresentKHR vkQueuePresentKHR;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkQueueWaitIdle vkQueueWaitIdle;
	PFN_vkResetCommandBuffer vkResetCommandBuffer;
	PFN_vkResetFences vkResetFences;
	PFN_vkUnmapMemory vkUnmapMemory;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
	PFN_vkWaitForFences vkWaitForFences = nullptr;

	PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
#ifndef NDEBUG
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif

protected:
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice pdev = VK_NULL_HANDLE;
	VkDevice ldev = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties pdevMemProperties;

	void initGlobalFunctions();
	void initLocalFunctions();
	bool functionsInitialized() const noexcept { return vkWaitForFences; }

public:
	VkDevice getLdev() const noexcept { return ldev; }

	Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	void recreateBuffer(Buffer& vkb, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	void cleanupBuffer(Buffer& vkb) const noexcept;
	Image createImage(u32vec2 size, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Swizzle swizzle = {}) const;
	void cleanupImage(Image& vki) const noexcept;
	VkImageView createImageView(VkImage image, VkFormat format, Swizzle swizzle = {}) const;
	VkFramebuffer createFramebuffer(VkRenderPass rpass, VkImageView* attach, uint32 acnt, u32vec2 size) const;
	void allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const;
	VkSemaphore createSemaphore() const;
	VkFence createFence(VkFenceCreateFlags flags = 0) const;
	VkSampler createSampler(VkFilter filter) const;
	VkShaderModule createShaderModule(std::span<const uint32> code) const;
private:
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const;
};

class FormatConverter {
public:
	static constexpr uint maxTransfers = 2;
	static constexpr uint numLayouts = 2;
	static constexpr uint32 convWgrpSize = 32;
	static constexpr uint32 convStep = convWgrpSize * 4;	// 4 texels per invocation

	enum class Pipeline : uint8 {
		rgb24,
		bgr24,
		index8
	};

	struct PushData {
		alignas(4) uint offset;
	};

	struct UniformData {
		alignas(16) uint colors[256];
	};

private:
	struct SpecializationData {
		VkBool32 orderRgb;
	};

	static constexpr uint32 bindingInput = 0;
	static constexpr uint32 bindingOutput = 1;
	static constexpr uint32 bindingUniform = 2;

	array<VkPipeline, eint(Pipeline::index8) + 1> pipelines{};
	VkPipelineLayout pipelineLayoutRgb = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayoutIdx = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutRgb = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutIdx = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	array<VkDescriptorSet, maxTransfers * numLayouts> descriptorSets{};

	array<InstanceVk::Buffer, maxTransfers> outputBufs;
	array<VkDeviceSize, maxTransfers> outputBufferSizesMax{};
	array<InstanceVk::Buffer, maxTransfers> uniformBufs;	// TODO: fuse into one and mapped when needed
	array<UniformData*, maxTransfers> uniformBufferMapped;

public:
	void init(const InstanceVk* vk);
	void updateBufferSize(const InstanceVk* vk, VkDescriptorSet dset, uint id, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update);
	void free(const InstanceVk* vk) noexcept;

	bool initialized() const noexcept { return descriptorSets[0]; }	// cause it's the last thing to be initialized
	VkPipeline getPipeline(Pipeline pid) const noexcept { return pipelines[eint(pid)]; }
	tuple<VkPipelineLayout, VkDescriptorSet, uint> getPipelineInfo(Pipeline pid, uint id) const noexcept;
	VkBuffer getOutputBuffer(uint id) const noexcept { return outputBufs[id]; }
	UniformData* getUniformBufferMapped(uint id) const noexcept { return uniformBufferMapped[id]; }

private:
	void createDescriptorSetLayoutRgb(const InstanceVk* vk);
	void createDescriptorSetLayoutIdx(const InstanceVk* vk);
	void createPipelines(const InstanceVk* vk);
	void createDescriptorPoolAndSets(const InstanceVk* vk);
};

inline tuple<VkPipelineLayout, VkDescriptorSet, uint> FormatConverter::getPipelineInfo(Pipeline pid, uint id) const noexcept {
	return pid != Pipeline::index8 ? tuple(pipelineLayoutRgb, descriptorSets[id], 0) : tuple(pipelineLayoutIdx, descriptorSets[maxTransfers + id], 1);
}

class RenderPass {
public:
	struct PushData {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
		alignas(4) uint color;
		alignas(4) uint sid;
	};

	struct ViewData {
		alignas(16) vec4 pview;
	};

	struct GlobalData {
		alignas(16) vec4 colors[Settings::defaultColors.size() - 1];
		alignas(4) float gamma;
	};

	static constexpr uint32 dsetGlob = 0;
	static constexpr uint32 dsetView = 1;
	static constexpr uint32 dsetModel = 2;
	static constexpr uint samplerNearest = 0;
	static constexpr uint samplerLinear = 1;
	static constexpr VkFormat subpassFormat = VK_FORMAT_A8B8G8R8_UNORM_PACK32;
private:
	static constexpr uint32 bindingGlobData = 0;
	static constexpr uint32 bindingGlobSamp = 1;
	static constexpr uint32 bindingViewData = 0;
	static constexpr uint32 bindingViewIn = 1;
	static constexpr uint32 bindingModelTex = 0;
	static constexpr uint32 subpassGui = 0;
	static constexpr uint32 subpassFin = 1;
	static constexpr uint32 textureSetStep = 128;

	struct PipelineCreateHelper {
		VkPipelineShaderStageCreateInfo shaderStages[2];
		VkVertexInputBindingDescription bindingDescription;
		VkVertexInputAttributeDescription attributeDescription;
		VkPipelineVertexInputStateCreateInfo vertexInputState;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
		VkPipelineViewportStateCreateInfo viewportState;
		VkPipelineRasterizationStateCreateInfo rasterizationState;
		VkPipelineMultisampleStateCreateInfo multisampleState;
		VkPipelineDepthStencilStateCreateInfo depthStencilState;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendState;
		VkDynamicState dynamicStates[2];
		VkPipelineDynamicStateCreateInfo dynamicState;
		VkGraphicsPipelineCreateInfo pipelineInfo;

		PipelineCreateHelper(VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass, bool blend) noexcept;
		void cleanup(const InstanceVk* vk) noexcept;
	};

	struct DescriptorSetBlock {
		uset<VkDescriptorSet> used;
		uset<VkDescriptorSet> free;

		DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets);
	};

	VkRenderPass handle = VK_NULL_HANDLE;
	VkPipeline guiPipeline = VK_NULL_HANDLE;
	VkPipelineLayout guiPipelineLayout = VK_NULL_HANDLE;
	VkPipeline finPipeline = VK_NULL_HANDLE;
	VkPipelineLayout finPipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutGlob = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutView = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutModel = VK_NULL_HANDLE;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	umap<VkDescriptorPool, DescriptorSetBlock> poolSetTex;
	array<VkSampler, 2> samplers{};
	VkDescriptorSet globDescriptorSet = VK_NULL_HANDLE;
	InstanceVk::Buffer globBuf;

public:
	void init(const InstanceVk* vk);
	void createPass(const InstanceVk* vk, VkFormat format, Settings::Gamma& gamma, bool canSrgb);
	void createDescriptorPoolAndSets(const InstanceVk* vk, InstanceVk::ViewVk* views, uint8 numViews);
	void free(const InstanceVk* vk) noexcept;
	void freePass(const InstanceVk* vk) noexcept;
	void freeDescriptorPool(const InstanceVk* vk) noexcept;

	VkRenderPass getHandle() const noexcept { return handle; }
	VkPipeline getGuiPipeline() const noexcept { return guiPipeline; }
	VkPipelineLayout getGuiPipelineLayout() const noexcept { return guiPipelineLayout; }
	VkPipeline getFinPipeline() const noexcept { return finPipeline; }
	VkPipelineLayout getFinPipelineLayout() const noexcept { return finPipelineLayout; }
	VkDescriptorSet getGlobDescriptorSet() const noexcept { return globDescriptorSet; }
	VkBuffer getGlobBuffer() const noexcept { return globBuf; }
	pair<VkDescriptorPool, VkDescriptorSet> newDescriptorSetTex(const InstanceVk* vk, VkImageView imageView);
	pair<VkDescriptorPool, VkDescriptorSet> getDescriptorSetTex(const InstanceVk* vk);
	void freeDescriptorSetTex(const InstanceVk* vk, VkDescriptorPool pool, VkDescriptorSet dset);
	static void updateDescriptorSetImg(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkImageView imageView) noexcept;

private:
	void createRenderPass(const InstanceVk* vk, VkFormat format, bool postp);
	void createDescriptorSetLayouts(const InstanceVk* vk, bool postp);
	void createGuiPipeline(const InstanceVk* vk);
	void createFinPipeline(const InstanceVk* vk);
};

class RendererVk final : public Renderer, public InstanceVk {
private:
	enum class OptTexFmt : uint8 {
		B5G6R5,
		R5G6B5,
		A1R5G5B5,
		B5G5R5A1,
		R5G5B5A1,
		A4B4G4R4,
		A4R4G4B4,
		B4G4R4A4,
		R4G4B4A4,
		A2B10G10R10,
		A2R10G10B10
	};
	static constexpr array optTexFmtMap = {
		pair(SDL_PIXELFORMAT_BGR565, VK_FORMAT_B5G6R5_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_RGB565, VK_FORMAT_R5G6B5_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_ARGB1555, VK_FORMAT_A1R5G5B5_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_BGRA5551, VK_FORMAT_B5G5R5A1_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_RGBA5551, VK_FORMAT_R5G5B5A1_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_ABGR4444, VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT),
		pair(SDL_PIXELFORMAT_ARGB4444, VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT),
		pair(SDL_PIXELFORMAT_BGRA4444, VK_FORMAT_B4G4R4A4_UNORM_PACK16),
		pair(SDL_PIXELFORMAT_RGBA4444, VK_FORMAT_R4G4B4A4_UNORM_PACK16),
#ifdef WITH_SDL3
		pair(SDL_PIXELFORMAT_ABGR2101010, VK_FORMAT_A2B10G10R10_UNORM_PACK32),
#else
		pair(SDL_PIXELFORMAT_UNKNOWN, VK_FORMAT_A2B10G10R10_UNORM_PACK32),
#endif
		pair(SDL_PIXELFORMAT_ARGB2101010, VK_FORMAT_A2R10G10B10_UNORM_PACK32)
	};

	class TextureVk : public Texture, public Image {
	private:
		VkDescriptorPool pool = VK_NULL_HANDLE;
		VkDescriptorSet set = VK_NULL_HANDLE;
		uint sid;

		TextureVk(uvec2 size, uint samplerId) noexcept : Texture(size), sid(samplerId) {}

		friend class RendererVk;
	};

	struct InstanceInfo {
#ifndef WITH_SDL3
		SDL_Window* window;
#endif
		bool khrGetPhysicalDeviceProperties2 = false;
#ifndef NDEBUG
		bool extDebugUtils = false;
#endif
	};

	struct DeviceInfo {
		static constexpr uint8 maxQueues = 3;

		VkPhysicalDevice dev = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties prop;
		VkPhysicalDeviceMemoryProperties memp;
		VkPhysicalDevice4444FormatsFeaturesEXT formatsFeatures = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT };
		vector<string> extensions;
		array<VkSurfaceFormatKHR, 2> surfaceFormats{};
		pair<uint32, uint32> qfqcnts[maxQueues]{};						// family index, queue count
		pair<uint32, uint32> graphicsQids, presentQids, transferQids;	// family index, queue index
		uint score;
		uint8 numQfams = 0;
		bool canCompute = false;
		bool canSrgb;
		array<bool, optTexFmtMap.size()> formats;

		DeviceInfo() = default;
		DeviceInfo(VkPhysicalDevice pdev) : dev(pdev) {}
	};

	struct SurfaceInfo {
		uptr<SDL_Surface> img;
		VkFormat fmt;
		Swizzle cmap{};
		optional<FormatConverter::Pipeline> pid;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, VkFormat format, Swizzle swizzle = {}) noexcept;
		SurfaceInfo(SDL_Surface* surface, bool srgb, FormatConverter::Pipeline conv) noexcept;
	};

	static constexpr array<VkMemoryPropertyFlags, 2> deviceMemoryTypes = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
#ifndef NDEBUG
	static inline const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
#endif
	static constexpr Swizzle textSwizzle = { .r = VK_COMPONENT_SWIZZLE_ONE, .g = VK_COMPONENT_SWIZZLE_ONE, .b = VK_COMPONENT_SWIZZLE_ONE, .a = VK_COMPONENT_SWIZZLE_R };

	static constexpr array scrVertices = {
		vec2(-1.f, -1.f),
		vec2(1.f, -1.f),
		vec2(-1.f, 1.f),
		vec2(1.f, 1.f)
	};

	uptr<ViewVk[]> views;
	VkQueue gqueue = VK_NULL_HANDLE;
	VkQueue pqueue = VK_NULL_HANDLE;
	VkQueue tqueue = VK_NULL_HANDLE;
	VkCommandPool gcmdPool = VK_NULL_HANDLE;
	VkCommandPool tcmdPool = VK_NULL_HANDLE;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT dbgMessenger = VK_NULL_HANDLE;
#endif
	uint32 gfamilyIndex, pfamilyIndex, tfamilyIndex;
	RenderPass renderPass;
	FormatConverter fmtConv;
	uptr<VkSwapchainKHR[]> swapchains;	// numViews
	array<VkCommandBuffer, ViewVk::maxFrames> commandBuffers{};
	uptr<VkPipelineStageFlags[]> waitStages;		// numViews
	uptr<VkSemaphore[]> imageAvailableSemaphores;	// numViews * maxFrames
	array<VkSemaphore, ViewVk::maxFrames> renderFinishedSemaphores{};
	array<VkFence, ViewVk::maxFrames> frameFences{};
	uptr<uint32[]> imageIndices;	// numViews
	Buffer vertexBuf;

	array<VkCommandBuffer, FormatConverter::maxTransfers> tcmdBuffers{};
	array<VkFence, FormatConverter::maxTransfers> tfences{};
	array<Buffer, FormatConverter::maxTransfers> inputBufs;
	array<void*, FormatConverter::maxTransfers> inputsMapped;
	array<VkDeviceSize, FormatConverter::maxTransfers> inputSizesMax{};
	VkDeviceSize transferAtomSize;

	uint currentFrame = 0;
	VkClearValue bgColor;
	array<VkSurfaceFormatKHR, 2> surfaceFormats;	// UNORM, SRGB
	uint32 maxComputeWorkGroups;
	bool refreshFramebuffers = false;
	uint8 currentTransfer = 0;
	array<array<bool, FormatConverter::numLayouts>, FormatConverter::maxTransfers> rebindInputBuffer{};
	array<bool, FormatConverter::maxTransfers> transferRunning{};
	array<bool, optTexFmtMap.size()> optionalFormats;
	bool immediatePresent;
	bool canSrgb, usesSrgb;

public:
	RendererVk(InitParams& initParams, Settings* sets);
	~RendererVk() override;

	void setColors(array<vec4, Settings::defaultColors.size()>& colors) override;
	bool setSettings(Settings* sets) override;
	void setGammaValue(int gamma) override;
	bool updateView(ivec2& viewRes) override;
	Info getInfo() const override;

	Action beginRender() noexcept override;
	Action startDraw(uint vid) noexcept override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept override;
	Action finishDraw(uint vid) noexcept override;
	Action finishRender() noexcept override;

	Texture* texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept override;
	bool texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;
	void waitIdle() noexcept override;

protected:
	pair<SDL_PixelFormatEnum, uint8> prepareImageFormat(SDL_Surface* img) const noexcept override;

private:
	void cleanup() noexcept;
	void createInstance(InstanceInfo& instInfo);
	uptr<DeviceInfo> pickPhysicalDevice(const InstanceInfo& instInfo, u32vec2& preferred);
	void createDevice(DeviceInfo& deviceInfo);
	VkCommandPool createCommandPool(uint32 family) const;
	void createSwapchain(uint vid, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void recreateSwapchain(uint vid);
	void freeFramebuffers(ViewVk& view) noexcept;
	void initGlobalData(const Settings* sets, array<vec4, Settings::defaultColors.size()>& colors);
	void prepareColorData(array<vec4, Settings::defaultColors.size()>& colors) noexcept;
	void setUsesSrgb(Settings* sets) noexcept;
	void setCompression(Settings* sets) noexcept;

	vector<const char*> getRequiredInstanceExtensions(InstanceInfo& instInfo) const;
	bool checkImageFormats(DeviceInfo& deviceInfo) const;
	bool findQueueFamilies(DeviceInfo& deviceInfo) const;
	static void assignQueueIndices(DeviceInfo& deviceInfo, const VkQueueFamilyProperties* families, uint32 fi, optional<pair<uint32, uint32>>& qids);
	bool chooseSurfaceFormat(DeviceInfo& deviceInfo) const;
	pair<VkPresentModeKHR, uint32> chooseSwapPresentMode(VkSurfaceKHR surface, const VkSurfaceCapabilitiesKHR& capabilities) const;
	static uint scoreDevice(const DeviceInfo& devi);
	void uploadBuffer(VkBuffer buffer, const void* data, VkDeviceSize dstOffs, VkDeviceSize size, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
	void copyInputBuffer(VkBuffer buffer, VkDeviceSize dstOffs, VkDeviceSize size, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
	void uploadTextureDirect(VkImage img, u32vec2 res, const void* pix, uint pitch, uint8 bpp);
	void uploadTextureIndirect(VkImage img, u32vec2 res, const SurfaceInfo& si);
	void checkInputBufferSize(VkDeviceSize size);
	void replaceTexture(TextureVk& tex, Image& vki, uvec2 res) noexcept;
	SurfaceInfo pickPixFormat(SDL_Surface* img, bool srgb) const noexcept;
	pair<VkFormat, Swizzle> pickPixFormat(SDL_PixelFormatEnum sfmt, std::initializer_list<OptTexFmt> fmtv) const noexcept;
	static Swizzle swizzlePixFormat(SDL_PackedOrder spo, SDL_PackedOrder dpo) noexcept;
	pair<SDL_PixelFormatEnum, uint8> pickImageFormat(std::initializer_list<OptTexFmt> fmtv, SDL_PixelFormatEnum orig) const noexcept;
	bool canTexturesB16() const noexcept;

	void beginTransferCommands();
	void endTransferCommands();
	void syncTransferCommands();
	void transitionBuffer(VkBuffer buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32 srcQfamily, uint32 dstQfamily) const noexcept;
	void transitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32 srcQfamily = VK_QUEUE_FAMILY_IGNORED, uint32 dstQfamily = VK_QUEUE_FAMILY_IGNORED) const noexcept;
	void transitionBufferToImageLayout(VkBuffer buffer, VkImage image) const noexcept;
	void copyBufferToImage(VkBuffer buffer, VkImage image, u32vec2 size) const noexcept;

#ifndef NDEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) noexcept;
#endif
};

inline bool RendererVk::canTexturesB16() const noexcept {
	return optionalFormats[eint(OptTexFmt::B5G6R5)] || optionalFormats[eint(OptTexFmt::R5G6B5)] || optionalFormats[eint(OptTexFmt::A1R5G5B5)] || optionalFormats[eint(OptTexFmt::B5G5R5A1)] || optionalFormats[eint(OptTexFmt::R5G5B5A1)];
}
#endif
