#ifdef WITH_VULKAN
#include "rendererVk.h"
#ifndef RODATA_VULKAN
#include "fileSys.h"
#include "world.h"
#endif
#ifdef WITH_SDL3
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>
#else
#include <SDL_log.h>
#include <SDL_vulkan.h>
#endif
#include <vulkan/vk_enum_string_helper.h>
#include <glm/gtc/round.hpp>
#include <list>
#include <numeric>

// INSTANCE VK

void InstanceVk::initGlobalFunctions() {
	if (!((vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()))
		&& (vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(nullptr, "vkCreateInstance")))
		&& (vkEnumerateInstanceExtensionProperties = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties")))
		&& (vkEnumerateInstanceLayerProperties = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceLayerProperties")))
	))
		throw std::runtime_error("Failed to find global Vulkan commands");
}

void InstanceVk::initLocalFunctions() {
	if (!((vkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetInstanceProcAddr(instance, "vkAcquireNextImageKHR")))
		&& (vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(vkGetInstanceProcAddr(instance, "vkAllocateCommandBuffers")))
		&& (vkAllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(vkGetInstanceProcAddr(instance, "vkAllocateDescriptorSets")))
		&& (vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(vkGetInstanceProcAddr(instance, "vkAllocateMemory")))
		&& (vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(vkGetInstanceProcAddr(instance, "vkBeginCommandBuffer")))
		&& (vkBindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(vkGetInstanceProcAddr(instance, "vkBindBufferMemory")))
		&& (vkBindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(vkGetInstanceProcAddr(instance, "vkBindImageMemory")))
		&& (vkCmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(vkGetInstanceProcAddr(instance, "vkCmdBeginRenderPass")))
		&& (vkCmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(vkGetInstanceProcAddr(instance, "vkCmdBindDescriptorSets")))
		&& (vkCmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(vkGetInstanceProcAddr(instance, "vkCmdBindPipeline")))
		&& (vkCmdBindVertexBuffers = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(vkGetInstanceProcAddr(instance, "vkCmdBindVertexBuffers")))
		&& (vkCmdCopyBuffer = reinterpret_cast<PFN_vkCmdCopyBuffer>(vkGetInstanceProcAddr(instance, "vkCmdCopyBuffer")))
		&& (vkCmdCopyBufferToImage = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(vkGetInstanceProcAddr(instance, "vkCmdCopyBufferToImage")))
		&& (vkCmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(vkGetInstanceProcAddr(instance, "vkCmdDispatch")))
		&& (vkCmdDraw = reinterpret_cast<PFN_vkCmdDraw>(vkGetInstanceProcAddr(instance, "vkCmdDraw")))
		&& (vkCmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(vkGetInstanceProcAddr(instance, "vkCmdEndRenderPass")))
		&& (vkCmdNextSubpass = reinterpret_cast<PFN_vkCmdNextSubpass>(vkGetInstanceProcAddr(instance, "vkCmdNextSubpass")))
		&& (vkCmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier")))
		&& (vkCmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(vkGetInstanceProcAddr(instance, "vkCmdPushConstants")))
		&& (vkCmdSetScissor = reinterpret_cast<PFN_vkCmdSetScissor>(vkGetInstanceProcAddr(instance, "vkCmdSetScissor")))
		&& (vkCmdSetViewport = reinterpret_cast<PFN_vkCmdSetViewport>(vkGetInstanceProcAddr(instance, "vkCmdSetViewport")))
		&& (vkCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(vkGetInstanceProcAddr(instance, "vkCreateBuffer")))
		&& (vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(vkGetInstanceProcAddr(instance, "vkCreateCommandPool")))
		&& (vkCreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(vkGetInstanceProcAddr(instance, "vkCreateComputePipelines")))
		&& (vkCreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(vkGetInstanceProcAddr(instance, "vkCreateDescriptorPool")))
		&& (vkCreateDescriptorSetLayout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(vkGetInstanceProcAddr(instance, "vkCreateDescriptorSetLayout")))
		&& (vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(instance, "vkCreateDevice")))
		&& (vkCreateFence = reinterpret_cast<PFN_vkCreateFence>(vkGetInstanceProcAddr(instance, "vkCreateFence")))
		&& (vkCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(vkGetInstanceProcAddr(instance, "vkCreateFramebuffer")))
		&& (vkCreateGraphicsPipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(vkGetInstanceProcAddr(instance, "vkCreateGraphicsPipelines")))
		&& (vkCreateImage = reinterpret_cast<PFN_vkCreateImage>(vkGetInstanceProcAddr(instance, "vkCreateImage")))
		&& (vkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(vkGetInstanceProcAddr(instance, "vkCreateImageView")))
		&& (vkCreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(vkGetInstanceProcAddr(instance, "vkCreatePipelineLayout")))
		&& (vkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(vkGetInstanceProcAddr(instance, "vkCreateRenderPass")))
		&& (vkCreateSampler = reinterpret_cast<PFN_vkCreateSampler>(vkGetInstanceProcAddr(instance, "vkCreateSampler")))
		&& (vkCreateSemaphore = reinterpret_cast<PFN_vkCreateSemaphore>(vkGetInstanceProcAddr(instance, "vkCreateSemaphore")))
		&& (vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(vkGetInstanceProcAddr(instance, "vkCreateShaderModule")))
		&& (vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetInstanceProcAddr(instance, "vkCreateSwapchainKHR")))
		&& (vkDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(vkGetInstanceProcAddr(instance, "vkDestroyBuffer")))
		&& (vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(vkGetInstanceProcAddr(instance, "vkDestroyCommandPool")))
		&& (vkDestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(vkGetInstanceProcAddr(instance, "vkDestroyDescriptorPool")))
		&& (vkDestroyDescriptorSetLayout = reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(vkGetInstanceProcAddr(instance, "vkDestroyDescriptorSetLayout")))
		&& (vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(vkGetInstanceProcAddr(instance, "vkDestroyDevice")))
		&& (vkDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(vkGetInstanceProcAddr(instance, "vkDestroyFence")))
		&& (vkDestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(vkGetInstanceProcAddr(instance, "vkDestroyFramebuffer")))
		&& (vkDestroyImage = reinterpret_cast<PFN_vkDestroyImage>(vkGetInstanceProcAddr(instance, "vkDestroyImage")))
		&& (vkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(vkGetInstanceProcAddr(instance, "vkDestroyImageView")))
		&& (vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(instance, "vkDestroyInstance")))
		&& (vkDestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(vkGetInstanceProcAddr(instance, "vkDestroyPipeline")))
		&& (vkDestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(vkGetInstanceProcAddr(instance, "vkDestroyPipelineLayout")))
		&& (vkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(vkGetInstanceProcAddr(instance, "vkDestroyRenderPass")))
		&& (vkDestroySampler = reinterpret_cast<PFN_vkDestroySampler>(vkGetInstanceProcAddr(instance, "vkDestroySampler")))
		&& (vkDestroySemaphore = reinterpret_cast<PFN_vkDestroySemaphore>(vkGetInstanceProcAddr(instance, "vkDestroySemaphore")))
		&& (vkDestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(vkGetInstanceProcAddr(instance, "vkDestroyShaderModule")))
		&& (vkDestroySurfaceKHR = reinterpret_cast<PFN_vkDestroySurfaceKHR>(vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR")))
		&& (vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetInstanceProcAddr(instance, "vkDestroySwapchainKHR")))
		&& (vkDeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(vkGetInstanceProcAddr(instance, "vkDeviceWaitIdle")))
		&& (vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(vkGetInstanceProcAddr(instance, "vkEndCommandBuffer")))
		&& (vkEnumerateDeviceExtensionProperties = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties")))
		&& (vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices")))
		&& (vkFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(vkGetInstanceProcAddr(instance, "vkFreeMemory")))
		&& (vkGetBufferMemoryRequirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(vkGetInstanceProcAddr(instance, "vkGetBufferMemoryRequirements")))
		&& (vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(vkGetInstanceProcAddr(instance, "vkGetDeviceQueue")))
		&& (vkGetImageMemoryRequirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(vkGetInstanceProcAddr(instance, "vkGetImageMemoryRequirements")))
		&& (vkGetPhysicalDeviceImageFormatProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties")))
		&& (vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties")))
		&& (vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties")))
		&& (vkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties")))
		&& (vkGetPhysicalDeviceSurfaceCapabilitiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")))
		&& (vkGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR")))
		&& (vkGetPhysicalDeviceSurfacePresentModesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR")))
		&& (vkGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR")))
		&& (vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetInstanceProcAddr(instance, "vkGetSwapchainImagesKHR")))
		&& (vkMapMemory = reinterpret_cast<PFN_vkMapMemory>(vkGetInstanceProcAddr(instance, "vkMapMemory")))
		&& (vkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetInstanceProcAddr(instance, "vkQueuePresentKHR")))
		&& (vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(vkGetInstanceProcAddr(instance, "vkQueueSubmit")))
		&& (vkQueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(vkGetInstanceProcAddr(instance, "vkQueueWaitIdle")))
		&& (vkResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(vkGetInstanceProcAddr(instance, "vkResetCommandBuffer")))
		&& (vkResetFences = reinterpret_cast<PFN_vkResetFences>(vkGetInstanceProcAddr(instance, "vkResetFences")))
		&& (vkUnmapMemory = reinterpret_cast<PFN_vkUnmapMemory>(vkGetInstanceProcAddr(instance, "vkUnmapMemory")))
		&& (vkUpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(vkGetInstanceProcAddr(instance, "vkUpdateDescriptorSets")))
		&& (vkWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(vkGetInstanceProcAddr(instance, "vkWaitForFences")))
	))
		throw std::runtime_error("Failed to find Vulkan functions");

	vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR"));
#ifndef NDEBUG
	vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
#endif
}

InstanceVk::Buffer InstanceVk::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	Buffer vkb;
	try {
		VkBufferCreateInfo bufferInfo = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		if (VkResult rs = vkCreateBuffer(ldev, &bufferInfo, nullptr, &vkb.buffer); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create buffer: {}", string_VkResult(rs)));

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(ldev, vkb.buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
		};
		if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &vkb.memory); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to allocate memory: {}", string_VkResult(rs)));
		if (VkResult rs = vkBindBufferMemory(ldev, vkb.buffer, vkb.memory, 0); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to bind memory: {}", string_VkResult(rs)));
	} catch (const std::runtime_error&) {
		cleanupBuffer(vkb);
		throw;
	}
	return vkb;
}

void InstanceVk::recreateBuffer(Buffer& vkb, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	cleanupBuffer(vkb);
	vkb = Buffer();
	vkb = createBuffer(size, usage, properties);
}

void InstanceVk::cleanupBuffer(Buffer& vkb) const noexcept {
	vkDestroyBuffer(ldev, vkb.buffer, nullptr);
	vkFreeMemory(ldev, vkb.memory, nullptr);
}

InstanceVk::Image InstanceVk::createImage(u32vec2 size, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Swizzle swizzle) const {
	Image vki;
	try {
		VkImageCreateInfo imageInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = { size.x, size.y, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		if (VkResult rs = vkCreateImage(ldev, &imageInfo, nullptr, &vki.image); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create image: {}", string_VkResult(rs)));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(ldev, vki.image, &memRequirements);
		VkMemoryAllocateInfo allocInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
		};
		if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &vki.memory); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to allocate memory: {}", string_VkResult(rs)));
		if (VkResult rs = vkBindImageMemory(ldev, vki.image, vki.memory, 0); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to bind memory: {}", string_VkResult(rs)));
		vki.view = createImageView(vki.image, format, swizzle);
	} catch (const std::runtime_error&) {
		cleanupImage(vki);
		throw;
	}
	return vki;
}

uint32 InstanceVk::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const {
	for (uint32 i = 0; i < pdevMemProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (pdevMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	throw std::runtime_error(fmt::format("Failed to find memory type for properties {}", string_VkMemoryPropertyFlags(properties)));
}

void InstanceVk::cleanupImage(Image& vki) const noexcept {
	vkDestroyImageView(ldev, vki.view, nullptr);
	vkDestroyImage(ldev, vki.image, nullptr);
	vkFreeMemory(ldev, vki.memory, nullptr);
}

VkImageView InstanceVk::createImageView(VkImage image, VkFormat format, Swizzle swizzle) const {
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = { .r = VkComponentSwizzle(swizzle.r), .g = VkComponentSwizzle(swizzle.g), .b = VkComponentSwizzle(swizzle.b), .a = VkComponentSwizzle(swizzle.a) },
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	VkImageView imageView;
	if (VkResult rs = vkCreateImageView(ldev, &viewInfo, nullptr, &imageView); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create image view: {}", string_VkResult(rs)));
	return imageView;
}

VkFramebuffer InstanceVk::createFramebuffer(VkRenderPass rpass, VkImageView* attach, uint32 acnt, u32vec2 size) const {
	VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = rpass,
		.attachmentCount = acnt,
		.pAttachments = attach,
		.width = size.x,
		.height = size.y,
		.layers = 1
	};
	VkFramebuffer framebuffer;
	if (VkResult rs = vkCreateFramebuffer(ldev, &framebufferInfo, nullptr, &framebuffer); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create framebuffer: {}", string_VkResult(rs)));
	return framebuffer;
}

void InstanceVk::allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const {
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count
	};
	if (VkResult rs = vkAllocateCommandBuffers(ldev, &allocInfo, cmdBuffers); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to allocate command buffers: {}", string_VkResult(rs)));
}

VkSemaphore InstanceVk::createSemaphore() const {
	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkSemaphore semaphore;
	if (VkResult rs = vkCreateSemaphore(ldev, &semaphoreInfo, nullptr, &semaphore); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create semaphore: {}", string_VkResult(rs)));
	return semaphore;
}

VkFence InstanceVk::createFence(VkFenceCreateFlags flags) const {
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = flags
	};
	VkFence fence;
	if (VkResult rs = vkCreateFence(ldev, &fenceInfo, nullptr, &fence); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create fence: {}", string_VkResult(rs)));
	return fence;
}

VkSampler InstanceVk::createSampler(VkFilter filter) const {
	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = filter,
		.minFilter = filter,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.minLod = 0.f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
	};
	VkSampler sampler;
	if (VkResult rs = vkCreateSampler(ldev, &samplerInfo, nullptr, &sampler); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create sampler: {}", string_VkResult(rs)));
	return sampler;
}

VkShaderModule InstanceVk::createShaderModule(std::span<const uint32> code) const {
	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode = code.data()
	};
	VkShaderModule shaderModule;
	if (VkResult rs = vkCreateShaderModule(ldev, &createInfo, nullptr, &shaderModule); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create shader: {}", string_VkResult(rs)));
	return shaderModule;
}

// FORMAT CONVERTER

void FormatConverter::init(const InstanceVk* vk) {
	createDescriptorSetLayoutRgb(vk);
	createDescriptorSetLayoutIdx(vk);
	createPipelines(vk);
	createDescriptorPoolAndSets(vk);
}

void FormatConverter::createDescriptorSetLayoutRgb(const InstanceVk* vk) {
	VkDescriptorSetLayoutBinding bindings[2] = { {
		.binding = bindingInput,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	}, {
		.binding = bindingOutput,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	} };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings),
		.pBindings = bindings
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayoutRgb); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));
}

void FormatConverter::createDescriptorSetLayoutIdx(const InstanceVk* vk) {
	VkDescriptorSetLayoutBinding bindings[3] = { {
		.binding = bindingInput,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	}, {
		.binding = bindingOutput,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	}, {
		.binding = bindingUniform,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	} };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings),
		.pBindings = bindings
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayoutIdx); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));
}

void FormatConverter::createPipelines(const InstanceVk* vk) {
#ifdef EXT_VULKAN_SHADERS
	auto [rgbCodeData, rgbCodeSize] = World::fileSys()->readShaderFile("vkRgb.comp.spv");
	auto [idxCodeData, idxCodeSize] = World::fileSys()->readShaderFile("vkIdx.comp.spv");
	std::span<const uint32> rgbCode(rgbCodeData.get(), rgbCodeSize), idxCode(idxCodeData.get(), idxCodeSize);
#else
	static constexpr uint32 rgbCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkRgb.comp.rel.h"
#else
#include "shaders/vkRgb.comp.dbg.h"
#endif
	};
	static constexpr uint32 idxCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkIdx.comp.rel.h"
#else
#include "shaders/vkIdx.comp.dbg.h"
#endif
	};
	std::span<const uint32> rgbCode = rgbCodeData, idxCode = idxCodeData;
#endif
	VkShaderModule rgbShaderModule = VK_NULL_HANDLE;
	VkShaderModule idxShaderModule = VK_NULL_HANDLE;
	try {
		rgbShaderModule = vk->createShaderModule(rgbCode);
		idxShaderModule = vk->createShaderModule(idxCode);

		VkPushConstantRange pushConstant = {
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.size = sizeof(PushData)
		};
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayoutRgb,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstant
		};
		if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &pipelineLayoutRgb); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutIdx;
		if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &pipelineLayoutIdx); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

		constexpr uint li = eint(Pipeline::index8);
		VkSpecializationMapEntry specializationEntry = {
			.constantID = 0,
			.offset = offsetof(SpecializationData, orderRgb),
			.size = sizeof(SpecializationData::orderRgb)
		};
		SpecializationData specializationData[2] = { { .orderRgb = VK_TRUE }, { .orderRgb = VK_FALSE } };
		VkSpecializationInfo specializationInfos[std::size(specializationData)]{};
		VkComputePipelineCreateInfo pipelineInfos[li + 1]{};
		for (uint i = 0; i < std::size(specializationData); ++i) {
			specializationInfos[i].mapEntryCount = 1;
			specializationInfos[i].pMapEntries = &specializationEntry;
			specializationInfos[i].dataSize = sizeof(SpecializationData);
			specializationInfos[i].pData = &specializationData[i];

			pipelineInfos[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfos[i].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			pipelineInfos[i].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			pipelineInfos[i].stage.module = rgbShaderModule;
			pipelineInfos[i].stage.pName = "main";
			pipelineInfos[i].stage.pSpecializationInfo = &specializationInfos[i];
			pipelineInfos[i].layout = pipelineLayoutRgb;
		}
		pipelineInfos[li].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfos[li].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineInfos[li].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineInfos[li].stage.module = idxShaderModule;
		pipelineInfos[li].stage.pName = "main";
		pipelineInfos[li].layout = pipelineLayoutIdx;
		if (VkResult rs = vk->vkCreateComputePipelines(vk->getLdev(), VK_NULL_HANDLE, pipelines.size(), pipelineInfos, nullptr, pipelines.data()); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create pipelines: {}", string_VkResult(rs)));

		vk->vkDestroyShaderModule(vk->getLdev(), rgbShaderModule, nullptr);
		vk->vkDestroyShaderModule(vk->getLdev(), idxShaderModule, nullptr);
	} catch (const std::runtime_error&) {
		vk->vkDestroyShaderModule(vk->getLdev(), rgbShaderModule, nullptr);
		vk->vkDestroyShaderModule(vk->getLdev(), idxShaderModule, nullptr);
		throw;
	}
}

