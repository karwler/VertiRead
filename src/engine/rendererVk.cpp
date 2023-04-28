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

// GENERIC PIPELINE

VkSampler GenericPipeline::createSampler(const RendererVk* rend, VkFilter filter) {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

	VkSampler sampler;
	if (VkResult rs = vkCreateSampler(rend->getLogicalDevice(), &samplerInfo, nullptr, &sampler); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture sampler: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(sampler);
#endif
	return sampler;
}

VkShaderModule GenericPipeline::createShaderModule(const RendererVk* rend, const uint32* code, size_t clen) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = clen;
	createInfo.pCode = code;

	VkShaderModule shaderModule;
	if (VkResult rs = vkCreateShaderModule(rend->getLogicalDevice(), &createInfo, nullptr, &shaderModule); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(shaderModule);
#endif
	return shaderModule;
}

// FORMAT CONVERTER

void FormatConverter::init(const RendererVk* rend) {
	createDescriptorSetLayout(rend);
	createPipelines(rend);
	createDescriptorPoolAndSet(rend);
}

void FormatConverter::createDescriptorSetLayout(const RendererVk* rend) {
	array<VkDescriptorSetLayoutBinding, 2> bindings{};
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create converter descriptor set layout: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorSetLayout);
#endif
}

void FormatConverter::createPipelines(const RendererVk* rend) {
	constexpr uint32 compCode[] = {
#ifdef NDEBUG
#include "shaders/vk.conv.comp.rel.h"
#else
#include "shaders/vk.conv.comp.dbg.h"
#endif
	};
	VkShaderModule compShaderModule = createShaderModule(rend, compCode, sizeof(compCode));

	VkPushConstantRange pushConstant{};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.size = sizeof(PushData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create converter pipeline layout: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(pipelineLayout);
#endif

	for (uint i = 0; i < pipelines.size(); ++i) {
		VkSpecializationMapEntry specializationEntry{};
		specializationEntry.constantID = 0;
		specializationEntry.offset = offsetof(SpecializationData, orderRgb);
		specializationEntry.size = sizeof(SpecializationData::orderRgb);

		SpecializationData specializationData = { i };
		VkSpecializationInfo specializationInfo{};
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationEntry;
		specializationInfo.dataSize = sizeof(specializationData);
		specializationInfo.pData = &specializationData;

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineInfo.stage.module = compShaderModule;
		pipelineInfo.stage.pName = "main";
		pipelineInfo.stage.pSpecializationInfo = &specializationInfo;
		pipelineInfo.layout = pipelineLayout;
		if (VkResult rs = vkCreateComputePipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelines[i]); rs != VK_SUCCESS)
			throw std::runtime_error("Failed to create converter pipeline " + toStr(i) + ": " + string_VkResult(rs));
#ifndef NDEBUG
		rend->setObjectDebugName(pipelines[i]);
#endif
	}
	vkDestroyShaderModule(rend->getLogicalDevice(), compShaderModule, nullptr);
}

void FormatConverter::createDescriptorPoolAndSet(const RendererVk* rend) {
	array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[0].descriptorCount = maxTransfers;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[1].descriptorCount = maxTransfers;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = maxTransfers;
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create converter descriptor pool: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorPool);
#endif

	array<VkDescriptorSetLayout, maxTransfers> layouts;
	layouts.fill(descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate converter descriptor sets: "s + string_VkResult(rs));
#ifndef NDEBUG
	for (VkDescriptorSet it : descriptorSets)
		rend->setObjectDebugName(it);
#endif
}

