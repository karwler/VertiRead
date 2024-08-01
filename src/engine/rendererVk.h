#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#include <vulkan/vulkan.h>
#include <set>

class InstanceVk {
public:
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
	bool functionsInitialized() const { return vkWaitForFences; }

public:
	VkDevice getLdev() const { return ldev; }

	pair<VkBuffer, VkDeviceMemory> createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	void recreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
protected:
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const;
};

class GenericPipeline {
protected:
	static VkSampler createSampler(const InstanceVk* vk, VkFilter filter);
	static VkShaderModule createShaderModule(const InstanceVk* vk, const uint32* code, size_t clen);
};

class FormatConverter : public GenericPipeline {
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

	array<VkBuffer, maxTransfers> uniformBuffers{};
	array<VkDeviceMemory, maxTransfers> uniformBufferMemory{};
	array<UniformData*, maxTransfers> uniformBufferMapped;

public:
	void init(const InstanceVk* vk);
	void updateDescriptorSet(const InstanceVk* vk, VkDescriptorSet dset, VkImageView view, VkBuffer inputBuffer, VkDeviceSize inputSize) noexcept;
	void free(const InstanceVk* vk) noexcept;

	bool initialized() const { return descriptorSets[0]; }	// cause it's the last thing to be initialized
	VkPipeline getPipeline(Pipeline pid) const { return pipelines[eint(pid)]; }
	VkPipelineLayout getPipelineLayout(Pipeline pid) const;
	pair<VkDescriptorSet, uint> getDescriptorSet(Pipeline pid, uint id) const;
	UniformData* getUniformBufferMapped(uint id) const { return uniformBufferMapped[id]; }

private:
	void createDescriptorSetLayoutRgb(const InstanceVk* vk);
	void createDescriptorSetLayoutIdx(const InstanceVk* vk);
	void createPipelines(const InstanceVk* vk);
	void createDescriptorPoolAndSets(const InstanceVk* vk);
};

inline VkPipelineLayout FormatConverter::getPipelineLayout(Pipeline pid) const {
	return pid != Pipeline::index8 ? pipelineLayoutRgb : pipelineLayoutIdx;
}

inline pair<VkDescriptorSet, uint> FormatConverter::getDescriptorSet(Pipeline pid, uint id) const {
	return pid != Pipeline::index8 ? pair(descriptorSets[id], 0) : pair(descriptorSets[maxTransfers + id], 1);
}

class RenderPass : public GenericPipeline {
public:
	struct PushData {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
		alignas(16) vec4 color;
		alignas(4) uint sid;
	};

	struct UniformData {
		alignas(16) vec4 pview;
	};

	static constexpr uint32 dsetView = 0;
	static constexpr uint32 dsetModel = 1;
	static constexpr uint samplerLinear = 0;
	static constexpr uint samplerNearest = 1;
private:
	static constexpr uint32 bindingPview = 0;
	static constexpr uint32 bindingSampler = 1;
	static constexpr uint32 bindingTexture = 0;
	static constexpr uint32 textureSetStep = 128;

	struct DescriptorSetBlock {
		uset<VkDescriptorSet> used;
		uset<VkDescriptorSet> free;

		DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets);
	};

	VkRenderPass handle = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	array<VkDescriptorSetLayout, 2> descriptorSetLayouts{};
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	umap<VkDescriptorPool, DescriptorSetBlock> poolSetTex;
	array<VkSampler, 2> samplers{};

public:
	vector<VkDescriptorSet> init(const InstanceVk* vk, VkFormat format, uint32 numViews);
	VkRenderPass getHandle() const { return handle; }
	VkPipeline getPipeline() const { return pipeline; }
	VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
	pair<VkDescriptorPool, VkDescriptorSet> newDescriptorSetTex(const InstanceVk* vk, VkImageView imageView);
	pair<VkDescriptorPool, VkDescriptorSet> getDescriptorSetTex(const InstanceVk* vk);
	void freeDescriptorSetTex(const InstanceVk* vk, VkDescriptorPool pool, VkDescriptorSet dset);
	static void updateDescriptorSetBuf(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer) noexcept;
	static void updateDescriptorSetImg(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkImageView imageView) noexcept;
	void free(const InstanceVk* vk);

