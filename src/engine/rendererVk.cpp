#ifdef WITH_VULKAN
#include "rendererVk.h"
#include <format>
#include <list>
#include <numeric>
#include <source_location>
#include <SDL_vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

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
		&& (vkCmdCopyImageToBuffer = reinterpret_cast<PFN_vkCmdCopyImageToBuffer>(vkGetInstanceProcAddr(instance, "vkCmdCopyImageToBuffer")))
		&& (vkCmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(vkGetInstanceProcAddr(instance, "vkCmdDispatch")))
		&& (vkCmdDraw = reinterpret_cast<PFN_vkCmdDraw>(vkGetInstanceProcAddr(instance, "vkCmdDraw")))
		&& (vkCmdEndRenderPass = reinterpret_cast<PFN_vkCmdEndRenderPass>(vkGetInstanceProcAddr(instance, "vkCmdEndRenderPass")))
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
		&& (vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR")))
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
}

pair<VkBuffer, VkDeviceMemory> InstanceVk::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VkBuffer buffer;
	if (VkResult rs = vkCreateBuffer(ldev, &bufferInfo, nullptr, &buffer); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create buffer: {}", string_VkResult(rs)));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(ldev, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};
	VkDeviceMemory memory;
	if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &memory); rs != VK_SUCCESS) {
		vkDestroyBuffer(ldev, buffer, nullptr);
		throw std::runtime_error(std::format("Failed to allocate buffer memory: {}", string_VkResult(rs)));
	}
	if (VkResult rs = vkBindBufferMemory(ldev, buffer, memory, 0); rs != VK_SUCCESS) {
		vkDestroyBuffer(ldev, buffer, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		throw std::runtime_error(std::format("Failed to bind buffer memory: {}", string_VkResult(rs)));
	}
	return pair(buffer, memory);
}

void InstanceVk::recreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	vkDestroyBuffer(ldev, buffer, nullptr);
	vkFreeMemory(ldev, memory, nullptr);
	buffer = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
	std::tie(buffer, memory) = createBuffer(size, usage, properties);
}

