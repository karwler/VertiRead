#ifdef WITH_VULKAN
#include "rendererVk.h"
#include "utils/settings.h"
#include <vulkan/vk_enum_string_helper.h>
#ifdef _WIN32
#include <SDL_vulkan.h>
#else
#include <SDL2/SDL_vulkan.h>
#endif
#include <list>
#include <source_location>

// GENERIC PIPELINE

VkSampler GenericPipeline::createSampler(const RendererVk* rend, VkFilter filter) {
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
	if (VkResult rs = vkCreateSampler(rend->getLogicalDevice(), &samplerInfo, nullptr, &sampler); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create texture sampler: {}", string_VkResult(rs)));
	return sampler;
}

VkShaderModule GenericPipeline::createShaderModule(const RendererVk* rend, const uint32* code, size_t clen) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = clen;
	createInfo.pCode = code;

	VkShaderModule shaderModule;
	if (VkResult rs = vkCreateShaderModule(rend->getLogicalDevice(), &createInfo, nullptr, &shaderModule); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create shader module: {}", string_VkResult(rs)));
	return shaderModule;
}

// FORMAT CONVERTER

void FormatConverter::init(const RendererVk* rend) {
	createDescriptorSetLayout(rend);
	createPipelines(rend);
	createDescriptorPoolAndSet(rend);
}

void FormatConverter::createDescriptorSetLayout(const RendererVk* rend) {
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
		.bindingCount = std::extent_v<decltype(bindings)>,
		.pBindings = bindings
	};
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter descriptor set layout: {}", string_VkResult(rs)));
}

void FormatConverter::createPipelines(const RendererVk* rend) {
	static constexpr uint32 compCode[] = {
#ifdef NDEBUG
#include "shaders/vkConv.comp.rel.h"
#else
#include "shaders/vkConv.comp.dbg.h"
#endif
	};
	VkShaderModule compShaderModule = createShaderModule(rend, compCode, sizeof(compCode));

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
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter pipeline layout: {}", string_VkResult(rs)));

	for (uint i = 0; i < pipelines.size(); ++i) {
		VkSpecializationMapEntry specializationEntry = {
			.constantID = 0,
			.offset = offsetof(SpecializationData, orderRgb),
			.size = sizeof(SpecializationData::orderRgb)
		};
		SpecializationData specializationData = { .orderRgb = i };
		VkSpecializationInfo specializationInfo = {
			.mapEntryCount = 1,
			.pMapEntries = &specializationEntry,
			.dataSize = sizeof(specializationData),
			.pData = &specializationData
		};
		VkComputePipelineCreateInfo pipelineInfo = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = compShaderModule,
				.pName = "main",
				.pSpecializationInfo = &specializationInfo
			},
			.layout = pipelineLayout
		};
		if (VkResult rs = vkCreateComputePipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelines[i]); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to create converter pipeline {}: {}", i, string_VkResult(rs)));
	}
	vkDestroyShaderModule(rend->getLogicalDevice(), compShaderModule, nullptr);
}

void FormatConverter::createDescriptorPoolAndSet(const RendererVk* rend) {
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
		.poolSizeCount = std::extent_v<decltype(poolSizes)>,
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create converter descriptor pool: {}", string_VkResult(rs)));

	array<VkDescriptorSetLayout, maxTransfers> layouts;
	layouts.fill(descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = layouts.size(),
		.pSetLayouts = layouts.data()
	};
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate converter descriptor sets: {}", string_VkResult(rs)));
}

void FormatConverter::updateBufferSize(const RendererVk* rend, uint id, VkDeviceSize texSize, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update) {
	VkDeviceSize outputSize = roundToMulOf(texSize * 4, convWgrpSize * 4 * sizeof(uint32));
	if (outputSize > outputSizesMax[id]) {
		rend->recreateBuffer(outputBuffers[id], outputMemory[id], outputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
		vkUpdateDescriptorSets(rend->getLogicalDevice(), std::extent_v<decltype(descriptorWrites)>, descriptorWrites, 0, nullptr);
		update = false;
	}
}

void FormatConverter::free(VkDevice dev) {
	for (VkPipeline it : pipelines)
		vkDestroyPipeline(dev, it, nullptr);
	vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
	vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(dev, descriptorSetLayout, nullptr);
	for (VkBuffer it : outputBuffers)
		vkDestroyBuffer(dev, it, nullptr);
	for (VkDeviceMemory it : outputMemory)
		vkFreeMemory(dev, it, nullptr);
}

// RENDER PASS

RenderPass::DescriptorSetBlock::DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets) :
	used(descriptorSets.begin(), descriptorSets.begin() + 1),
	free(descriptorSets.begin() + 1, descriptorSets.end())
{}

vector<VkDescriptorSet> RenderPass::init(const RendererVk* rend, VkFormat format, uint32 numViews) {
	createRenderPass(rend, format);
	samplers = { createSampler(rend, VK_FILTER_LINEAR), createSampler(rend, VK_FILTER_NEAREST) };
	createDescriptorSetLayout(rend);
	createPipeline(rend);
	return createDescriptorPoolAndSets(rend, numViews);
}

