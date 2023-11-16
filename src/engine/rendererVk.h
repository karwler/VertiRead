#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#include <vulkan/vulkan.h>

class RendererVk;

class GenericPipeline {
protected:
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

public:
	VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

protected:
	static VkSampler createSampler(const RendererVk* rend, VkFilter filter);
	static VkShaderModule createShaderModule(const RendererVk* rend, const uint32* code, size_t clen);
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
	void init(const RendererVk* rend);
	void updateBufferSize(const RendererVk* rend, uint id, VkDeviceSize texSize, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update);
	void free(VkDevice dev);

	bool initialized() const { return descriptorSets[maxTransfers - 1]; }	// cause it's the last thing to be initialized
	VkPipeline getPipeline(bool rgb) const { return pipelines[rgb]; }
	VkDescriptorSet getDescriptorSet(uint id) const { return descriptorSets[id]; }
	VkBuffer getOutputBuffer(uint id) const { return outputBuffers[id]; }

private:
	void createDescriptorSetLayout(const RendererVk* rend);
	void createPipelines(const RendererVk* rend);
	void createDescriptorPoolAndSet(const RendererVk* rend);
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
	vector<VkDescriptorSet> init(const RendererVk* rend, VkFormat format, uint32 numViews);
	pair<VkDescriptorPool, VkDescriptorSet> newDescriptorSetTex(const RendererVk* rend, VkImageView imageView);
	pair<VkDescriptorPool, VkDescriptorSet> getDescriptorSetTex(const RendererVk* rend);
	void freeDescriptorSetTex(VkDevice dev, VkDescriptorPool pool, VkDescriptorSet dset);
	static void updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer);
	static void updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkImageView imageView);
	void free(VkDevice dev);

private:
	void createRenderPass(const RendererVk* rend, VkFormat format);
	void createDescriptorSetLayout(const RendererVk* rend);
	void createPipeline(const RendererVk* rend);
	vector<VkDescriptorSet> createDescriptorPoolAndSets(const RendererVk* rend, uint32 numViews);
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
	void init(const RendererVk* rend);
	void free(VkDevice dev);

	VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
	UniformData* getUniformBufferMapped() const { return uniformBufferMapped; }

private:
	void createRenderPass(const RendererVk* rend);
	void createDescriptorSetLayout(const RendererVk* rend);
	void createPipeline(const RendererVk* rend);
	void createUniformBuffer(const RendererVk* rend);
	void createDescriptorPoolAndSet(const RendererVk* rend);
	void updateDescriptorSet(VkDevice dev);
};

class RendererVk final : public Renderer {
private:
	static constexpr array<const char*, 1> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	static constexpr array<VkMemoryPropertyFlags, 2> deviceMemoryTypes = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
#ifndef NDEBUG
	static constexpr array<const char*, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	static constexpr array<VkValidationFeatureEnableEXT, 2> validationFeatureEnables = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT };
#endif
	static constexpr uint32 maxPossibleQueues = 3;

	struct QueueInfo {
		umap<uint32, u32vec2> idcnt;
		optional<uint32> gfam, pfam, tfam;
		bool canCompute = false;
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
		TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet);
		TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet, uint samplerId);

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

	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice pdev = VK_NULL_HANDLE;
	VkDevice ldev = VK_NULL_HANDLE;
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

	VkPhysicalDeviceProperties pdevProperties;
	VkPhysicalDeviceMemoryProperties pdevMemProperties;
	ViewVk* currentView;
	uint currentFrame = 0;
	uint32 imageIndex;
	VkClearValue bgColor;
	VkPresentModeKHR presentMode;
	uint currentTransfer = 0;
	bool refreshFramebuffer = false;
	array<bool, FormatConverter::maxTransfers> rebindInputBuffer{};
	bool squashPicTexels;

public:
	RendererVk(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererVk() override;

	void setClearColor(const vec4& color) override;
	void setVsync(bool vsync) override;
	void updateView(ivec2& viewRes) override;
	void setCompression(Settings::Compression compression) override;
	pair<uint, Settings::Compression> getSettings(vector<pair<u32vec2, string>>& devices) const override;

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
	void freeTexture(Texture* tex) override;
	void synchTransfer() override;

	VkDevice getLogicalDevice() const { return ldev; }
	pair<VkImage, VkDeviceMemory> createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;
	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format) const;
	VkFramebuffer createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const;
	void recreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	pair<VkBuffer, VkDeviceMemory> createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const;
	void allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const;
	VkSemaphore createSemaphore() const;
	VkFence createFence(VkFenceCreateFlags flags = 0) const;

	static void beginSingleTimeCommands(VkCommandBuffer cmdBuffer);
	void endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence, VkQueue queue) const;
	void synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const;
	template <VkImageLayout srcLay, VkImageLayout dstLay> static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image);
	template <VkImageLayout srcLay, VkImageLayout dstLay> static void transitionBufferToImageLayout(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image);
	static void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size, uint32 pitch = 0);
	static void copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size);

protected:
	uint maxTexSize() const override;
	const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* getSquashableFormats() const override;

private:
	void createInstance(SDL_Window* window);
	QueueInfo pickPhysicalDevice(u32vec2& preferred);
	void createDevice(QueueInfo& queueInfo);
	VkCommandPool createCommandPool(uint32 family) const;
	VkFormat createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void freeFramebuffers(ViewVk* view);
	void recreateSwapchain(ViewVk* view);
	void initView(ViewVk* view, VkDescriptorSet descriptorSet);
	void freeView(ViewVk* view);
	void createFramebuffers(ViewVk* view);
	void setPresentMode(bool vsync);

#ifdef NDEBUG
	static vector<const char*> getRequiredExtensions(SDL_Window* win);
#else
	static vector<const char*> getRequiredExtensions(SDL_Window* win, bool debugUtils, bool validationFeatures);
#endif
	QueueInfo findQueueFamilies(VkPhysicalDevice dev) const;
	pair<uint32, VkQueue> acquireNextQueue(QueueInfo& queueInfo, uint32 family) const;
	static vector<VkSurfaceFormatKHR> querySurfaceFormatSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);
	static vector<VkPresentModeKHR> queryPresentModeSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes) const;
	static uint scoreDevice(const VkPhysicalDeviceProperties& prop, const VkPhysicalDeviceMemoryProperties& memp);
	template <bool fresh = true> void createTextureDirect(const byte_t* pix, uint32 pitch, uint8 bpp, VkFormat format, TextureVk& tex);
	template <bool fresh = true> void createTextureIndirect(const SDL_Surface* img, VkFormat format, TextureVk& tex);
	template <bool conv> void uploadInputData(const byte_t* pix, u32vec2 res, uint32 pitch, uint8 bpp);
	template <bool fresh> void finalizeTexture(TextureVk& tex, VkFormat format);
	void replaceTexture(TextureVk& tex, TextureVk& ntex);
	tuple<SDL_Surface*, VkFormat, bool> pickPixFormat(SDL_Surface* img) const;
#ifndef NDEBUG
	static pair<bool, bool> checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif
};
#endif