void FormatConverter::createDescriptorPoolAndSets(const InstanceVk* vk) {
	VkDescriptorPoolSize poolSizes[2] = { {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = maxTransfers * numLayouts * 2	// 2 for input + output
	}, {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = maxTransfers
	} };
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxTransfers * numLayouts,
		.poolSizeCount = std::size(poolSizes),
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor pool: {}", string_VkResult(rs)));

	VkDescriptorSetLayout layouts[maxTransfers * numLayouts];
	std::fill_n(layouts, maxTransfers, descriptorSetLayoutRgb);
	std::fill_n(layouts + maxTransfers, maxTransfers, descriptorSetLayoutIdx);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = std::size(layouts),
		.pSetLayouts = layouts
	};
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to allocate descriptor sets: {}", string_VkResult(rs)));

	VkDescriptorBufferInfo uniformBufferInfos[maxTransfers]{};
	VkWriteDescriptorSet descriptorWrites[maxTransfers]{};
	for (uint i = 0; i < maxTransfers; ++i) {
		uniformBufs[i] = vk->createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (VkResult rs = vk->vkMapMemory(vk->getLdev(), uniformBufs[i].memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&uniformBufferMapped[i])); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to map memory: {}", string_VkResult(rs)));

		uniformBufferInfos[i].buffer = uniformBufs[i];
		uniformBufferInfos[i].range = sizeof(UniformData);

		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = descriptorSets[maxTransfers + i];
		descriptorWrites[i].dstBinding = bindingUniform;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[i].pBufferInfo = &uniformBufferInfos[i];
	}
	vk->vkUpdateDescriptorSets(vk->getLdev(), std::size(descriptorWrites), descriptorWrites, 0, nullptr);
}

void FormatConverter::updateBufferSize(const InstanceVk* vk, VkDescriptorSet dset, uint id, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update) {
	VkDeviceSize outputSize = glm::ceilMultiple(inputSize * 4, VkDeviceSize(convWgrpSize * 4) * sizeof(uint32));
	if (outputSize > outputBufferSizesMax[id]) {
		vk->recreateBuffer(outputBufs[id], outputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		outputBufferSizesMax[id] = outputSize;
		update = true;
	}
	if (update) {
		VkDescriptorBufferInfo inputBufferInfo = {
			.buffer = inputBuffer,
			.range = inputSize
		};
		VkDescriptorBufferInfo outputBufferInfo = {
			.buffer = outputBufs[id],
			.range = outputSize
		};
		VkWriteDescriptorSet descriptorWrites[2] = { {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dset,
			.dstBinding = bindingInput,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &inputBufferInfo
		}, {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = dset,
			.dstBinding = bindingOutput,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &outputBufferInfo
		} };
		vk->vkUpdateDescriptorSets(vk->getLdev(), std::size(descriptorWrites), descriptorWrites, 0, nullptr);
		update = false;
	}
}

void FormatConverter::free(const InstanceVk* vk) noexcept {
	for (VkPipeline it : pipelines)
		vk->vkDestroyPipeline(vk->getLdev(), it, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), pipelineLayoutRgb, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), pipelineLayoutIdx, nullptr);
	for (size_t i = 0; i < outputBufs.size(); ++i)
		vk->cleanupBuffer(outputBufs[i]);
	for (size_t i = 0; i < uniformBufs.size(); ++i)
		vk->cleanupBuffer(uniformBufs[i]);
	vk->vkDestroyDescriptorPool(vk->getLdev(), descriptorPool, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayoutRgb, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayoutIdx, nullptr);
}

// RENDER PASS

RenderPass::PipelineCreateHelper::PipelineCreateHelper(VkPipelineLayout layout, VkRenderPass renderPass, uint32_t subpass, bool blend) noexcept :
	shaderStages{ {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.pName = "main"
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pName = "main"
	} },
	bindingDescription{
		.binding = 0,
		.stride = sizeof(vec2),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	},
	attributeDescription{
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = 0
	},
	vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &attributeDescription
	},
	inputAssemblyState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	},
	viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	},
	rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.f
	},
	multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	},
	depthStencilState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO },
	colorBlendAttachment{ .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT },
	colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	},
	dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR },
	dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = uint32(std::size(dynamicStates)),
		.pDynamicStates = dynamicStates
	},
	pipelineInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = uint32(std::size(shaderStages)),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = layout,
		.renderPass = renderPass,
		.subpass = subpass
	}
{
	if (blend) {
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
}

void RenderPass::PipelineCreateHelper::cleanup(const InstanceVk* vk) noexcept {
	for (VkPipelineShaderStageCreateInfo& it : shaderStages)
		vk->vkDestroyShaderModule(vk->getLdev(), it.module, nullptr);
}

RenderPass::DescriptorSetBlock::DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets) :
	used(descriptorSets.begin(), descriptorSets.begin() + 1),
	free(descriptorSets.begin() + 1, descriptorSets.end())
{}

