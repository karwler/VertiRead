#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#include <vulkan/vulkan.h>

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
	PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
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
	PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
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
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

public:
	VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

protected:
	static VkSampler createSampler(const InstanceVk* vk, VkFilter filter);
	static VkShaderModule createShaderModule(const InstanceVk* vk, const uint32* code, size_t clen);
};

class FormatConverter : public GenericPipeline {
public:
	static constexpr uint maxTransfers = 2;
	static constexpr uint32 convWgrpSize = 32;
	static constexpr uint32 convStep = convWgrpSize * 4;	// 4 texels per invocation

	struct PushData {
		alignas(4) uint offset;
	};

private:
	struct SpecializationData {
		VkBool32 orderRgb;
	};

	array<VkPipeline, 2> pipelines{};
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	array<VkDescriptorSet, maxTransfers> descriptorSets{};

	array<VkBuffer, maxTransfers> outputBuffers{};
	array<VkDeviceMemory, maxTransfers> outputMemory{};
	array<VkDeviceSize, maxTransfers> outputSizesMax{};

public:
	void init(const InstanceVk* vk);
	void updateBufferSize(const InstanceVk* vk, uint id, VkDeviceSize texSize, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update);
	void free(const InstanceVk* vk);

	bool initialized() const { return descriptorSets[maxTransfers - 1]; }	// cause it's the last thing to be initialized
	VkPipeline getPipeline(bool rgb) const { return pipelines[rgb]; }
	VkDescriptorSet getDescriptorSet(uint id) const { return descriptorSets[id]; }
	VkBuffer getOutputBuffer(uint id) const { return outputBuffers[id]; }

private:
	void createDescriptorSetLayout(const InstanceVk* vk);
	void createPipelines(const InstanceVk* vk);
	void createDescriptorPoolAndSet(const InstanceVk* vk);
};

class GenericPass : public GenericPipeline {
protected:
	VkRenderPass handle = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

public:
	VkRenderPass getHandle() const { return handle; }
	VkPipeline getPipeline() const { return pipeline; }
};

class RenderPass : public GenericPass {
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

	static constexpr uint samplerLinear = 0;
	static constexpr uint samplerNearest = 1;
private:
	static constexpr uint32 textureSetStep = 128;

	struct DescriptorSetBlock {
		uset<VkDescriptorSet> used;
		uset<VkDescriptorSet> free;

		DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets);
	};

	array<VkDescriptorSetLayout, 2> descriptorSetLayouts{};
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	umap<VkDescriptorPool, DescriptorSetBlock> poolSetTex;
	array<VkSampler, 2> samplers{};

public:
	vector<VkDescriptorSet> init(const InstanceVk* vk, VkFormat format, uint32 numViews);
	pair<VkDescriptorPool, VkDescriptorSet> newDescriptorSetTex(const InstanceVk* vk, VkImageView imageView);
	pair<VkDescriptorPool, VkDescriptorSet> getDescriptorSetTex(const InstanceVk* vk);
	void freeDescriptorSetTex(const InstanceVk* vk, VkDescriptorPool pool, VkDescriptorSet dset);
	static void updateDescriptorSet(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer);
	static void updateDescriptorSet(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkImageView imageView);
	void free(const InstanceVk* vk);

private:
	void createRenderPass(const InstanceVk* vk, VkFormat format);
	void createDescriptorSetLayout(const InstanceVk* vk);
	void createPipeline(const InstanceVk* vk);
	vector<VkDescriptorSet> createDescriptorPoolAndSets(const InstanceVk* vk, uint32 numViews);
};

class AddressPass : public GenericPass {
public:
	static constexpr uint32 bindingUdat = 0;
	static constexpr VkFormat format = VK_FORMAT_R32G32_UINT;

	struct PushData {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
		alignas(8) uvec2 addr;
	};

	struct UniformData {
		alignas(16) vec4 pview;
	};

private:
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	VkBuffer uniformBuffer = VK_NULL_HANDLE;
	VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
	UniformData* uniformBufferMapped;

public:
	void init(const InstanceVk* vk);
	void free(const InstanceVk* vk);

	VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
	UniformData* getUniformBufferMapped() const { return uniformBufferMapped; }

private:
	void createRenderPass(const InstanceVk* vk);
	void createDescriptorSetLayout(const InstanceVk* vk);
	void createPipeline(const InstanceVk* vk);
	void createUniformBuffer(const InstanceVk* vk);
	void createDescriptorPoolAndSet(const InstanceVk* vk);
	void updateDescriptorSet(const InstanceVk* vk);
};

class RendererVk final : public Renderer, public InstanceVk {
private:
	enum class OptionalTextureFormat {
		B5G6R5,
		R5G6B5,
		A1R5G5B5,
		B5G5R5A1,
		R5G5B5A1,
		A4B4G4R4,
		A4R4G4B4,
		B4G4R4A4,
		R4G4B4A4,
		A2R10G10B10
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
		array<bool, size_t(OptionalTextureFormat::A2R10G10B10) + 1> formats;
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
	AddressPass addressPass;
	FormatConverter fmtConv;

	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

