#pragma once

#ifdef WITH_VULKAN
#include "renderer.h"
#include <vulkan/vulkan.h>

class RendererVk;

struct TexLoc {
	uint tid;
	Rectu rct;

	TexLoc() = default;
	constexpr TexLoc(uint page, const Rectu& posize);

	constexpr bool empty() const;
};

constexpr TexLoc::TexLoc(uint page, const Rectu& posize) :
	tid(page),
	rct(posize)
{}

constexpr bool TexLoc::empty() const {
	return rct.w == 0 || rct.h == 0;
}

class TextureColConst {
protected:
	static constexpr VkDeviceSize bpp = 4;
	static constexpr VkFormat iformat = VK_FORMAT_A8B8G8R8_UNORM_PACK32;
	static constexpr SDL_PixelFormatEnum pformat = SDL_PIXELFORMAT_RGBA32;

	VkImage texImg = VK_NULL_HANDLE;
	VkDeviceMemory texMem = VK_NULL_HANDLE;
	VkImageView texView = VK_NULL_HANDLE;
	u32vec2 res;

private:
	static constexpr uint32 pad = 2;

public:
	vector<pair<sizet, TexLoc>> init(const RendererVk* rend, vector<pair<sizet, SDL_Surface*>>&& imgs, bool addBlank);
	void free(VkDevice dev);

	u32vec2 getRes() const;
	VkImageView getTexView() const;

protected:
	static VkBufferImageCopy makeRegion(VkDeviceSize offset, uint32 pitch, int32 x, int32 y, uint32 w, uint32 h, uint32 page);
	static void packCopy(uint32* dst, const uint32* src, u32vec2 size, uint32 pitch);
};

inline u32vec2 TextureColConst::getRes() const {
	return res;
}

inline VkImageView TextureColConst::getTexView() const {
	return texView;
}

class TextureCol : public TextureColConst {
private:
	struct Less {
		bool operator()(const Rectu& a, const Rectu& b) const;
	};

	struct Find {
		uint page, index;
		Rectu rect;

		Find() = default;
		Find(uint pg, uint id, const Rectu& posize);
	};

	static constexpr uint32 pageNumStep = 4;

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	uint32* mappedMemory;
	VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
	vector<vector<Rectu>> pages;
	uint32 pageReserve;

public:
	void init(const RendererVk* rend, uint32 requestSize);
	void free(const RendererVk* rend);

	pair<TexLoc, bool> insert(const RendererVk* rend, SDL_Surface* img);
	pair<TexLoc, bool> insert(const RendererVk* rend, const SDL_Surface* img, u32vec2 size, u32vec2 offset = u32vec2(0));
	bool replace(const RendererVk* rend, TexLoc& loc, SDL_Surface* img);
	bool replace(const RendererVk* rend, TexLoc& loc, const SDL_Surface* img, u32vec2 size, u32vec2 offset = u32vec2(0));
	bool erase(const RendererVk* rend, TexLoc& loc);

private:
	void uploadSubTex(const RendererVk* rend, const SDL_Surface* img, u32vec2 offset, const Rectu& loc, uint32 page);
	pair<vector<Rectu>::iterator, bool> findReplaceable(const TexLoc& loc, u32vec2& size);	// returns previous location and whether it can be replaced
	Find findLocation(u32vec2 size) const;
	bool maybeResize(const RendererVk* rend);
	void calcPageReserve();
};

inline bool TextureCol::Less::operator()(const Rectu& a, const Rectu& b) const {
	return a.y < b.y || (a.y == b.y && a.x < b.x);
}

inline void TextureCol::calcPageReserve() {
	pageReserve = (pages.size() / pageNumStep + 1) * pageNumStep;
}

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
	static constexpr uint32 bindingUdat = 0;
	static constexpr uint32 bindingIcon = 1;
	static constexpr uint32 bindingText = 2;
	static constexpr uint32 bindingRpic = 3;

	struct PushData {
		alignas(16) ivec4 rect;
		alignas(16) ivec4 frame;
		alignas(16) ivec4 txloc;
		alignas(16) vec4 color;
		alignas(8) uvec2 tid;
	};

	struct UniformData0 {
		alignas(16) vec4 tbounds[3];
	};

	struct UniformData1 {
		alignas(16) vec4 pview;
	};