void RenderPass::init(const InstanceVk* vk) {
	samplers[samplerNearest] = vk->createSampler(VK_FILTER_NEAREST);
	samplers[samplerLinear] = vk->createSampler(VK_FILTER_LINEAR);
	globBuf = vk->createBuffer(sizeof(GlobalData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void RenderPass::createPass(const InstanceVk* vk, VkFormat format, Settings::Gamma& gamma, bool canSrgb) {
	if (gamma != Settings::Gamma::value) {
		createRenderPass(vk, format, false);
		createDescriptorSetLayouts(vk, false);
		createGuiPipeline(vk);
	} else {
		try {
			createRenderPass(vk, format, true);
			createDescriptorSetLayouts(vk, true);
			createGuiPipeline(vk);
			createFinPipeline(vk);
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			freePass(vk);
			gamma = canSrgb ? Settings::Gamma::srgb : Settings::Gamma::none;
			createPass(vk, format, gamma, canSrgb);
		}
	}
}

void RenderPass::createRenderPass(const InstanceVk* vk, VkFormat format, bool postp) {
	VkAttachmentDescription colorAttachments[2] = { {
		.format = subpassFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	}, {
		.format = format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = postp ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	} };
	VkAttachmentReference colorAttachmentRefs[2] = { {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	}, {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	} };
	VkAttachmentReference inputAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkSubpassDescription subpasses[2] = { {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRefs[0]
	}, {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 1,
		.pInputAttachments = &inputAttachmentRef,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRefs[1]
	} };
	VkSubpassDependency dependencies[2] = { {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = subpassGui,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	}, {
		.srcSubpass = subpassGui,
		.dstSubpass = subpassFin,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
	} };
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = uint32(std::size(colorAttachments) - !postp),
		.pAttachments = colorAttachments + !postp,
		.subpassCount = 1 + uint(postp),
		.pSubpasses = subpasses,
		.dependencyCount = uint32(std::size(dependencies) - !postp),
		.pDependencies = dependencies
	};
	if (VkResult rs = vk->vkCreateRenderPass(vk->getLdev(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create render pass: {}", string_VkResult(rs)));
}

void RenderPass::createDescriptorSetLayouts(const InstanceVk* vk, bool postp) {
	VkDescriptorSetLayoutBinding layoutBindingsGlob[2] = { {
		.binding = bindingGlobData,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	}, {
		.binding = bindingGlobSamp,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32(samplers.size()),
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = samplers.data()
	} };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(layoutBindingsGlob),
		.pBindings = layoutBindingsGlob
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayoutGlob); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));

	VkDescriptorSetLayoutBinding layoutBindingsView[2] = { {
		.binding = bindingViewData,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	}, {
		.binding = bindingViewIn,
		.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	} };
	layoutInfo.bindingCount = std::size(layoutBindingsView) - !postp;
	layoutInfo.pBindings = layoutBindingsView;
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayoutView); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));

	VkDescriptorSetLayoutBinding layoutBindingModel = {
		.binding = bindingModelTex,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &layoutBindingModel;
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayoutModel); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));
}

void RenderPass::createGuiPipeline(const InstanceVk* vk) {
	VkPushConstantRange pushConstant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.size = sizeof(PushData)
	};
	VkDescriptorSetLayout descriptorSetLayouts[3] = { descriptorSetLayoutGlob, descriptorSetLayoutView, descriptorSetLayoutModel };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = std::size(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstant
	};
	if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &guiPipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

#ifdef EXT_VULKAN_SHADERS
	auto [vertCodeData, vertCodeSize] = World::fileSys()->readShaderFile("vkGui.vert.spv");
	auto [fragCodeData, fragCodeSize] = World::fileSys()->readShaderFile("vkGui.frag.spv");
	std::span<const uint32> vertCode(vertCodeData.get(), vertCodeSize), fragCode(fragCodeData.get(), fragCodeSize);
#else
	static constexpr uint32 vertCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkGui.vert.rel.h"
#else
#include "shaders/vkGui.vert.dbg.h"
#endif
	};
	static constexpr uint32 fragCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkGui.frag.rel.h"
#else
#include "shaders/vkGui.frag.dbg.h"
#endif
	};
	std::span<const uint32> vertCode = vertCodeData, fragCode = fragCodeData;
#endif
	PipelineCreateHelper helper(guiPipelineLayout, handle, subpassGui, true);
	try {
		helper.shaderStages[0].module = vk->createShaderModule(vertCode);
		helper.shaderStages[1].module = vk->createShaderModule(fragCode);
		if (VkResult rs = vk->vkCreateGraphicsPipelines(vk->getLdev(), VK_NULL_HANDLE, 1, &helper.pipelineInfo, nullptr, &guiPipeline); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create pipeline: {}", string_VkResult(rs)));
		helper.cleanup(vk);
	} catch (const std::runtime_error&) {
		helper.cleanup(vk);
		throw;
	}
}

void RenderPass::createFinPipeline(const InstanceVk* vk) {
	VkDescriptorSetLayout descriptorSetLayouts[2] = { descriptorSetLayoutGlob, descriptorSetLayoutView };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = std::size(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
	};
	if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &finPipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

#ifdef EXT_VULKAN_SHADERS
	auto [vertCodeData, vertCodeSize] = World::fileSys()->readShaderFile("vkFin.vert.spv");
	auto [fragCodeData, fragCodeSize] = World::fileSys()->readShaderFile("vkFin.frag.spv");
	std::span<const uint32> vertCode(vertCodeData.get(), vertCodeSize), fragCode(fragCodeData.get(), fragCodeSize);
#else
	static constexpr uint32 vertCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkFin.vert.rel.h"
#else
#include "shaders/vkFin.vert.dbg.h"
#endif
	};
	static constexpr uint32 fragCodeData[] = {
#ifdef NDEBUG
#include "shaders/vkFin.frag.rel.h"
#else
#include "shaders/vkFin.frag.dbg.h"
#endif
	};
	std::span<const uint32> vertCode = vertCodeData, fragCode = fragCodeData;
#endif
	PipelineCreateHelper helper(finPipelineLayout, handle, subpassFin, false);
	try {
		helper.shaderStages[0].module = vk->createShaderModule(vertCode);
		helper.shaderStages[1].module = vk->createShaderModule(fragCode);
		if (VkResult rs = vk->vkCreateGraphicsPipelines(vk->getLdev(), VK_NULL_HANDLE, 1, &helper.pipelineInfo, nullptr, &finPipeline); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to create pipeline: {}", string_VkResult(rs)));
		helper.cleanup(vk);
	} catch (const std::runtime_error&) {
		helper.cleanup(vk);
		throw;
	}
}

void RenderPass::createDescriptorPoolAndSets(const InstanceVk* vk, InstanceVk::ViewVk* views, uint8 numViews) {
	VkDescriptorPoolSize poolSizes[3] = { {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = uint32(numViews) + 1	// Pview + Global
	}, {
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32(samplers.size())
	}, {
		.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
		.descriptorCount = uint32(numViews)
	} };
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = uint32(numViews) + 1,	// view rect & input + global
		.poolSizeCount = uint32(std::size(poolSizes)) - !finPipeline,
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor pool: {}", string_VkResult(rs)));

	uptr<VkDescriptorSetLayout[]> layouts = std::make_unique_for_overwrite<VkDescriptorSetLayout[]>(poolInfo.maxSets);
	std::fill_n(layouts.get(), numViews, descriptorSetLayoutView);
	layouts[numViews] = descriptorSetLayoutGlob;
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = poolInfo.maxSets,
		.pSetLayouts = layouts.get()
	};
	uptr<VkDescriptorSet[]> descriptorSets = std::make_unique_for_overwrite<VkDescriptorSet[]>(poolInfo.maxSets);
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.get()); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to allocate descriptor sets: {}", string_VkResult(rs)));

	uint32 ve = numViews;
	uint32 pe = numViews + (finPipeline ? numViews : 0);
	uptr<VkDescriptorBufferInfo[]> viewBufferInfos = std::make_unique<VkDescriptorBufferInfo[]>(numViews);
	uptr<VkDescriptorImageInfo[]> viewImageInfos;
	uptr<VkWriteDescriptorSet[]> descriptorWrites = std::make_unique<VkWriteDescriptorSet[]>(pe + 1);
	for (uint8 i = 0; i < numViews; ++i) {
		views[i].descriptorSet = descriptorSets[i];

		viewBufferInfos[i].buffer = views[i].uniformBuf;
		viewBufferInfos[i].range = sizeof(ViewData);

		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = views[i].descriptorSet;
		descriptorWrites[i].dstBinding = bindingViewData;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[i].pBufferInfo = &viewBufferInfos[i];
	}
	if (finPipeline) {
		viewImageInfos = std::make_unique<VkDescriptorImageInfo[]>(numViews);
		for (uint8 i = 0; i < numViews; ++i) {
			viewImageInfos[i].imageView = views[i].pp.view;
			viewImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			descriptorWrites[ve + i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[ve + i].dstSet = views[i].descriptorSet;
			descriptorWrites[ve + i].dstBinding = bindingViewIn;
			descriptorWrites[ve + i].descriptorCount = 1;
			descriptorWrites[ve + i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			descriptorWrites[ve + i].pImageInfo = &viewImageInfos[i];
		}
	}
	globDescriptorSet = descriptorSets[numViews];
	VkDescriptorBufferInfo globBufferInfo = {
		.buffer = globBuf,
		.range = sizeof(GlobalData)
	};
	descriptorWrites[pe].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[pe].dstSet = globDescriptorSet;
	descriptorWrites[pe].dstBinding = bindingGlobData;
	descriptorWrites[pe].descriptorCount = 1;
	descriptorWrites[pe].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[pe].pBufferInfo = &globBufferInfo;
	vk->vkUpdateDescriptorSets(vk->getLdev(), pe + 1, descriptorWrites.get(), 0, nullptr);
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::newDescriptorSetTex(const InstanceVk* vk, VkImageView imageView) {
	pair<VkDescriptorPool, VkDescriptorSet> dpds = getDescriptorSetTex(vk);
	updateDescriptorSetImg(vk, dpds.second, imageView);
	return dpds;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::getDescriptorSetTex(const InstanceVk* vk) {
	if (auto psit = rng::find_if(poolSetTex, [](const pair<const VkDescriptorPool, DescriptorSetBlock>& it) -> bool { return !it.second.free.empty(); }); psit != poolSetTex.end()) {
		auto frit = psit->second.free.begin();
		auto usit = psit->second.used.insert(*frit).first;
		psit->second.free.erase(frit);
		return pair(psit->first, *usit);
	}

	VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = textureSetStep
	};
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = textureSetStep,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize
	};
	VkDescriptorPool descPool;
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descPool); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create descriptor pool: {}", string_VkResult(rs)));

	array<VkDescriptorSetLayout, textureSetStep> layouts;
	layouts.fill(descriptorSetLayoutModel);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descPool,
		.descriptorSetCount = layouts.size(),
		.pSetLayouts = layouts.data()
	};
	array<VkDescriptorSet, textureSetStep> descriptorSets;
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS) {
		vk->vkDestroyDescriptorPool(vk->getLdev(), descPool, nullptr);
		throw std::runtime_error(fmt::format("Failed to allocate descriptor sets: {}", string_VkResult(rs)));
	}
	return pair(descPool, *poolSetTex.emplace(descPool, descriptorSets).first->second.used.begin());
}

void RenderPass::freeDescriptorSetTex(const InstanceVk* vk, VkDescriptorPool pool, VkDescriptorSet dset) {
	auto psit = poolSetTex.find(pool);
	if (psit == poolSetTex.end())
		return;
	auto duit = psit->second.used.find(dset);
	if (duit == psit->second.used.end())
		return;

	VkDescriptorSet descriptorSet = *duit;
	if (psit->second.used.erase(duit); !psit->second.used.empty())
		psit->second.free.insert(descriptorSet);
	else {
		vk->vkDestroyDescriptorPool(vk->getLdev(), pool, nullptr);
		poolSetTex.erase(psit);
	}
}

void RenderPass::updateDescriptorSetImg(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkImageView imageView) noexcept {
	VkDescriptorImageInfo imageInfo = {
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = bindingModelTex,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = &imageInfo
	};
	vk->vkUpdateDescriptorSets(vk->getLdev(), 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::free(const InstanceVk* vk) noexcept {
	freePass(vk);
	freeDescriptorPool(vk);
	for (auto& [pool, block] : poolSetTex)
		vk->vkDestroyDescriptorPool(vk->getLdev(), pool, nullptr);
	vk->cleanupBuffer(globBuf);
	for (VkSampler it : samplers)
		vk->vkDestroySampler(vk->getLdev(), it, nullptr);
}

void RenderPass::freePass(const InstanceVk* vk) noexcept {
	vk->vkDestroyPipeline(vk->getLdev(), guiPipeline, nullptr);
	vk->vkDestroyPipeline(vk->getLdev(), finPipeline, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), guiPipelineLayout, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), finPipelineLayout, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayoutGlob, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayoutView, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayoutModel, nullptr);
	vk->vkDestroyRenderPass(vk->getLdev(), handle, nullptr);
	handle = VK_NULL_HANDLE;
	guiPipeline = VK_NULL_HANDLE;
	guiPipelineLayout = VK_NULL_HANDLE;
	finPipeline = VK_NULL_HANDLE;
	finPipelineLayout = VK_NULL_HANDLE;
	descriptorSetLayoutGlob = VK_NULL_HANDLE;
	descriptorSetLayoutView = VK_NULL_HANDLE;
	descriptorSetLayoutModel = VK_NULL_HANDLE;
}