	VkImage addrImage = VK_NULL_HANDLE;
	VkDeviceMemory addrImageMemory = VK_NULL_HANDLE;
	VkImageView addrView = VK_NULL_HANDLE;
	VkFramebuffer addrFramebuffer = VK_NULL_HANDLE;
	VkBuffer addrBuffer = VK_NULL_HANDLE;
	VkDeviceMemory addrBufferMemory = VK_NULL_HANDLE;
	u32vec2* addrMappedMemory;
	VkCommandBuffer commandBufferAddr = VK_NULL_HANDLE;
	VkFence addrFence = VK_NULL_HANDLE;

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
	array<bool, FormatConverter::maxTransfers> rebindInputBuffer{};
	array<bool, size_t(OptionalTextureFormat::A2R10G10B10) + 1> optionalFormats;
	bool immediatePresent;

public:
	RendererVk(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererVk() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression compression) override;
	Info getInfo() const override;

	void startDraw(View* view) override;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) override;
	void finishDraw(View* view) override;
	void finishRender() override;

	void startSelDraw(View* view, ivec2 pos) override;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) override;
	Widget* finishSelDraw(View* view) override;

	Texture* texFromEmpty() override;
	Texture* texFromIcon(SDL_Surface* img) override;
	bool texFromIcon(Texture* tex, SDL_Surface* img) override;
	Texture* texFromRpic(SDL_Surface* img) override;
	Texture* texFromText(const PixmapRgba& pm) override;
	bool texFromText(Texture* tex, const PixmapRgba& pm) override;
	void freeTexture(Texture* tex) noexcept override;
	void synchTransfer() override;

	pair<VkImage, VkDeviceMemory> createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;
	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format) const;
	VkFramebuffer createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const;
	void allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const;
	VkSemaphore createSemaphore() const;
	VkFence createFence(VkFenceCreateFlags flags = 0) const;

	void beginSingleTimeCommands(VkCommandBuffer cmdBuffer);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence, VkQueue queue) const;
	void synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const;
	template <VkImageLayout srcLay, VkImageLayout dstLay> void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image) const;
	template <VkImageLayout srcLay, VkImageLayout dstLay> void transitionBufferToImageLayout(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image) const;
	void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size) const;
	void copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size) const;

private:
	void cleanup() noexcept;
	InstanceInfo createInstance(SDL_Window* window);
	uptr<DeviceInfo> pickPhysicalDevice(const InstanceInfo& instanceInfo, u32vec2& preferred);
	void createDevice(DeviceInfo& deviceInfo);
	VkCommandPool createCommandPool(uint32 family) const;
	void createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void freeFramebuffers(ViewVk* view);
	void recreateSwapchain(ViewVk* view);
	void initView(ViewVk* view, VkDescriptorSet descriptorSet);
	void freeView(ViewVk* view);
	void createFramebuffers(ViewVk* view);
	void createVertexBuffer();

	vector<const char*> getRequiredInstanceExtensions(const InstanceInfo& instanceInfo, SDL_Window* win) const;
	bool checkImageFormats(DeviceInfo& deviceInfo) const;
	bool findQueueFamilies(DeviceInfo& deviceInfo) const;
	pair<uint32, VkQueue> acquireNextQueue(DeviceInfo& deviceInfo, uint32 family) const;
	bool chooseSurfaceFormat(DeviceInfo& deviceInfo) const;
	VkPresentModeKHR chooseSwapPresentMode(VkSurfaceKHR surface) const;
	static uint scoreDevice(const DeviceInfo& devi);
	template <bool fresh = true> void createTextureDirect(const byte_t* pix, uint32 pitch, uint8 bpp, VkFormat format, TextureVk& tex);
	template <bool fresh = true> void createTextureIndirect(const SDL_Surface* img, VkFormat format, TextureVk& tex);
	template <bool conv> void uploadInputData(const byte_t* pix, u32vec2 res, uint32 pitch, uint8 bpp);
	template <bool fresh> void finalizeTexture(TextureVk& tex, VkFormat format);
	void replaceTexture(TextureVk& tex, TextureVk& ntex);
	tuple<SDL_Surface*, VkFormat, bool> pickPixFormat(SDL_Surface* img) const noexcept;
	bool canSquashTextures() const;
	InstanceInfo checkInstanceExtensionSupport() const;
#ifndef NDEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif
};

inline bool RendererVk::canSquashTextures() const {
	return optionalFormats[eint(OptionalTextureFormat::B5G6R5)] && optionalFormats[eint(OptionalTextureFormat::R5G6B5)] && optionalFormats[eint(OptionalTextureFormat::A1R5G5B5)] && optionalFormats[eint(OptionalTextureFormat::B5G5R5A1)] && optionalFormats[eint(OptionalTextureFormat::R5G5B5A1)];
}
#endif