private:
	array<VkDescriptorSetLayout, 2> descriptorSetLayouts{};
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet0 = VK_NULL_HANDLE;

	VkBuffer uniformBuffer0 = VK_NULL_HANDLE;
	VkDeviceMemory uniformMemory0 = VK_NULL_HANDLE;
	UniformData0* uniformMapped0;

	VkSampler iconSampler = VK_NULL_HANDLE;
	VkSampler textSampler = VK_NULL_HANDLE;
	VkImage dummyImg = VK_NULL_HANDLE;
	VkDeviceMemory dummyMem = VK_NULL_HANDLE;
	VkImageView dummyView = VK_NULL_HANDLE;

public:
	vector<VkDescriptorSet> init(const RendererVk* rend, VkFormat format, uint32 numViews);
	void updateDescriptorSet(VkDevice dev, const array<pair<VkImageView, u32vec2>, 3>& images);
	void updateDescriptorSetImage(VkDevice dev, uint32 binding, VkImageView imageView, u32vec2 imageSize);
	static void updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet1, VkBuffer uniformBuffer1);
	void free(VkDevice dev);

	VkDescriptorSet getDescriptorSet0() const;

private:
	void createRenderPass(VkDevice dev, VkFormat format);
	void createDescriptorSetLayout(VkDevice dev);
	void createPipeline(VkDevice dev);
	void createUniformBuffer(const RendererVk* rend);
	vector<VkDescriptorSet> createDescriptorPoolAndSets(VkDevice dev, uint32 numViews);
};

inline VkDescriptorSet RenderPass::getDescriptorSet0() const {
	return descriptorSet0;
}

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
		ivec2 pos;
		uint page;
		uint8 type;

		TextureVk(ivec2 size, ivec2 location, uint pageId, uint8 arrayId);

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

		VkDescriptorSet descriptorSet1 = VK_NULL_HANDLE;
		VkBuffer uniformBuffer1 = VK_NULL_HANDLE;
		VkDeviceMemory uniformMemory1 = VK_NULL_HANDLE;
		RenderPass::UniformData1* uniformMapped1;

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

	TextureColConst icons, rpics;
	TextureCol texts;
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

	vector<pair<sizet, Texture*>> initIconTextures(vector<pair<sizet, SDL_Surface*>>&& iconImp) final;
	vector<pair<sizet, Texture*>> initRpicTextures(vector<pair<sizet, SDL_Surface*>>&& rpicImp) final;
	Texture* texFromText(SDL_Surface* img) final;
	void freeIconTextures(umap<string, Texture*>& texes) final;
	void freeRpicTextures(vector<pair<string, Texture*>>&& texes) final;
	void freeTextTexture(Texture* tex) final;

	VkPhysicalDevice getPhysicalDevice() const;
	VkDevice getLogicalDevice() const;

	tuple<VkImage, VkDeviceMemory, VkImageView> createDummyTexture(u32vec2 size, uint32 layers, VkFormat format, VkBufferUsageFlags usage) const;
	pair<VkImage, VkDeviceMemory> createImage(u32vec2 size, uint32 layers, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const;
	VkImageView createImageView(VkImage image, VkImageViewType type, VkFormat format, uint32 layers) const;
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
	template <VkImageLayout srcLay, VkImageLayout dstLay> static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, uint32 layer = 0, uint32 numLayers = 1);
	static void copyImage(VkCommandBuffer commandBuffer, VkImage src, VkImage dst, u32vec2 size, uint32 numLayers);
	static void copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size, uint32 numLayers = 1);

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
	static uint32 recommendTextTexSize();
#ifndef NDEBUG
	static bool checkValidationLayerSupport();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif
};

inline VkPhysicalDevice RendererVk::getPhysicalDevice() const {
	return pdev;
}

inline VkDevice RendererVk::getLogicalDevice() const {
	return ldev;
}

inline void RendererVk::freeCommandBuffers(VkCommandBuffer* cmdBuffers, uint32 count) const {
	vkFreeCommandBuffers(ldev, cmdPool, count, cmdBuffers);
}
#endif