uint32 InstanceVk::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const {
	for (uint32 i = 0; i < pdevMemProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (pdevMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	throw std::runtime_error(std::format("Failed to find suitable memory type for properties {}", string_VkMemoryPropertyFlags(properties)));
}

// GENERIC PIPELINE

VkSampler GenericPipeline::createSampler(const InstanceVk* vk, VkFilter filter) {
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
	if (VkResult rs = vk->vkCreateSampler(vk->getLdev(), &samplerInfo, nullptr, &sampler); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create texture sampler: {}", string_VkResult(rs)));
	return sampler;
}

VkShaderModule GenericPipeline::createShaderModule(const InstanceVk* vk, const uint32* code, size_t clen) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = clen;
	createInfo.pCode = code;

	VkShaderModule shaderModule;
	if (VkResult rs = vk->vkCreateShaderModule(vk->getLdev(), &createInfo, nullptr, &shaderModule); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create shader module: {}", string_VkResult(rs)));
	return shaderModule;
}

// FORMAT CONVERTER

void FormatConverter::init(const InstanceVk* vk) {
	createDescriptorSetLayout(vk);
	createPipelines(vk);
	createDescriptorPoolAndSet(vk);
}

void FormatConverter::createDescriptorSetLayout(const InstanceVk* vk) {
	VkDescriptorSetLayoutBinding bindings[2] = { {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	}, {
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	} };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings),
		.pBindings = bindings
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter descriptor set layout: {}", string_VkResult(rs)));
}

void FormatConverter::createPipelines(const InstanceVk* vk) {
	static constexpr uint32 compCode[] = {
#ifdef NDEBUG
#include "shaders/vkConv.comp.rel.h"
#else
#include "shaders/vkConv.comp.dbg.h"
#endif
	};
	VkShaderModule compShaderModule = createShaderModule(vk, compCode, sizeof(compCode));

	VkPushConstantRange pushConstant = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(PushData)
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstant
	};
	if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter pipeline layout: {}", string_VkResult(rs)));

	VkSpecializationMapEntry specializationEntry = {
		.constantID = 0,
		.offset = offsetof(SpecializationData, orderRgb),
		.size = sizeof(SpecializationData::orderRgb)
	};
	SpecializationData specializationDatas[2] = { { .orderRgb = VK_FALSE }, { .orderRgb = VK_TRUE } };
	VkSpecializationInfo specializationInfos[2]{};
	VkComputePipelineCreateInfo pipelineInfos[2]{};
	for (uint i = 0; i < pipelines.size(); ++i) {
		specializationInfos[i].mapEntryCount = 1;
		specializationInfos[i].pMapEntries = &specializationEntry;
		specializationInfos[i].dataSize = sizeof(SpecializationData);
		specializationInfos[i].pData = &specializationDatas[i];

		pipelineInfos[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfos[i].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineInfos[i].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineInfos[i].stage.module = compShaderModule;
		pipelineInfos[i].stage.pName = "main";
		pipelineInfos[i].stage.pSpecializationInfo = &specializationInfos[i];
		pipelineInfos[i].layout = pipelineLayout;
	}
	if (VkResult rs = vk->vkCreateComputePipelines(vk->getLdev(), VK_NULL_HANDLE, pipelines.size(), pipelineInfos, nullptr, pipelines.data()); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter pipelines: {}", string_VkResult(rs)));
	vk->vkDestroyShaderModule(vk->getLdev(), compShaderModule, nullptr);
}

void FormatConverter::createDescriptorPoolAndSet(const InstanceVk* vk) {
	VkDescriptorPoolSize poolSizes[2] = { {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = maxTransfers
	}, {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = maxTransfers
	} };
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = maxTransfers,
		.poolSizeCount = std::size(poolSizes),
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter descriptor pool: {}", string_VkResult(rs)));

	array<VkDescriptorSetLayout, maxTransfers> layouts;
	layouts.fill(descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = layouts.size(),
		.pSetLayouts = layouts.data()
	};
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate converter descriptor sets: {}", string_VkResult(rs)));
}

void FormatConverter::updateBufferSize(const InstanceVk* vk, uint id, VkDeviceSize texSize, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update) {
	VkDeviceSize outputSize = roundToMultiple(texSize * 4, convWgrpSize * 4 * sizeof(uint32));
	if (outputSize > outputSizesMax[id]) {
		vk->recreateBuffer(outputBuffers[id], outputMemory[id], outputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		outputSizesMax[id] = outputSize;
		update = true;
	}
	if (update) {
		VkDescriptorBufferInfo inputBufferInfo = {
			.buffer = inputBuffer,
			.range = inputSize
		};
		VkDescriptorBufferInfo outputBufferInfo = {
			.buffer = outputBuffers[id],
			.range = outputSize
		};
		VkWriteDescriptorSet descriptorWrites[2] = { {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[id],
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &inputBufferInfo
		}, {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[id],
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &outputBufferInfo
		} };
		vk->vkUpdateDescriptorSets(vk->getLdev(), std::size(descriptorWrites), descriptorWrites, 0, nullptr);
		update = false;
	}
}

void FormatConverter::free(const InstanceVk* vk) {
	for (VkPipeline it : pipelines)
		vk->vkDestroyPipeline(vk->getLdev(), it, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), pipelineLayout, nullptr);
	vk->vkDestroyDescriptorPool(vk->getLdev(), descriptorPool, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayout, nullptr);
	for (VkBuffer it : outputBuffers)
		vk->vkDestroyBuffer(vk->getLdev(), it, nullptr);
	for (VkDeviceMemory it : outputMemory)
		vk->vkFreeMemory(vk->getLdev(), it, nullptr);
}

// RENDER PASS

RenderPass::DescriptorSetBlock::DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets) :
	used(descriptorSets.begin(), descriptorSets.begin() + 1),
	free(descriptorSets.begin() + 1, descriptorSets.end())
{}

vector<VkDescriptorSet> RenderPass::init(const InstanceVk* vk, VkFormat format, uint32 numViews) {
	createRenderPass(vk, format);
	samplers = { createSampler(vk, VK_FILTER_LINEAR), createSampler(vk, VK_FILTER_NEAREST) };
	createDescriptorSetLayout(vk);
	createPipeline(vk);
	return createDescriptorPoolAndSets(vk, numViews);
}

void RenderPass::createRenderPass(const InstanceVk* vk, VkFormat format) {
	VkAttachmentDescription colorAttachment = {
		.format = format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};
	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef
	};
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	if (VkResult rs = vk->vkCreateRenderPass(vk->getLdev(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create render pass: {}", string_VkResult(rs)));
}

void RenderPass::createDescriptorSetLayout(const InstanceVk* vk) {
	VkDescriptorSetLayoutBinding bindings0[2] = { {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	}, {
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32(samplers.size()),
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = samplers.data()
	} };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings0),
		.pBindings = bindings0
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayouts[0]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout 0: {}", string_VkResult(rs)));

	VkDescriptorSetLayoutBinding binding1 = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding1;
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayouts[1]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout 1: {}", string_VkResult(rs)));
}

void RenderPass::createPipeline(const InstanceVk* vk) {
	static constexpr uint32 vertCode[] = {
#ifdef NDEBUG
#include "shaders/vkGui.vert.rel.h"
#else
#include "shaders/vkGui.vert.dbg.h"
#endif
	};
	static constexpr uint32 fragCode[] = {
#ifdef NDEBUG
#include "shaders/vkGui.frag.rel.h"
#else
#include "shaders/vkGui.frag.dbg.h"
#endif
	};
	VkShaderModule vertShaderModule = createShaderModule(vk, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(vk, fragCode, sizeof(fragCode));

	VkPipelineShaderStageCreateInfo shaderStages[2] = { {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main"
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	} };
	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = sizeof(vec2),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
	VkVertexInputAttributeDescription attributeDescription = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = 0
	};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &attributeDescription
	};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	};
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.lineWidth = 1.f
	};
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	};
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = std::size(dynamicStates),
		.pDynamicStates = dynamicStates
	};
	VkPushConstantRange pushConstant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.size = sizeof(PushData)
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = uint32(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstant
	};
	if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = std::size(shaderStages),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout,
		.renderPass = handle,
		.subpass = 0
	};
	if (VkResult rs = vk->vkCreateGraphicsPipelines(vk->getLdev(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create graphics pipeline: {}", string_VkResult(rs)));

	vk->vkDestroyShaderModule(vk->getLdev(), fragShaderModule, nullptr);
	vk->vkDestroyShaderModule(vk->getLdev(), vertShaderModule, nullptr);
}

vector<VkDescriptorSet> RenderPass::createDescriptorPoolAndSets(const InstanceVk* vk, uint32 numViews) {
	VkDescriptorPoolSize poolSizes[2] = { {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = numViews
	}, {
		.type = VK_DESCRIPTOR_TYPE_SAMPLER,
		.descriptorCount = uint32(samplers.size()) * numViews
	} };
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = numViews,
		.poolSizeCount = std::size(poolSizes),
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create view descriptor pool: {}", string_VkResult(rs)));

	vector<VkDescriptorSetLayout> layouts(poolInfo.maxSets, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = uint32(layouts.size()),
		.pSetLayouts = layouts.data()
	};
	vector<VkDescriptorSet> descriptorSets(layouts.size());
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate view descriptor sets: {}", string_VkResult(rs)));
	return descriptorSets;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::newDescriptorSetTex(const InstanceVk* vk, VkImageView imageView) {
	pair<VkDescriptorPool, VkDescriptorSet> dpds = getDescriptorSetTex(vk);
	updateDescriptorSet(vk, dpds.second, imageView);
	return dpds;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::getDescriptorSetTex(const InstanceVk* vk) {
	if (umap<VkDescriptorPool, DescriptorSetBlock>::iterator psit = rng::find_if(poolSetTex, [](const pair<const VkDescriptorPool, DescriptorSetBlock>& it) -> bool { return !it.second.free.empty(); }); psit != poolSetTex.end()) {
		VkDescriptorSet descriptorSet = *psit->second.free.begin();
		psit->second.free.erase(psit->second.free.begin());
		return pair(psit->first, *psit->second.used.insert(descriptorSet).first);
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
		throw std::runtime_error(std::format("Failed to create texture descriptor pool: {}", string_VkResult(rs)));

	array<VkDescriptorSetLayout, textureSetStep> layouts;
	layouts.fill(descriptorSetLayouts[1]);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descPool,
		.descriptorSetCount = layouts.size(),
		.pSetLayouts = layouts.data()
	};
	array<VkDescriptorSet, textureSetStep> descriptorSets;
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS) {
		vk->vkDestroyDescriptorPool(vk->getLdev(), descPool, nullptr);
		throw std::runtime_error(std::format("Failed to allocate texture descriptor sets: {}", string_VkResult(rs)));
	}
	return pair(descPool, *poolSetTex.emplace(descPool, descriptorSets).first->second.used.begin());
}

void RenderPass::freeDescriptorSetTex(const InstanceVk* vk, VkDescriptorPool pool, VkDescriptorSet dset) {
	umap<VkDescriptorPool, DescriptorSetBlock>::iterator psit = poolSetTex.find(pool);
	if (psit == poolSetTex.end())
		return;
	uset<VkDescriptorSet>::iterator duit = psit->second.used.find(dset);
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

void RenderPass::updateDescriptorSet(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer) {
	VkDescriptorBufferInfo bufferInfo = {
		.buffer = uniformBuffer,
		.range = sizeof(UniformData)
	};
	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferInfo
	};
	vk->vkUpdateDescriptorSets(vk->getLdev(), 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::updateDescriptorSet(const InstanceVk* vk, VkDescriptorSet descriptorSet, VkImageView imageView) {
	VkDescriptorImageInfo imageInfo = {
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = &imageInfo
	};
	vk->vkUpdateDescriptorSets(vk->getLdev(), 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::free(const InstanceVk* vk) {
	vk->vkDestroyPipeline(vk->getLdev(), pipeline, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), pipelineLayout, nullptr);
	vk->vkDestroyRenderPass(vk->getLdev(), handle, nullptr);

	for (auto& [pool, block] : poolSetTex)
		vk->vkDestroyDescriptorPool(vk->getLdev(), pool, nullptr);
	vk->vkDestroyDescriptorPool(vk->getLdev(), descriptorPool, nullptr);
	for (VkDescriptorSetLayout it : descriptorSetLayouts)
		vk->vkDestroyDescriptorSetLayout(vk->getLdev(), it, nullptr);
	for (VkSampler it : samplers)
		vk->vkDestroySampler(vk->getLdev(), it, nullptr);
}

// ADDRESS PASS

void AddressPass::init(const InstanceVk* vk) {
	createRenderPass(vk);
	createDescriptorSetLayout(vk);
	createPipeline(vk);
	createUniformBuffer(vk);
	createDescriptorPoolAndSet(vk);
	updateDescriptorSet(vk);
}

void AddressPass::createRenderPass(const InstanceVk* vk) {
	VkAttachmentDescription colorAttachment = {
		.format = format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef
	};
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};
	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	if (VkResult rs = vk->vkCreateRenderPass(vk->getLdev(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create render pass: {}", string_VkResult(rs)));
}

void AddressPass::createDescriptorSetLayout(const InstanceVk* vk) {
	VkDescriptorSetLayoutBinding binding = {
		.binding = bindingUdat,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding
	};
	if (VkResult rs = vk->vkCreateDescriptorSetLayout(vk->getLdev(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));
}

void AddressPass::createPipeline(const InstanceVk* vk) {
	static constexpr uint32 vertCode[] = {
#ifdef NDEBUG
#include "shaders/vkSel.vert.rel.h"
#else
#include "shaders/vkSel.vert.dbg.h"
#endif
	};
	static constexpr uint32 fragCode[] = {
#ifdef NDEBUG
#include "shaders/vkSel.frag.rel.h"
#else
#include "shaders/vkSel.frag.dbg.h"
#endif
	};
	VkShaderModule vertShaderModule = createShaderModule(vk, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(vk, fragCode, sizeof(fragCode));

	VkPipelineShaderStageCreateInfo shaderStages[2] = { {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main"
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main"
	} };
	VkVertexInputBindingDescription bindingDescription = {
		.binding = 0,
		.stride = sizeof(vec2),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
	VkVertexInputAttributeDescription attributeDescription = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = 0
	};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 1,
		.pVertexAttributeDescriptions = &attributeDescription
	};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
	};
	VkRect2D scissor = { .extent = { 1, 1 } };
	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
		.pScissors = &scissor
	};
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.lineWidth = 1.f
	};
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
	};
	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment
	};
	VkDynamicState dynamicStateList = VK_DYNAMIC_STATE_VIEWPORT;
	VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateList
	};
	VkPushConstantRange pushConstant = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.size = sizeof(PushData)
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstant
	};
	if (VkResult rs = vk->vkCreatePipelineLayout(vk->getLdev(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = std::size(shaderStages),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pColorBlendState = &colorBlending,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout,
		.renderPass = handle,
		.subpass = 0
	};
	if (VkResult rs = vk->vkCreateGraphicsPipelines(vk->getLdev(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create graphics pipeline: {}", string_VkResult(rs)));

	vk->vkDestroyShaderModule(vk->getLdev(), fragShaderModule, nullptr);
	vk->vkDestroyShaderModule(vk->getLdev(), vertShaderModule, nullptr);
}

void AddressPass::createUniformBuffer(const InstanceVk* vk) {
	std::tie(uniformBuffer, uniformBufferMemory) = vk->createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vk->vkMapMemory(vk->getLdev(), uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&uniformBufferMapped)); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to map uniform buffer memory: {}", string_VkResult(rs)));
}

void AddressPass::createDescriptorPoolAndSet(const InstanceVk* vk) {
	VkDescriptorPoolSize poolSize = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1
	};
	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize
	};
	if (VkResult rs = vk->vkCreateDescriptorPool(vk->getLdev(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor pool: {}", string_VkResult(rs)));

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};
	if (VkResult rs = vk->vkAllocateDescriptorSets(vk->getLdev(), &allocInfo, &descriptorSet); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate descriptor sets: {}", string_VkResult(rs)));
}

void AddressPass::updateDescriptorSet(const InstanceVk* vk) {
	VkDescriptorBufferInfo bufferInfo = {
		.buffer = uniformBuffer,
		.range = sizeof(UniformData)
	};
	VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet,
		.dstBinding = bindingUdat,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &bufferInfo
	};
	vk->vkUpdateDescriptorSets(vk->getLdev(), 1, &descriptorWrite, 0, nullptr);
}

void AddressPass::free(const InstanceVk* vk) {
	vk->vkDestroyPipeline(vk->getLdev(), pipeline, nullptr);
	vk->vkDestroyPipelineLayout(vk->getLdev(), pipelineLayout, nullptr);
	vk->vkDestroyRenderPass(vk->getLdev(), handle, nullptr);
	vk->vkDestroyBuffer(vk->getLdev(), uniformBuffer, nullptr);
	vk->vkFreeMemory(vk->getLdev(), uniformBufferMemory, nullptr);
	vk->vkDestroyDescriptorPool(vk->getLdev(), descriptorPool, nullptr);
	vk->vkDestroyDescriptorSetLayout(vk->getLdev(), descriptorSetLayout, nullptr);
}

// RENDERER VK

RendererVk::TextureVk::TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet) noexcept :
	Texture(size),
	pool(descriptorPool),
	set(descriptorSet)
{}

RendererVk::TextureVk::TextureVk(uvec2 size, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet, uint samplerId) noexcept :
	Texture(size),
	pool(descriptorPool),
	set(descriptorSet),
	sid(samplerId)
{}

RendererVk::RendererVk(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	Renderer(0),
	bgColor{ { { bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a } } },
	immediatePresent(!sets->vsync)
{
	ViewVk* vtmp = nullptr;
	try {
		InstanceInfo instanceInfo = createInstance(windows.begin()->second);	// using just one window to get extensions should be fine
		if (isSingleWindow(windows)) {
			SDL_Vulkan_GetDrawableSize(windows.begin()->second, &viewRes.x, &viewRes.y);
			auto view = static_cast<ViewVk*>(views.emplace(singleDspId, vtmp = new ViewVk(windows.begin()->second, Recti(ivec2(0), viewRes))).first->second);
			vtmp = nullptr;
			if (!SDL_Vulkan_CreateSurface(windows.begin()->second, instance, &view->surface))
				throw std::runtime_error(SDL_GetError());
		} else {
			views.reserve(windows.size());
			for (auto [id, win] : windows) {
				Recti wrect = sets->displays.at(id).translate(-origin);
				SDL_Vulkan_GetDrawableSize(windows.begin()->second, &wrect.w, &wrect.h);
				viewRes = glm::max(viewRes, wrect.end());
				auto view = static_cast<ViewVk*>(views.emplace(id, new ViewVk(win, wrect)).first->second);
				vtmp = nullptr;
				if (!SDL_Vulkan_CreateSurface(win, instance, &view->surface))
					throw std::runtime_error(SDL_GetError());
			}
		}
		uptr<DeviceInfo> deviceInfo = pickPhysicalDevice(instanceInfo, sets->device);
		createDevice(*deviceInfo);

		tcmdPool = createCommandPool(deviceInfo->tfam);
		allocateCommandBuffers(tcmdPool, tcmdBuffers.data(), tcmdBuffers.size());
		rng::generate(tfences, [this]() -> VkFence { return createFence(VK_FENCE_CREATE_SIGNALED_BIT); });
		if (deviceInfo->canCompute) {
			try {
				fmtConv.init(this);
				transferAtomSize = roundToMultiple(FormatConverter::convWgrpSize * 3 * sizeof(uint32), transferAtomSize);	// before this transferAtomSize must be set to nonCoherentAtomSize
			} catch (const std::runtime_error& err) {
				logError(err.what());
				fmtConv.free(this);
				fmtConv = FormatConverter();
			}
		}

		gcmdPool = createCommandPool(deviceInfo->gfam);
		vector<VkDescriptorSet> descriptorSets = renderPass.init(this, surfaceFormat.format, views.size());
		for (size_t d = 0; auto [id, view] : views) {
			auto vw = static_cast<ViewVk*>(view);
			createSwapchain(vw);
			initView(vw, descriptorSets[d++]);
		}

		addressPass.init(this);
		std::tie(addrImage, addrImageMemory) = createImage(u32vec2(1), VK_IMAGE_TYPE_1D, AddressPass::format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		addrView = createImageView(addrImage, VK_IMAGE_VIEW_TYPE_1D, AddressPass::format);
		addrFramebuffer = createFramebuffer(addressPass.getHandle(), addrView, u32vec2(1));
		std::tie(addrBuffer, addrBufferMemory) = createBuffer(sizeof(u32vec2), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (VkResult rs = vkMapMemory(ldev, addrBufferMemory, 0, sizeof(u32vec2), 0, reinterpret_cast<void**>(&addrMappedMemory)); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to map address lookup memory: {}", string_VkResult(rs)));
		allocateCommandBuffers(gcmdPool, &commandBufferAddr, 1);
		addrFence = createFence();

		createVertexBuffer();
		setCompression(sets->compression);
		setMaxPicRes(sets->maxPicRes);
		if (!sets->picLim.size) {
			for (uint32 i = 0; i < pdevMemProperties.memoryHeapCount; ++i)
				if ((pdevMemProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && pdevMemProperties.memoryHeaps[i].size > sets->picLim.size)
					sets->picLim.size = pdevMemProperties.memoryHeaps[i].size;
			sets->picLim.size /= 2;
			recommendPicRamLimit(sets->picLim.size);
		}
	} catch (...) {
		delete vtmp;
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
	vkDeviceWaitIdle(ldev);

	vkDestroyBuffer(ldev, vertexBuffer, nullptr);
	vkFreeMemory(ldev, vertexBufferMemory, nullptr);

	addressPass.free(this);
	vkDestroyFramebuffer(ldev, addrFramebuffer, nullptr);
	vkDestroyImageView(ldev, addrView, nullptr);
	vkDestroyImage(ldev, addrImage, nullptr);
	vkFreeMemory(ldev, addrImageMemory, nullptr);
	vkDestroyBuffer(ldev, addrBuffer, nullptr);
	vkFreeMemory(ldev, addrBufferMemory, nullptr);
	vkDestroyFence(ldev, addrFence, nullptr);

	for (auto [id, view] : views) {
		auto vw = static_cast<ViewVk*>(view);
		freeFramebuffers(vw);
		vkDestroySwapchainKHR(ldev, vw->swapchain, nullptr);
		freeView(vw);
	}
	renderPass.free(this);
	vkDestroyCommandPool(ldev, gcmdPool, nullptr);

	fmtConv.free(this);
	for (VkBuffer it : inputBuffers)
		vkDestroyBuffer(ldev, it, nullptr);
	for (VkDeviceMemory it : inputMemory)
		vkFreeMemory(ldev, it, nullptr);
	for (VkFence it : tfences)
		vkDestroyFence(ldev, it, nullptr);
	vkDestroyCommandPool(ldev, tcmdPool, nullptr);

	vkDestroyDevice(ldev, nullptr);
#ifndef NDEBUG
	if (dbgMessenger != VK_NULL_HANDLE)
		if (auto pfnDestroyDebugUtilsMessengerExt = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")))
			pfnDestroyDebugUtilsMessengerExt(instance, dbgMessenger, nullptr);
#endif
	for (auto [id, view] : views) {
		auto vw = static_cast<ViewVk*>(view);
		vkDestroySurfaceKHR(instance, vw->surface, nullptr);
		delete vw;
	}
	vkDestroyInstance(instance, nullptr);
}

RendererVk::InstanceInfo RendererVk::createInstance(SDL_Window* window) {
	initGlobalFunctions();

	InstanceInfo instInfo = checkInstanceExtensionSupport();
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
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
		.pfnUserCallback = debugCallback
	};
#endif
	vector<const char*> extensions = getRequiredInstanceExtensions(instInfo, window);
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
		if (auto pfnCreateDebugUtilsMessengerExt = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); !pfnCreateDebugUtilsMessengerExt || pfnCreateDebugUtilsMessengerExt(instance, &debugCreateInfo, nullptr, &dbgMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger");
#endif
	return instInfo;
}

uptr<RendererVk::DeviceInfo> RendererVk::pickPhysicalDevice(const InstanceInfo& instanceInfo, u32vec2& preferred) {
	uint32 deviceCount;
	if (VkResult rs = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to find devices: {}", string_VkResult(rs)));
	uptr<VkPhysicalDevice[]> devices = std::make_unique_for_overwrite<VkPhysicalDevice[]>(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.get());

	uptr<DeviceInfo> last = std::make_unique<DeviceInfo>();
	uptr<DeviceInfo> devi = std::make_unique<DeviceInfo>();
	for (uint32 d = 0; d < deviceCount; ++d) {
		devi->dev = devices[d];
		uint32 extensionCount;
		if (vkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extensionCount, nullptr) != VK_SUCCESS)
			continue;
		uptr<VkExtensionProperties[]> availableExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
		vkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extensionCount, availableExtensions.get());

		std::set<string> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::set<string> optionalExtensions = instanceInfo.khrGetPhysicalDeviceProperties2 ? std::set<string>{ VK_EXT_4444_FORMATS_EXTENSION_NAME } : std::set<string>();
		for (uint32 i = 0; i < extensionCount && !(requiredExtensions.empty() && optionalExtensions.empty()); ++i)
			if (string name = availableExtensions[i].extensionName; requiredExtensions.erase(name) || optionalExtensions.erase(name))
				devi->extensions.insert(std::move(name));
		if (!requiredExtensions.empty())
			continue;

		vkGetPhysicalDeviceProperties(devices[d], &devi->prop);
		if (devi->prop.limits.maxUniformBufferRange < std::max(sizeof(RenderPass::UniformData), sizeof(AddressPass::UniformData))
			|| devi->prop.limits.maxPushConstantsSize < std::max(sizeof(RenderPass::PushData), sizeof(AddressPass::PushData))
			|| devi->prop.limits.minMemoryMapAlignment < alignof(void*)
		)
			continue;

		vkGetPhysicalDeviceMemoryProperties(devices[d], &devi->memp);
		std::list<VkMemoryPropertyFlags> requiredMemoryTypes(deviceMemoryTypes.begin(), deviceMemoryTypes.end());
		for (uint32 i = 0; i < devi->memp.memoryTypeCount && !requiredMemoryTypes.empty(); ++i)
			if (std::list<VkMemoryPropertyFlags>::iterator it = rng::find_if(requiredMemoryTypes, [&devi, i](VkMemoryPropertyFlags flg) -> bool { return (devi->memp.memoryTypes[i].propertyFlags & flg) == flg; }); it != requiredMemoryTypes.end())
				requiredMemoryTypes.erase(it);
		if (!requiredMemoryTypes.empty())
			continue;

		if (!(checkImageFormats(*devi) && findQueueFamilies(*devi) && chooseSurfaceFormat(*devi)))
			continue;

		if (devi->prop.vendorID == preferred.x && devi->prop.deviceID == preferred.y)
			return devi;
		if (devi->score = scoreDevice(*devi); last->dev == VK_NULL_HANDLE || devi->score > last->score)
			*last = *devi;
	}
	if (last->dev == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable device");
	preferred = u32vec2(0);
	return last;
}

void RendererVk::createDevice(DeviceInfo& deviceInfo) {
	array<VkDeviceQueueCreateInfo, maxPossibleQueues> queueCreateInfos{};
	array<float, maxPossibleQueues> queuePriorities;
	queuePriorities.fill(1.f);

	for (uint32 i = 0; auto [qfam, qcnt] : deviceInfo.qfIdCnt) {
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = qfam;
		queueCreateInfos[i].queueCount = qcnt.y;
		queueCreateInfos[i++].pQueuePriorities = queuePriorities.data();
	}

	uptr<const char*[]> extensions = std::make_unique_for_overwrite<const char*[]>(deviceInfo.extensions.size());
	rng::transform(deviceInfo.extensions, extensions.get(), [](const string& it) -> const char* { return it.data(); });
	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = uint32(deviceInfo.qfIdCnt.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = uint32(deviceInfo.extensions.size()),
		.ppEnabledExtensionNames = extensions.get(),
		.pEnabledFeatures = &deviceFeatures
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
		throw std::runtime_error(std::format("Failed to create logical device: {}", string_VkResult(rs)));

	pdev = deviceInfo.dev;
	pdevMemProperties = deviceInfo.memp;
	maxTextureSize = deviceInfo.prop.limits.maxImageDimension2D;
	transferAtomSize = deviceInfo.prop.limits.nonCoherentAtomSize;
	maxComputeWorkGroups = deviceInfo.prop.limits.maxComputeWorkGroupCount[0];
	surfaceFormat = deviceInfo.surfaceFormat;
	rng::copy(deviceInfo.formats, optionalFormats.begin());
	std::tie(gfamilyIndex, gqueue) = acquireNextQueue(deviceInfo, deviceInfo.gfam);
	std::tie(pfamilyIndex, pqueue) = acquireNextQueue(deviceInfo, deviceInfo.pfam);
	std::tie(tfamilyIndex, tqueue) = acquireNextQueue(deviceInfo, deviceInfo.tfam);
}

pair<uint32, VkQueue> RendererVk::acquireNextQueue(DeviceInfo& deviceInfo, uint32 family) const {
	VkQueue queue;
	u32vec2& cnt = deviceInfo.qfIdCnt.at(family);
	vkGetDeviceQueue(ldev, family, cnt.x, &queue);
	if (cnt.x + 1 < cnt.y)
		++cnt.x;
	return pair(family, queue);
}

VkCommandPool RendererVk::createCommandPool(uint32 family) const {
	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = family
	};
	VkCommandPool commandPool;
	if (VkResult rs = vkCreateCommandPool(ldev, &poolInfo, nullptr, &commandPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create command pool: {}", string_VkResult(rs)));
	return commandPool;
}

void RendererVk::createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain) {
	VkSurfaceCapabilitiesKHR capabilities;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, view->surface, &capabilities); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get surface capabilities: {}", string_VkResult(rs)));

	view->extent = capabilities.currentExtent.width != UINT32_MAX ? capabilities.currentExtent : VkExtent2D{
		std::clamp(uint32(view->rect.w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(uint32(view->rect.h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
	uint32 queueFamilyIndices[2] = { gfamilyIndex, pfamilyIndex };
	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = view->surface,
		.minImageCount = capabilities.maxImageCount == 0 || capabilities.minImageCount < capabilities.maxImageCount ? capabilities.minImageCount + 1 : capabilities.maxImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = view->extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = chooseSwapPresentMode(view->surface),
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapchain
	};
	if (gfamilyIndex != pfamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = std::size(queueFamilyIndices);
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (VkResult rs = vkCreateSwapchainKHR(ldev, &createInfo, nullptr, &view->swapchain); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create swapchain: {}", string_VkResult(rs)));

	if (VkResult rs = vkGetSwapchainImagesKHR(ldev, view->swapchain, &view->imageCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get swapchain images: {}", string_VkResult(rs)));
	view->images = std::make_unique<VkImage[]>(view->imageCount);
	view->framebuffers = std::make_unique<pair<VkImageView, VkFramebuffer>[]>(view->imageCount);
	vkGetSwapchainImagesKHR(ldev, view->swapchain, &view->imageCount, view->images.get());
	for (uint32 i = 0; i < view->imageCount; ++i)
		view->framebuffers[i].first = createImageView(view->images[i], VK_IMAGE_VIEW_TYPE_2D, surfaceFormat.format);
}

void RendererVk::freeFramebuffers(ViewVk* view) {
	for (uint32 i = 0; i < view->imageCount; ++i) {
		vkDestroyFramebuffer(ldev, view->framebuffers[i].second, nullptr);
		vkDestroyImageView(ldev, view->framebuffers[i].first, nullptr);
	}
	view->framebuffers.reset();
	view->images.reset();
	view->imageCount = 0;
}

void RendererVk::recreateSwapchain(ViewVk* view) {
	vkDeviceWaitIdle(ldev);
	freeFramebuffers(view);
	VkSwapchainKHR oldSwapchain = view->swapchain;
	view->swapchain = VK_NULL_HANDLE;
	try {
		createSwapchain(view, oldSwapchain);
		vkDestroySwapchainKHR(ldev, oldSwapchain, nullptr);
	} catch (const std::runtime_error&) {
		vkDestroySwapchainKHR(ldev, oldSwapchain, nullptr);
		throw;
	}
	createFramebuffers(view);
	view->uniformMapped->pview = vec4(view->rect.pos(), vec2(view->rect.size()) / 2.f);
	refreshFramebuffer = false;
}

void RendererVk::initView(ViewVk* view, VkDescriptorSet descriptorSet) {
	createFramebuffers(view);
	allocateCommandBuffers(gcmdPool, view->commandBuffers.data(), view->commandBuffers.size());
	for (uint i = 0; i < ViewVk::maxFrames; ++i) {
		view->imageAvailableSemaphores[i] = createSemaphore();
		view->renderFinishedSemaphores[i] = createSemaphore();
		view->frameFences[i] = createFence(VK_FENCE_CREATE_SIGNALED_BIT);
	}

	std::tie(view->uniformBuffer, view->uniformMemory) = createBuffer(sizeof(RenderPass::UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(ldev, view->uniformMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&view->uniformMapped)); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to map uniform buffer memory 1: {}", string_VkResult(rs)));

	view->uniformMapped->pview = vec4(view->rect.pos(), vec2(view->rect.size()) / 2.f);
	view->descriptorSet = descriptorSet;
	renderPass.updateDescriptorSet(this, view->descriptorSet, view->uniformBuffer);
}

void RendererVk::freeView(ViewVk* view) {
	vkDestroyBuffer(ldev, view->uniformBuffer, nullptr);
	vkFreeMemory(ldev, view->uniformMemory, nullptr);

	for (uint i = 0; i < ViewVk::maxFrames; ++i) {
		vkDestroySemaphore(ldev, view->renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(ldev, view->imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(ldev, view->frameFences[i], nullptr);
	}
}

void RendererVk::createFramebuffers(ViewVk* view) {
	for (uint32 i = 0; i < view->imageCount; ++i)
		view->framebuffers[i].second = createFramebuffer(renderPass.getHandle(), view->framebuffers[i].first, u32vec2(view->extent.width, view->extent.height));
}

void RendererVk::createVertexBuffer() {
	VkBuffer stage = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	std::tie(vertexBuffer, vertexBufferMemory) = createBuffer(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	try {
		std::tie(stage, memory) = createBuffer(sizeof(vertices), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		void* mappedMemory;
		if (VkResult rs = vkMapMemory(ldev, memory, 0, VK_WHOLE_SIZE, 0, &mappedMemory); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to map vertex buffer memory 1: {}", string_VkResult(rs)));
		memcpy(mappedMemory, vertices.data(), sizeof(vertices));
		vkUnmapMemory(ldev, memory);

		VkBufferCopy region = { .size = sizeof(vertices) };
		beginSingleTimeCommands(commandBufferAddr);
		vkCmdCopyBuffer(commandBufferAddr, stage, vertexBuffer, 1, &region);
		endSingleTimeCommands(commandBufferAddr, addrFence, gqueue);	// using addrFence because it's the only one that starts out not signaled and needs to stay in that state anyway
		synchSingleTimeCommands(commandBufferAddr, addrFence);

		vkDestroyBuffer(ldev, stage, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
	} catch (const std::runtime_error&) {
		vkWaitForFences(ldev, 1, &addrFence, VK_TRUE, UINT64_MAX);
		vkDestroyBuffer(ldev, stage, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		throw;
	}
}

void RendererVk::setClearColor(const vec4& color) {
	bgColor = { { { color.r, color.g, color.b, color.a } } };
}

void RendererVk::setVsync(bool vsync) {
	immediatePresent = !vsync;
	refreshFramebuffer = true;
}

void RendererVk::updateView(ivec2& viewRes) {
	if (views.size() == 1) {
		SDL_Vulkan_GetDrawableSize(views.begin()->second->win, &viewRes.x, &viewRes.y);
		views.begin()->second->rect.size() = viewRes;
		refreshFramebuffer = true;
	}
}

void RendererVk::startDraw(View* view) {
	currentView = static_cast<ViewVk*>(view);
	vkWaitForFences(ldev, 1, &currentView->frameFences[currentFrame], VK_TRUE, UINT64_MAX);
	if (VkResult rs = vkAcquireNextImageKHR(ldev, currentView->swapchain, UINT64_MAX, currentView->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); rs == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain(currentView);
		throw ErrorSkip();
	} else if (rs != VK_SUCCESS && rs != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error(std::format("Failed to acquire swapchain image: {}", string_VkResult(rs)));
	vkResetFences(ldev, 1, &currentView->frameFences[currentFrame]);
	vkResetCommandBuffer(currentView->commandBuffers[currentFrame], 0);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT	// true but retarded
	};
	if (VkResult rs = vkBeginCommandBuffer(currentView->commandBuffers[currentFrame], &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to begin recording command buffer: {}", string_VkResult(rs)));

	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass.getHandle(),
		.framebuffer = currentView->framebuffers[imageIndex].second,
		.renderArea = { .extent = currentView->extent },
		.clearValueCount = 1,
		.pClearValues = &bgColor
	};
	vkCmdBeginRenderPass(currentView->commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipeline());

	VkViewport viewport = {
		.width = float(currentView->extent.width),
		.height = float(currentView->extent.height)
	};
	vkCmdSetViewport(currentView->commandBuffers[currentFrame], 0, 1, &viewport);

	VkRect2D scissor = { .extent = currentView->extent };
	vkCmdSetScissor(currentView->commandBuffers[currentFrame], 0, 1, &scissor);
	vkCmdBindDescriptorSets(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipelineLayout(), 0, 1, &currentView->descriptorSet, 0, nullptr);

	VkDeviceSize vertexOffset = 0;
	vkCmdBindVertexBuffers(currentView->commandBuffers[currentFrame], 0, 1, &vertexBuffer, &vertexOffset);
}

void RendererVk::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	auto vtx = static_cast<const TextureVk*>(tex);
	RenderPass::PushData pd = {
		.rect = rect.asVec(),
		.frame = frame.asVec(),
		.color = color,
		.sid = vtx->sid
	};
	vkCmdBindDescriptorSets(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipelineLayout(), 1, 1, &vtx->set, 0, nullptr);
	vkCmdPushConstants(currentView->commandBuffers[currentFrame], renderPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RenderPass::PushData), &pd);
	vkCmdDraw(currentView->commandBuffers[currentFrame], vertices.size(), 1, 0, 0);
}

void RendererVk::finishDraw(View*) {
	vkCmdEndRenderPass(currentView->commandBuffers[currentFrame]);
	if (VkResult rs = vkEndCommandBuffer(currentView->commandBuffers[currentFrame]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to end recording command buffer: {}", string_VkResult(rs)));

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentView->imageAvailableSemaphores[currentFrame],
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &currentView->commandBuffers[currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &currentView->renderFinishedSemaphores[currentFrame]
	};
	if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, currentView->frameFences[currentFrame]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to submit draw command buffer: {}", string_VkResult(rs)));

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &currentView->renderFinishedSemaphores[currentFrame],
		.swapchainCount = 1,
		.pSwapchains = &currentView->swapchain,
		.pImageIndices = &imageIndex
	};
	if (VkResult rs = vkQueuePresentKHR(pqueue, &presentInfo); rs == VK_ERROR_OUT_OF_DATE_KHR || rs == VK_SUBOPTIMAL_KHR || refreshFramebuffer)
		recreateSwapchain(currentView);
	else if (rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to present swapchain image: {}", string_VkResult(rs)));
}

void RendererVk::finishRender() {
	currentFrame = (currentFrame + 1) % ViewVk::maxFrames;
}

void RendererVk::startSelDraw(View* view, ivec2 pos) {
	auto vkw = static_cast<ViewVk*>(view);
	addressPass.getUniformBufferMapped()->pview = vec4(vkw->rect.pos(), vec2(vkw->rect.size()) / 2.f);
	beginSingleTimeCommands(commandBufferAddr);

	VkClearValue zero{};
	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = addressPass.getHandle(),
		.framebuffer = addrFramebuffer,
		.renderArea = { .extent = { 1, 1 } },
		.clearValueCount = 1,
		.pClearValues = &zero
	};
	vkCmdBeginRenderPass(commandBufferAddr, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBufferAddr, VK_PIPELINE_BIND_POINT_GRAPHICS, addressPass.getPipeline());

	VkViewport viewport = {
		.x = float(-pos.x),
		.y = float(-pos.y),
		.width = float(vkw->extent.width),
		.height = float(vkw->extent.height)
	};
	vkCmdSetViewport(commandBufferAddr, 0, 1, &viewport);

	VkDescriptorSet descriptorSet = addressPass.getDescriptorSet();
	vkCmdBindDescriptorSets(commandBufferAddr, VK_PIPELINE_BIND_POINT_GRAPHICS, addressPass.getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

	VkDeviceSize vertexOffset = 0;
	vkCmdBindVertexBuffers(commandBufferAddr, 0, 1, &vertexBuffer, &vertexOffset);
}

void RendererVk::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	AddressPass::PushData pd = {
		.rect = rect.asVec(),
		.frame = frame.asVec(),
		.addr = uvec2(uintptr_t(wgt), uintptr_t(wgt) >> 32)
	};
	vkCmdPushConstants(commandBufferAddr, addressPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AddressPass::PushData), &pd);
	vkCmdDraw(commandBufferAddr, vertices.size(), 1, 0, 0);
}

Widget* RendererVk::finishSelDraw(View*) {
	vkCmdEndRenderPass(commandBufferAddr);
	transitionImageLayout<VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL>(commandBufferAddr, addrImage);
	copyImageToBuffer(commandBufferAddr, addrImage, addrBuffer, u32vec2(1));
	endSingleTimeCommands(commandBufferAddr, addrFence, gqueue);
	synchSingleTimeCommands(commandBufferAddr, addrFence);
	return std::bit_cast<Widget*>(uintptr_t(addrMappedMemory->x) | (uintptr_t(addrMappedMemory->y) << 32));
}

Texture* RendererVk::texFromEmpty() {
	auto [pool, dset] = renderPass.getDescriptorSetTex(this);
	return new TextureVk(uvec2(0), pool, dset, RenderPass::samplerNearest);
}

Texture* RendererVk::texFromIcon(SDL_Surface* img) {
	return texFromRpic(limitSize(img, maxTextureSize));
}

bool RendererVk::texFromIcon(Texture* tex, SDL_Surface* img) {
	if (auto [pic, fmt, direct] = pickPixFormat(limitSize(img, maxTextureSize)); pic) {
		try {
			auto vtx = static_cast<TextureVk*>(tex);
			TextureVk ntex(uvec2(pic->w, pic->h), vtx->pool, vtx->set);
			if (direct)
				createTextureDirect<false>(static_cast<byte_t*>(pic->pixels), pic->pitch, pic->format->BytesPerPixel, fmt, ntex);
			else
				createTextureIndirect<false>(pic, fmt, ntex);
			SDL_FreeSurface(pic);
			replaceTexture(*vtx, ntex);
			return true;
		} catch (const std::runtime_error&) {
			SDL_FreeSurface(pic);
		}
	}
	return false;
}

Texture* RendererVk::texFromRpic(SDL_Surface* img) {
	if (auto [pic, fmt, direct] = pickPixFormat(img); pic) {
		auto tex = new TextureVk(uvec2(pic->w, pic->h), RenderPass::samplerLinear);
		try {
			if (direct)
				createTextureDirect(static_cast<byte_t*>(pic->pixels), pic->pitch, pic->format->BytesPerPixel, fmt, *tex);
			else
				createTextureIndirect(pic, fmt, *tex);
			SDL_FreeSurface(pic);
			return tex;
		} catch (const std::runtime_error&) {
			delete tex;
			SDL_FreeSurface(pic);
		}
	}
	return nullptr;
}

Texture* RendererVk::texFromText(const PixmapRgba& pm) {
	if (pm.res.x) {
		auto tex = new TextureVk(glm::min(pm.res, uvec2(maxTextureSize)), RenderPass::samplerNearest);
		try {
			createTextureDirect(reinterpret_cast<const byte_t*>(pm.pix.get()), pm.res.x * 4, 4, VK_FORMAT_A8B8G8R8_UNORM_PACK32, *tex);
			return tex;
		} catch (const std::runtime_error&) {
			delete tex;
		}
	}
	return nullptr;
}

bool RendererVk::texFromText(Texture* tex, const PixmapRgba& pm) {
	if (pm.res.x) {
		try {
			auto vtx = static_cast<TextureVk*>(tex);
			TextureVk ntex(glm::min(pm.res, uvec2(maxTextureSize)), vtx->pool, vtx->set);
			createTextureDirect<false>(reinterpret_cast<const byte_t*>(pm.pix.get()), pm.res.x * 4, 4, VK_FORMAT_A8B8G8R8_UNORM_PACK32, ntex);
			replaceTexture(*vtx, ntex);
			return true;
		} catch (const std::runtime_error&) {}
	}
	return false;
}

void RendererVk::freeTexture(Texture* tex) noexcept {
	if (auto vtx = static_cast<TextureVk*>(tex)) {
		vkQueueWaitIdle(gqueue);
		renderPass.freeDescriptorSetTex(this, vtx->pool, vtx->set);
		vkDestroyImageView(ldev, vtx->view, nullptr);
		vkDestroyImage(ldev, vtx->image, nullptr);
		vkFreeMemory(ldev, vtx->memory, nullptr);
		delete vtx;
	}
}

void RendererVk::replaceTexture(TextureVk& tex, TextureVk& ntex) {
	vkDestroyImageView(ldev, tex.view, nullptr);
	vkDestroyImage(ldev, tex.image, nullptr);
	vkFreeMemory(ldev, tex.memory, nullptr);
	tex.res = ntex.res;
	tex.image = ntex.image;
	tex.memory = ntex.memory;
	tex.view = ntex.view;
}

void RendererVk::synchTransfer() {
	vkWaitForFences(ldev, tfences.size(), tfences.data(), VK_TRUE, UINT64_MAX);
}

template <bool fresh>
void RendererVk::createTextureDirect(const byte_t* pix, uint32 pitch, uint8 bpp, VkFormat format, TextureVk& tex) {
	try {
		std::tie(tex.image, tex.memory) = createImage(tex.res, VK_IMAGE_TYPE_2D, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		synchSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer]);
		uploadInputData<false>(pix, tex.res, pitch, bpp);

		beginSingleTimeCommands(tcmdBuffers[currentTransfer]);
		transitionImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(tcmdBuffers[currentTransfer], tex.image);
		copyBufferToImage(tcmdBuffers[currentTransfer], inputBuffers[currentTransfer], tex.image, tex.res);
		transitionImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(tcmdBuffers[currentTransfer], tex.image);
		endSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer], tqueue);

		finalizeTexture<fresh>(tex, format);
	} catch (const std::runtime_error& err) {
		logError(err.what());
		vkDestroyImage(ldev, tex.image, nullptr);
		vkFreeMemory(ldev, tex.memory, nullptr);
		throw;
	}
}

template <bool fresh>
void RendererVk::createTextureIndirect(const SDL_Surface* img, VkFormat format, TextureVk& tex) {
	try {
		std::tie(tex.image, tex.memory) = createImage(tex.res, VK_IMAGE_TYPE_2D, VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		synchSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer]);
		uploadInputData<true>(static_cast<byte_t*>(img->pixels), tex.res, img->pitch, img->format->BytesPerPixel);

		VkDescriptorSet descriptorSet = fmtConv.getDescriptorSet(currentTransfer);
		beginSingleTimeCommands(tcmdBuffers[currentTransfer]);
		vkCmdBindPipeline(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, fmtConv.getPipeline(format == VK_FORMAT_R8G8B8_UNORM));
		vkCmdBindDescriptorSets(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, fmtConv.getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		uint32 texels = tex.res.x * tex.res.y;
		uint32 numGroups = texels / FormatConverter::convStep + bool(texels % FormatConverter::convStep);
		for (uint32 gcnt, offs = 0; offs < numGroups; offs += gcnt) {
			gcnt = std::min(numGroups - offs, maxComputeWorkGroups);
			FormatConverter::PushData pushData = { offs };
			vkCmdPushConstants(tcmdBuffers[currentTransfer], fmtConv.getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushData), &pushData);
			vkCmdDispatch(tcmdBuffers[currentTransfer], gcnt, 1, 1);
		}
		transitionBufferToImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(tcmdBuffers[currentTransfer], fmtConv.getOutputBuffer(currentTransfer), tex.image);
		copyBufferToImage(tcmdBuffers[currentTransfer], fmtConv.getOutputBuffer(currentTransfer), tex.image, tex.res);
		transitionImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(tcmdBuffers[currentTransfer], tex.image);
		endSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer], tqueue);

		finalizeTexture<fresh>(tex, VK_FORMAT_A8B8G8R8_UNORM_PACK32);
	} catch (const std::runtime_error& err) {
		logError(err.what());
		vkDestroyImage(ldev, tex.image, nullptr);
		vkFreeMemory(ldev, tex.memory, nullptr);
		throw;
	}
}

template <bool conv>
void RendererVk::uploadInputData(const byte_t* pix, u32vec2 res, uint32 pitch, uint8 bpp) {
	uint32 rowSize = res.x * bpp;
	VkDeviceSize inputSize = roundToMultiple(VkDeviceSize(rowSize) * VkDeviceSize(res.y), transferAtomSize);
	if (inputSize > inputSizesMax[currentTransfer]) {
		recreateBuffer(inputBuffers[currentTransfer], inputMemory[currentTransfer], inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (VkResult rs = vkMapMemory(ldev, inputMemory[currentTransfer], 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&inputsMapped[currentTransfer])); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to map input buffer memory: {}", string_VkResult(rs)));
		inputSizesMax[currentTransfer] = inputSize;
		rebindInputBuffer[currentTransfer] = true;
	}
	if constexpr (conv)
		fmtConv.updateBufferSize(this, currentTransfer, res.x * res.y, inputBuffers[currentTransfer], inputSizesMax[currentTransfer], rebindInputBuffer[currentTransfer]);
	copyPixels(inputsMapped[currentTransfer], pix, rowSize, pitch, rowSize, res.y);
}

template <bool fresh>
void RendererVk::finalizeTexture(TextureVk& tex, VkFormat format) {
	try {
		tex.view = createImageView(tex.image, VK_IMAGE_VIEW_TYPE_2D, format);
		if constexpr (fresh)
			std::tie(tex.pool, tex.set) = renderPass.newDescriptorSetTex(this, tex.view);
		else {
			vkQueueWaitIdle(gqueue);
			renderPass.updateDescriptorSet(this, tex.set, tex.view);
		}
		currentTransfer = (currentTransfer + 1) % FormatConverter::maxTransfers;
	} catch (const std::runtime_error&) {
		if constexpr (fresh)
			renderPass.freeDescriptorSetTex(this, tex.pool, tex.set);
		vkDestroyImageView(ldev, tex.view, nullptr);
		vkWaitForFences(ldev, 1, &tfences[currentTransfer], VK_TRUE, UINT64_MAX);
		throw;
	}
}

pair<VkImage, VkDeviceMemory> RendererVk::createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const {
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = type,
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
	VkImage image;
	if (VkResult rs = vkCreateImage(ldev, &imageInfo, nullptr, &image); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create image: {}", string_VkResult(rs)));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ldev, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
	};
	VkDeviceMemory memory;
	if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &memory); rs != VK_SUCCESS) {
		vkDestroyImage(ldev, image, nullptr);
		throw std::runtime_error(std::format("Failed to allocate image memory: {}", string_VkResult(rs)));
	}
	if (VkResult rs = vkBindImageMemory(ldev, image, memory, 0); rs != VK_SUCCESS) {
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		throw std::runtime_error(std::format("Failed to bind image memory: {}", string_VkResult(rs)));
	}
	return pair(image, memory);
}

VkImageView RendererVk::createImageView(VkImage image, VkImageViewType type, VkFormat format) const {
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = type,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	VkImageView imageView;
	if (VkResult rs = vkCreateImageView(ldev, &viewInfo, nullptr, &imageView); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create image view: {}", string_VkResult(rs)));
	return imageView;
}

VkFramebuffer RendererVk::createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const {
	VkFramebufferCreateInfo framebufferInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = rpass,
		.attachmentCount = 1,
		.pAttachments = &view,
		.width = size.x,
		.height = size.y,
		.layers = 1
	};
	VkFramebuffer framebuffer;
	if (VkResult rs = vkCreateFramebuffer(ldev, &framebufferInfo, nullptr, &framebuffer); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create framebuffer: {}", string_VkResult(rs)));
	return framebuffer;
}

void RendererVk::allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const {
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count
	};
	if (VkResult rs = vkAllocateCommandBuffers(ldev, &allocInfo, cmdBuffers); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate command buffers: {}", string_VkResult(rs)));
}

VkSemaphore RendererVk::createSemaphore() const {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (VkResult rs = vkCreateSemaphore(ldev, &semaphoreInfo, nullptr, &semaphore); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create semaphore: {}", string_VkResult(rs)));
	return semaphore;
}

VkFence RendererVk::createFence(VkFenceCreateFlags flags) const {
	VkFenceCreateInfo fenceInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = flags
	};
	VkFence fence;
	if (VkResult rs = vkCreateFence(ldev, &fenceInfo, nullptr, &fence); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create fence: {}", string_VkResult(rs)));
	return fence;
}

void RendererVk::beginSingleTimeCommands(VkCommandBuffer cmdBuffer) {
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	if (VkResult rs = vkBeginCommandBuffer(cmdBuffer, &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to begin single time command buffer: {}", string_VkResult(rs)));
}

void RendererVk::endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence, VkQueue queue) const {
	if (VkResult rs = vkEndCommandBuffer(cmdBuffer); rs != VK_SUCCESS) {
		vkResetCommandBuffer(cmdBuffer, 0);
		throw std::runtime_error(std::format("Failed to end single time command buffer: {}", string_VkResult(rs)));
	}

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuffer
	};
	if (VkResult rs = vkQueueSubmit(queue, 1, &submitInfo, fence); rs != VK_SUCCESS) {
		vkResetCommandBuffer(cmdBuffer, 0);
		throw std::runtime_error(std::format("Failed to submit single time command buffer: {}", string_VkResult(rs)));
	}
}

void RendererVk::synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const {
	vkWaitForFences(ldev, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(ldev, 1, &fence);
	vkResetCommandBuffer(cmdBuffer, 0);
}

template <VkImageLayout srcLay, VkImageLayout dstLay>
void RendererVk::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image) const {
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = srcLay,
		.newLayout = dstLay,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if constexpr (srcLay == VK_IMAGE_LAYOUT_UNDEFINED && dstLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else
		throw std::invalid_argument(std::format("Unsupported layout transition from {} to {} in {}", string_VkImageLayout(srcLay), string_VkImageLayout(dstLay), std::source_location::current().function_name()));
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

template <VkImageLayout srcLay, VkImageLayout dstLay>
void RendererVk::transitionBufferToImageLayout(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image) const {
	VkBufferMemoryBarrier bufferBarrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer,
		.size = VK_WHOLE_SIZE
	};
	VkImageMemoryBarrier imageBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = srcLay,
		.newLayout = dstLay,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if constexpr (srcLay == VK_IMAGE_LAYOUT_UNDEFINED && dstLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_NONE;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else
		throw std::invalid_argument(std::format("Unsupported layout transition from {} to {} in {}", string_VkImageLayout(srcLay), string_VkImageLayout(dstLay), std::source_location::current().function_name()));
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 1, &bufferBarrier, 1, &imageBarrier);
}

void RendererVk::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size) const {
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.imageExtent = { size.x, size.y, 1 }
	};
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RendererVk::copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size) const {
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.imageExtent = { size.x, size.y, 1 }
	};
	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
}

tuple<SDL_Surface*, VkFormat, bool> RendererVk::pickPixFormat(SDL_Surface* img) const noexcept {
	if (!img)
		return tuple(nullptr, VK_FORMAT_UNDEFINED, false);

	switch (img->format->format) {
	case SDL_PIXELFORMAT_ABGR8888:
		return tuple(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
	case SDL_PIXELFORMAT_ARGB8888:
		return tuple(img, VK_FORMAT_B8G8R8A8_UNORM, true);
	case SDL_PIXELFORMAT_RGB24:
		if (fmtConv.initialized())
			return tuple(img, VK_FORMAT_R8G8B8_UNORM, false);
		break;
	case SDL_PIXELFORMAT_BGR24:
		if (fmtConv.initialized())
			return tuple(img, VK_FORMAT_B8G8R8_UNORM, false);
		break;
	case SDL_PIXELFORMAT_BGR565:
		if (optionalFormats[eint(OptionalTextureFormat::B5G6R5)])
			return tuple(img, VK_FORMAT_B5G6R5_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_RGB565:
		if (optionalFormats[eint(OptionalTextureFormat::R5G6B5)])
			return tuple(img, VK_FORMAT_R5G6B5_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_ARGB1555:
		if (optionalFormats[eint(OptionalTextureFormat::A1R5G5B5)])
			return tuple(img, VK_FORMAT_A1R5G5B5_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_BGRA5551:
		if (optionalFormats[eint(OptionalTextureFormat::B5G5R5A1)])
			return tuple(img, VK_FORMAT_B5G5R5A1_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_RGBA5551:
		if (optionalFormats[eint(OptionalTextureFormat::R5G5B5A1)])
			return tuple(img, VK_FORMAT_R5G5B5A1_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_ABGR4444:
		if (optionalFormats[eint(OptionalTextureFormat::A4B4G4R4)])
			return tuple(img, VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT, true);
		break;
	case SDL_PIXELFORMAT_ARGB4444:
		if (optionalFormats[eint(OptionalTextureFormat::A4R4G4B4)])
			return tuple(img, VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT, true);
		break;
	case SDL_PIXELFORMAT_BGRA4444:
		if (optionalFormats[eint(OptionalTextureFormat::B4G4R4A4)])
			return tuple(img, VK_FORMAT_B4G4R4A4_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_RGBA4444:
		if (optionalFormats[eint(OptionalTextureFormat::R4G4B4A4)])
			return tuple(img, VK_FORMAT_R4G4B4A4_UNORM_PACK16, true);
		break;
	case SDL_PIXELFORMAT_ARGB2101010:
		if (optionalFormats[eint(OptionalTextureFormat::A2R10G10B10)])
			return tuple(img, VK_FORMAT_A2R10G10B10_UNORM_PACK32, true);
	}

	if (img->format->BytesPerPixel < 3) {
		if (img->format->Amask && optionalFormats[eint(OptionalTextureFormat::A1R5G5B5)])
			return tuple(convertReplace(img, SDL_PIXELFORMAT_ARGB1555), VK_FORMAT_A1R5G5B5_UNORM_PACK16, true);
		if (!img->format->Amask && optionalFormats[eint(OptionalTextureFormat::B5G6R5)])
			return tuple(convertReplace(img, SDL_PIXELFORMAT_BGR565), VK_FORMAT_B5G6R5_UNORM_PACK16, true);
	}
	return tuple(convertReplace(img), VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
}

vector<const char*> RendererVk::getRequiredInstanceExtensions(const InstanceInfo& instanceInfo, SDL_Window* win) const {
	uint count;
	if (!SDL_Vulkan_GetInstanceExtensions(win, &count, nullptr))
		throw std::runtime_error(SDL_GetError());
	vector<const char*> extensions(count);
	SDL_Vulkan_GetInstanceExtensions(win, &count, extensions.data());
	if (instanceInfo.khrGetPhysicalDeviceProperties2)
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifndef NDEBUG
	if (instanceInfo.extDebugUtils)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	return extensions;
}

bool RendererVk::checkImageFormats(DeviceInfo& deviceInfo) const {
	VkImageFormatProperties imgp;
	if (vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R32G32_UINT, VK_IMAGE_TYPE_1D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imgp) != VK_SUCCESS)
		return false;
	for (VkFormat fmt : { VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM })
		if (vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, fmt, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) != VK_SUCCESS)
			return false;

	deviceInfo.formatsFeatures = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT };
	if (std::set<string>::iterator df = deviceInfo.extensions.find(VK_EXT_4444_FORMATS_EXTENSION_NAME); df != deviceInfo.extensions.end()) {
		VkPhysicalDeviceFeatures2KHR deviceFeatures2 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.pNext = &deviceInfo.formatsFeatures
		};
		vkGetPhysicalDeviceFeatures2KHR(deviceInfo.dev, &deviceFeatures2);
		if (!(deviceInfo.formatsFeatures.formatA4R4G4B4 || deviceInfo.formatsFeatures.formatA4B4G4R4))
			deviceInfo.extensions.erase(df);
	}
	deviceInfo.formats[eint(OptionalTextureFormat::B5G6R5)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B5G6R5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::R5G6B5)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R5G6B5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::A1R5G5B5)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A1R5G5B5_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::B5G5R5A1)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B5G5R5A1_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::R5G5B5A1)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R5G5B5A1_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::A4B4G4R4)] = deviceInfo.formatsFeatures.formatA4B4G4R4;
	deviceInfo.formats[eint(OptionalTextureFormat::A4R4G4B4)] = deviceInfo.formatsFeatures.formatA4R4G4B4;
	deviceInfo.formats[eint(OptionalTextureFormat::B4G4R4A4)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_B4G4R4A4_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::R4G4B4A4)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_R4G4B4A4_UNORM_PACK16, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	deviceInfo.formats[eint(OptionalTextureFormat::A2R10G10B10)] = vkGetPhysicalDeviceImageFormatProperties(deviceInfo.dev, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) == VK_SUCCESS;
	return true;
}

bool RendererVk::findQueueFamilies(DeviceInfo& deviceInfo) const {
	uint32 count;
	vkGetPhysicalDeviceQueueFamilyProperties(deviceInfo.dev, &count, nullptr);
	uptr<VkQueueFamilyProperties[]> families = std::make_unique_for_overwrite<VkQueueFamilyProperties[]>(count);
	vkGetPhysicalDeviceQueueFamilyProperties(deviceInfo.dev, &count, families.get());

	deviceInfo.qfIdCnt.clear();
	deviceInfo.canCompute = false;
	optional<uint32> gfam, pfam, tfam;
	for (uint32 i = 0; i < count; ++i) {
		if (!pfam) {
			bool presentSupport = true;
			for (auto [id, view] : views)
				if (VkBool32 support = false; vkGetPhysicalDeviceSurfaceSupportKHR(deviceInfo.dev, i, static_cast<ViewVk*>(view)->surface, &support) != VK_SUCCESS || !support) {
					presentSupport = false;
					break;
				}
			if (presentSupport)
				pfam = i;
		}
		if (VkQueueFlagBits(families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT) && (!gfam || gfam == pfam))
			gfam = i;
		if ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && (!tfam || ((tfam == gfam || tfam == pfam) && !deviceInfo.canCompute && (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)))) {
			tfam = i;
			deviceInfo.canCompute = families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
		}
	}
	for (optional<uint32> fid : { gfam, pfam, tfam })
		if (fid)
			if (auto [it, isnew] = deviceInfo.qfIdCnt.try_emplace(*fid, 0, 1); !isnew && it->second.y < families[it->first].queueCount)
				++it->second.y;

	if (!(gfam && pfam && tfam))
		return false;
	deviceInfo.gfam = *gfam;
	deviceInfo.pfam = *pfam;
	deviceInfo.tfam = *tfam;
	return true;
}

bool RendererVk::chooseSurfaceFormat(DeviceInfo& deviceInfo) const {
	std::vector<VkSurfaceFormatKHR> commonFormats;
	std::vector<VkSurfaceFormatKHR>::iterator cfend = commonFormats.end();
	for (auto [id, view] : views) {
		VkSurfaceKHR surface = static_cast<ViewVk*>(view)->surface;
		uint32 count;
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(deviceInfo.dev, surface, &count, nullptr) != VK_SUCCESS || vkGetPhysicalDeviceSurfaceFormatsKHR(deviceInfo.dev, surface, &count, nullptr) != VK_SUCCESS)
			return false;
		uptr<VkSurfaceFormatKHR[]> formats = std::make_unique_for_overwrite<VkSurfaceFormatKHR[]>(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(deviceInfo.dev, surface, &count, formats.get());

		if (commonFormats.empty())
			commonFormats.assign(formats.get(), formats.get() + count);
		else {
			cfend = std::remove_if(commonFormats.begin(), cfend, [&formats, count](const VkSurfaceFormatKHR& cf) -> bool {
				return std::none_of(formats.get(), formats.get() + count, [cf](const VkSurfaceFormatKHR& lf) -> bool { return lf.format == cf.format && lf.colorSpace == cf.colorSpace; });
			});
			if (cfend == commonFormats.begin())
				return false;
		}
	}
	std::vector<VkSurfaceFormatKHR>::iterator it = rng::find_if(commonFormats, [](const VkSurfaceFormatKHR& cf) -> bool { return (cf.format == VK_FORMAT_B8G8R8A8_UNORM || cf.format == VK_FORMAT_R8G8B8A8_UNORM) && cf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; });
	deviceInfo.surfaceFormat = it != commonFormats.end() ? *it : commonFormats[0];
	return true;
}

VkPresentModeKHR RendererVk::chooseSwapPresentMode(VkSurfaceKHR surface) const {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get present modes: {}", string_VkResult(rs)));
	uptr<VkPresentModeKHR[]> presentModes = std::make_unique_for_overwrite<VkPresentModeKHR[]>(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surface, &count, presentModes.get());

	if (immediatePresent && std::any_of(presentModes.get(), presentModes.get() + count, [](VkPresentModeKHR pm) -> bool { return pm == VK_PRESENT_MODE_IMMEDIATE_KHR; }))
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	for (VkPresentModeKHR it : { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR })
		if (std::any_of(presentModes.get(), presentModes.get() + count, [it](VkPresentModeKHR pm) -> bool { return pm == it; }))
			return it;
	return VK_PRESENT_MODE_FIFO_KHR;
}

void RendererVk::setCompression(Settings::Compression compression) {
	if (compression == Settings::Compression::b16 && canSquashTextures()) {
		preconvertFormats = {
			{ SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_BGRA5551 },
			{ SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_ARGB1555 },
			{ SDL_PIXELFORMAT_BGRA8888, SDL_PIXELFORMAT_BGRA5551 },
			{ SDL_PIXELFORMAT_RGBA8888, SDL_PIXELFORMAT_RGBA5551 },
			{ SDL_PIXELFORMAT_XBGR8888, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_XRGB8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_BGRX8888, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_RGBX8888, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_BGR565 },
			{ SDL_PIXELFORMAT_BGR24, SDL_PIXELFORMAT_RGB565 },
			{ SDL_PIXELFORMAT_ARGB2101010, SDL_PIXELFORMAT_ARGB1555 }
		};
	} else
		preconvertFormats.clear();
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
			if (rng::any_of(deviceMemoryTypes, [devi, i](VkMemoryPropertyFlags it) -> bool { return it == devi.memp.memoryTypes[i].propertyFlags; }))
				score += devi.memp.memoryHeaps[devi.memp.memoryTypes[i].heapIndex].size / 1024 / 1024 / 1024;
	}
	score += devi.prop.limits.maxImageDimension2D / 2048;
	score += devi.prop.limits.maxMemoryAllocationCount / 1024 / 1024;
	return score + std::accumulate(devi.formats.begin(), devi.formats.end(), 0u) / 2;
}

Renderer::Info RendererVk::getInfo() const {
	Info info = {
		.devices = { Info::Device(u32vec2(0), "auto") },
		.compressions = { Settings::Compression::none },
		.texSize = maxTextureSize,
		.selecting = true
	};
	if (canSquashTextures())
		info.compressions.insert(Settings::Compression::b16);

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

RendererVk::InstanceInfo RendererVk::checkInstanceExtensionSupport() const {
	InstanceInfo info;
	if (uint32 extensionCount; vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) == VK_SUCCESS) {
		uptr<VkExtensionProperties[]> instanceExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, instanceExtensions.get());
		info.khrGetPhysicalDeviceProperties2 = std::any_of(instanceExtensions.get(), instanceExtensions.get() + extensionCount, [](const VkExtensionProperties& it) -> bool { return !strcmp(it.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); });
	}
#ifndef NDEBUG
	try {
		uint32 layerCount;
		if (VkResult rs = vkEnumerateInstanceLayerProperties(&layerCount, nullptr); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to enumerate layers: {}", string_VkResult(rs)));
		uptr<VkLayerProperties[]> availableLayers = std::make_unique_for_overwrite<VkLayerProperties[]>(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.get());
		if (std::none_of(availableLayers.get(), availableLayers.get() + layerCount, [](const VkLayerProperties& lp) -> bool { return !strcmp(lp.layerName, validationLayerName); }))
			throw std::runtime_error("Validation layers not available");

		uint32 extensionCount;
		if (VkResult rs = vkEnumerateInstanceExtensionProperties(validationLayerName, &extensionCount, nullptr); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to enumerate layer extensions: {}", string_VkResult(rs)));
		uptr<VkExtensionProperties[]> availableExtensions = std::make_unique_for_overwrite<VkExtensionProperties[]>(extensionCount);
		vkEnumerateInstanceExtensionProperties(validationLayerName, &extensionCount, availableExtensions.get());
		if (info.extDebugUtils = std::any_of(availableExtensions.get(), availableExtensions.get() + extensionCount, [](const VkExtensionProperties& it) -> bool { return !strcmp(it.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }); !info.extDebugUtils)
			logError("Instance extension " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " not available");
	} catch (const std::runtime_error& err) {
		logError(err.what());
	}
#endif
	return info;
}

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL RendererVk::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
	const char* sever;
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		sever = "verbose";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		sever = "info";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		sever = "warning";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		sever = "error";
		break;
	default:
		sever = "unknown";
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
	case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
		type = "address binding";
		break;
	default:
		type = "unknown";
	}
	logError("Debug message ", pCallbackData->messageIdNumber, ", Severity: ", sever, ", Type: ", type, ", Message: ", coalesce(pCallbackData->pMessage, ""));
	return VK_FALSE;
}
#endif
#endif