void FormatConverter::updateBufferSize(const RendererVk* rend, uint id, VkDeviceSize texSize, VkBuffer inputBuffer, VkDeviceSize inputSize, bool& update) {
	VkDeviceSize outputSize = roundToMulOf(texSize * 4, convWgrpSize * 4 * sizeof(uint32));
	if (outputSize > outputSizesMax[id]) {
		rend->recreateBuffer(outputBuffers[id], outputMemory[id], outputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		outputSizesMax[id] = outputSize;
		update = true;
	}
	if (update) {
		VkDescriptorBufferInfo inputBufferInfo{};
		inputBufferInfo.buffer = inputBuffer;
		inputBufferInfo.range = inputSize;

		VkDescriptorBufferInfo outputBufferInfo{};
		outputBufferInfo.buffer = outputBuffers[id];
		outputBufferInfo.range = outputSize;

		array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[id];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &inputBufferInfo;
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[id];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &outputBufferInfo;
		vkUpdateDescriptorSets(rend->getLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
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
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if (VkResult rs = vkCreateRenderPass(rend->getLogicalDevice(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(handle);
#endif
}

void RenderPass::createDescriptorSetLayout(const RendererVk* rend) {
	array<VkDescriptorSetLayoutBinding, 2> bindings0{};
	bindings0[0].binding = 0;
	bindings0[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings0[0].descriptorCount = 1;
	bindings0[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings0[1].binding = 1;
	bindings0[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	bindings0[1].descriptorCount = samplers.size();
	bindings0[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings0[1].pImmutableSamplers = samplers.data();

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings0.size();
	layoutInfo.pBindings = bindings0.data();
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[0]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout 0: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorSetLayouts[0]);
#endif

	VkDescriptorSetLayoutBinding binding1{};
	binding1.binding = 0;
	binding1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	binding1.descriptorCount = 1;
	binding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding1;
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayouts[1]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout 1: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorSetLayouts[1]);
#endif
}

void RenderPass::createPipeline(const RendererVk* rend) {
	constexpr uint32 vertCode[] = {
#ifdef NDEBUG
#include "shaders/vk.gui.vert.rel.h"
#else
#include "shaders/vk.gui.vert.dbg.h"
#endif
	};
	constexpr uint32 fragCode[] = {
#ifdef NDEBUG
#include "shaders/vk.gui.frag.rel.h"
#else
#include "shaders/vk.gui.frag.dbg.h"
#endif
	};
	VkShaderModule vertShaderModule = createShaderModule(rend, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(rend, fragCode, sizeof(fragCode));

	array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragShaderModule;
	shaderStages[1].pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPushConstantRange pushConstant{};
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstant.size = sizeof(PushData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(pipelineLayout);
#endif

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = handle;
	pipelineInfo.subpass = 0;
	if (VkResult rs = vkCreateGraphicsPipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(pipeline);
#endif

	vkDestroyShaderModule(rend->getLogicalDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(rend->getLogicalDevice(), vertShaderModule, nullptr);
}

vector<VkDescriptorSet> RenderPass::createDescriptorPoolAndSets(const RendererVk* rend, uint32 numViews) {
	array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = numViews;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	poolSizes[1].descriptorCount = samplers.size() * numViews;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = numViews;
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create view descriptor pool: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorPool);
#endif

	vector<VkDescriptorSetLayout> layouts(poolInfo.maxSets, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	vector<VkDescriptorSet> descriptorSets(layouts.size());
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate view descriptor sets: "s + string_VkResult(rs));
#ifndef NDEBUG
	for (VkDescriptorSet it : descriptorSets)
		rend->setObjectDebugName(it);
#endif
	return descriptorSets;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::newDescriptorSetTex(const RendererVk* rend, VkImageView imageView) {
	pair<VkDescriptorPool, VkDescriptorSet> dpds = getDescriptorSetTex(rend);
	updateDescriptorSet(rend->getLogicalDevice(), dpds.second, imageView);
	return dpds;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::getDescriptorSetTex(const RendererVk* rend) {
	if (umap<VkDescriptorPool, DescriptorSetBlock>::iterator psit = std::find_if(poolSetTex.begin(), poolSetTex.end(), [](const pair<const VkDescriptorPool, DescriptorSetBlock>& it) -> bool { return !it.second.free.empty(); }); psit != poolSetTex.end()) {
		VkDescriptorSet descriptorSet = *psit->second.free.begin();
		psit->second.free.erase(psit->second.free.begin());
		return pair(psit->first, *psit->second.used.insert(descriptorSet).first);
	}

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	poolSize.descriptorCount = textureSetStep;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = textureSetStep;

	VkDescriptorPool descPool;
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture descriptor pool: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descPool, "texture");
#endif

	array<VkDescriptorSetLayout, textureSetStep> layouts;
	layouts.fill(descriptorSetLayouts[1]);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	array<VkDescriptorSet, textureSetStep> descriptorSets;
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, descriptorSets.data()); rs != VK_SUCCESS) {
		vkDestroyDescriptorPool(rend->getLogicalDevice(), descPool, nullptr);
		throw std::runtime_error("Failed to allocate texture descriptor sets: "s + string_VkResult(rs));
	}
#ifndef NDEBUG
	for (VkDescriptorSet it : descriptorSets)
		rend->setObjectDebugName(it, "texture");
#endif
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
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.range = sizeof(UniformData);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(dev, 1, &descriptorWrite, 0, nullptr);
}

void RenderPass::updateDescriptorSet(VkDevice dev, VkDescriptorSet descriptorSet, VkImageView imageView) {
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = imageView;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;
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
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	if (VkResult rs = vkCreateRenderPass(rend->getLogicalDevice(), &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(handle);
#endif
}

void AddressPass::createDescriptorSetLayout(const RendererVk* rend) {
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = bindingUdat;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;
	if (VkResult rs = vkCreateDescriptorSetLayout(rend->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorSetLayout);
#endif
}

void AddressPass::createPipeline(const RendererVk* rend) {
	constexpr uint32 vertCode[] = {
#ifdef NDEBUG
#include "shaders/vk.sel.vert.rel.h"
#else
#include "shaders/vk.sel.vert.dbg.h"
#endif
	};
	constexpr uint32 fragCode[] = {
#ifdef NDEBUG
#include "shaders/vk.sel.frag.rel.h"
#else
#include "shaders/vk.sel.frag.dbg.h"
#endif
	};
	VkShaderModule vertShaderModule = createShaderModule(rend, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(rend, fragCode, sizeof(fragCode));

	array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragShaderModule;
	shaderStages[1].pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	VkRect2D scissor{};
	scissor.extent = { 1, 1 };

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	array<VkDynamicState, 1> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPushConstantRange pushConstant{};
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstant.size = sizeof(PushData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	if (VkResult rs = vkCreatePipelineLayout(rend->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(pipelineLayout);
#endif

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = handle;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	if (VkResult rs = vkCreateGraphicsPipelines(rend->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(pipeline);
#endif

	vkDestroyShaderModule(rend->getLogicalDevice(), fragShaderModule, nullptr);
	vkDestroyShaderModule(rend->getLogicalDevice(), vertShaderModule, nullptr);
}

void AddressPass::createUniformBuffer(const RendererVk* rend) {
	std::tie(uniformBuffer, uniformBufferMemory) = rend->createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(rend->getLogicalDevice(), uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&uniformBufferMapped)); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to map uniform buffer memory: "s + string_VkResult(rs));
}

void AddressPass::createDescriptorPoolAndSet(const RendererVk* rend) {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;
	if (VkResult rs = vkCreateDescriptorPool(rend->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorPool);
#endif

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	if (VkResult rs = vkAllocateDescriptorSets(rend->getLogicalDevice(), &allocInfo, &descriptorSet); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets: "s + string_VkResult(rs));
#ifndef NDEBUG
	rend->setObjectDebugName(descriptorSet);
#endif
}

void AddressPass::updateDescriptorSet(VkDevice dev) {
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.range = sizeof(UniformData);

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = bindingUdat;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pBufferInfo = &bufferInfo;
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

#ifndef NDEBUG
	nextObjectDebugName.emplace_back("transfer");
#endif
	tcmdPool = createCommandPool(*queueInfo.tfam);
	allocateCommandBuffers(tcmdPool, tcmdBuffers.data(), tcmdBuffers.size());
	std::generate(tfences.begin(), tfences.end(), [this]() -> VkFence { return createFence(VK_FENCE_CREATE_SIGNALED_BIT); });
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

#ifndef NDEBUG
	nextObjectDebugName.back() = "graphics";
#endif
	gcmdPool = createCommandPool(*queueInfo.gfam);
	umap<VkFormat, uint> formatCounter;
	for (auto [id, view] : views)
		++formatCounter[createSwapchain(static_cast<ViewVk*>(view))];
	vector<VkDescriptorSet> descriptorSets = renderPass.init(this, std::max_element(formatCounter.begin(), formatCounter.end(), [](const pair<const VkFormat, uint>& a, const pair<const VkFormat, uint>& b) -> bool { return a.second < b.second; })->first, views.size());
	size_t d = 0;
	for (auto [id, view] : views)
		initView(static_cast<ViewVk*>(view), descriptorSets[d++]);

#ifndef NDEBUG
	nextObjectDebugName.back() = "addr";
#endif
	addressPass.init(this);
	std::tie(addrImage, addrImageMemory) = createImage(u32vec2(1), VK_IMAGE_TYPE_1D, AddressPass::format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	addrView = createImageView(addrImage, VK_IMAGE_VIEW_TYPE_1D, AddressPass::format);
	addrFramebuffer = createFramebuffer(addressPass.getHandle(), addrView, u32vec2(1));
	std::tie(addrBuffer, addrBufferMemory) = createBuffer(sizeof(u32vec2), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(ldev, addrBufferMemory, 0, sizeof(u32vec2), 0, reinterpret_cast<void**>(&addrMappedMemory)); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to map address lookup memory: "s + string_VkResult(rs));
	allocateCommandBuffers(gcmdPool, &commandBufferAddr, 1);
	addrFence = createFence();
#ifndef NDEBUG
	nextObjectDebugName.pop_back();
#endif
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
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "VertiRead";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "VertiRead_RendererVk";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

#ifdef NDEBUG
	vector<const char*> extensions = getRequiredExtensions(window);
#else
	auto [debugUtils, validationFeatures] = checkValidationLayerSupport();
	vector<const char*> extensions = getRequiredExtensions(window, debugUtils, validationFeatures);

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
	debugCreateInfo.pfnUserCallback = debugCallback;

	VkValidationFeaturesEXT validationFeaturesInfo{};
	validationFeaturesInfo.pNext = &debugCreateInfo;
	validationFeaturesInfo.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validationFeaturesInfo.enabledValidationFeatureCount = validationFeatureEnables.size();
	validationFeaturesInfo.pEnabledValidationFeatures = validationFeatureEnables.data();
#endif

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
#ifndef NDEBUG
	if (debugUtils) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = validationFeatures ? reinterpret_cast<void*>(&validationFeaturesInfo) : reinterpret_cast<void*>(&debugCreateInfo);
	}
#endif
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance");

#ifndef NDEBUG
	if (debugUtils) {
		if (PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); !pfnCreateDebugUtilsMessengerEXT || pfnCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &dbgMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger");
		if (pfnSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT")); !pfnSetDebugUtilsObjectNameEXT)
			logError("Failed to get address of vkSetDebugUtilsObjectNameEXT");
	}
#endif
}

RendererVk::QueueInfo RendererVk::pickPhysicalDevice(u32vec2& preferred) {
	uint32 deviceCount;
	if (VkResult rs = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to find devices: "s + string_VkResult(rs));
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
			if (std::list<VkMemoryPropertyFlags>::iterator it = std::find_if(requiredMemoryTypes.begin(), requiredMemoryTypes.end(), [&memp, i](VkMemoryPropertyFlags flg) -> bool { return (memp.memoryTypes[i].propertyFlags & flg) == flg; }); it != requiredMemoryTypes.end())
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

	uint32 i = 0;
	for (auto [qfam, qcnt] : queueInfo.idcnt) {
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = qfam;
		queueCreateInfos[i].queueCount = qcnt.y;
		queueCreateInfos[i++].pQueuePriorities = queuePriorities.data();
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueInfo.idcnt.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
#ifndef NDEBUG
	if (dbgMessenger != VK_NULL_HANDLE) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
#endif
	if (VkResult rs = vkCreateDevice(pdev, &createInfo, nullptr, &ldev); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical device: "s + string_VkResult(rs));
#ifndef NDEBUG
	string deviceName = toStr<0x10>(pdevProperties.vendorID, 4) + ':' + toStr<0x10>(pdevProperties.deviceID, 4);
	setObjectDebugName(pdev, deviceName.c_str());
	setObjectDebugName(ldev, deviceName.c_str());
#endif

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
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = family;

	VkCommandPool commandPool;
	if (VkResult rs = vkCreateCommandPool(ldev, &poolInfo, nullptr, &commandPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(commandPool);
#endif
	return commandPool;
}

VkFormat RendererVk::createSwapchain(ViewVk* view, VkSwapchainKHR oldSwapchain) {
	VkSurfaceCapabilitiesKHR capabilities;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, view->surface, &capabilities); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to get surface capabilities: "s + string_VkResult(rs));

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(querySurfaceFormatSupport(pdev, view->surface));
	view->extent = capabilities.currentExtent.width != UINT32_MAX ? capabilities.currentExtent : VkExtent2D{
		std::clamp(uint32(view->rect.w), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp(uint32(view->rect.h), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
	array<uint32, 2> queueFamilyIndices = { gfamilyIndex, pfamilyIndex };
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = view->surface;
	createInfo.minImageCount = capabilities.maxImageCount == 0 || capabilities.minImageCount < capabilities.maxImageCount ? capabilities.minImageCount + 1 : capabilities.maxImageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = view->extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = chooseSwapPresentMode(queryPresentModeSupport(pdev, view->surface));
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;
	if (gfamilyIndex != pfamilyIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	} else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (VkResult rs = vkCreateSwapchainKHR(ldev, &createInfo, nullptr, &view->swapchain); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain: "s + string_VkResult(rs));

	if (VkResult rs = vkGetSwapchainImagesKHR(ldev, view->swapchain, &view->imageCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to get swapchain images: "s + string_VkResult(rs));
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
		throw std::runtime_error("Failed to map uniform buffer memory 1: "s + string_VkResult(rs));

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
	if (!vsync && modes.count(VK_PRESENT_MODE_IMMEDIATE_KHR))
		presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	else if (modes.count(VK_PRESENT_MODE_MAILBOX_KHR))
		presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	else if (modes.count(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
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
		throw std::runtime_error("Failed to acquire swapchain image: "s + string_VkResult(rs));
	vkResetFences(ldev, 1, &currentView->frameFences[currentFrame]);
	vkResetCommandBuffer(currentView->commandBuffers[currentFrame], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// true but retarded
	if (VkResult rs = vkBeginCommandBuffer(currentView->commandBuffers[currentFrame], &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer: "s + string_VkResult(rs));

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass.getHandle();
	renderPassInfo.framebuffer = currentView->framebuffers[imageIndex].second;
	renderPassInfo.renderArea.extent = currentView->extent;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &bgColor;
	vkCmdBeginRenderPass(currentView->commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipeline());

	VkViewport viewport{};
	viewport.width = float(currentView->extent.width);
	viewport.height = float(currentView->extent.height);
	vkCmdSetViewport(currentView->commandBuffers[currentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent = currentView->extent;
	vkCmdSetScissor(currentView->commandBuffers[currentFrame], 0, 1, &scissor);
	vkCmdBindDescriptorSets(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipelineLayout(), 0, 1, &currentView->descriptorSet, 0, nullptr);
}

void RendererVk::drawRect(const Texture* tex, const Recti& rect, const Recti& frame, const vec4& color) {
	const TextureVk* vtx = static_cast<const TextureVk*>(tex);
	RenderPass::PushData pd;
	pd.rect = rect.toVec();
	pd.frame = frame.toVec();
	pd.color = color;
	pd.sid = vtx->sid;
	vkCmdBindDescriptorSets(currentView->commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass.getPipelineLayout(), 1, 1, &vtx->set, 0, nullptr);
	vkCmdPushConstants(currentView->commandBuffers[currentFrame], renderPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(RenderPass::PushData), &pd);
	vkCmdDraw(currentView->commandBuffers[currentFrame], 4, 1, 0, 0);
}

void RendererVk::finishDraw(View*) {
	vkCmdEndRenderPass(currentView->commandBuffers[currentFrame]);
	if (VkResult rs = vkEndCommandBuffer(currentView->commandBuffers[currentFrame]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to end recording command buffer: "s + string_VkResult(rs));

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &currentView->imageAvailableSemaphores[currentFrame];
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentView->commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &currentView->renderFinishedSemaphores[currentFrame];
	if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, currentView->frameFences[currentFrame]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer: "s + string_VkResult(rs));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &currentView->renderFinishedSemaphores[currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &currentView->swapchain;
	presentInfo.pImageIndices = &imageIndex;
	if (VkResult rs = vkQueuePresentKHR(pqueue, &presentInfo); rs == VK_ERROR_OUT_OF_DATE_KHR || rs == VK_SUBOPTIMAL_KHR || refreshFramebuffer)
		recreateSwapchain(currentView);
	else if (rs != VK_SUCCESS)
		throw std::runtime_error("Failed to present swapchain image: "s + string_VkResult(rs));
}

void RendererVk::finishRender() {
	currentFrame = (currentFrame + 1) % ViewVk::maxFrames;
}

void RendererVk::startSelDraw(View* view, ivec2 pos) {
	ViewVk* vkw = static_cast<ViewVk*>(view);
	addressPass.getUniformBufferMapped()->pview = vec4(vkw->rect.pos(), vec2(vkw->rect.size()) / 2.f);
	beginSingleTimeCommands(commandBufferAddr);

	VkClearValue zero{};
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = addressPass.getHandle();
	renderPassInfo.framebuffer = addrFramebuffer;
	renderPassInfo.renderArea.extent = { 1, 1 };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &zero;
	vkCmdBeginRenderPass(commandBufferAddr, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBufferAddr, VK_PIPELINE_BIND_POINT_GRAPHICS, addressPass.getPipeline());

	VkViewport viewport{};
	viewport.x = float(-pos.x);
	viewport.y = float(-pos.y);
	viewport.width = float(vkw->extent.width);
	viewport.height = float(vkw->extent.height);
	vkCmdSetViewport(commandBufferAddr, 0, 1, &viewport);

	array<VkDescriptorSet, 1> descriptorSets = { addressPass.getDescriptorSet() };
	vkCmdBindDescriptorSets(commandBufferAddr, VK_PIPELINE_BIND_POINT_GRAPHICS, addressPass.getPipelineLayout(), 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
}

void RendererVk::drawSelRect(const Widget* wgt, const Recti& rect, const Recti& frame) {
	AddressPass::PushData pd;
	pd.rect = rect.toVec();
	pd.frame = frame.toVec();
	pd.addr = uvec2(uintptr_t(wgt), uintptr_t(wgt) >> 32);
	vkCmdPushConstants(commandBufferAddr, addressPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AddressPass::PushData), &pd);
	vkCmdDraw(commandBufferAddr, 4, 1, 0, 0);
}

Widget* RendererVk::finishSelDraw(View*) {
	vkCmdEndRenderPass(commandBufferAddr);
	transitionImageLayout<VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL>(commandBufferAddr, addrImage);
	copyImageToBuffer(commandBufferAddr, addrImage, addrBuffer, u32vec2(1));
	endSingleTimeCommands(commandBufferAddr, addrFence, gqueue);
	synchSingleTimeCommands(commandBufferAddr, addrFence);
	return reinterpret_cast<Widget*>(uintptr_t(addrMappedMemory->x) | (uintptr_t(addrMappedMemory->y) << 32));
}

Texture* RendererVk::texFromIcon(SDL_Surface* img) {
	img = limitSize(img, pdevProperties.limits.maxImageDimension2D);
	if (auto [pic, fmt] = pickPixFormat(img); pic) {
#ifndef NDEBUG
		nextObjectDebugName.emplace_back("icon");
#endif
		TextureVk* tex = fmt >= VK_FORMAT_R8G8B8A8_UNORM ? createTextureDirect(pic->pixels, u32vec2(pic->w, pic->h), pic->pitch, pic->format->BytesPerPixel, fmt, false) : createTextureIndirect(pic, fmt);
		SDL_FreeSurface(pic);
#ifndef NDEBUG
		nextObjectDebugName.pop_back();
#endif
		return tex;
	}
	return nullptr;
}

Texture* RendererVk::texFromRpic(SDL_Surface* img) {
	if (auto [pic, fmt] = pickPixFormat(img); pic) {
#ifndef NDEBUG
		nextObjectDebugName.emplace_back("rpic");
#endif
		TextureVk* tex = fmt >= VK_FORMAT_R8G8B8A8_UNORM ? createTextureDirect(pic->pixels, u32vec2(pic->w, pic->h), pic->pitch, pic->format->BytesPerPixel, fmt, false) : createTextureIndirect(pic, fmt);
		SDL_FreeSurface(pic);
#ifndef NDEBUG
		nextObjectDebugName.pop_back();
#endif
		return tex;
	}
	return nullptr;
}

Texture* RendererVk::texFromText(const Pixmap& pm) {
	if (pm.pix) {
#ifndef NDEBUG
		nextObjectDebugName.emplace_back("text");
#endif
		TextureVk* tex = createTextureDirect(pm.pix.get(), glm::min(pm.res, u32vec2(pdevProperties.limits.maxImageDimension2D)), pm.res.x * 4, 4, VK_FORMAT_A8B8G8R8_UNORM_PACK32, true);
#ifndef NDEBUG
		nextObjectDebugName.pop_back();
#endif
		return tex;
	}
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

RendererVk::TextureVk* RendererVk::createTextureDirect(const void* pix, u32vec2 res, uint32 pitch, uint8 bpp, VkFormat format, bool nearest) {
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

RendererVk::TextureVk* RendererVk::createTextureIndirect(SDL_Surface* img, VkFormat format) {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet dset = VK_NULL_HANDLE;
	u32vec2 res(img->w, img->h);
	try {
		std::tie(image, memory) = createImage(res, VK_IMAGE_TYPE_2D, VK_FORMAT_A8B8G8R8_UNORM_PACK32, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		synchSingleTimeCommands(tcmdBuffers[currentTransfer], tfences[currentTransfer]);
		uploadInputData<true>(img, res, img->pitch, img->format->BytesPerPixel);

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
		SDL_FreeSurface(img);
		currentTransfer = (currentTransfer + 1) % FormatConverter::maxTransfers;
	} catch (const std::runtime_error& err) {
		logError(err.what());
		renderPass.freeDescriptorSetTex(ldev, pool, dset);
		vkDestroyImageView(ldev, view, nullptr);
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		SDL_FreeSurface(img);
		return nullptr;
	}
	return new TextureVk(res, image, memory, view, pool, dset, false);
}

template <bool conv>
void RendererVk::uploadInputData(const void* pix, u32vec2 res, uint32 pitch, uint8 bpp) {
	VkDeviceSize texSize, pixSize;
	if constexpr (conv) {
		texSize = VkDeviceSize(res.x) * VkDeviceSize(res.y);
		pixSize = texSize * VkDeviceSize(bpp);
	} else
		pixSize = VkDeviceSize(pitch) * VkDeviceSize(res.y);

	VkDeviceSize inputSize = roundToMulOf(pixSize, transferAtomSize);
	if (inputSize > inputSizesMax[currentTransfer]) {
#ifndef NDEBUG
		nextObjectDebugName.emplace_back("transfer");
#endif
		recreateBuffer(inputBuffers[currentTransfer], inputMemory[currentTransfer], inputSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
#ifndef NDEBUG
		nextObjectDebugName.pop_back();
#endif
		if (VkResult rs = vkMapMemory(ldev, inputMemory[currentTransfer], 0, VK_WHOLE_SIZE, 0, &inputsMapped[currentTransfer]); rs != VK_SUCCESS)
			throw std::runtime_error("Failed to map input buffer memory: "s + string_VkResult(rs));
		inputSizesMax[currentTransfer] = inputSize;
		rebindInputBuffer[currentTransfer] = true;
	}
	if constexpr (conv) {
		fmtConv.updateBufferSize(this, currentTransfer, texSize, inputBuffers[currentTransfer], inputSizesMax[currentTransfer], rebindInputBuffer[currentTransfer]);

		if (size_t packPitch = size_t(res.x) * size_t(bpp); pitch == packPitch)
			memcpy(inputsMapped[currentTransfer], pix, pixSize);
		else
			for (size_t o = 0, i = 0, e = size_t(pitch) * size_t(res.y); i < e; i += pitch, o += packPitch)
				memcpy(reinterpret_cast<uint8*>(inputsMapped[currentTransfer]) + o, static_cast<const uint8*>(pix) + i, packPitch);
	} else
		memcpy(inputsMapped[currentTransfer], pix, pixSize);

	VkMappedMemoryRange flushRange{};
	flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	flushRange.memory = inputMemory[currentTransfer];
	flushRange.size = inputSize;
	if (VkResult rs = vkFlushMappedMemoryRanges(ldev, 1, &flushRange); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to flush input memory: "s + string_VkResult(rs));
}

pair<VkImage, VkDeviceMemory> RendererVk::createImage(u32vec2 size, VkImageType type, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) const {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = type;
	imageInfo.extent = { size.x, size.y, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImage image;
	if (VkResult rs = vkCreateImage(ldev, &imageInfo, nullptr, &image); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create image: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(image);
#endif

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ldev, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	VkDeviceMemory memory;
	if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &memory); rs != VK_SUCCESS) {
		vkDestroyImage(ldev, image, nullptr);
		throw std::runtime_error("Failed to allocate image memory: "s + string_VkResult(rs));
	}
#ifndef NDEBUG
	setObjectDebugName(memory);
#endif
	if (VkResult rs = vkBindImageMemory(ldev, image, memory, 0); rs != VK_SUCCESS) {
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		throw std::runtime_error("Failed to bind image memory: "s + string_VkResult(rs));
	}
	return pair(image, memory);
}

VkImageView RendererVk::createImageView(VkImage image, VkImageViewType type, VkFormat format) const {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = type;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (VkResult rs = vkCreateImageView(ldev, &viewInfo, nullptr, &imageView); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create image view: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(imageView);
#endif
	return imageView;
}

VkFramebuffer RendererVk::createFramebuffer(VkRenderPass rpass, VkImageView view, u32vec2 size) const {
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = rpass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &view;
	framebufferInfo.width = size.x;
	framebufferInfo.height = size.y;
	framebufferInfo.layers = 1;

	VkFramebuffer framebuffer;
	if (VkResult rs = vkCreateFramebuffer(ldev, &framebufferInfo, nullptr, &framebuffer); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create framebuffer: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(framebuffer);
#endif
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
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	if (VkResult rs = vkCreateBuffer(ldev, &bufferInfo, nullptr, &buffer); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create buffer: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(buffer);
#endif

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(ldev, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	VkDeviceMemory memory;
	if (VkResult rs = vkAllocateMemory(ldev, &allocInfo, nullptr, &memory); rs != VK_SUCCESS) {
		vkDestroyBuffer(ldev, buffer, nullptr);
		throw std::runtime_error("Failed to allocate buffer memory: "s + string_VkResult(rs));
	}
#ifndef NDEBUG
	setObjectDebugName(memory);
#endif
	if (VkResult rs = vkBindBufferMemory(ldev, buffer, memory, 0); rs != VK_SUCCESS) {
		vkDestroyBuffer(ldev, buffer, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		throw std::runtime_error("Failed to bind buffer memory: "s + string_VkResult(rs));
	}
	return pair(buffer, memory);
}

uint32 RendererVk::findMemoryType(uint32 typeFilter, VkMemoryPropertyFlags properties) const {
	for (uint32 i = 0; i < pdevMemProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (pdevMemProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	throw std::runtime_error("Failed to find suitable memory type for properties "s + string_VkMemoryPropertyFlags(properties));
}

void RendererVk::allocateCommandBuffers(VkCommandPool commandPool, VkCommandBuffer* cmdBuffers, uint32 count) const {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	if (VkResult rs = vkAllocateCommandBuffers(ldev, &allocInfo, cmdBuffers); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers: "s + string_VkResult(rs));
#ifndef NDEBUG
	for (uint32 i = 0; i < count; ++i)
		setObjectDebugName(cmdBuffers[i]);
#endif
}

VkSemaphore RendererVk::createSemaphore() const {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (VkResult rs = vkCreateSemaphore(ldev, &semaphoreInfo, nullptr, &semaphore); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create semaphore: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(semaphore);
#endif
	return semaphore;
}

VkFence RendererVk::createFence(VkFenceCreateFlags flags) const {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = flags;

	VkFence fence;
	if (VkResult rs = vkCreateFence(ldev, &fenceInfo, nullptr, &fence); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create fence: "s + string_VkResult(rs));
#ifndef NDEBUG
	setObjectDebugName(fence);
#endif
	return fence;
}

void RendererVk::beginSingleTimeCommands(VkCommandBuffer cmdBuffer) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (VkResult rs = vkBeginCommandBuffer(cmdBuffer, &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to begin single time command buffer: "s + string_VkResult(rs));
}

void RendererVk::endSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence, VkQueue queue) const {
	if (VkResult rs = vkEndCommandBuffer(cmdBuffer); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to end single time command buffer: "s + string_VkResult(rs));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
	if (VkResult rs = vkQueueSubmit(queue, 1, &submitInfo, fence); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to submit single time command buffer: "s + string_VkResult(rs));
}

void RendererVk::synchSingleTimeCommands(VkCommandBuffer cmdBuffer, VkFence fence) const {
	vkWaitForFences(ldev, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(ldev, 1, &fence);
	vkResetCommandBuffer(cmdBuffer, 0);
}

template <VkImageLayout srcLay, VkImageLayout dstLay>
void RendererVk::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = srcLay;
	barrier.newLayout = dstLay;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;

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
		throw std::invalid_argument("Unsupported layout transition from "s + string_VkImageLayout(srcLay) + " to " + string_VkImageLayout(dstLay) + " in " + __func__);
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

template <VkImageLayout srcLay, VkImageLayout dstLay>
void RendererVk::transitionBufferToImageLayout(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image) {
	VkBufferMemoryBarrier bufferBarrier{};
	bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.buffer = buffer;
	bufferBarrier.size = VK_WHOLE_SIZE;

	VkImageMemoryBarrier imageBarrier{};
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.oldLayout = srcLay;
	imageBarrier.newLayout = dstLay;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = image;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if constexpr (srcLay == VK_IMAGE_LAYOUT_UNDEFINED && dstLay == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;;
		bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_NONE;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else
		throw std::invalid_argument("Unsupported layout transition from "s + string_VkImageLayout(srcLay) + " to " + string_VkImageLayout(dstLay) + " in " + __func__);
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 1, &bufferBarrier, 1, &imageBarrier);
}

void RendererVk::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, u32vec2 size, uint32 pitch) {
	VkBufferImageCopy region{};
	region.bufferRowLength = pitch;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { size.x, size.y, 1 };
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void RendererVk::copyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, u32vec2 size) {
	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { size.x, size.y, 1 };
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
			if (auto [it, isnew] = qi.idcnt.emplace(*fid, u32vec2(0, 1)); !isnew && it->second.y < families[it->first].queueCount)
				++it->second.y;
	return qi;
}

vector<VkSurfaceFormatKHR> RendererVk::querySurfaceFormatSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to get surface formats: "s + string_VkResult(rs));
	vector<VkSurfaceFormatKHR> formats(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &count, formats.data());
	return formats;
}

vector<VkPresentModeKHR> RendererVk::queryPresentModeSupport(VkPhysicalDevice dev, VkSurfaceKHR surf) {
	uint32 count;
	if (VkResult rs = vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &count, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to get present modes: "s + string_VkResult(rs));
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

pair<SDL_Surface*, VkFormat> RendererVk::pickPixFormat(SDL_Surface* img) const {
	if (img)
		switch (img->format->format) {
		case SDL_PIXELFORMAT_RGBA32:
			return pair(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32);
		case SDL_PIXELFORMAT_BGRA32:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			return pair(img, VK_FORMAT_R8G8B8A8_UNORM);	// TODO: check if these formats are correct
#else
			return pair(img, VK_FORMAT_B8G8R8A8_UNORM);
#endif
		case SDL_PIXELFORMAT_RGB24:
			if (fmtConv.initialized())
				return pair(img, VK_FORMAT_R8G8B8_UNORM);
			break;
		case SDL_PIXELFORMAT_BGR24:
			if (fmtConv.initialized())
				return pair(img, VK_FORMAT_B8G8R8_UNORM);
			break;
		case SDL_PIXELFORMAT_RGBA5551:
			return pair(img, VK_FORMAT_R5G5B5A1_UNORM_PACK16);
		case SDL_PIXELFORMAT_BGRA5551:
			return pair(img, VK_FORMAT_B5G5R5A1_UNORM_PACK16);
		case SDL_PIXELFORMAT_RGB565:
			return pair(img, VK_FORMAT_R5G6B5_UNORM_PACK16);
		case SDL_PIXELFORMAT_BGR565:
			return pair(img, VK_FORMAT_B5G6R5_UNORM_PACK16);
		default:
			img = convertReplace(img);
		}
	return pair(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32);
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
		logError("Failed to enumerate layers: "s + string_VkResult(rs));
		return pair(false, false);
	}
	vector<VkLayerProperties> availableLayers(count);
	vkEnumerateInstanceLayerProperties(&count, availableLayers.data());
	if (!std::any_of(availableLayers.begin(), availableLayers.end(), [](const VkLayerProperties& lp) -> bool { return !strcmp(lp.layerName, validationLayers[0]); })) {
		logError("Validation layers not available");
		return pair(false, false);
	}

	if (VkResult rs = vkEnumerateInstanceExtensionProperties(validationLayers[0], &count, nullptr); rs != VK_SUCCESS) {
		logError("Failed to enumerate layer extensions: "s + string_VkResult(rs));
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

template <class T, std::enable_if_t<std::is_pointer_v<T>, int>>
void RendererVk::setObjectDebugName(T object) const {
	if (!nextObjectDebugName.empty())
		setObjectDebugName(object, nextObjectDebugName.back().c_str());
}

template <class T, std::enable_if_t<std::is_pointer_v<T>, int>>
void RendererVk::setObjectDebugName(T object, const char* name) const {
	if (!pfnSetDebugUtilsObjectNameEXT || !name[0])
		return;

	VkDebugUtilsObjectNameInfoEXT objectNameInfo{};
	objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	objectNameInfo.objectHandle = uintptr_t(object);
	objectNameInfo.pObjectName = name;
	if constexpr (std::is_same_v<T, VkInstance>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_INSTANCE;
	else if constexpr (std::is_same_v<T, VkPhysicalDevice>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	else if constexpr (std::is_same_v<T, VkDevice>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE;
	else if constexpr (std::is_same_v<T, VkQueue>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
	else if constexpr (std::is_same_v<T, VkSemaphore>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
	else if constexpr (std::is_same_v<T, VkCommandBuffer>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
	else if constexpr (std::is_same_v<T, VkFence>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_FENCE;
	else if constexpr (std::is_same_v<T, VkDeviceMemory>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
	else if constexpr (std::is_same_v<T, VkBuffer>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
	else if constexpr (std::is_same_v<T, VkImage>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
	else if constexpr (std::is_same_v<T, VkEvent>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_EVENT;
	else if constexpr (std::is_same_v<T, VkQueryPool>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_QUERY_POOL;
	else if constexpr (std::is_same_v<T, VkBufferView>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER_VIEW;
	else if constexpr (std::is_same_v<T, VkImageView>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
	else if constexpr (std::is_same_v<T, VkShaderModule>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
	else if constexpr (std::is_same_v<T, VkPipelineCache>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_CACHE;
	else if constexpr (std::is_same_v<T, VkPipelineLayout>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	else if constexpr (std::is_same_v<T, VkRenderPass>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_RENDER_PASS;
	else if constexpr (std::is_same_v<T, VkPipeline>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_PIPELINE;
	else if constexpr (std::is_same_v<T, VkDescriptorSetLayout>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	else if constexpr (std::is_same_v<T, VkSampler>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_SAMPLER;
	else if constexpr (std::is_same_v<T, VkDescriptorPool>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	else if constexpr (std::is_same_v<T, VkDescriptorSet>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
	else if constexpr (std::is_same_v<T, VkFramebuffer>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
	else if constexpr (std::is_same_v<T, VkCommandPool>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
	else if constexpr (std::is_same_v<T, VkSurfaceKHR>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_SURFACE_KHR;
	else if constexpr (std::is_same_v<T, VkSwapchainKHR>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR;
	else if constexpr (std::is_same_v<T, VkDebugUtilsMessengerEXT>)
		objectNameInfo.objectType = VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT;
	else
		objectNameInfo.objectType = VK_OBJECT_TYPE_UNKNOWN;
	if (VkResult rs = pfnSetDebugUtilsObjectNameEXT(ldev, &objectNameInfo); rs != VK_SUCCESS)
		logError("Failed to set object debug name: "s + string_VkResult(rs));
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