void RenderPass::createRenderPass(const RendererVk* rend, VkFormat format) {
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
	if (VkResult rs = vkCreateRenderPass(rend->getLogicalDevice(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create render pass: {}", string_VkResult(rs)));
}

void RenderPass::createDescriptorSetLayout(const RendererVk* rend) {
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
		.bindingCount = std::extent_v<decltype(bindings0)>,
		.pBindings = bindings0
	};
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[0]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout 0: {}", string_VkResult(rs)));

	VkDescriptorSetLayoutBinding binding1 = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding1;
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[1]); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout 1: {}", string_VkResult(rs)));
}

void RenderPass::createPipeline(const RendererVk* rend) {
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
	VkShaderModule vertShaderModule = createShaderModule(rend, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(rend, fragCode, sizeof(fragCode));

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
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
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
	array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = dynamicStates.size(),
		.pDynamicStates = dynamicStates.data()
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
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = std::extent_v<decltype(shaderStages)>,
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
	if (VkResult rs = vkCreateGraphicsPipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create graphics pipeline: {}", string_VkResult(rs)));

	vkDestroyShaderModule(rend->getLogicalDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(rend->getLogicalDevice(), vertShaderModule, nullptr);
}

vector<VkDescriptorSet> RenderPass::createDescriptorPoolAndSets(const RendererVk* rend, uint32 numViews) {
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
		.poolSizeCount = std::extent_v<decltype(poolSizes)>,
		.pPoolSizes = poolSizes
	};
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create view descriptor pool: {}", string_VkResult(rs)));

	vector<VkDescriptorSetLayout> layouts(poolInfo.maxSets, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = uint32(layouts.size()),
		.pSetLayouts = layouts.data()
	};
	vector<VkDescriptorSet> descriptorSets(layouts.size());
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate view descriptor sets: {}", string_VkResult(rs)));
	return descriptorSets;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::newDescriptorSetTex(const RendererVk* rend, VkImageView imageView) {
	pair<VkDescriptorPool, VkDescriptorSet> dpds = getDescriptorSetTex(rend);
	updateDescriptorSet(rend->getLogicalDevice(), dpds.second, imageView);
	return dpds;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::getDescriptorSetTex(const RendererVk* rend) {
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
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descPool); rs != VK_SUCCESS)
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
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS) {
		vkDestroyDescriptorPool(rend->getLogicalDevice(), descPool, nullptr);
		throw std::runtime_error(std::format("Failed to allocate texture descriptor sets: {}", string_VkResult(rs)));
	}
	return pair(descPool, *poolSetTex.emplace(descPool, descriptorSets).first->second.used.begin());
}

void RenderPass::freeDescriptorSetTex(VkDevice dev, VkDescriptorPool pool, VkDescriptorSet dset) {
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
		vkDestroyDescriptorPool(dev, pool, nullptr);
		poolSetTex.erase(psit);
	}
}

void RenderPass::updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkBuffer uniformBuffer) {
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
	vkUpdateDescriptorSets(dev, 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkImageView imageView) {
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
	vkUpdateDescriptorSets(dev, 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::free(VkDevice dev) {
	vkDestroyPipeline(dev, pipeline, nullptr);
	vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
	vkDestroyRenderPass(dev, handle, nullptr);

	for (auto& [pool, block] : poolSetTex)
		vkDestroyDescriptorPool(dev, pool, nullptr);
	vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
	for (VkDescriptorSetLayout it : descriptorSetLayouts)
		vkDestroyDescriptorSetLayout(dev, it, nullptr);
	for (VkSampler it : samplers)
		vkDestroySampler(dev, it, nullptr);
}

// ADDRESS PASS

void AddressPass::init(const RendererVk* rend) {
	createRenderPass(rend);
	createDescriptorSetLayout(rend);
	createPipeline(rend);
	createUniformBuffer(rend);
	createDescriptorPoolAndSet(rend);
	updateDescriptorSet(rend->getLogicalDevice());
}

void AddressPass::createRenderPass(const RendererVk* rend) {
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
	if (VkResult rs = vkCreateRenderPass(rend->getLogicalDevice(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create render pass: {}", string_VkResult(rs)));
}

void AddressPass::createDescriptorSetLayout(const RendererVk* rend) {
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
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor set layout: {}", string_VkResult(rs)));
}

void AddressPass::createPipeline(const RendererVk* rend) {
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
	VkShaderModule vertShaderModule = createShaderModule(rend, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(rend, fragCode, sizeof(fragCode));

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
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
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
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
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
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create pipeline layout: {}", string_VkResult(rs)));

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = std::extent_v<decltype(shaderStages)>,
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
	if (VkResult rs = vkCreateGraphicsPipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create graphics pipeline: {}", string_VkResult(rs)));

	vkDestroyShaderModule(rend->getLogicalDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(rend->getLogicalDevice(), vertShaderModule, nullptr);
}

void AddressPass::createUniformBuffer(const RendererVk* rend) {
	std::tie(uniformBuffer, uniformBufferMemory) = rend->createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(rend->getLogicalDevice(), uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&uniformBufferMapped)); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to map uniform buffer memory: {}", string_VkResult(rs)));
}

void AddressPass::createDescriptorPoolAndSet(const RendererVk* rend) {
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
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create descriptor pool: {}", string_VkResult(rs)));

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout
	};
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, &descriptorSet); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to allocate descriptor sets: {}", string_VkResult(rs)));
}

void AddressPass::updateDescriptorSet(VkDevice dev) {
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
	vkUpdateDescriptorSets(dev, 1, &descriptorWrite, 0, nullptr);
}

void AddressPass::free(VkDevice dev) {
	vkDestroyPipeline(dev, pipeline, nullptr);
	vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
	vkDestroyRenderPass(dev, handle, nullptr);
	vkDestroyBuffer(dev, uniformBuffer, nullptr);
	vkFreeMemory(dev, uniformBufferMemory, nullptr);
	vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(dev, descriptorSetLayout, nullptr);
}

// RENDERER VK

RendererVk::TextureVk::TextureVk(ivec2 size, VkImage img, VkDeviceMemory mem, VkImageView imageView, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet, uint samplerId) :
	Texture(size),
	image(img),
	memory(mem),
	view(imageView),
	pool(descriptorPool),
	set(descriptorSet),
	sid(samplerId)
{}

RendererVk::RendererVk(const umap<int, SDL_Window*>& windows, Settings* sets, ivec2& viewRes, ivec2 origin, const vec4& bgcolor) :
	Renderer({
		SDL_PIXELFORMAT_RGBA32, SDL_PIXELFORMAT_BGRA32,
		SDL_PIXELFORMAT_RGBA5551, SDL_PIXELFORMAT_BGRA5551, SDL_PIXELFORMAT_RGB565, SDL_PIXELFORMAT_BGR565
	}),
	bgColor{ { { bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a } } },
	squashPicTexels(sets->compression == Settings::Compression::b16)
{
	createInstance(windows.begin()->second);	// using just one window to get extensions should be fine
	if (windows.size() == 1 && windows.begin()->first == singleDspId) {
		SDL_Vulkan_GetDrawableSize(windows.begin()->second, &viewRes.x, &viewRes.y);
		views.emplace(singleDspId, new ViewVk(windows.begin()->second, Recti(ivec2(0), viewRes)));
		if (!SDL_Vulkan_CreateSurface(windows.begin()->second, instance, &static_cast<ViewVk*>(views.begin()->second)->surface))
			throw std::runtime_error(SDL_GetError());
	} else {
		views.reserve(windows.size());
		for (auto [id, win] : windows) {
			Recti wrect = sets->displays.at(id).translate(-origin);
			SDL_Vulkan_GetDrawableSize(windows.begin()->second, &wrect.w, &wrect.h);
			if (ViewVk* view = static_cast<ViewVk*>(views.emplace(id, new ViewVk(win, wrect)).first->second); !SDL_Vulkan_CreateSurface(win, instance, &view->surface))
				throw std::runtime_error(SDL_GetError());
			viewRes = glm::max(viewRes, wrect.end());
		}
	}
	QueueInfo queueInfo = pickPhysicalDevice(sets->device);
	createDevice(queueInfo);
	setPresentMode(sets->vsync);

	tcmdPool = createCommandPool(*queueInfo.tfam);
	allocateCommandBuffers(tcmdPool, tcmdBuffers.data(), tcmdBuffers.size());
	rng::generate(tfences, [this]() -> VkFence { return createFence(VK_FENCE_CREATE_SIGNALED_BIT); });
	if (queueInfo.canCompute) {
		try {
			fmtConv.init(this);
			supportedFormats.insert({ SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_BGR24 });
			transferAtomSize = roundToMulOf(FormatConverter::convWgrpSize * 3 * sizeof(uint32), pdevProperties.limits.nonCoherentAtomSize);
		} catch (const std::runtime_error& err) {
			logError(err.what());
			fmtConv.free(ldev);
			fmtConv = FormatConverter();
		}
	}

	gcmdPool = createCommandPool(*queueInfo.gfam);
	umap<VkFormat, uint> formatCounter;
	for (auto [id, view] : views)
		++formatCounter[createSwapchain(static_cast<ViewVk*>(view))];
	vector<VkDescriptorSet> descriptorSets = renderPass.init(this, rng::max_element(formatCounter, [](const pair<const VkFormat, uint>& a, const pair<const VkFormat, uint>& b) -> bool { return a.second < b.second; })->first, views.size());
	for (size_t d = 0; auto [id, view] : views)
		initView(static_cast<ViewVk*>(view), descriptorSets[d++]);

	addressPass.init(this);
	std::tie(addrImage, addrImageMemory) = createImage(u32vec2(1), VK_IMAGE_TYPE_1D, AddressPass::format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	addrView = createImageView(addrImage, VK_IMAGE_VIEW_TYPE_1D, AddressPass::format);
	addrFramebuffer = createFramebuffer(addressPass.getHandle(), addrView, u32vec2(1));
	std::tie(addrBuffer, addrBufferMemory) = createBuffer(sizeof(u32vec2), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(ldev, addrBufferMemory, 0, sizeof(u32vec2), 0, reinterpret_cast<void**>(&addrMappedMemory)); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to map address lookup memory: {}", string_VkResult(rs)));
	allocateCommandBuffers(gcmdPool, &commandBufferAddr, 1);
	addrFence = createFence();

	setMaxPicRes(sets->maxPicRes);
}

RendererVk::~RendererVk() {
	vkDeviceWaitIdle(ldev);

	addressPass.free(ldev);
	vkDestroyFramebuffer(ldev, addrFramebuffer, nullptr);
	vkDestroyImageView(ldev, addrView, nullptr);
	vkDestroyImage(ldev, addrImage, nullptr);
	vkFreeMemory(ldev, addrImageMemory, nullptr);
	vkDestroyBuffer(ldev, addrBuffer, nullptr);
	vkFreeMemory(ldev, addrBufferMemory, nullptr);
	vkDestroyFence(ldev, addrFence, nullptr);

	for (auto [id, view] : views) {
		ViewVk* vw = static_cast<ViewVk*>(view);
		freeFramebuffers(vw);
		vkDestroySwapchainKHR(ldev, vw->swapchain, nullptr);
	}
	renderPass.free(ldev);
	for (auto [id, view] : views)
		freeView(static_cast<ViewVk*>(view));
	vkDestroyCommandPool(ldev, gcmdPool, nullptr);

	fmtConv.free(ldev);
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
		if (PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")))
			pfnDestroyDebugUtilsMessengerEXT(instance, dbgMessenger, nullptr);
#endif
	for (auto [id, view] : views) {
		vkDestroySurfaceKHR(instance, static_cast<ViewVk*>(view)->surface, nullptr);
		delete view;
	}
	vkDestroyInstance(instance, nullptr);
}

void RendererVk::createInstance(SDL_Window* window) {
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "VertiRead",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "VertiRead_RendererVk",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
#ifdef NDEBUG
	vector<const char*> extensions = getRequiredExtensions(window);
#else
	auto [debugUtils, validationFeatures] = checkValidationLayerSupport();
	vector<const char*> extensions = getRequiredExtensions(window, debugUtils, validationFeatures);

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
		.pfnUserCallback = debugCallback
	};
	VkValidationFeaturesEXT validationFeaturesInfo = {
		.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.pNext = &debugCreateInfo,
		.enabledValidationFeatureCount = validationFeatureEnables.size(),
		.pEnabledValidationFeatures = validationFeatureEnables.data()
	};
#endif

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = uint32(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};
#ifndef NDEBUG
	if (debugUtils) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = validationFeatures ? static_cast<void*>(&validationFeaturesInfo) : static_cast<void*>(&debugCreateInfo);
	}
#endif
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance");

#ifndef NDEBUG
	if (debugUtils)
		if (PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); !pfnCreateDebugUtilsMessengerEXT || pfnCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &dbgMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger");
#endif
}

RendererVk::QueueInfo RendererVk::pickPhysicalDevice(u32vec2& preferred) {
	uint32 deviceCount;
	if (VkResult rs = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to find devices: {}", string_VkResult(rs)));
	vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	VkPhysicalDevice lastDev = VK_NULL_HANDLE;
	QueueInfo queueInfo;
	VkPhysicalDeviceProperties devProp;
	VkPhysicalDeviceMemoryProperties devMemp;
	uint score = 0;
	for (VkPhysicalDevice dev : devices) {
		uint32 extensionCount;
		if (vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
			continue;
		vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());
		std::set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (size_t i = 0; i < availableExtensions.size() && !requiredExtensions.empty(); ++i)
			requiredExtensions.erase(availableExtensions[i].extensionName);
		if (!requiredExtensions.empty())
			continue;

		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(dev, &prop);
		if (prop.limits.maxUniformBufferRange < std::max(sizeof(RenderPass::UniformData), sizeof(AddressPass::UniformData)))
			continue;
		if (prop.limits.maxPushConstantsSize < std::max(sizeof(RenderPass::PushData), sizeof(AddressPass::PushData)))
			continue;
		if (prop.limits.minMemoryMapAlignment < alignof(void*))
			continue;

		VkPhysicalDeviceMemoryProperties memp;
		vkGetPhysicalDeviceMemoryProperties(dev, &memp);
		std::list<VkMemoryPropertyFlags> requiredMemoryTypes(deviceMemoryTypes.begin(), deviceMemoryTypes.end());
		for (uint32 i = 0; i < memp.memoryTypeCount && !requiredMemoryTypes.empty(); ++i)
			if (std::list<VkMemoryPropertyFlags>::iterator it = rng::find_if(requiredMemoryTypes, [&memp, i](VkMemoryPropertyFlags flg) -> bool { return (memp.memoryTypes[i].propertyFlags & flg) == flg; }); it != requiredMemoryTypes.end())
				requiredMemoryTypes.erase(it);
		if (!requiredMemoryTypes.empty())
			continue;

		VkImageFormatProperties imgp;
		if (vkGetPhysicalDeviceImageFormatProperties(dev, VK_FORMAT_R32G32_UINT, VK_IMAGE_TYPE_1D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imgp) != VK_SUCCESS)
			continue;
		bool ok = true;
		for (VkFormat fmt : { VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM })
			if (vkGetPhysicalDeviceImageFormatProperties(dev, fmt, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, &imgp) != VK_SUCCESS) {
				ok = false;
				break;
			}
		if (!ok)
			continue;

		QueueInfo qi = findQueueFamilies(dev);
		if (!(qi.gfam && qi.pfam && qi.tfam))
			continue;

		for (auto [id, view] : views)
			if (VkSurfaceKHR surf = static_cast<ViewVk*>(view)->surface; querySurfaceFormatSupport(dev, surf).empty() || queryPresentModeSupport(dev, surf).empty()) {
				ok = false;
				break;
			}
		if (!ok)
			continue;

		if (prop.vendorID == preferred.x && prop.deviceID == preferred.y) {
			std::tie(pdev, pdevProperties, pdevMemProperties, transferAtomSize) = tuple(dev, prop, memp, prop.limits.nonCoherentAtomSize);
			return queueInfo;
		}
		if (uint points = scoreDevice(prop, memp); points > score)
			std::tie(score, lastDev, queueInfo, devProp, devMemp) = tuple(points, dev, std::move(qi), prop, memp);
	}
	if (lastDev == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable device");

	std::tie(pdev, pdevProperties, pdevMemProperties, transferAtomSize) = tuple(lastDev, devProp, devMemp, devProp.limits.nonCoherentAtomSize);
	if (preferred != u32vec2(0))
		preferred = u32vec2(0);
	return queueInfo;
}

void RendererVk::createDevice(QueueInfo& queueInfo) {
	array<VkDeviceQueueCreateInfo, maxPossibleQueues> queueCreateInfos{};
	array<float, maxPossibleQueues> queuePriorities;
	queuePriorities.fill(1.f);

	for (uint32 i = 0; auto [qfam, qcnt] : queueInfo.idcnt) {
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = qfam;
		queueCreateInfos[i].queueCount = qcnt.y;
		queueCreateInfos[i++].pQueuePriorities = queuePriorities.data();
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = uint32(queueInfo.idcnt.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = deviceExtensions.size(),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};
#ifndef NDEBUG
	if (dbgMessenger != VK_NULL_HANDLE) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
#endif
	if (VkResult rs = vkCreateDevice(pdev, &createInfo, nullptr, &ldev); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to create logical device: {}", string_VkResult(rs)));

	std::tie(gfamilyIndex, gqueue) = acquireNextQueue(queueInfo, *queueInfo.gfam);
	std::tie(pfamilyIndex, pqueue) = acquireNextQueue(queueInfo, *queueInfo.pfam);
	std::tie(tfamilyIndex, tqueue) = acquireNextQueue(queueInfo, *queueInfo.tfam);
}

pair<uint32, VkQueue> RendererVk::acquireNextQueue(QueueInfo& queueInfo, uint32 family) const {
	VkQueue queue;
	u32vec2& cnt = queueInfo.idcnt.at(family);
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

VkFormat RendererVk::createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain) {
	VkSurfaceCapabilitiesKHR capabilities;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, view->surface, &capabilities); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get surface capabilities: {}", string_VkResult(rs)));

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(querySurfaceFormatSupport(pdev, view->surface));
	view->extent = capabilities.currentExtent.width != UINT32_MAX ? capabilities.currentExtent : VkExtent2D{
		std::clamp(uint32(view->rect.w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(uint32(view->rect.h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
	array<uint32, 2> queueFamilyIndices = { gfamilyIndex, pfamilyIndex };
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
		.presentMode = chooseSwapPresentMode(queryPresentModeSupport(pdev, view->surface)),
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapchain
	};
	if (gfamilyIndex != pfamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
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
	return surfaceFormat.format;
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
	renderPass.updateDescriptorSet(ldev, view->descriptorSet, view->uniformBuffer);
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

void RendererVk::setClearColor(const vec4& color) {
	bgColor = { { { color.r, color.g, color.b, color.a } } };
}

void RendererVk::setVsync(bool vsync) {
	setPresentMode(vsync);
	refreshFramebuffer = true;
}

void RendererVk::setPresentMode(bool vsync) {
	std::set<VkPresentModeKHR> modes;
	for (auto [id, view] : views) {
		vector<VkPresentModeKHR> cmde = queryPresentModeSupport(pdev, static_cast<ViewVk*>(view)->surface);
		modes.insert(cmde.begin(), cmde.end());
	}
	if (!vsync && modes.contains(VK_PRESENT_MODE_IMMEDIATE_KHR))
		presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	else if (modes.contains(VK_PRESENT_MODE_MAILBOX_KHR))
		presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	else if (modes.contains(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
		presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	else
		presentMode = VK_PRESENT_MODE_FIFO_KHR;
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
}

void RendererVk::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	const TextureVk* vtx = static_cast<const TextureVk*>(tex);
	RenderPass::PushData pd = {
		.rect = rect.toVec(),
		.frame = frame.toVec(),
		.color = color,
		.sid = vtx->sid
	};
	vkCmdBindDescriptorSets(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipelineLayout(), 1, 1, &vtx->set, 0, nullptr);
	vkCmdPushConstants(currentView->commandBuffers[currentFrame], renderPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RenderPass::PushData), &pd);
	vkCmdDraw(currentView->commandBuffers[currentFrame], 4, 1, 0, 0);
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
	ViewVk* vkw = static_cast<ViewVk*>(view);
	addressPass.getUniformBufferMapped()->pview = vec4(vkw->rect.pos(), vec2(vkw->rect.size()) / 2.f);
	beginSingleTimeCommands(commandBufferAddr);

	VkClearValue zero{};
	VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = addressPass.getHandle(),
		.framebuffer = addrFramebuffer,
		.renderArea = { .extent = {1, 1} },
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
}

void RendererVk::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	AddressPass::PushData pd = {
		.rect = rect.toVec(),
		.frame = frame.toVec(),
		.addr = uvec2(uintptr_t(wgt), uintptr_t(wgt) >> 32)
	};
	vkCmdPushConstants(commandBufferAddr, addressPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AddressPass::PushData), &pd);
	vkCmdDraw(commandBufferAddr, 4, 1, 0, 0);
}

Widget* RendererVk::finishSelDraw(View*) {
	vkCmdEndRenderPass(commandBufferAddr);
	transitionImageLayout<VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL>(commandBufferAddr, addrImage);
	copyImageToBuffer(commandBufferAddr, addrImage, addrBuffer, u32vec2(1));
	endSingleTimeCommands(commandBufferAddr, addrFence, gqueue);
	synchSingleTimeCommands(commandBufferAddr, addrFence);
	return std::bit_cast<Widget*>(uintptr_t(addrMappedMemory->x) | (uintptr_t(addrMappedMemory->y) << 32));
}

Texture* RendererVk::texFromIcon(SDL_Surface* img) {
	img = limitSize(img, pdevProperties.limits.maxImageDimension2D);
	if (auto [pic, fmt, direct] = pickPixFormat(img); pic) {
		TextureVk* tex = direct
			? createTextureDirect(static_cast<cbyte*>(pic->pixels), u32vec2(pic->w, pic->h), pic->pitch, pic->format->BytesPerPixel, fmt, false)
			: createTextureIndirect(pic, fmt);
		SDL_FreeSurface(pic);
		return tex;
	}
	return nullptr;
}

Texture* RendererVk::texFromRpic(SDL_Surface* img) {
	if (auto [pic, fmt, direct] = pickPixFormat(img); pic) {
		TextureVk* tex = direct
			? createTextureDirect(static_cast<cbyte*>(pic->pixels), u32vec2(pic->w, pic->h), pic->pitch, pic->format->BytesPerPixel, fmt, false)
			: createTextureIndirect(pic, fmt);
		SDL_FreeSurface(pic);
		return tex;
	}
	return nullptr;
}

Texture* RendererVk::texFromText(const PixmapRgba& pm) {
	if (pm.pix)
		return createTextureDirect(reinterpret_cast<const cbyte*>(pm.pix), glm::min(pm.res, u32vec2(pdevProperties.limits.maxImageDimension2D)), pm.res.x * 4, 4, VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
	return nullptr;
}

void RendererVk::freeTexture(Texture* tex) {
	TextureVk* vtx = static_cast<TextureVk*>(tex);
	vkQueueWaitIdle(gqueue);
	renderPass.freeDescriptorSetTex(ldev, vtx->pool, vtx->set);
	vkDestroyImageView(ldev, vtx->view, nullptr);
	vkDestroyImage(ldev, vtx->image, nullptr);
	vkFreeMemory(ldev, vtx->memory, nullptr);
	delete vtx;
}

void RendererVk::synchTransfer() {
	vkWaitForFences(ldev, tfences.size(), tfences.data(), VK_TRUE, UINT64_MAX);
}

RendererVk::TextureVk* RendererVk::createTextureDirect(const cbyte* pix, u32vec2 res, uint32 pitch, uint8 bpp, VkFormat format, bool nearest) {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet dset = VK_NULL_HANDLE;
	try {
		std::tie(image, memory) = createImage(res, VK_IMAGE_TYPE_2D, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		synchSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer]);
		uploadInputData<false>(pix, res, pitch, bpp);

		beginSingleTimeCommands(tcmdBuffers[currentTransfer]);
		transitionImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(tcmdBuffers[currentTransfer], image);
		copyBufferToImage(tcmdBuffers[currentTransfer], inputBuffers[currentTransfer], image, res, pitch / bpp);
		transitionImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(tcmdBuffers[currentTransfer], image);
		endSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer], tqueue);

		view = createImageView(image, VK_IMAGE_VIEW_TYPE_2D, format);
		std::tie(pool, dset) = renderPass.newDescriptorSetTex(this, view);
		currentTransfer = (currentTransfer + 1) % FormatConverter::maxTransfers;
	} catch (const std::runtime_error& err) {
		logError(err.what());
		renderPass.freeDescriptorSetTex(ldev, pool, dset);
		vkDestroyImageView(ldev, view, nullptr);
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		return nullptr;
	}
	return new TextureVk(res, image, memory, view, pool, dset, nearest);
}

RendererVk::TextureVk* RendererVk::createTextureIndirect(const SDL_Surface* img, VkFormat format) {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet dset = VK_NULL_HANDLE;
	u32vec2 res(img->w, img->h);
	try {
		std::tie(image, memory) = createImage(res, VK_IMAGE_TYPE_2D, VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		synchSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer]);
		uploadInputData<true>(static_cast<cbyte*>(img->pixels), res, img->pitch, img->format->BytesPerPixel);

		array<VkDescriptorSet, 1> descriptorSets = { fmtConv.getDescriptorSet(currentTransfer) };
		beginSingleTimeCommands(tcmdBuffers[currentTransfer]);
		vkCmdBindPipeline(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, fmtConv.getPipeline(format == VK_FORMAT_R8G8B8_UNORM));
		vkCmdBindDescriptorSets(tcmdBuffers[currentTransfer], VK_PIPELINE_BIND_POINT_COMPUTE, fmtConv.getPipelineLayout(), 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);

		uint32 texels = res.x * res.y;
		uint32 numGroups = texels / FormatConverter::convStep + bool(texels % FormatConverter::convStep);
		for (uint32 gcnt, offs = 0; offs < numGroups; offs += gcnt) {
			gcnt = std::min(numGroups - offs, pdevProperties.limits.maxComputeWorkGroupCount[0]);
			FormatConverter::PushData pushData = { offs };
			vkCmdPushConstants(tcmdBuffers[currentTransfer], fmtConv.getPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushData), &pushData);
			vkCmdDispatch(tcmdBuffers[currentTransfer], gcnt, 1, 1);
		}
		transitionBufferToImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(tcmdBuffers[currentTransfer], fmtConv.getOutputBuffer(currentTransfer), image);
		copyBufferToImage(tcmdBuffers[currentTransfer], fmtConv.getOutputBuffer(currentTransfer), image, res);
		transitionImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(tcmdBuffers[currentTransfer], image);
		endSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer], tqueue);

		view = createImageView(image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_A8B8G8R8_UNORM_PACK32);
		std::tie(pool, dset) = renderPass.newDescriptorSetTex(this, view);
		currentTransfer = (currentTransfer + 1) % FormatConverter::maxTransfers;
	} catch (const std::runtime_error& err) {
		logError(err.what());
		renderPass.freeDescriptorSetTex(ldev, pool, dset);
		vkDestroyImageView(ldev, view, nullptr);
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		return nullptr;
	}
	return new TextureVk(res, image, memory, view, pool, dset, false);
}

template <bool conv>
void RendererVk::uploadInputData(const cbyte* pix, u32vec2 res, uint32 pitch, uint8 bpp) {
	VkDeviceSize texSize, pixSize;
	if constexpr (conv) {
		texSize = VkDeviceSize(res.x) * VkDeviceSize(res.y);
		pixSize = texSize * VkDeviceSize(bpp);
	} else
		pixSize = VkDeviceSize(pitch) * VkDeviceSize(res.y);

	VkDeviceSize inputSize = roundToMulOf(pixSize, transferAtomSize);
	if (inputSize > inputSizesMax[currentTransfer]) {
		recreateBuffer(inputBuffers[currentTransfer], inputMemory[currentTransfer], inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (VkResult rs = vkMapMemory(ldev, inputMemory[currentTransfer], 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&inputsMapped[currentTransfer])); rs != VK_SUCCESS)
			throw std::runtime_error(std::format("Failed to map input buffer memory: {}", string_VkResult(rs)));
		inputSizesMax[currentTransfer] = inputSize;
		rebindInputBuffer[currentTransfer] = true;
	}
	if constexpr (conv) {
		fmtConv.updateBufferSize(this, currentTransfer, texSize, inputBuffers[currentTransfer], inputSizesMax[currentTransfer], rebindInputBuffer[currentTransfer]);

		if (size_t packPitch = size_t(res.x) * size_t(bpp); pitch == packPitch)
			std::copy_n(pix, pixSize, inputsMapped[currentTransfer]);
		else
			for (size_t o = 0, i = 0, e = size_t(pitch) * size_t(res.y); i < e; i += pitch, o += packPitch)
				std::copy_n(pix + i, packPitch, inputsMapped[currentTransfer] + o);
	} else
		std::copy_n(pix, pixSize, inputsMapped[currentTransfer]);
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

void RendererVk::recreateBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
	vkDestroyBuffer(ldev, buffer, nullptr);
	vkFreeMemory(ldev, memory, nullptr);
	buffer = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
	std::tie(buffer, memory) = createBuffer(size, usage, properties);
}

pair<VkBuffer, VkDeviceMemory> RendererVk::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) const {
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

uint32 RendererVk::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const {
	for (uint32 i = 0; i < pdevMemProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (pdevMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	throw std::runtime_error(std::format("Failed to find suitable memory type for properties {}", string_VkMemoryPropertyFlags(properties)));
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
	if (VkResult rs = vkEndCommandBuffer(cmdBuffer); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to end single time command buffer: {}", string_VkResult(rs)));

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuffer
	};
	if (VkResult rs = vkQueueSubmit(queue, 1, &submitInfo, fence); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to submit single time command buffer: {}", string_VkResult(rs)));
}

void RendererVk::synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const {
	vkWaitForFences(ldev, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(ldev, 1, &fence);
	vkResetCommandBuffer(cmdBuffer, 0);
}

template <VkImageLayout srcLay, VkImageLayout dstLay>
void RendererVk::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image) {
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
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if constexpr (srcLay == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && dstLay == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
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
void RendererVk::transitionBufferToImageLayout(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image) {
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

void RendererVk::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size, uint32 pitch) {
	VkBufferImageCopy region = {
		.bufferRowLength = pitch,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.imageExtent = { size.x, size.y, 1 }
	};
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RendererVk::copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size) {
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.imageExtent = { size.x, size.y, 1 }
	};
	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
}

#ifdef NDEBUG
vector<const char*> RendererVk::getRequiredExtensions(SDL_Window* win) {
#else
vector<const char*> RendererVk::getRequiredExtensions(SDL_Window* win, bool debugUtils, bool validationFeatures) {
#endif
	uint count;
	if (!SDL_Vulkan_GetInstanceExtensions(win, &count, nullptr))
		throw std::runtime_error(SDL_GetError());
	vector<const char*> extensions(count);
	SDL_Vulkan_GetInstanceExtensions(win, &count, extensions.data());
#ifndef NDEBUG
	if (debugUtils) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		if (validationFeatures)
			extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
	}
#endif
	return extensions;
}

RendererVk::QueueInfo RendererVk::findQueueFamilies(VkPhysicalDevice dev) const {
	uint32 count;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
	vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

	QueueInfo qi;
	for (uint32 i = 0; i < count; ++i) {
		if (!qi.pfam) {
			bool presentSupport = true;
			for (auto [id, view] : views)
				if (VkBool32 support = false; vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, static_cast<ViewVk*>(view)->surface, &support) != VK_SUCCESS || !support) {
					presentSupport = false;
					break;
				}
			if (presentSupport)
				qi.pfam = i;
		}
		if (VkQueueFlagBits(families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT) && (!qi.gfam || qi.gfam == qi.pfam))
			qi.gfam = i;
		if ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && (!qi.tfam || ((qi.tfam == qi.gfam || qi.tfam == qi.pfam) && !qi.canCompute && (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)))) {
			qi.tfam = i;
			qi.canCompute = families[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
		}
	}
	for (optional<uint32> fid : { qi.gfam, qi.pfam, qi.tfam })
		if (fid)
			if (auto [it, isnew] = qi.idcnt.try_emplace(*fid, 0, 1); !isnew && it->second.y < families[it->first].queueCount)
				++it->second.y;
	return qi;
}

vector<VkSurfaceFormatKHR> RendererVk::querySurfaceFormatSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get surface formats: {}", string_VkResult(rs)));
	vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &count, formats.data());
	return formats;
}

vector<VkPresentModeKHR> RendererVk::queryPresentModeSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error(std::format("Failed to get present modes: {}", string_VkResult(rs)));
	vector<VkPresentModeKHR> modes(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &count, modes.data());
	return modes;
}

VkSurfaceFormatKHR RendererVk::chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const VkSurfaceFormatKHR& form : availableFormats)
		if (form.format == VK_FORMAT_B8G8R8A8_UNORM && form.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return form;
	return availableFormats[0];
}

VkPresentModeKHR RendererVk::chooseSwapPresentMode(const vector<VkPresentModeKHR>& availablePresentModes) const {
	for (VkPresentModeKHR mode : availablePresentModes)
		if (mode == presentMode)
			return mode;
	return availablePresentModes[0];
}

void RendererVk::setCompression(Settings::Compression compression) {
	squashPicTexels = compression == Settings::Compression::b16;
}

pair<uint, Settings::Compression> RendererVk::getSettings(vector<pair<u32vec2, string>>& devices) const {
	uint32 count = 0;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);
	vector<VkPhysicalDevice> pdevs(count);
	vkEnumeratePhysicalDevices(instance, &count, pdevs.data());

	devices.resize(count + 1);
	devices[0] = pair(u32vec2(0), "auto");
	for (uint32 i = 0; i < count; ++i) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(pdevs[i], &prop);
		devices[i + 1] = pair(u32vec2(prop.vendorID, prop.deviceID), prop.deviceName);
	}
	return pair(pdevProperties.limits.maxImageDimension2D, Settings::Compression::b16);
}

uint RendererVk::scoreDevice(const VkPhysicalDeviceProperties& prop, const VkPhysicalDeviceMemoryProperties& memp) {
	uint score = 0;
	switch (prop.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 8;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 16;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score += 4;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		score += 2;
	}
	score += prop.limits.maxImageDimension2D / 2048;
	score += prop.limits.maxImageArrayLayers / 512;
	score += prop.limits.maxMemoryAllocationCount / 1024 / 1024;
	score += prop.limits.maxFramebufferWidth / 8192;
	score += prop.limits.maxFramebufferHeight / 8192;

	if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		for (VkMemoryPropertyFlags mask : deviceMemoryTypes)
			if (uint32 id = std::find_if(memp.memoryTypes, memp.memoryTypes + memp.memoryTypeCount, [mask](VkMemoryType it) -> bool { return (it.propertyFlags & mask) == mask; }) - memp.memoryTypes; id < memp.memoryTypeCount)
				score += memp.memoryHeaps[memp.memoryTypes[id].heapIndex].size / 1024 / 1024 / 1024;
	return score;
}

tuple<SDL_Surface*, VkFormat, bool> RendererVk::pickPixFormat(SDL_Surface* img) const {
	if (img)
		switch (img->format->format) {
		case SDL_PIXELFORMAT_RGBA32:
			return tuple(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
		case SDL_PIXELFORMAT_BGRA32:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			return tuple(img, VK_FORMAT_B8G8R8A8_UNORM, true);
#else
			return tuple(img, VK_FORMAT_R8G8B8A8_UNORM, true);
#endif
		case SDL_PIXELFORMAT_RGB24:
			if (fmtConv.initialized())
				return tuple(img, VK_FORMAT_R8G8B8_UNORM, false);
			break;
		case SDL_PIXELFORMAT_BGR24:
			if (fmtConv.initialized())
				return tuple(img, VK_FORMAT_B8G8R8_UNORM, false);
			break;
		case SDL_PIXELFORMAT_RGBA5551:
			return tuple(img, VK_FORMAT_R5G5B5A1_UNORM_PACK16, true);
		case SDL_PIXELFORMAT_BGRA5551:
			return tuple(img, VK_FORMAT_B5G5R5A1_UNORM_PACK16, true);
		case SDL_PIXELFORMAT_RGB565:
			return tuple(img, VK_FORMAT_R5G6B5_UNORM_PACK16, true);
		case SDL_PIXELFORMAT_BGR565:
			return tuple(img, VK_FORMAT_B5G6R5_UNORM_PACK16, true);
		default:
			img = convertReplace(img);
		}
	return tuple(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
}

uint RendererVk::maxTexSize() const {
	return pdevProperties.limits.maxImageDimension2D;
}

const umap<SDL_PixelFormatEnum, SDL_PixelFormatEnum>* RendererVk::getSquashableFormats() const {
	return !squashPicTexels ? nullptr : &squashableFormats;
}

#ifndef NDEBUG
pair<bool, bool> RendererVk::checkValidationLayerSupport() {
	uint32 count;
	if (VkResult rs = vkEnumerateInstanceLayerProperties(&count, nullptr); rs != VK_SUCCESS) {
		logError("Failed to enumerate layers: ", string_VkResult(rs));
		return pair(false, false);
	}
	vector<VkLayerProperties> availableLayers(count);
	vkEnumerateInstanceLayerProperties(&count, availableLayers.data());
	if (!rng::any_of(availableLayers, [](const VkLayerProperties& lp) -> bool { return !strcmp(lp.layerName, validationLayers[0]); })) {
		logError("Validation layers not available");
		return pair(false, false);
	}

	if (VkResult rs = vkEnumerateInstanceExtensionProperties(validationLayers[0], &count, nullptr); rs != VK_SUCCESS) {
		logError("Failed to enumerate layer extensions: ", string_VkResult(rs));
		return pair(false, false);
	}
	vector<VkExtensionProperties> availableExtensions(count);
	vkEnumerateInstanceExtensionProperties(validationLayers[0], &count, availableExtensions.data());

	bool debugUtils = false, validationFeatures = false;
	for (size_t i = 0; i < availableExtensions.size() && !(debugUtils && validationFeatures); ++i) {
		if (!strcmp(availableExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
			debugUtils = true;
		else if (!strcmp(availableExtensions[i].extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME))
			validationFeatures = true;
	}
	if (!debugUtils)
		logError("Instance extension " VK_EXT_DEBUG_UTILS_EXTENSION_NAME " not available");
	if (!validationFeatures)
		logError("Instance extension " VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME " not available");
	return pair(debugUtils, validationFeatures);
}

VKAPI_ATTR VkBool32 VKAPI_CALL RendererVk::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
	string msg = "Validation:";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		msg += " verbose";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		msg += " info";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		msg += " warning";
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		msg += " error";
	msg += " -";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		msg += " general";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		msg += " validation";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
		msg += " performance";
	if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
		msg += " device_address_binding";
	logError(std::move(msg), " - ", pCallbackData->pMessage);
	return VK_FALSE;
}
#endif
#endif