private:
	void createRenderPass(const InstanceVk* vk, VkFormat format);
	void createDescriptorSetLayout(const InstanceVk* vk);
	void createPipeline(const InstanceVk* vk);
	vector<VkDescriptorSet> createDescriptorPoolAndSets(const InstanceVk* vk, uint32 numViews);
};

class RendererVk final : public Renderer, public InstanceVk {
private:
	enum class OptionalTextureFormat : uint8 {
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
		A2R10G10B10,
		B16G16R16,
		A16B16G16R16
	};

	class TextureVk : public Texture {
	private:
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkDescriptorPool pool = VK_NULL_HANDLE;
		VkDescriptorSet set = VK_NULL_HANDLE;
		uint sid;

		TextureVk(uvec2 size, uint samplerId) : Texture(size), sid(samplerId) {}
		TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet) noexcept;
		TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet, uint samplerId) noexcept;

		friend class RendererVk;
	};

	struct ViewVk : View {
		static constexpr uint maxFrames = 2;

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkExtent2D extent{};
		uptr<VkImage[]> images;
		uptr<pair<VkImageView, VkFramebuffer>[]> framebuffers;
		uint32 imageCount = 0;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkBuffer uniformBuffer = VK_NULL_HANDLE;
		VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
		RenderPass::UniformData* uniformMapped;

		array<VkCommandBuffer, maxFrames> commandBuffers{};
		array<VkSemaphore, maxFrames> imageAvailableSemaphores{};
		array<VkSemaphore, maxFrames> renderFinishedSemaphores{};
		array<VkFence, maxFrames> frameFences{};

		using View::View;
	};

	struct InstanceInfo {
		bool khrGetPhysicalDeviceProperties2 = false;
#ifndef NDEBUG
		bool extDebugUtils = false;
#endif
	};

	struct DeviceInfo {
		VkPhysicalDevice dev = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties prop;
		VkPhysicalDeviceMemoryProperties memp;
		VkPhysicalDevice4444FormatsFeaturesEXT formatsFeatures;
		std::set<string> extensions;
		VkSurfaceFormatKHR surfaceFormat;
		umap<uint32, u32vec2> qfIdCnt;
		uint32 gfam, pfam, tfam;
		uint score;
		bool canCompute;
		array<bool, eint(OptionalTextureFormat::A16B16G16R16) + 1> formats;
	};

	struct Swizzle {
		uint8 r, g, b, a;
	};

	struct SurfaceInfo {
		SDL_Surface* img = nullptr;
		VkFormat fmt;
		uint8 use;
		Swizzle cmap;
		FormatConverter::Pipeline pid;
		bool direct;

		SurfaceInfo() = default;
		SurfaceInfo(SDL_Surface* surface, VkFormat format, Swizzle swizzle = {});
		SurfaceInfo(SDL_Surface* surface, FormatConverter::Pipeline conv);
	};

	static constexpr array<VkMemoryPropertyFlags, 2> deviceMemoryTypes = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
#ifndef NDEBUG
	static inline const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
#endif
	static constexpr uint32 maxPossibleQueues = 3;

	InstanceVk vk;
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

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

	array<VkCommandBuffer, FormatConverter::maxTransfers> tcmdBuffers{};
	array<VkFence, FormatConverter::maxTransfers> tfences{};
	array<VkBuffer, FormatConverter::maxTransfers> inputBuffers{};
	array<VkDeviceMemory, FormatConverter::maxTransfers> inputMemory{};
	array<byte_t*, FormatConverter::maxTransfers> inputsMapped;
	array<VkDeviceSize, FormatConverter::maxTransfers> inputSizesMax{};
	VkDeviceSize transferAtomSize;

	ViewVk* currentView;
	uint currentFrame = 0;
	uint32 imageIndex;
	VkClearValue bgColor;
	VkSurfaceFormatKHR surfaceFormat;
	uint currentTransfer = 0;
	uint32 maxComputeWorkGroups;
	bool refreshFramebuffer = false;
	array<array<bool, FormatConverter::numLayouts>, FormatConverter::maxTransfers> rebindInputBuffer{};
	array<bool, eint(OptionalTextureFormat::A16B16G16R16) + 1> optionalFormats;
	bool immediatePresent;