void RenderPass::freeDescriptorPool(const InstanceVk* vk) noexcept {
	vk->vkDestroyDescriptorPool(vk->getLdev(), descriptorPool, nullptr);
	descriptorPool = VK_NULL_HANDLE;
}

// RENDERER VK

RendererVk::SurfaceInfo::SurfaceInfo(SDL_Surface* surface, VkFormat format, Swizzle swizzle) noexcept :
	img(surface),
	fmt(format),
	cmap(swizzle)
{}

RendererVk::SurfaceInfo::SurfaceInfo(SDL_Surface* surface, bool srgb, FormatConverter::Pipeline conv) noexcept :
	img(surface),
	fmt(srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32),
	pid(conv)
{}

RendererVk::RendererVk(InitParams& initParams, Settings* sets) :
	Renderer(initParams.numWindows, 0),
	views(std::make_unique<ViewVk[]>(initParams.numWindows)),
	swapchains(std::make_unique<VkSwapchainKHR[]>(initParams.numWindows)),
	waitStages(std::make_unique_for_overwrite<VkPipelineStageFlags[]>(initParams.numWindows)),
	imageAvailableSemaphores(std::make_unique<VkSemaphore[]>(initParams.numWindows * ViewVk::maxFrames)),
	imageIndices(std::make_unique_for_overwrite<uint32[]>(initParams.numWindows)),
	immediatePresent(!sets->vsync)
{
	InstanceInfo instInfo;
#ifndef WITH_SDL3
	instInfo.window = initParams.windows[0];	// using just one window to get extensions should be fine
#endif
	try {
		createInstance(instInfo);
		for (uint8 i = 0; i < numViews; ++i) {
			viewRefs[i] = &views[i];
			views[i].win = initParams.windows[i];
			views[i].rect.pos() = initParams.vofs[i] - initParams.vofs[numViews];
			SDL_Vulkan_GetDrawableSize(initParams.windows[i], &views[i].rect.w, &views[i].rect.h);
			initParams.viewRes = glm::max(initParams.viewRes, views[i].rect.end());
#ifdef WITH_SDL3
			if (!SDL_Vulkan_CreateSurface(initParams.windows[i], instance, nullptr, &views[i].surface))
#else
			if (!SDL_Vulkan_CreateSurface(initParams.windows[i], instance, &views[i].surface))
#endif
				throw std::runtime_error(SDL_GetError());
		}
		uptr<DeviceInfo> deviceInfo = pickPhysicalDevice(instInfo, sets->device);
		createDevice(*deviceInfo);
		setUsesSrgb(sets);

		tcmdPool = createCommandPool(tfamilyIndex);
		allocateCommandBuffers(tcmdPool, tcmdBuffers.data(), tcmdBuffers.size());
		for (size_t i = 0; i < tfences.size(); ++i)
			tfences[i] = createFence();

		if (deviceInfo->canCompute) {
			try {
				fmtConv.init(this);
				transferAtomSize = glm::ceilMultiple(VkDeviceSize(FormatConverter::convWgrpSize * 3) * sizeof(uint32), transferAtomSize);
			} catch (const std::runtime_error& err) {
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
				fmtConv.free(this);
				fmtConv = FormatConverter();
			}
		}

		gcmdPool = createCommandPool(gfamilyIndex);
		allocateCommandBuffers(gcmdPool, commandBuffers.data(), commandBuffers.size());
		std::fill_n(waitStages.get(), numViews, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		for (uint i = 0; i < numViews * ViewVk::maxFrames; ++i)
			imageAvailableSemaphores[i] = createSemaphore();
		for (uint i = 0; i < ViewVk::maxFrames; ++i) {
			renderFinishedSemaphores[i] = createSemaphore();
			frameFences[i] = createFence(VK_FENCE_CREATE_SIGNALED_BIT);
		}

		renderPass.init(this);
		renderPass.createPass(this, surfaceFormats[usesSrgb].format, sets->gammaType, canSrgb);
		initGlobalData(sets, initParams.colors);
		for (uint8 i = 0; i < numViews; ++i) {
			createSwapchain(i);

			vec4 pview(views[i].rect.pos(), vec2(views[i].rect.size()) / 2.f);
			views[i].uniformBuf = createBuffer(sizeof(RenderPass::ViewData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			uploadBuffer(views[i].uniformBuf, &pview, 0, sizeof(pview), VK_PIPELINE_STAGE_NONE, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
		}
		renderPass.createDescriptorPoolAndSets(this, views.get(), numViews);

		TextureVk* tooltipTex = static_cast<TextureVk*>(initParams.tooltipTexture = new TextureVk(uvec2(0), RenderPass::samplerNearest));
		std::tie(tooltipTex->pool, tooltipTex->set) = renderPass.getDescriptorSetTex(this);

		setCompression(sets);
		setMaxPicRes(sets->maxPicRes);
		if (!sets->picLim.size) {
			for (uint32 i = 0; i < pdevMemProperties.memoryHeapCount; ++i)
				if ((pdevMemProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && pdevMemProperties.memoryHeaps[i].size > sets->picLim.size)
					sets->picLim.size = pdevMemProperties.memoryHeaps[i].size;
			sets->picLim.size /= 2;
			recommendPicRamLimit(sets->picLim.size);
		}
	} catch (const std::exception&) {
		waitIdle();
		freeTexture(initParams.tooltipTexture);
		cleanup();
		throw;
	}
}

RendererVk::~RendererVk() {
	cleanup();
}

void RendererVk::cleanup() noexcept {
	if (!functionsInitialized())
		return;
	if (ldev) {
		vkDeviceWaitIdle(ldev);

		cleanupBuffer(vertexBuf);
		for (uint8 i = 0; i < numViews; ++i) {
			freeFramebuffers(views[i]);
			vkDestroySwapchainKHR(ldev, swapchains[i], nullptr);
			cleanupBuffer(views[i].uniformBuf);
		}
		renderPass.free(this);
		for (uint i = 0; i < ViewVk::maxFrames; ++i) {
			vkDestroyFence(ldev, frameFences[i], nullptr);
			vkDestroySemaphore(ldev, renderFinishedSemaphores[i], nullptr);
		}
		for (uint i = 0; i < numViews * ViewVk::maxFrames; ++i)
			vkDestroySemaphore(ldev, imageAvailableSemaphores[i], nullptr);
		vkDestroyCommandPool(ldev, gcmdPool, nullptr);

		fmtConv.free(this);
		for (size_t i = 0; i < inputBufs.size(); ++i)
			cleanupBuffer(inputBufs[i]);

		for (VkFence it : tfences)
			vkDestroyFence(ldev, it, nullptr);
		vkDestroyCommandPool(ldev, tcmdPool, nullptr);

		vkDestroyDevice(ldev, nullptr);
	}
#ifndef NDEBUG
	if (dbgMessenger)
		vkDestroyDebugUtilsMessengerEXT(instance, dbgMessenger, nullptr);
#endif
	for (uint8 i = 0; i < numViews; ++i)
		vkDestroySurfaceKHR(instance, views[i].surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void RendererVk::createInstance(InstanceInfo& instInfo) {
	initGlobalFunctions();

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "VertiRead",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "VertiRead_RendererVk",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
#ifndef NDEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debugCallback
	};
#endif
	vector<const char*> extensions = getRequiredInstanceExtensions(instInfo);
	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = uint32(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};
#ifndef NDEBUG
	if (instInfo.extDebugUtils) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validationLayerName;
		createInfo.pNext = &debugCreateInfo;
	}
#endif
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance");

	initLocalFunctions();
#ifndef NDEBUG
	if (instInfo.extDebugUtils)
		if (VkResult rs = vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &dbgMessenger); rs != VK_SUCCESS)
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create debug messenger: %s", string_VkResult(rs));
#endif
}

uptr<RendererVk::DeviceInfo> RendererVk::pickPhysicalDevice(const InstanceInfo& instInfo, u32vec2& preferred) {
	uint32 deviceCount;
	if (VkResult rs = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to enumerate devices: {}", string_VkResult(rs)));
	uptr<VkPhysicalDevice[]> devices = std::make_unique_for_overwrite<VkPhysicalDevice[]>(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.get());

	uptr<DeviceInfo> last = std::make_unique<DeviceInfo>();
	uptr<DeviceInfo> devi = std::make_unique<DeviceInfo>();
	for (uint32 d = 0; d < deviceCount; ++d) {
		*devi = devices[d];
		uint32 extensionCount;
		if (vkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extensionCount, nullptr) != VK_SUCCESS)
			continue;
		uptr<VkExtensionProperties[]> availableExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
		vkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extensionCount, availableExtensions.get());

		std::set<string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::set<string> optionalExtensions = instInfo.khrGetPhysicalDeviceProperties2 ? std::set<string>{ VK_EXT_4444_FORMATS_EXTENSION_NAME } : std::set<string>();
		for (uint32 i = 0; i < extensionCount && !(requiredExtensions.empty() && optionalExtensions.empty()); ++i)
			if (string name = availableExtensions[i].extensionName; requiredExtensions.erase(name) || optionalExtensions.erase(name))
				devi->extensions.push_back(std::move(name));
		if (!requiredExtensions.empty())
			continue;

		vkGetPhysicalDeviceMemoryProperties(devices[d], &devi->memp);
		std::list<VkMemoryPropertyFlags> requiredMemoryTypes(deviceMemoryTypes.begin(), deviceMemoryTypes.end());
		for (uint32 i = 0; i < devi->memp.memoryTypeCount && !requiredMemoryTypes.empty(); ++i)
			for (std::list<VkMemoryPropertyFlags>::iterator it; (it = rng::find_if(requiredMemoryTypes, [&devi, i](VkMemoryPropertyFlags flg) -> bool { return (devi->memp.memoryTypes[i].propertyFlags & flg) == flg; })) != requiredMemoryTypes.end() && !requiredMemoryTypes.empty();)
				requiredMemoryTypes.erase(it);
		if (!requiredMemoryTypes.empty())
			continue;

		if (!(checkImageFormats(*devi) && findQueueFamilies(*devi) && chooseSurfaceFormat(*devi)))
			continue;

		vkGetPhysicalDeviceProperties(devices[d], &devi->prop);
		if (devi->prop.vendorID == preferred.x && devi->prop.deviceID == preferred.y)
			return devi;
		if (devi->score = scoreDevice(*devi); last->dev == VK_NULL_HANDLE || devi->score > last->score)
			*last = std::move(*devi);
	}
	if (last->dev == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable device");
	preferred = u32vec2(0);
	return last;
}

void RendererVk::createDevice(DeviceInfo& deviceInfo) {
	float queuePriorities[DeviceInfo::maxQueues];
	rng::fill(queuePriorities, 1.f);
	VkDeviceQueueCreateInfo queueCreateInfos[DeviceInfo::maxQueues]{};
	for (uint8 i = 0; i < deviceInfo.numQfams; ++i) {
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = deviceInfo.qfqcnts[i].first;
		queueCreateInfos[i].queueCount = deviceInfo.qfqcnts[i].second;
		queueCreateInfos[i].pQueuePriorities = queuePriorities;
	}

	uptr<const char*[]> extensions = std::make_unique_for_overwrite<const char*[]>(deviceInfo.extensions.size());
	rng::transform(deviceInfo.extensions, extensions.get(), [](const string& it) -> const char* { return it.data(); });
	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = deviceInfo.numQfams,
		.pQueueCreateInfos = queueCreateInfos,
		.enabledExtensionCount = uint32(deviceInfo.extensions.size()),
		.ppEnabledExtensionNames = extensions.get()
	};
	if (deviceInfo.formatsFeatures.formatA4R4G4B4 || deviceInfo.formatsFeatures.formatA4B4G4R4)
		createInfo.pNext = &deviceInfo.formatsFeatures;
#ifndef NDEBUG
	if (dbgMessenger != VK_NULL_HANDLE) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validationLayerName;
	}
#endif
	if (VkResult rs = vkCreateDevice(deviceInfo.dev, &createInfo, nullptr, &ldev); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create device: {}", string_VkResult(rs)));

	pdev = deviceInfo.dev;
	pdevMemProperties = deviceInfo.memp;
	maxTextureSize = deviceInfo.prop.limits.maxImageDimension2D;
	transferAtomSize = std::max(deviceInfo.prop.limits.nonCoherentAtomSize, VkDeviceSize(4));	// should be at least 4 so shaders can accept buffers of uints
	maxComputeWorkGroups = deviceInfo.prop.limits.maxComputeWorkGroupCount[0];
	surfaceFormats = deviceInfo.surfaceFormats;
	rng::copy(deviceInfo.formats, optionalFormats.begin());
	canSrgb = deviceInfo.canSrgb;

	gfamilyIndex = deviceInfo.graphicsQids.first;
	vkGetDeviceQueue(ldev, gfamilyIndex, deviceInfo.graphicsQids.second, &gqueue);
	pfamilyIndex = deviceInfo.presentQids.first;
	vkGetDeviceQueue(ldev, pfamilyIndex, deviceInfo.presentQids.second, &pqueue);
	tfamilyIndex = deviceInfo.transferQids.first;
	vkGetDeviceQueue(ldev, tfamilyIndex, deviceInfo.transferQids.second, &tqueue);
}

VkCommandPool RendererVk::createCommandPool(uint32 family) const {
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = family
	};
	VkCommandPool commandPool;
	if (VkResult rs = vkCreateCommandPool(ldev, &poolInfo, nullptr, &commandPool); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create command pool: {}", string_VkResult(rs)));
	return commandPool;
}

void RendererVk::createSwapchain(uint vid, VkSwapchainKHR oldSwapchain) {
	VkSurfaceCapabilitiesKHR capabilities;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, views[vid].surface, &capabilities); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to get surface capabilities: {}", string_VkResult(rs)));

	views[vid].extent = capabilities.currentExtent.width != UINT32_MAX || capabilities.currentExtent.height != UINT32_MAX ? capabilities.currentExtent : VkExtent2D{
		std::clamp(uint32(views[vid].rect.w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(uint32(views[vid].rect.h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
	auto [presentMode, minImgReq] = chooseSwapPresentMode(views[vid].surface, capabilities);
	uint32 queueFamilyIndices[2] = { gfamilyIndex, pfamilyIndex };
	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = views[vid].surface,
		.minImageCount = minImgReq,
		.imageFormat = surfaceFormats[usesSrgb].format,
		.imageColorSpace = surfaceFormats[usesSrgb].colorSpace,
		.imageExtent = views[vid].extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = capabilities.currentTransform,
#ifdef WITH_SDL3
		.compositeAlpha = bgColor.color.float32[3] >= 1.f || !(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
#else
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
#endif
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapchain
	};
	if (gfamilyIndex != pfamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = std::size(queueFamilyIndices);
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	if (VkResult rs = vkCreateSwapchainKHR(ldev, &createInfo, nullptr, &swapchains[vid]); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to create swapchain: {}", string_VkResult(rs)));

	u32vec2 res(views[vid].extent.width, views[vid].extent.height);
	if (renderPass.getFinPipeline())
		views[vid].pp = createImage(res, RenderPass::subpassFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	uint32 imgCount;
	if (VkResult rs = vkGetSwapchainImagesKHR(ldev, swapchains[vid], &imgCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to get swapchain images: {}", string_VkResult(rs)));
	uptr<VkImage[]> images = std::make_unique<VkImage[]>(imgCount);
	vkGetSwapchainImagesKHR(ldev, swapchains[vid], &imgCount, images.get());
	views[vid].frames = std::make_unique<ViewFrame[]>(imgCount);
	views[vid].imageCount = imgCount;
	for (uint32 i = 0; i < imgCount; ++i) {
		views[vid].frames[i].view = createImageView(images[i], surfaceFormats[usesSrgb].format);
		if (renderPass.getFinPipeline()) {
			VkImageView attach[2] = { views[vid].pp.view, views[vid].frames[i].view };
			views[vid].frames[i].framebuffer = createFramebuffer(renderPass.getHandle(), attach, std::size(attach), res);
		} else
			views[vid].frames[i].framebuffer = createFramebuffer(renderPass.getHandle(), &views[vid].frames[i].view, 1, res);
	}
}

void RendererVk::recreateSwapchain(uint vid) {
	freeFramebuffers(views[vid]);
	VkSwapchainKHR oldSwapchain = swapchains[vid];
	swapchains[vid] = VK_NULL_HANDLE;
	try {
		createSwapchain(vid, oldSwapchain);
		vkDestroySwapchainKHR(ldev, oldSwapchain, nullptr);
	} catch (const std::runtime_error&) {
		vkDestroySwapchainKHR(ldev, oldSwapchain, nullptr);
		throw;
	}
	vec4 pview(views[vid].rect.pos(), vec2(views[vid].rect.size()) / 2.f);
	uploadBuffer(views[vid].uniformBuf, &pview, 0, sizeof(pview), VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
}

void RendererVk::freeFramebuffers(ViewVk& view) noexcept {
	for (uint32 i = 0; i < view.imageCount; ++i) {
		vkDestroyFramebuffer(ldev, view.frames[i].framebuffer, nullptr);
		vkDestroyImageView(ldev, view.frames[i].view, nullptr);
	}
	cleanupImage(view.pp);

	view.frames.reset();
	view.imageCount = 0;
	view.pp = Image();
}

void RendererVk::setColors(array<vec4, Settings::defaultColors.size()>& colors) {
	prepareColorData(colors);
	uploadBuffer(renderPass.getGlobBuffer(), colors.data(), 0, sizeof(RenderPass::GlobalData::colors), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void RendererVk::initGlobalData(const Settings* sets, array<vec4, Settings::defaultColors.size()>& colors) {
	VkBufferCopy vertRegion = { .size = sizeof(vertices) + sizeof(scrVertices) };
	VkBufferCopy globRegion = { .srcOffset = vertRegion.size, .size = sizeof(RenderPass::GlobalData) };
	vertexBuf = createBuffer(vertRegion.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	float gamma = 10.f / float(sets->gammaValue);
	prepareColorData(colors);
	syncTransferCommands();
	checkInputBufferSize(vertRegion.size + globRegion.size);

	auto idst = static_cast<uint8*>(inputsMapped[currentTransfer]);
	memcpy(idst, vertices.data(), sizeof(vertices));
	idst += sizeof(vertices);
	memcpy(idst, scrVertices.data(), sizeof(scrVertices));
	idst += sizeof(scrVertices);
	memcpy(idst, colors.data(), sizeof(RenderPass::GlobalData::colors));
	idst += sizeof(RenderPass::GlobalData::colors);
	memcpy(idst, &gamma, sizeof(gamma));

	beginTransferCommands();
	vkCmdCopyBuffer(tcmdBuffers[currentTransfer], inputBufs[currentTransfer], vertexBuf, 1, &vertRegion);
	vkCmdCopyBuffer(tcmdBuffers[currentTransfer], inputBufs[currentTransfer], renderPass.getGlobBuffer(), 1, &globRegion);
	transitionBuffer(vertexBuf, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, tfamilyIndex, gfamilyIndex);	// TODO: wrong stage (need to look over all barriers)
	transitionBuffer(renderPass.getGlobBuffer(), VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, tfamilyIndex, gfamilyIndex);
	endTransferCommands();
}

void RendererVk::prepareColorData(array<vec4, Settings::defaultColors.size()>& colors) noexcept {
	convertColors(colors.data(), colors.size(), usesSrgb, renderPass.getFinPipeline());
	const vec4& bclr = colors[eint(Color::background)];
	bgColor.color = { .float32 = { bclr.r, bclr.g, bclr.b, bclr.a } };
}

bool RendererVk::setSettings(Settings* sets) {
	bool prevPresent = immediatePresent;
	immediatePresent = !sets->vsync;
	bool prevSrgb = usesSrgb;
	setUsesSrgb(sets);
	bool reloadSrgb = usesSrgb != prevSrgb;
	bool reloadGamma = bool(renderPass.getFinPipeline()) != (sets->gammaType == Settings::Gamma::value);
	bool recreate = (immediatePresent != prevPresent) || reloadSrgb || reloadGamma;
	if (recreate)
		vkDeviceWaitIdle(ldev);
	if (reloadSrgb || reloadGamma) {
		renderPass.freePass(this);
		renderPass.createPass(this, surfaceFormats[usesSrgb].format, sets->gammaType, canSrgb);
	}
	if (recreate)
		for (uint8 i = 0; i < numViews; ++i)
			recreateSwapchain(i);
	if (reloadGamma) {
		renderPass.freeDescriptorPool(this);
		renderPass.createDescriptorPoolAndSets(this, views.get(), numViews);
	}
	setCompression(sets);
	return reloadSrgb || reloadGamma;
}

void RendererVk::setGammaValue(int gamma) {
	float gval = 10.f / float(gamma);
	uploadBuffer(renderPass.getGlobBuffer(), &gval, offsetof(RenderPass::GlobalData, gamma), sizeof(gval), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

bool RendererVk::updateView(ivec2& viewRes) {
	if (numViews == 1) {
		SDL_Vulkan_GetDrawableSize(views[0].win, &views[0].rect.w, &views[0].rect.h);
		if (views[0].rect.size() != viewRes) {
			viewRes = views[0].rect.size();
			return refreshFramebuffers = true;
		}
	}
	return false;
}

void RendererVk::uploadBuffer(VkBuffer buffer, const void* data, VkDeviceSize dstOffs, VkDeviceSize size, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
	syncTransferCommands();
	checkInputBufferSize(size);
	memcpy(inputsMapped[currentTransfer], data, size);
	copyInputBuffer(buffer, dstOffs, size, srcStage, dstStage);
}

void RendererVk::copyInputBuffer(VkBuffer buffer, VkDeviceSize dstOffs, VkDeviceSize size, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
	VkBufferCopy region = {
		.dstOffset = dstOffs,
		.size = size
	};
	beginTransferCommands();
	if (srcStage != VK_PIPELINE_STAGE_NONE)
		transitionBuffer(buffer, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT, gfamilyIndex, tfamilyIndex);
	vkCmdCopyBuffer(tcmdBuffers[currentTransfer], inputBufs[currentTransfer], buffer, 1, &region);
	transitionBuffer(buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage, tfamilyIndex, gfamilyIndex);
	endTransferCommands();
}

Renderer::Action RendererVk::beginRender() noexcept {
	try {
		if (VkResult rs = vkWaitForFences(ldev, 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX); rs != VK_SUCCESS && rs != VK_TIMEOUT)
			throw std::runtime_error(fmt::format("Failed to wait for fence: {}", string_VkResult(rs)));
		for (uint8 i = 0; i < numViews; ++i)
			if (VkResult rs = vkAcquireNextImageKHR(ldev, swapchains[i], UINT64_MAX, imageAvailableSemaphores[currentFrame * numViews + i], VK_NULL_HANDLE, &imageIndices[i]); rs != VK_SUCCESS && rs != VK_SUBOPTIMAL_KHR) {
				if (rs == VK_ERROR_OUT_OF_DATE_KHR) {
					refreshFramebuffers = true;
					return Action::skip;
				}
				throw std::runtime_error(fmt::format("Failed to acquire image: %s", string_VkResult(rs)));
			}
		if (VkResult rs = vkResetFences(ldev, 1, &frameFences[currentFrame]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to reset fence: {}", string_VkResult(rs)));
		if (VkResult rs = vkResetCommandBuffer(commandBuffers[currentFrame], 0); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to reset command buffer: {}", string_VkResult(rs)));

		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};
		if (VkResult rs = vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to begin command buffer: {}", string_VkResult(rs)));
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return Action::no;
	}
	return Action::yes;
}

Renderer::Action RendererVk::startDraw(uint vid) noexcept {
	VkViewport viewport = {
		.width = float(views[vid].extent.width),
		.height = float(views[vid].extent.height)
	};
	VkRect2D scissor = { .extent = views[vid].extent };
	vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass.getHandle(),
		.framebuffer = views[vid].frames[imageIndices[vid]].framebuffer,
		.renderArea = { .extent = views[vid].extent },
		.clearValueCount = 1,
		.pClearValues = &bgColor
	};
	VkDescriptorSet descriptorSets[] = { renderPass.getGlobDescriptorSet(), views[vid].descriptorSet };
	VkDeviceSize vertexOffset = 0;
	vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getGuiPipeline());
	vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getGuiPipelineLayout(), RenderPass::dsetGlob, std::size(descriptorSets), descriptorSets, 0, nullptr);
	vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, &vertexBuf.buffer, &vertexOffset);
	return Action::yes;
}

void RendererVk::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, Color color) noexcept {
	auto vtx = static_cast<const TextureVk*>(tex);
	RenderPass::PushData pd = {
		.rect = rect.asVec(),
		.frame = frame.asVec(),
		.color = eint(color),
		.sid = vtx->sid
	};
	vkCmdPushConstants(commandBuffers[currentFrame], renderPass.getGuiPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RenderPass::PushData), &pd);
	vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getGuiPipelineLayout(), RenderPass::dsetModel, 1, &vtx->set, 0, nullptr);
	vkCmdDraw(commandBuffers[currentFrame], vertices.size(), 1, 0, 0);
}

Renderer::Action RendererVk::finishDraw(uint vid) noexcept {
	if (renderPass.getFinPipeline()) {
		vkCmdNextSubpass(commandBuffers[currentFrame], VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getFinPipeline());

		VkDescriptorSet descriptorSets[2] = { renderPass.getGlobDescriptorSet(), views[vid].descriptorSet };
		vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getFinPipelineLayout(), RenderPass::dsetGlob, std::size(descriptorSets), descriptorSets, 0, nullptr);
		vkCmdDraw(commandBuffers[currentFrame], scrVertices.size(), 1, vertices.size(), 0);
	}
	vkCmdEndRenderPass(commandBuffers[currentFrame]);
	return Action::yes;
}

Renderer::Action RendererVk::finishRender() noexcept {
	try {
		if (VkResult rs = vkEndCommandBuffer(commandBuffers[currentFrame]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to end command buffer: {}", string_VkResult(rs)));

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = numViews,
			.pWaitSemaphores = imageAvailableSemaphores.get() + currentFrame * numViews,
			.pWaitDstStageMask = waitStages.get(),
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffers[currentFrame],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderFinishedSemaphores[currentFrame]
		};
		if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, frameFences[currentFrame]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to submit command buffer: {}", string_VkResult(rs)));

		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderFinishedSemaphores[currentFrame],
			.swapchainCount = numViews,
			.pSwapchains = swapchains.get(),
			.pImageIndices = imageIndices.get()
		};
		if (VkResult rs = vkQueuePresentKHR(pqueue, &presentInfo); rs != VK_SUCCESS) {
			if (rs != VK_ERROR_OUT_OF_DATE_KHR && rs != VK_SUBOPTIMAL_KHR)
				throw std::runtime_error(fmt::format("Failed to present image: {}", string_VkResult(rs)));
			refreshFramebuffers = true;
		}
		currentFrame = (currentFrame + 1) % ViewVk::maxFrames;

		if (refreshFramebuffers) {
			vkDeviceWaitIdle(ldev);
			for (uint8 i = 0; i < numViews; ++i)
				recreateSwapchain(i);
			if (renderPass.getFinPipeline()) {
				renderPass.freeDescriptorPool(this);
				renderPass.createDescriptorPoolAndSets(this, views.get(), numViews);
			}
			refreshFramebuffers = false;
		}
	} catch (const std::runtime_error& err) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		return Action::no;
	}
	return Action::yes;
}

Texture* RendererVk::texFromSurface(SDL_Surface* img, bool rpic, bool linear) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, maxTextureSize), rpic && usesSrgb); si.img) {
		TextureVk* tex = nullptr;
		try {
			tex = new TextureVk(uvec2(si.img->w, si.img->h), linear);
			*static_cast<Image*>(tex) = createImage(tex->res, si.fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, si.cmap);
			std::tie(tex->pool, tex->set) = renderPass.newDescriptorSetTex(this, tex->view);
			if (!si.pid)
				uploadTextureDirect(tex->image, tex->res, si.img->pixels, si.img->pitch, surfaceBytesPpx(si.img));
			else
				uploadTextureIndirect(tex->image, tex->res, si);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			freeTexture(tex);
		}
	}
	return nullptr;
}

bool RendererVk::texFromSurface(Texture* tex, SDL_Surface* img, bool rpic) noexcept {
	if (SurfaceInfo si = pickPixFormat(limitSize(img, maxTextureSize), rpic && usesSrgb); si.img) {
		Image vki;
		try {
			uvec2 nres(si.img->w, si.img->h);
			vki = createImage(nres, si.fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, si.cmap);
			if (!si.pid)
				uploadTextureDirect(vki, nres, si.img->pixels, si.img->pitch, surfaceBytesPpx(si.img));
			else
				uploadTextureIndirect(vki, nres, si);
			replaceTexture(*static_cast<TextureVk*>(tex), vki, nres);
			return true;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			cleanupImage(vki);
		}
	}
	return false;
}

Texture* RendererVk::texFromText(const Pixmap& pm) noexcept {
	if (pm.res.x) {
		TextureVk* tex = nullptr;
		try {
			tex = new TextureVk(glm::min(pm.res, uvec2(maxTextureSize)), RenderPass::samplerNearest);
			*static_cast<Image*>(tex) = createImage(tex->res, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textSwizzle);
			std::tie(tex->pool, tex->set) = renderPass.newDescriptorSetTex(this, tex->view);
			uploadTextureDirect(tex->image, tex->res, pm.pix.get(), pm.res.x, 1);
			return tex;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			freeTexture(tex);
		}
	}
	return nullptr;
}

bool RendererVk::texFromText(Texture* tex, const Pixmap& pm) noexcept {
	if (pm.res.x) {
		Image vki;
		try {
			uvec2 nres = glm::min(pm.res, uvec2(maxTextureSize));
			vki = createImage(nres, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textSwizzle);
			uploadTextureDirect(vki, nres, pm.pix.get(), pm.res.x, 1);
			replaceTexture(*static_cast<TextureVk*>(tex), vki, nres);
			return true;
		} catch (const std::exception& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
			cleanupImage(vki);
		}
	}
	return false;
}

void RendererVk::freeTexture(Texture* tex) noexcept {
	if (auto vtx = static_cast<TextureVk*>(tex)) {
		renderPass.freeDescriptorSetTex(this, vtx->pool, vtx->set);
		cleanupImage(*vtx);
		delete vtx;
	}
}

void RendererVk::replaceTexture(TextureVk& tex, Image& vki, uvec2 res) noexcept {
	cleanupImage(tex);
	tex.res = res;
	static_cast<Image&>(tex) = vki;
	renderPass.updateDescriptorSetImg(this, tex.set, tex.view);
}

void RendererVk::waitIdle() noexcept {
	if (VkResult rs = vkQueueWaitIdle(tqueue); rs != VK_SUCCESS)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to wait for queue: %s", string_VkResult(rs));
	if (VkResult rs = vkQueueWaitIdle(gqueue); rs != VK_SUCCESS)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to wait for queue: %s", string_VkResult(rs));
}

void RendererVk::uploadTextureDirect(VkImage img, u32vec2 res, const void* pix, uint pitch, uint8 bpp) {
	uint32 rowSize = res.x * bpp;
	syncTransferCommands();
	checkInputBufferSize(VkDeviceSize(rowSize) * VkDeviceSize(res.y));
	copyPixels(inputsMapped[currentTransfer], pix, rowSize, pitch, res.y);

	beginTransferCommands();
	transitionImageLayout(img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	copyBufferToImage(inputBufs[currentTransfer], img, res);
	transitionImageLayout(img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, tfamilyIndex, gfamilyIndex);
	endTransferCommands();
}

void RendererVk::uploadTextureIndirect(VkImage img, u32vec2 res, const SurfaceInfo& si) {
	auto [pipelineLayout, descriptorSet, layoutId] = fmtConv.getPipelineInfo(*si.pid, currentTransfer);
	uint32 rowSize = res.x * surfaceBytesPpx(si.img);
	syncTransferCommands();
	checkInputBufferSize(VkDeviceSize(rowSize) * VkDeviceSize(res.y));
	fmtConv.updateBufferSize(this, descriptorSet, currentTransfer, inputBufs[currentTransfer], inputSizesMax[currentTransfer], rebindInputBuffer[currentTransfer][layoutId]);
	copyPixels(inputsMapped[currentTransfer], si.img->pixels, rowSize, si.img->pitch, res.y);
	if (*si.pid == FormatConverter::Pipeline::index8)
		copyPalette(fmtConv.getUniformBufferMapped(currentTransfer)->colors, surfacePalette(si.img.get()));

	beginTransferCommands();
	vkCmdBindPipeline(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, fmtConv.getPipeline(*si.pid));
	vkCmdBindDescriptorSets(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

	uint32 texels = res.x * res.y;
	uint32 numGroups = texels / FormatConverter::convStep + bool(texels % FormatConverter::convStep);
	FormatConverter::PushData pd = { 0 };
	for (uint32 gcnt; pd.offset < numGroups; pd.offset += gcnt) {
		gcnt = std::min(numGroups - pd.offset, maxComputeWorkGroups);
		vkCmdPushConstants(tcmdBuffers[currentTransfer], pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FormatConverter::PushData), &pd);
		vkCmdDispatch(tcmdBuffers[currentTransfer], gcnt, 1, 1);
	}
	transitionBufferToImageLayout(fmtConv.getOutputBuffer(currentTransfer), img);
	copyBufferToImage(fmtConv.getOutputBuffer(currentTransfer), img, res);
	transitionImageLayout(img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, tfamilyIndex, gfamilyIndex);
	endTransferCommands();
}

void RendererVk::checkInputBufferSize(VkDeviceSize size) {
	if (size > inputSizesMax[currentTransfer]) {
		size = glm::ceilMultiple(size, transferAtomSize);
		recreateBuffer(inputBufs[currentTransfer], size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (VkResult rs = vkMapMemory(ldev, inputBufs[currentTransfer].memory, 0, VK_WHOLE_SIZE, 0, &inputsMapped[currentTransfer]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to map memory: {}", string_VkResult(rs)));
		inputSizesMax[currentTransfer] = size;
		rebindInputBuffer[currentTransfer].fill(true);
	}
}

void RendererVk::beginTransferCommands() {
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if (VkResult rs = vkBeginCommandBuffer(tcmdBuffers[currentTransfer], &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to begin command buffer: {}", string_VkResult(rs)));
}

void RendererVk::endTransferCommands() {
	try {
		if (VkResult rs = vkEndCommandBuffer(tcmdBuffers[currentTransfer]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to end command buffer: {}", string_VkResult(rs)));
		if (VkResult rs = vkResetFences(ldev, 1, &tfences[currentTransfer]); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to reset fence: {}", string_VkResult(rs)));

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &tcmdBuffers[currentTransfer]
		};
		if (VkResult rs = vkQueueSubmit(tqueue, 1, &submitInfo, tfences[currentTransfer]); rs != VK_SUCCESS)
			throw std::runtime_error(fmt::format("Failed to submit command buffer: {}", string_VkResult(rs)));

		transferRunning[currentTransfer] = true;
		currentTransfer = (currentTransfer + 1) % FormatConverter::maxTransfers;
	} catch (const std::runtime_error&) {
		if (VkResult rs = vkResetCommandBuffer(tcmdBuffers[currentTransfer], 0); rs != VK_SUCCESS)
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to reset command buffer: %s", string_VkResult(rs));
		throw;
	}
}

void RendererVk::syncTransferCommands() {
	if (transferRunning[currentTransfer]) {
		if (VkResult rs = vkWaitForFences(ldev, 1, &tfences[currentTransfer], VK_TRUE, UINT64_MAX); rs != VK_SUCCESS && rs != VK_TIMEOUT)
			throw std::runtime_error(std::format("Failed to wait for fence: {}", string_VkResult(rs)));
		if (VkResult rs = vkResetCommandBuffer(tcmdBuffers[currentTransfer], 0); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to reset command buffer: {}", string_VkResult(rs)));
		transferRunning[currentTransfer] = false;
	}
}

void RendererVk::transitionBuffer(VkBuffer buffer, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32 srcQfamily, uint32 dstQfamily) const noexcept {
	VkBufferMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.srcQueueFamilyIndex = srcQfamily,
		.dstQueueFamilyIndex = dstQfamily,
		.buffer = buffer,
		.size = VK_WHOLE_SIZE
	};
	vkCmdPipelineBarrier(tcmdBuffers[currentTransfer], srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void RendererVk::transitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32 srcQfamily, uint32 dstQfamily) const noexcept {
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.oldLayout = srcLayout,
		.newLayout = dstLayout,
		.srcQueueFamilyIndex = srcQfamily,
		.dstQueueFamilyIndex = dstQfamily,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	vkCmdPipelineBarrier(tcmdBuffers[currentTransfer], srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void RendererVk::transitionBufferToImageLayout(VkBuffer buffer, VkImage image) const noexcept {
	VkBufferMemoryBarrier bufferBarrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.size = VK_WHOLE_SIZE
	};
	VkImageMemoryBarrier imageBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	vkCmdPipelineBarrier(tcmdBuffers[currentTransfer], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 1, &imageBarrier);
}

void RendererVk::copyBufferToImage(VkBuffer buffer, VkImage image, u32vec2 size) const noexcept {
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.imageExtent = { size.x, size.y, 1 }
	};
	vkCmdCopyBufferToImage(tcmdBuffers[currentTransfer], buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

RendererVk::SurfaceInfo RendererVk::pickPixFormat(SDL_Surface* img, bool srgb) const noexcept {
	if (!img)
		return SurfaceInfo();

	switch (SDL_PixelFormatEnum sfmt = surfaceFormat(img); sfmt) {
	case SDL_PIXELFORMAT_ABGR8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32);
	case SDL_PIXELFORMAT_ARGB8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM);
	case SDL_PIXELFORMAT_BGRA8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_R });
	case SDL_PIXELFORMAT_RGBA8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R });
	case SDL_PIXELFORMAT_XBGR8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ONE });
	case SDL_PIXELFORMAT_XRGB8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE });
	case SDL_PIXELFORMAT_BGRX8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_ONE });
	case SDL_PIXELFORMAT_RGBX8888:
		return SurfaceInfo(img, srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_A, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ONE });
	case SDL_PIXELFORMAT_RGB24:
		if (fmtConv.initialized())
			return SurfaceInfo(img, srgb, FormatConverter::Pipeline::rgb24);
		break;
	case SDL_PIXELFORMAT_BGR24:
		if (fmtConv.initialized())
			return SurfaceInfo(img, srgb, FormatConverter::Pipeline::bgr24);
		break;
#ifdef WITH_SDL3
	case SDL_PIXELFORMAT_ABGR2101010: case SDL_PIXELFORMAT_XBGR2101010:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::A2B10G10R10, OptTexFmt::A2R10G10B10 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_ARGB2101010: case SDL_PIXELFORMAT_XRGB2101010:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::A2R10G10B10, OptTexFmt::A2B10G10R10 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
#else
	case SDL_PIXELFORMAT_ARGB2101010:
		if (optionalFormats[eint(OptTexFmt::A2R10G10B10)])
			return SurfaceInfo(img, VK_FORMAT_A2R10G10B10_UNORM_PACK32);
		if (optionalFormats[eint(OptTexFmt::A2B10G10R10)])
			return SurfaceInfo(img, VK_FORMAT_A2B10G10R10_UNORM_PACK32, { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A });
		break;
#endif
	case SDL_PIXELFORMAT_BGR565:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::B5G6R5, OptTexFmt::R5G6B5 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_RGB565:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::R5G6B5, OptTexFmt::B5G6R5 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_ABGR1555: case SDL_PIXELFORMAT_ARGB1555: case SDL_PIXELFORMAT_XBGR1555: case SDL_PIXELFORMAT_XRGB1555:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::A1R5G5B5, OptTexFmt::B5G5R5A1, OptTexFmt::R5G5B5A1 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_BGRA5551:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::B5G5R5A1, OptTexFmt::R5G5B5A1, OptTexFmt::A1R5G5B5 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_RGBA5551:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::R5G5B5A1, OptTexFmt::B5G5R5A1, OptTexFmt::A1R5G5B5 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_ABGR4444: case SDL_PIXELFORMAT_XBGR4444:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::A4B4G4R4, OptTexFmt::A4R4G4B4, OptTexFmt::B4G4R4A4, OptTexFmt::R4G4B4A4 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_ARGB4444: case SDL_PIXELFORMAT_XRGB4444:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::A4R4G4B4, OptTexFmt::A4B4G4R4, OptTexFmt::R4G4B4A4, OptTexFmt::B4G4R4A4 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_BGRA4444:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::B4G4R4A4, OptTexFmt::R4G4B4A4, OptTexFmt::A4B4G4R4, OptTexFmt::A4R4G4B4 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_RGBA4444:
		if (auto [fmt, swizzle] = pickPixFormat(sfmt, { OptTexFmt::R4G4B4A4, OptTexFmt::B4G4R4A4, OptTexFmt::A4R4G4B4, OptTexFmt::A4B4G4R4 }); fmt != VK_FORMAT_UNDEFINED)
			return SurfaceInfo(img, fmt, swizzle);
		break;
	case SDL_PIXELFORMAT_INDEX8:
		if (isIndexedGrayscale(img))
			return SurfaceInfo(img, srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM, { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE });
		if (fmtConv.initialized())
			return SurfaceInfo(img, srgb, FormatConverter::Pipeline::index8);
	}
	return SurfaceInfo(convertReplace(img), srgb ? VK_FORMAT_A8B8G8R8_SRGB_PACK32 : VK_FORMAT_A8B8G8R8_UNORM_PACK32);
}

pair<VkFormat, InstanceVk::Swizzle> RendererVk::pickPixFormat(SDL_PixelFormatEnum sfmt, std::initializer_list<OptTexFmt> fmtv) const noexcept {
	SDL_PackedOrder spo = SDL_PackedOrder(SDL_PIXELORDER(sfmt));
	for (OptTexFmt it : fmtv)
		if (optionalFormats[eint(it)]) {
			auto [sdlf, vkf] = optTexFmtMap[eint(it)];
			return pair(vkf, swizzlePixFormat(spo, SDL_PackedOrder(SDL_PIXELORDER(sdlf))));
		}
	return pair(VK_FORMAT_UNDEFINED, Swizzle());
}

InstanceVk::Swizzle RendererVk::swizzlePixFormat(SDL_PackedOrder spo, SDL_PackedOrder dpo) noexcept {
	Swizzle swizzle{};
	if (spo != dpo) {
		if (spo + 2 != dpo) {	// rule out same format but source without alpha
			if ((spo & 1) == (dpo & 1))	// alpha in same position and colors reversed
				swizzle = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
			else {	// everything shifted
				if (spo == SDL_PACKEDORDER_ARGB || spo == SDL_PACKEDORDER_XRGB || spo == SDL_PACKEDORDER_BGRA || spo == SDL_PACKEDORDER_BGRX) {
					swizzle.r = VK_COMPONENT_SWIZZLE_G;
					swizzle.b = VK_COMPONENT_SWIZZLE_A;
				} else {
					swizzle.r = VK_COMPONENT_SWIZZLE_A;
					swizzle.b = VK_COMPONENT_SWIZZLE_G;
				}
				if (dpo == SDL_PACKEDORDER_ARGB || dpo == SDL_PACKEDORDER_BGRA) {
					swizzle.g = VK_COMPONENT_SWIZZLE_R;
					swizzle.a = VK_COMPONENT_SWIZZLE_B;
				} else {
					swizzle.g = VK_COMPONENT_SWIZZLE_B;
					swizzle.a = VK_COMPONENT_SWIZZLE_R;
				}
			}
		}
		if ((spo - 1) % 4 < 2)
			swizzle.a = VK_COMPONENT_SWIZZLE_ONE;
	}
	return swizzle;
}

pair<SDL_PixelFormatEnum, uint8> RendererVk::prepareImageFormat(SDL_Surface* img) const noexcept {
	SDL_PixelFormatEnum fmt = surfaceFormat(img);
	if (fmt == SDL_PIXELFORMAT_INDEX8 && isIndexedGrayscale(img))
		return pair(SDL_PIXELFORMAT_INDEX8, 1);
	if (compression == Settings::Compression::b16 && (SDL_BYTESPERPIXEL(fmt) > 2 || SDL_ISPIXELFORMAT_INDEXED(fmt)))
		fmt = SDL_ISPIXELFORMAT_ALPHA(fmt) ? SDL_PIXELFORMAT_ARGB1555 : SDL_PIXELFORMAT_BGR565;

	switch (SDL_PIXELLAYOUT(fmt)) {
	case SDL_PACKEDLAYOUT_565:
		if (optionalFormats[eint(OptTexFmt::B5G6R5)] || optionalFormats[eint(OptTexFmt::R5G6B5)])
			return pair(fmt, 2);
		return pickImageFormat({ OptTexFmt::A1R5G5B5, OptTexFmt::B5G5R5A1, OptTexFmt::R5G5B5A1 }, fmt);
	case SDL_PACKEDLAYOUT_1555: case SDL_PACKEDLAYOUT_5551:
		if (optionalFormats[eint(OptTexFmt::A1R5G5B5)] || optionalFormats[eint(OptTexFmt::B5G5R5A1)] || optionalFormats[eint(OptTexFmt::R5G5B5A1)])
			return pair(fmt, 2);
		return pickImageFormat({ OptTexFmt::B5G6R5, OptTexFmt::R5G6B5 }, fmt);
	case SDL_PACKEDLAYOUT_4444:
		if (optionalFormats[eint(OptTexFmt::A4B4G4R4)] || optionalFormats[eint(OptTexFmt::A4R4G4B4)] || optionalFormats[eint(OptTexFmt::B4G4R4A4)] || optionalFormats[eint(OptTexFmt::R4G4B4A4)])
			return pair(fmt, 2);
		return pickImageFormat({ OptTexFmt::A1R5G5B5, OptTexFmt::B5G5R5A1, OptTexFmt::R5G5B5A1, OptTexFmt::B5G6R5, OptTexFmt::R5G6B5 }, fmt);
	case SDL_PACKEDLAYOUT_332:
		return pickImageFormat({ OptTexFmt::R5G6B5, OptTexFmt::B5G6R5, OptTexFmt::A1R5G5B5, OptTexFmt::B5G5R5A1, OptTexFmt::R5G5B5A1 }, fmt);
#ifdef WITH_SDL3
	default:
		if (SDL_BYTESPERPIXEL(fmt) > 4)
			return pickImageFormat({ OptTexFmt::A2B10G10R10, OptTexFmt::A2R10G10B10 }, fmt);
#endif
	}
	return pair(fmt, 4);
}

pair<SDL_PixelFormatEnum, uint8> RendererVk::pickImageFormat(std::initializer_list<OptTexFmt> fmtv, SDL_PixelFormatEnum orig) const noexcept {
	for (OptTexFmt it : fmtv)
		if (optionalFormats[eint(it)])
			return pair(optTexFmtMap[eint(it)].first, SDL_BYTESPERPIXEL(optTexFmtMap[eint(it)].first));
	return pair(orig, 4);
}

void RendererVk::setCompression(Settings* sets) noexcept {
	if (sets->compression != Settings::Compression::none && !(sets->compression == Settings::Compression::b16 && canTexturesB16()))
		sets->compression = Settings::Compression::none;
	compression = sets->compression;
}

vector<const char*> RendererVk::getRequiredInstanceExtensions(InstanceInfo& instInfo) const {
	uint32 extensionCount;
#ifdef WITH_SDL3
	const char* const* iexts = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	if (!iexts)
		throw std::runtime_error(SDL_GetError());
	vector<const char*> extensions(iexts, iexts + extensionCount);
#else
	if (!SDL_Vulkan_GetInstanceExtensions(instInfo.window, &extensionCount, nullptr))
		throw std::runtime_error(SDL_GetError());
	vector<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(instInfo.window, &extensionCount, extensions.data());
#endif
	if (vkGetPhysicalDeviceFeatures2KHR) {
		if (VkResult rs = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); rs == VK_SUCCESS) {
			uptr<VkExtensionProperties[]> instanceExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.get());
			if (instInfo.khrGetPhysicalDeviceProperties2 = std::any_of(instanceExtensions.get(), instanceExtensions.get() + extensionCount, [](const VkExtensionProperties& it) -> bool { return !strcmp(it.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); }); instInfo.khrGetPhysicalDeviceProperties2)
				extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		} else
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to enumerate instance extensions: %s", string_VkResult(rs));
	}
#ifndef NDEBUG
	if (vkCreateDebugUtilsMessengerEXT && vkDestroyDebugUtilsMessengerEXT) {
		try {
			uint32 layerCount;
			if (VkResult rs = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); rs != VK_SUCCESS)
				throw std::runtime_error(fmt::format("Failed to enumerate layers: {}", string_VkResult(rs)));
			uptr<VkLayerProperties[]> availableLayers = std::make_unique_for_overwrite<VkLayerProperties[]>(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.get());
			if (std::none_of(availableLayers.get(), availableLayers.get() + layerCount, [](const VkLayerProperties& lp) -> bool { return !strcmp(lp.layerName, validationLayerName); }))
				throw std::runtime_error("Validation layers not available");

			if (VkResult rs = vkEnumerateInstanceExtensionProperties(validationLayerName, &extensionCount, nullptr); rs != VK_SUCCESS)
				throw std::runtime_error(fmt::format("Failed to enumerate layer extensions: {}", string_VkResult(rs)));
			uptr<VkExtensionProperties[]> availableExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
			vkEnumerateInstanceExtensionProperties(validationLayerName, &extensionCount, availableExtensions.get());
			if (instInfo.extDebugUtils = std::any_of(availableExtensions.get(), availableExtensions.get() + extensionCount, [](const VkExtensionProperties& it) -> bool { return !strcmp(it.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }); !instInfo.extDebugUtils)
				throw std::runtime_error("Instance extension " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " not available");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} catch (const std::runtime_error& err) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", err.what());
		}
	}
#endif
	return extensions;
}

bool RendererVk::checkImageFormats(DeviceInfo& deviceInfo) const {
	constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VkImageFormatProperties imgp;
	for (VkFormat fmt : { VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8_UNORM })
		if (vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, fmt, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) != VK_SUCCESS)
			return false;

	if (auto df4 = rng::find(deviceInfo.extensions, VK_EXT_4444_FORMATS_EXTENSION_NAME); df4 != deviceInfo.extensions.end()) {
		VkPhysicalDeviceFeatures2KHR deviceFeatures2 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.pNext = &deviceInfo.formatsFeatures
		};
		vkGetPhysicalDeviceFeatures2KHR(deviceInfo.dev, &deviceFeatures2);
		deviceInfo.formatsFeatures.formatA4B4G4R4 = deviceInfo.formatsFeatures.formatA4B4G4R4 && vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS;
		deviceInfo.formatsFeatures.formatA4R4G4B4 = deviceInfo.formatsFeatures.formatA4R4G4B4 && vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS;
		if (!(deviceInfo.formatsFeatures.formatA4R4G4B4 || deviceInfo.formatsFeatures.formatA4B4G4R4))
			deviceInfo.extensions.erase(df4);
	}
	deviceInfo.canSrgb = rng::all_of(array{ VK_FORMAT_A8B8G8R8_SRGB_PACK32, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8_SRGB }, [this, &deviceInfo, &imgp](VkFormat fmt) -> bool { return vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, fmt, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS; });
	deviceInfo.formats = {
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B5G6R5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R5G6B5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A1R5G5B5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B5G5R5A1_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R5G5B5A1_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		bool(deviceInfo.formatsFeatures.formatA4B4G4R4),
		bool(deviceInfo.formatsFeatures.formatA4R4G4B4),
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B4G4R4A4_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R4G4B4A4_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS,
		vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &imgp) == VK_SUCCESS
	};
	return true;
}

