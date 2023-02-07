#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#include <vulkan/vulkan.h>

class RendererVk;

class GenericPass {
protected:
	VkRenderPass handle = VK_NULL_HANDLE;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

public:
	VkRenderPass getHandle() const;
	VkPipelineLayout getPipelineLayout() const;
	VkPipeline getPipeline() const;

protected:
	static VkSampler createSampler(VkDevice dev, VkFilter filter);
	static VkShaderModule createShaderModule(VkDevice dev, const uint32* code, sizet clen);
};

inline VkRenderPass GenericPass::getHandle() const {
	return handle;
}

inline VkPipelineLayout GenericPass::getPipelineLayout() const {
	return pipelineLayout;
}

inline VkPipeline GenericPass::getPipeline() const {
	return pipeline;
}

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
	pair<VkDescriptorPool, VkDescriptorSet> newDescriptorSetTex(VkDevice dev);
	void freeDescriptorSetTex(VkDevice dev, VkDescriptorPool pool, VkDescriptorSet dset);
	static void updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer);
	static void updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkImageView imageView);
	void free(VkDevice dev);

private:
	void createRenderPass(VkDevice dev, VkFormat format);
	void createDescriptorSetLayout(VkDevice dev);
	void createPipeline(VkDevice dev);
	vector<VkDescriptorSet> createDescriptorPoolAndSets(VkDevice dev, uint32 numViews);
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

	VkDescriptorSet getDescriptorSet() const;
	UniformData* getUniformBufferMapped() const;

private:
	void createRenderPass(VkDevice dev);
	void createDescriptorSetLayout(VkDevice dev);
	void createPipeline(VkDevice dev);
	void createUniformBuffer(const RendererVk* rend);
	void createDescriptorPoolAndSet(VkDevice dev);
	void updateDescriptorSet(VkDevice dev);
};

inline VkDescriptorSet AddressPass::getDescriptorSet() const {
	return descriptorSet;
}

inline AddressPass::UniformData* AddressPass::getUniformBufferMapped() const {
	return uniformBufferMapped;
}

class RendererVk : public Renderer {
private:
	static constexpr array<const char*, 1> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	static constexpr array<VkMemoryPropertyFlags, 2> deviceMemoryTypes = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
#ifndef NDEBUG
	static constexpr array<const char*, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };
#endif

	class TextureVk : public Texture {
	private:
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkDescriptorPool pool;
		VkDescriptorSet set;
		uint sid;

		TextureVk(ivec2 size, VkImage img, VkDeviceMemory mem, VkImageView imageView, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet, uint samplerId);

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
	VkCommandPool cmdPool = VK_NULL_HANDLE;
#ifndef NDEBUG
	VkDebugUtilsMessengerEXT dbgMessenger = VK_NULL_HANDLE;
#endif
	VkFence singleTimeFence = VK_NULL_HANDLE;
	uint32 gfamilyIndex, pfamilyIndex;
	RenderPass renderPass;
	AddressPass addressPass;

	VkImage addrImage = VK_NULL_HANDLE;
	VkDeviceMemory addrImageMemory = VK_NULL_HANDLE;
	VkImageView addrView = VK_NULL_HANDLE;
	VkFramebuffer addrFramebuffer = VK_NULL_HANDLE;
	VkBuffer addrBuffer = VK_NULL_HANDLE;
	VkDeviceMemory addrBufferMemory = VK_NULL_HANDLE;
	u32vec2* addrMappedMemory;
	VkCommandBuffer commandBufferAddr = VK_NULL_HANDLE;
	VkFence addrFence = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties pdevProperties;
	VkPhysicalDeviceMemoryProperties pdevMemProperties;
	ViewVk* currentView;
	uint currentFrame = 0;
	uint32 imageIndex;
	VkClearValue bgColor;
	VkPresentModeKHR presentMode;
	bool refreshFramebuffer = false;

public:
	RendererVk(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor);
	~RendererVk() final;

	void setClearColor(const vec4& color) final;
	void setVsync(bool vsync) final;
	void updateView(ivec2& viewRes) final;
	void getAdditionalSettings(bool& compression, vector<pair<u32vec2, string>>& devices) final;

	void startDraw(View* view) final;
	void drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) final;
	void finishDraw(View* view) final;
	void finishRender() final;

	void startSelDraw(View* view, ivec2 pos) final;
	void drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) final;
	Widget* finishSelDraw(View* view) final;

	Texture* texFromImg(SDL_Surface* img) final;
	Texture* texFromText(SDL_Surface* img) final;
	void freeTexture(Texture* tex) final;

	VkDevice getLogicalDevice() const;
	pair<VkImage, VkDeviceMemory> createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;
	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format) const;
	VkFramebuffer createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const;
	pair<VkBuffer, VkDeviceMemory> createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const;
	uint32 findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const;
	void allocateCommandBuffers(VkCommandBuffer* cmdBuffers, uint32 count) const;
	void freeCommandBuffers(VkCommandBuffer* cmdBuffers, uint32 count) const;
	VkSemaphore createSemaphore() const;
	VkFence createFence(VkFenceCreateFlags flags = 0) const;

	VkCommandBuffer beginSingleTimeCommands() const;
	void beginSingleTimeCommands(VkCommandBuffer commandBuffer) const;
	void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
	void submitSingleTimeCommands(VkCommandBuffer commandBuffer) const;
	template <VkImageLayout srcLay, VkImageLayout dstLay> static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image);
	static void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size, uint32 pitch);
	static void copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size);

private:
	void createInstance(SDL_Window* window);
	void pickPhysicalDevice(u32vec2& preferred);
	void createDevice();
	void createCommandPool();
	VkFormat createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void freeFramebuffers(ViewVk* view);
	void recreateSwapchain(ViewVk* view);
	void createFramebuffers(ViewVk* view);
	void createUniformBuffer(ViewVk* view);
	void setPresentMode(bool vsync);

#ifdef NDEBUG
	static vector<const char*> getRequiredExtensions(SDL_Window* win);
#else
	static vector<const char*> getRequiredExtensions(SDL_Window* win, bool validation);
#endif
	tuple<uint32, uint32, bool> findQueueFamilies(VkPhysicalDevice dev) const;
	static vector<VkSurfaceFormatKHR> querySurfaceFormatSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);
	static vector<VkPresentModeKHR> queryPresentModeSupport(VkPhysicalDevice dev, VkSurfaceKHR surf);
	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes) const;
	static uint scoreDevice(const VkPhysicalDeviceProperties& prop, const VkPhysicalDeviceMemoryProperties& memp);
	TextureVk* createTexture(SDL_Surface* img, u32vec2 res, VkFormat format, bool nearest);
	pair<SDL_Surface*, VkFormat> pickPixFormat(SDL_Surface* img) const;
#ifndef NDEBUG
	static bool checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif
};

inline VkDevice RendererVk::getLogicalDevice() const {
	return ldev;
}

inline void RendererVk::freeCommandBuffers(VkCommandBuffer* cmdBuffers, uint32 count) const {
	vkFreeCommandBuffers(ldev, cmdPool, count, cmdBuffers);
}
#endif
