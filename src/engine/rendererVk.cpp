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
#include <set>

// GENERIC PASS

VkSampler GenericPass::createSampler(VkDevice dev, VkFilter filter) {
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
	if (VkResult rs = vkCreateSampler(dev, &samplerInfo, nullptr, &sampler); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture sampler: "s + string_VkResult(rs));
	return sampler;
}

VkShaderModule GenericPass::createShaderModule(VkDevice dev, const uint32* code, sizet clen) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = clen;
	createInfo.pCode = code;

	VkShaderModule shaderModule;
	if (VkResult rs = vkCreateShaderModule(dev, &createInfo, nullptr, &shaderModule); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module: "s + string_VkResult(rs));
	return shaderModule;
}

// RENDER PASS

RenderPass::DescriptorSetBlock::DescriptorSetBlock(const array<VkDescriptorSet, textureSetStep>& descriptorSets) :
	used(descriptorSets.begin(), descriptorSets.begin() + 1),
	free(descriptorSets.begin() + 1, descriptorSets.end())
{}

vector<VkDescriptorSet> RenderPass::init(const RendererVk* rend, VkFormat format, uint32 numViews) {
	createRenderPass(rend->getLogicalDevice(), format);
	samplers = { createSampler(rend->getLogicalDevice(), VK_FILTER_LINEAR), createSampler(rend->getLogicalDevice(), VK_FILTER_NEAREST) };
	createDescriptorSetLayout(rend->getLogicalDevice());
	createPipeline(rend->getLogicalDevice());
	return createDescriptorPoolAndSets(rend->getLogicalDevice(), numViews);
}

void RenderPass::createRenderPass(VkDevice dev, VkFormat format) {
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
	if (VkResult rs = vkCreateRenderPass(dev, &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass: "s + string_VkResult(rs));
}

void RenderPass::createDescriptorSetLayout(VkDevice dev) {
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
	if (VkResult rs = vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &descriptorSetLayouts[0]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout 0: "s + string_VkResult(rs));

	VkDescriptorSetLayoutBinding binding1{};
	binding1.binding = 0;
	binding1.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	binding1.descriptorCount = 1;
	binding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding1;
	if (VkResult rs = vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &descriptorSetLayouts[1]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout 1: "s + string_VkResult(rs));
}

void RenderPass::createPipeline(VkDevice dev) {
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
	VkShaderModule vertShaderModule = createShaderModule(dev, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(dev, fragCode, sizeof(fragCode));

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
	if (VkResult rs = vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout: "s + string_VkResult(rs));

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
	if (VkResult rs = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline: "s + string_VkResult(rs));

	vkDestroyShaderModule(dev, fragShaderModule, nullptr);
	vkDestroyShaderModule(dev, vertShaderModule, nullptr);
}

vector<VkDescriptorSet> RenderPass::createDescriptorPoolAndSets(VkDevice dev, uint32 numViews) {
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
	if (VkResult rs = vkCreateDescriptorPool(dev, &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create view descriptor pool: "s + string_VkResult(rs));

	vector<VkDescriptorSetLayout> layouts(poolInfo.maxSets, descriptorSetLayouts[0]);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	vector<VkDescriptorSet> descriptorSets(layouts.size());
	if (VkResult rs = vkAllocateDescriptorSets(dev, &allocInfo, descriptorSets.data()); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate view descriptor sets: "s + string_VkResult(rs));
	return descriptorSets;
}

pair<VkDescriptorPool, VkDescriptorSet> RenderPass::newDescriptorSetTex(VkDevice dev) {
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
	if (VkResult rs = vkCreateDescriptorPool(dev, &poolInfo, nullptr, &descPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture descriptor pool: "s + string_VkResult(rs));

	array<VkDescriptorSetLayout, textureSetStep> layouts;
	layouts.fill(descriptorSetLayouts[1]);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	array<VkDescriptorSet, textureSetStep> descriptorSets;
	if (VkResult rs = vkAllocateDescriptorSets(dev, &allocInfo, descriptorSets.data()); rs != VK_SUCCESS) {
		vkDestroyDescriptorPool(dev, descPool, nullptr);
		throw std::runtime_error("Failed to allocate texture descriptor sets: "s + string_VkResult(rs));
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
	createRenderPass(rend->getLogicalDevice());
	createDescriptorSetLayout(rend->getLogicalDevice());
	createPipeline(rend->getLogicalDevice());
	createUniformBuffer(rend);
	createDescriptorPoolAndSet(rend->getLogicalDevice());
	updateDescriptorSet(rend->getLogicalDevice());
}

void AddressPass::createRenderPass(VkDevice dev) {
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
	if (VkResult rs = vkCreateRenderPass(dev, &renderPassInfo, nullptr, &handle); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create render pass: "s + string_VkResult(rs));
}

void AddressPass::createDescriptorSetLayout(VkDevice dev) {
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = bindingUdat;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &binding;
	if (VkResult rs = vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &descriptorSetLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor set layout: "s + string_VkResult(rs));
}

void AddressPass::createPipeline(VkDevice dev) {
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
	VkShaderModule vertShaderModule = createShaderModule(dev, vertCode, sizeof(vertCode));
	VkShaderModule fragShaderModule = createShaderModule(dev, fragCode, sizeof(fragCode));

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
	if (VkResult rs = vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &pipelineLayout); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout: "s + string_VkResult(rs));

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
	if (VkResult rs = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create graphics pipeline: "s + string_VkResult(rs));

	vkDestroyShaderModule(dev, fragShaderModule, nullptr);
	vkDestroyShaderModule(dev, vertShaderModule, nullptr);
}

void AddressPass::createUniformBuffer(const RendererVk* rend) {
	std::tie(uniformBuffer, uniformBufferMemory) = rend->createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(rend->getLogicalDevice(), uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&uniformBufferMapped)); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to map uniform buffer memory: "s + string_VkResult(rs));
}

void AddressPass::createDescriptorPoolAndSet(VkDevice dev) {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;
	if (VkResult rs = vkCreateDescriptorPool(dev, &poolInfo, nullptr, &descriptorPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool: "s + string_VkResult(rs));

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	if (VkResult rs = vkAllocateDescriptorSets(dev, &allocInfo, &descriptorSet); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets: "s + string_VkResult(rs));
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
	bgColor{ { { bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a } } }
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
	pickPhysicalDevice(sets->device);
	createDevice();
	singleTimeFence = createFence();
	createCommandPool();
	setPresentMode(sets->vsync);

	umap<VkFormat, uint> formatCounter;
	for (auto [id, view] : views)
		++formatCounter[createSwapchain(static_cast<ViewVk*>(view))];

	vector<VkDescriptorSet> descriptorSets = renderPass.init(this, std::max_element(formatCounter.begin(), formatCounter.end(), [](const pair<const VkFormat, uint>& a, const pair<const VkFormat, uint>& b) -> bool { return a.second < b.second; })->first, views.size());
	sizet d = 0;
	for (auto [id, view] : views) {
		ViewVk* vw = static_cast<ViewVk*>(view);
		createFramebuffers(vw);
		allocateCommandBuffers(vw->commandBuffers.data(), vw->commandBuffers.size());
		for (uint i = 0; i < ViewVk::maxFrames; ++i) {
			vw->imageAvailableSemaphores[i] = createSemaphore();
			vw->renderFinishedSemaphores[i] = createSemaphore();
			vw->frameFences[i] = createFence(VK_FENCE_CREATE_SIGNALED_BIT);
		}

		createUniformBuffer(vw);
		vw->uniformMapped->pview = vec4(vw->rect.pos(), vec2(vw->rect.size()) / 2.f);
		vw->descriptorSet = descriptorSets[d++];
		renderPass.updateDescriptorSet(ldev, vw->descriptorSet, vw->uniformBuffer);
	}

	addressPass.init(this);
	std::tie(addrImage, addrImageMemory) = createImage(u32vec2(1), VK_IMAGE_TYPE_1D, AddressPass::format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	addrView = createImageView(addrImage, VK_IMAGE_VIEW_TYPE_1D, AddressPass::format);
	addrFramebuffer = createFramebuffer(addressPass.getHandle(), addrView, u32vec2(1));
	std::tie(addrBuffer, addrBufferMemory) = createBuffer(sizeof(u32vec2), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(ldev, addrBufferMemory, 0, sizeof(u32vec2), 0, reinterpret_cast<void**>(&addrMappedMemory)); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to map address lookup memory: "s + string_VkResult(rs));
	allocateCommandBuffers(&commandBufferAddr, 1);
	addrFence = createFence();
}

RendererVk::~RendererVk() {
	vkDeviceWaitIdle(ldev);
	for (auto [id, view] : views) {
		ViewVk* vw = static_cast<ViewVk*>(view);
		freeFramebuffers(vw);
		vkDestroySwapchainKHR(ldev, vw->swapchain, nullptr);
	}

	renderPass.free(ldev);
	for (auto [id, view] : views) {
		ViewVk* vw = static_cast<ViewVk*>(view);
		vkDestroyBuffer(ldev, vw->uniformBuffer, nullptr);
		vkFreeMemory(ldev, vw->uniformMemory, nullptr);

		for (uint i = 0; i < ViewVk::maxFrames; ++i) {
			vkDestroySemaphore(ldev, vw->renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(ldev, vw->imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(ldev, vw->frameFences[i], nullptr);
		}
	}

	addressPass.free(ldev);
	vkDestroyFramebuffer(ldev, addrFramebuffer, nullptr);
	vkDestroyImageView(ldev, addrView, nullptr);
	vkDestroyImage(ldev, addrImage, nullptr);
	vkFreeMemory(ldev, addrImageMemory, nullptr);
	vkDestroyBuffer(ldev, addrBuffer, nullptr);
	vkFreeMemory(ldev, addrBufferMemory, nullptr);
	vkFreeCommandBuffers(ldev, cmdPool, 1, &commandBufferAddr);
	vkDestroyFence(ldev, addrFence, nullptr);

	vkDestroyCommandPool(ldev, cmdPool, nullptr);
	vkDestroyFence(ldev, singleTimeFence, nullptr);
	vkDestroyDevice(ldev, nullptr);
#ifndef NDEBUG
	if (dbgMessenger != VK_NULL_HANDLE)
		if (PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")))
			destroyDebugUtilsMessengerEXT(instance, dbgMessenger, nullptr);
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
	bool validation = checkValidationLayerSupport();
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
	debugCreateInfo.pfnUserCallback = debugCallback;
	vector<const char*> extensions = getRequiredExtensions(window, validation);
#endif

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
#ifndef NDEBUG
	if (validation) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = &debugCreateInfo;
	}
#endif
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance");

#ifndef NDEBUG
	if (validation)
		if (PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")); !createDebugUtilsMessengerEXT || createDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &dbgMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger");
#endif
}

void RendererVk::pickPhysicalDevice(u32vec2& preferred) {
	uint32 deviceCount;
	if (VkResult rs = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to find devices: "s + string_VkResult(rs));
	vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	VkPhysicalDevice lastDev = VK_NULL_HANDLE;
	uint32 gfam, pfam;
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
		for (const VkExtensionProperties& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);
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
		for (uint32 i = 0; i < memp.memoryTypeCount; ++i)
			if (std::list<VkMemoryPropertyFlags>::iterator it = std::find_if(requiredMemoryTypes.begin(), requiredMemoryTypes.end(), [&memp, i](VkMemoryPropertyFlags flg) -> bool { return (memp.memoryTypes[i].propertyFlags & flg) == flg; }); it != requiredMemoryTypes.end())
				requiredMemoryTypes.erase(it);
		if (!requiredMemoryTypes.empty())
			continue;

		auto [gid, pid, ok] = findQueueFamilies(dev);
		if (!ok)
			continue;

		for (auto [id, view] : views)
			if (VkSurfaceKHR surf = static_cast<ViewVk*>(view)->surface; querySurfaceFormatSupport(dev, surf).empty() || queryPresentModeSupport(dev, surf).empty()) {
				ok = false;
				break;
			}
		if (!ok)
			continue;

		if (prop.vendorID == preferred.x && prop.deviceID == preferred.y) {
			std::tie(pdev, gfamilyIndex, pfamilyIndex, pdevProperties, pdevMemProperties) = tuple(dev, gid, pid, prop, memp);
			return;
		}
		if (uint points = scoreDevice(prop, memp); points > score)
			std::tie(score, lastDev, gfam, pfam, devProp, devMemp) = tuple(points, dev, gid, pid, prop, memp);
	}
	if (lastDev == VK_NULL_HANDLE)
		throw std::runtime_error("Failed to find a suitable device");

	std::tie(pdev, gfamilyIndex, pfamilyIndex, pdevProperties, pdevMemProperties) = tuple(lastDev, gfam, pfam, devProp, devMemp);
	if (preferred != u32vec2(0))
		preferred = u32vec2(0);
}

void RendererVk::createDevice() {
	vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	float queuePriority = 1.f;
	for (uint32 qfam : std::set<uint32>{ gfamilyIndex, pfamilyIndex }) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = qfam;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
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

	vkGetDeviceQueue(ldev, gfamilyIndex, 0, &gqueue);
	vkGetDeviceQueue(ldev, pfamilyIndex, 0, &pqueue);
}

void RendererVk::createCommandPool() {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = gfamilyIndex;
	if (VkResult rs = vkCreateCommandPool(ldev, &poolInfo, nullptr, &cmdPool); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool: "s + string_VkResult(rs));
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

void RendererVk::createFramebuffers(ViewVk* view) {
	for (uint32 i = 0; i < view->imageCount; ++i)
		view->framebuffers[i].second = createFramebuffer(renderPass.getHandle(), view->framebuffers[i].first, u32vec2(view->extent.width, view->extent.height));
}

void RendererVk::createUniformBuffer(ViewVk* view) {
	std::tie(view->uniformBuffer, view->uniformMemory) = createBuffer(sizeof(RenderPass::UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (VkResult rs = vkMapMemory(ldev, view->uniformMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&view->uniformMapped)); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to map uniform buffer memory 1: "s + string_VkResult(rs));
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

	array<VkSemaphore, 1> waitSemaphores = { currentView->imageAvailableSemaphores[currentFrame] };
	array<VkSemaphore, 1> signalSemaphores = { currentView->renderFinishedSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[waitSemaphores.size()] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = waitSemaphores.size();
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentView->commandBuffers[currentFrame];
	submitInfo.signalSemaphoreCount = signalSemaphores.size();
	submitInfo.pSignalSemaphores = signalSemaphores.data();
	if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, currentView->frameFences[currentFrame]); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer: "s + string_VkResult(rs));

	array<VkSwapchainKHR, 1> swapChains = { currentView->swapchain };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = signalSemaphores.size();
	presentInfo.pWaitSemaphores = signalSemaphores.data();
	presentInfo.swapchainCount = swapChains.size();
	presentInfo.pSwapchains = swapChains.data();
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

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (VkResult rs = vkBeginCommandBuffer(commandBufferAddr, &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to begin recording command buffer: "s + string_VkResult(rs));

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
	pd.addr = uvec2(uptrt(wgt), uptrt(wgt) >> 32);
	vkCmdPushConstants(commandBufferAddr, addressPass.getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(AddressPass::PushData), &pd);
	vkCmdDraw(commandBufferAddr, 4, 1, 0, 0);
}

Widget* RendererVk::finishSelDraw(View*) {
	vkCmdEndRenderPass(commandBufferAddr);
	transitionImageLayout<VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL>(commandBufferAddr, addrImage);
	copyImageToBuffer(commandBufferAddr, addrImage, addrBuffer, u32vec2(1));
	if (VkResult rs = vkEndCommandBuffer(commandBufferAddr); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to end recording command buffer: "s + string_VkResult(rs));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBufferAddr;
	if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, addrFence); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to submit draw command buffer: "s + string_VkResult(rs));
	vkWaitForFences(ldev, 1, &addrFence, VK_TRUE, UINT64_MAX);
	vkResetFences(ldev, 1, &addrFence);
	vkResetCommandBuffer(commandBufferAddr, 0);
	return reinterpret_cast<Widget*>(uptrt(addrMappedMemory->x) | (uptrt(addrMappedMemory->y) << 32));
}

Texture* RendererVk::texFromImg(SDL_Surface* img) {
	if (auto [pic, fmt] = pickPixFormat(img); pic)
		return createTexture(pic, u32vec2(pic->w, pic->h), fmt, false);
	return nullptr;
}

Texture* RendererVk::texFromText(SDL_Surface* img) {
	return img ? createTexture(img, glm::min(u32vec2(img->w, img->h), u32vec2(pdevProperties.limits.maxImageDimension2D)), VK_FORMAT_A8B8G8R8_UNORM_PACK32, true) : nullptr;
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

RendererVk::TextureVk* RendererVk::createTexture(SDL_Surface* img, u32vec2 res, VkFormat format, bool nearest) {
	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkDescriptorSet dset = VK_NULL_HANDLE;
	try {
		VkDeviceSize bufferSize = VkDeviceSize(img->pitch) * VkDeviceSize(res.y);
		std::tie(stagingBuffer, stagingMemory) = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		void* stagingMapped;
		if (VkResult rs = vkMapMemory(ldev, stagingMemory, 0, VK_WHOLE_SIZE, 0, &stagingMapped); rs != VK_SUCCESS)
			throw std::runtime_error("Failed to map texture staging memory: "s + string_VkResult(rs));
		memcpy(stagingMapped, img->pixels, bufferSize);
		vkUnmapMemory(ldev, stagingMemory);

		std::tie(image, memory) = createImage(res, VK_IMAGE_TYPE_2D, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		commandBuffer = beginSingleTimeCommands();
		transitionImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(commandBuffer, image);
		copyBufferToImage(commandBuffer, stagingBuffer, image, res, img->pitch / img->format->BytesPerPixel);
		transitionImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(commandBuffer, image);
		endSingleTimeCommands(commandBuffer);

		view = createImageView(image, VK_IMAGE_VIEW_TYPE_2D, format);
		std::tie(pool, dset) = renderPass.newDescriptorSetTex(ldev);
		RenderPass::updateDescriptorSet(ldev, dset, view);

		vkDestroyBuffer(ldev, stagingBuffer, nullptr);
		vkFreeMemory(ldev, stagingMemory, nullptr);
		SDL_FreeSurface(img);
	} catch (const std::runtime_error& err) {
		logError(err.what());
		freeCommandBuffers(&commandBuffer, 1);
		renderPass.freeDescriptorSetTex(ldev, pool, dset);
		vkDestroyImageView(ldev, view, nullptr);
		vkDestroyImage(ldev, image, nullptr);
		vkFreeMemory(ldev, memory, nullptr);
		vkDestroyBuffer(ldev, stagingBuffer, nullptr);
		vkFreeMemory(ldev, stagingMemory, nullptr);
		SDL_FreeSurface(img);
		return nullptr;
	}
	return new TextureVk(res, image, memory, view, pool, dset, nearest);
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
	vkBindImageMemory(ldev, image, memory, 0);
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
	return framebuffer;
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

void RendererVk::allocateCommandBuffers(VkCommandBuffer* cmdBuffers, uint32 count) const {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = cmdPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = count;
	if (VkResult rs = vkAllocateCommandBuffers(ldev, &allocInfo, cmdBuffers); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate command buffers: "s + string_VkResult(rs));
}

VkSemaphore RendererVk::createSemaphore() const {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (VkResult rs = vkCreateSemaphore(ldev, &semaphoreInfo, nullptr, &semaphore); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create semaphore: "s + string_VkResult(rs));
	return semaphore;
}

VkFence RendererVk::createFence(VkFenceCreateFlags flags) const {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = flags;

	VkFence fence;
	if (VkResult rs = vkCreateFence(ldev, &fenceInfo, nullptr, &fence); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to create fence: "s + string_VkResult(rs));
	return fence;
}

VkCommandBuffer RendererVk::beginSingleTimeCommands() const {
	VkCommandBuffer commandBuffer;
	allocateCommandBuffers(&commandBuffer, 1);
	try {
		beginSingleTimeCommands(commandBuffer);
	} catch (const std::runtime_error&) {
		freeCommandBuffers(&commandBuffer, 1);
		throw;
	}
	return commandBuffer;
}

void RendererVk::beginSingleTimeCommands(VkCommandBuffer commandBuffer) const {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (VkResult rs = vkBeginCommandBuffer(commandBuffer, &beginInfo); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to begin single time command buffer: "s + string_VkResult(rs));
}

void RendererVk::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
	try {
		if (VkResult rs = vkEndCommandBuffer(commandBuffer); rs != VK_SUCCESS)
			throw std::runtime_error("Failed to end single time command buffer: "s + string_VkResult(rs));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, singleTimeFence); rs != VK_SUCCESS)
			throw std::runtime_error("Failed to submit single time command buffer: "s + string_VkResult(rs));
		vkWaitForFences(ldev, 1, &singleTimeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(ldev, 1, &singleTimeFence);
		freeCommandBuffers(&commandBuffer, 1);
	} catch (const std::runtime_error&) {
		freeCommandBuffers(&commandBuffer, 1);
		throw;
	}
}

void RendererVk::submitSingleTimeCommands(VkCommandBuffer commandBuffer) const {
	if (VkResult rs = vkEndCommandBuffer(commandBuffer); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to end single time command buffer: "s + string_VkResult(rs));

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	if (VkResult rs = vkQueueSubmit(gqueue, 1, &submitInfo, singleTimeFence); rs != VK_SUCCESS)
		throw std::runtime_error("Failed to submit single time command buffer: "s + string_VkResult(rs));
	vkWaitForFences(ldev, 1, &singleTimeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(ldev, 1, &singleTimeFence);
	vkResetCommandBuffer(commandBuffer, 0);
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
		throw std::invalid_argument("Unsupported layout transition from "s + string_VkImageLayout(srcLay) + " to " + string_VkImageLayout(dstLay));
	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
vector<const char*> RendererVk::getRequiredExtensions(SDL_Window* win, bool validation) {
#endif
	uint count;
	if (!SDL_Vulkan_GetInstanceExtensions(win, &count, nullptr))
		throw std::runtime_error(SDL_GetError());
	vector<const char*> extensions(count);
	SDL_Vulkan_GetInstanceExtensions(win, &count, extensions.data());
#ifndef NDEBUG
	if (validation)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
	return extensions;
}

tuple<uint32, uint32, bool> RendererVk::findQueueFamilies(VkPhysicalDevice dev) const {
	uint32 count;
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
	vector<VkQueueFamilyProperties> families(count);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());

	optional<uint32> gid, pid;
	bool done = false;
	for (uint32 i = 0; i < count && !done; ++i, done = gid.has_value() && pid.has_value()) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			gid = i;

		bool presentSupport = true;
		for (auto [id, view] : views)
			if (VkBool32 support = false; vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, static_cast<ViewVk*>(view)->surface, &support) != VK_SUCCESS || !support) {
				presentSupport = false;
				break;
			}
		if (presentSupport)
			pid = i;
	}
	return done ? tuple(gid.value(), pid.value(), done) : tuple(UINT32_MAX, UINT32_MAX, done);
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

void RendererVk::getAdditionalSettings(bool& compression, vector<pair<u32vec2, string>>& devices) {
	compression = false;

	uint32 count;
	if (vkEnumeratePhysicalDevices(instance, &count, nullptr) != VK_SUCCESS)
		return;
	vector<VkPhysicalDevice> pdevs(count);
	vkEnumeratePhysicalDevices(instance, &count, pdevs.data());

	devices.resize(count + 1);
	devices[0] = pair(u32vec2(0), "auto");
	for (uint32 i = 0; i < count; ++i) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(pdevs[i], &prop);
		devices[i + 1] = pair(u32vec2(prop.vendorID, prop.deviceID), prop.deviceName);
	}
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
	if (img = limitSize(img, pdevProperties.limits.maxImageDimension2D); img && img->format->format != SDL_PIXELFORMAT_RGBA32) {
		SDL_Surface* dst = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
		SDL_FreeSurface(img);
		img = dst;
	}
	return pair(img, VK_FORMAT_A8B8G8R8_UNORM_PACK32);
}

#ifndef NDEBUG
bool RendererVk::checkValidationLayerSupport() {
	uint32 count;
	if (VkResult rs = vkEnumerateInstanceLayerProperties(&count, nullptr); rs != VK_SUCCESS) {
		logError("Failed to enumerate layers: "s + string_VkResult(rs));
		return false;
	}
	vector<VkLayerProperties> availableLayers(count);
	vkEnumerateInstanceLayerProperties(&count, availableLayers.data());
	for (const char* layerName : validationLayers)
		if (!std::any_of(availableLayers.begin(), availableLayers.end(), [layerName](const VkLayerProperties& lp) -> bool { return !strcmp(lp.layerName, layerName); })) {
			logError("Validation layers not available");
			return false;
		}
	return true;
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