bool RendererVk::findQueueFamilies(DeviceInfo& deviceInfo) const {
	uint32 count;
	vkGetPhysicalDeviceQueueFamilyProperties(deviceInfo.dev, &count, nullptr);
	uptr<VkQueueFamilyProperties[]> families = std::make_unique_for_overwrite<VkQueueFamilyProperties[]>(count);
	vkGetPhysicalDeviceQueueFamilyProperties(deviceInfo.dev, &count, families.get());

	VkBool32 support;
	optional<pair<uint32, uint32>> gqids, pqids, tqids;
	for (uint32 i = 0; i < count; ++i) {
		bool isGraphics = VkQueueFlagBits(families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
		bool isCompute = families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
		if (isGraphics && (!gqids || gqids == pqids || gqids == tqids))
			assignQueueIndices(deviceInfo, families.get(), i, gqids);
		if ((!pqids || pqids == gqids || pqids == tqids) && std::all_of(views.get(), views.get() + numViews, [this, &deviceInfo, i, &support](ViewVk& it) -> bool { return vkGetPhysicalDeviceSurfaceSupportKHR(deviceInfo.dev, i, it.surface, &support) == VK_SUCCESS && support; }))
			assignQueueIndices(deviceInfo, families.get(), i, pqids);
		if (isGraphics && (!tqids || (isCompute ? !deviceInfo.canCompute || tqids == gqids || tqids == pqids : !deviceInfo.canCompute && (tqids == gqids || tqids == pqids)))) {
			assignQueueIndices(deviceInfo, families.get(), i, tqids);
			deviceInfo.canCompute = isCompute;
		}
	}
	if (!(gqids && pqids && tqids))
		return false;
	deviceInfo.graphicsQids = *gqids;
	deviceInfo.presentQids = *pqids;
	deviceInfo.transferQids = *tqids;
	return true;
}

void RendererVk::assignQueueIndices(DeviceInfo& deviceInfo, const VkQueueFamilyProperties* families, uint32 fi, optional<pair<uint32, uint32>>& qids) {
	if (!deviceInfo.numQfams || deviceInfo.qfqcnts[deviceInfo.numQfams - 1].first != fi)
		deviceInfo.qfqcnts[deviceInfo.numQfams++] = pair(fi, 1);
	else if (deviceInfo.qfqcnts[deviceInfo.numQfams - 1].second < families[fi].queueCount)
		++deviceInfo.qfqcnts[deviceInfo.numQfams - 1].second;
	qids = pair(fi, deviceInfo.qfqcnts[deviceInfo.numQfams - 1].second - 1);
}

bool RendererVk::chooseSurfaceFormat(DeviceInfo& deviceInfo) const {
	std::vector<VkSurfaceFormatKHR> commonFormats;
	for (uint8 i = 0; i < numViews; ++i) {
		uint32 count;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(deviceInfo.dev, views[i].surface, &count, nullptr) != VK_SUCCESS || vkGetPhysicalDeviceSurfaceFormatsKHR(deviceInfo.dev, views[i].surface, &count, nullptr) != VK_SUCCESS)
			return false;
		uptr<VkSurfaceFormatKHR[]> formats = std::make_unique_for_overwrite<VkSurfaceFormatKHR[]>(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(deviceInfo.dev, views[i].surface, &count, formats.get());

		if (commonFormats.empty())
			commonFormats.assign(formats.get(), formats.get() + count);
		else {
			commonFormats.erase(std::remove_if(commonFormats.begin(), commonFormats.end(), [&formats, count](const VkSurfaceFormatKHR& cf) -> bool { return std::none_of(formats.get(), formats.get() + count, [&cf](const VkSurfaceFormatKHR& lf) -> bool { return lf.format == cf.format && lf.colorSpace == cf.colorSpace; }); }), commonFormats.end());
			if (commonFormats.empty())
				return false;
		}
	}

	for (VkSurfaceFormatKHR it : commonFormats)
		if (it.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			bool isSrgb = it.format == VK_FORMAT_B8G8R8A8_SRGB || it.format == VK_FORMAT_R8G8B8A8_SRGB || it.format == VK_FORMAT_A8B8G8R8_SRGB_PACK32 || it.format == VK_FORMAT_B8G8R8_SRGB || it.format == VK_FORMAT_R8G8B8_SRGB;
			if (deviceInfo.surfaceFormats[0].format == VK_FORMAT_UNDEFINED && !isSrgb)
				deviceInfo.surfaceFormats[0] = it;
			if (deviceInfo.surfaceFormats[1].format == VK_FORMAT_UNDEFINED && isSrgb == deviceInfo.canSrgb)
				deviceInfo.surfaceFormats[1] = it;
		}
	for (size_t i = 0; i < deviceInfo.surfaceFormats.size(); ++i)
		if (deviceInfo.surfaceFormats[i].format == VK_FORMAT_UNDEFINED)
			deviceInfo.surfaceFormats[i] = commonFormats[0];
	return true;
}

pair<VkPresentModeKHR, uint32> RendererVk::chooseSwapPresentMode(VkSurfaceKHR surface, const VkSurfaceCapabilitiesKHR& capabilities) const {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(fmt::format("Failed to get present modes: {}", string_VkResult(rs)));
	uptr<VkPresentModeKHR[]> presentModes = std::make_unique_for_overwrite<VkPresentModeKHR[]>(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &count, presentModes.get());

	VkPresentModeKHR rmode = VK_PRESENT_MODE_FIFO_KHR;
	if (immediatePresent) {
		if (std::any_of(presentModes.get(), presentModes.get() + count, [](VkPresentModeKHR pm) -> bool { return pm == VK_PRESENT_MODE_IMMEDIATE_KHR; }))
			rmode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		else if (std::any_of(presentModes.get(), presentModes.get() + count, [](VkPresentModeKHR pm) -> bool { return pm == VK_PRESENT_MODE_FIFO_RELAXED_KHR; }))
			rmode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	} else if (std::any_of(presentModes.get(), presentModes.get() + count, [](VkPresentModeKHR pm) -> bool { return pm == VK_PRESENT_MODE_MAILBOX_KHR; }))
		rmode = VK_PRESENT_MODE_MAILBOX_KHR;

	uint32 rimg = std::max(ViewVk::maxFrames, capabilities.minImageCount) + (rmode != VK_PRESENT_MODE_IMMEDIATE_KHR);
	return pair(rmode, !capabilities.maxImageCount || capabilities.maxImageCount >= rimg ? rimg : capabilities.maxImageCount);
}

void RendererVk::setUsesSrgb(Settings* sets) noexcept {
	if (usesSrgb = sets->gammaType == Settings::Gamma::srgb; usesSrgb)
		if (usesSrgb = canSrgb; !usesSrgb)
			sets->gammaType = Settings::Gamma::none;
}

uint RendererVk::scoreDevice(const DeviceInfo& devi) {
	uint score = 0;
	switch (devi.prop.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 4;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 16;
		for (uint32 i = 0; i < devi.memp.memoryTypeCount; ++i)
			if (rng::any_of(deviceMemoryTypes, [&devi, i](VkMemoryPropertyFlags it) -> bool { return devi.memp.memoryTypes[i].propertyFlags == it; }))
				score += devi.memp.memoryHeaps[devi.memp.memoryTypes[i].heapIndex].size / 1024 / 1024 / 1024;
	}
	score += devi.prop.limits.maxImageDimension2D / 2048;
	score += devi.prop.limits.maxMemoryAllocationCount / 1024 / 1024;
	return score + std::accumulate(devi.formats.begin(), devi.formats.end(), 0u) / 2;
}

Renderer::Info RendererVk::getInfo() const {
	Info info = {
		.devices = { Info::Device(u32vec2(0), "auto") },
		.gamma = { Settings::Gamma::none, Settings::Gamma::value },
		.compressions = { Settings::Compression::none },
		.texSize = maxTextureSize,
		.curGamma = usesSrgb ? Settings::Gamma::srgb : renderPass.getFinPipeline() ? Settings::Gamma::value : Settings::Gamma::none,
		.curCompression = compression
	};
	if (canSrgb)
		info.gamma.insert(info.gamma.begin() + 1, Settings::Gamma::srgb);
	if (canTexturesB16())
		info.compressions.push_back(Settings::Compression::b16);

	if (uint32 count; vkEnumeratePhysicalDevices(instance, &count, nullptr) == VK_SUCCESS) {
		uptr<VkPhysicalDevice[]> pdevs = std::make_unique_for_overwrite<VkPhysicalDevice[]>(count);
		vkEnumeratePhysicalDevices(instance, &count, pdevs.get());
		VkPhysicalDeviceProperties prop;
		VkPhysicalDeviceMemoryProperties memp;
		for (uint32 i = 0; i < count; ++i) {
			vkGetPhysicalDeviceProperties(pdevs[i], &prop);
			vkGetPhysicalDeviceMemoryProperties(pdevs[i], &memp);
			uintptr_t memest = 0;
			for (uint32 j = 0; j < memp.memoryTypeCount; ++j)
				if (memp.memoryTypes[j].propertyFlags == deviceMemoryTypes[0])
					if (VkMemoryHeap& heap = memp.memoryHeaps[memp.memoryTypes[j].heapIndex]; heap.size > memest)
						memest = heap.size;
			info.devices.emplace_back(u32vec2(prop.vendorID, prop.deviceID), prop.deviceName, memest);
		}
	}
	return info;
}

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL RendererVk::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) noexcept {
	if (pCallbackData->messageIdNumber && pCallbackData->messageIdNumber != 0x539277AF) {	// ignore Steam JSON and AMD semaphore errors
		SDL_LogPriority prio;
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			prio = SDL_LOG_PRIORITY_VERBOSE;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			prio = SDL_LOG_PRIORITY_INFO;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			prio = SDL_LOG_PRIORITY_WARN;
			break;
		default:
			prio = SDL_LOG_PRIORITY_ERROR;
		}

		const char* type;
		switch (messageType) {
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			type = "general";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			type = "validation";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			type = "performance";
			break;
		default:
			type = "unknown";
		}
		SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, prio, "%s (%X), Type: %s, Message: %s", coalesce(pCallbackData->pMessageIdName, ""), pCallbackData->messageIdNumber, type, coalesce(pCallbackData->pMessage, ""));
	}
	return VK_FALSE;
}
#endif
#endif