public:
	RendererVk(const vector<SDL_Window*>& windows, const ivec2* vofs, ivec2& viewRes, Settings* sets, const vec4& bgcolor);
	~RendererVk() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression cmpr) noexcept override;
	SDL_Surface* prepareImage(SDL_Surface* img, bool rpic) const noexcept override;
	Info getInfo() const noexcept override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;
	void finishDraw(View* view) override;
	void finishRender() override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) noexcept override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) noexcept override;
	Texture* texFromRpic(SDL_Surface* img) noexcept override;
	Texture* texFromText(const Pixmap& pm) noexcept override;
	bool texFromText(Texture* tex, const Pixmap& pm) noexcept override;
	void freeTexture(Texture* tex) noexcept override;
	void synchTransfer() noexcept override;

	pair<VkImage, VkDeviceMemory> createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;
	VkImageView createImageView(VkImage image, VkFormat format, Swizzle swizzle = {}) const;
	VkFramebuffer createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const;
	void allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const;
	VkSemaphore createSemaphore() const;
	VkFence createFence(VkFenceCreateFlags flags = 0) const;

	void beginSingleTimeCommands(VkCommandBuffer cmdBuffer);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence, VkQueue queue) const;
	void synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const noexcept;
	void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const noexcept;
	void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size) const noexcept;

private:
	void cleanup() noexcept;
	InstanceInfo createInstance(SDL_Window* window);
	uptr<DeviceInfo> pickPhysicalDevice(const InstanceInfo& instanceInfo, u32vec2& preferred);
	void createDevice(DeviceInfo& deviceInfo);
	VkCommandPool createCommandPool(uint32 family) const;
	void createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void freeFramebuffers(ViewVk* view) noexcept;
	void recreateSwapchain(ViewVk* view);
	void initView(ViewVk* view, VkDescriptorSet descriptorSet);
	void freeView(ViewVk* view) noexcept;
	void createFramebuffers(ViewVk* view);
	void createVertexBuffer();

	vector<const char*> getRequiredInstanceExtensions(const InstanceInfo& instanceInfo, SDL_Window* win) const;
	bool checkImageFormats(DeviceInfo& deviceInfo) const;
	bool findQueueFamilies(DeviceInfo& deviceInfo) const;
	pair<uint32, VkQueue> acquireNextQueue(DeviceInfo& deviceInfo, uint32 family) const;
	bool chooseSurfaceFormat(DeviceInfo& deviceInfo) const;
	VkPresentModeKHR chooseSwapPresentMode(VkSurfaceKHR surface) const;
	static uint scoreDevice(const DeviceInfo& devi);
	void createTexture(const SurfaceInfo& si, TextureVk& tex);
	void createTexture(const Pixmap& pm, TextureVk& tex);
	uint32 prepareInputBuffer(u32vec2 size, uint8 bpp);
	void uploadTextureDirect(const TextureVk& tex);
	void uploadTextureIndirect(const TextureVk& tex, VkDescriptorSet dset, FormatConverter::Pipeline pid);
	void finalizeFreshTexture(TextureVk& tex);
	void finalizeExistingTexture(TextureVk& tex);
	void cleanupTexture(TextureVk& tex) noexcept;
	void replaceTexture(TextureVk& tex, TextureVk& ntex) noexcept;
	SurfaceInfo pickPixFormat(SDL_Surface* img) const noexcept;
	InstanceInfo checkInstanceExtensionSupport() const;
#ifndef NDEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) noexcept;
#endif
};
#endif
