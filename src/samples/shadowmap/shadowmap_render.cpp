#include <cstdlib>
#include <iostream>
#include <random>

#include <etna/Etna.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/RenderTargetStates.hpp>
#include <geom/vk_mesh.h>
#include <loader_utils/images.h>
#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vulkan/vulkan_core.h>

#include "object.h"
#include "shadowmap_render.h"

static float get_random_float()
{
  static std::random_device dev;
  static std::mt19937 generator( dev() );
  std::uniform_real_distribution<float> uniform(0.f, 1.f);
  return uniform(generator);
}

/// RESOURCE ALLOCATION

void SimpleShadowmapRender::AllocateResources()
{
  gBuffer.mainViewDepth = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "main_view_depth",
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment
  });

  gBuffer.shadowMap = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{2048, 2048, 1},
    .name = "shadow_map",
    .format = vk::Format::eD16Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled
  });

  gBuffer.position = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "gbuffer_position",
    .format = vk::Format::eR32G32B32A32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
  });

  gBuffer.normal = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "gbuffer_normal",
    .format = vk::Format::eR32G32B32A32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
  });

  gBuffer.albedo = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "gbuffer_albedo",
    .format = vk::Format::eR8G8B8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
  });

  gBuffer.ssao = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "ssao_tex",
    .format = vk::Format::eR32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
  });

  gBuffer.blurredSsao = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "blurred_ssao_tex",
    .format = vk::Format::eR32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
  });

  frameBeforeTransparency = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "frame_before_transparency",
    .format = vk::Format::eR32G32B32A32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
  });

  frameTransparencyOnly = m_context->createImage(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = "frame_before_transparency",
    .format = vk::Format::eR32G32B32A32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
  });

  defaultSampler = etna::Sampler(etna::Sampler::CreateInfo{.name = "default_sampler"});
  constants = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(UniformParams),
    .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "constants"
  });

  m_uboMappedMem = constants.map();

  ssaoSamples = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size        = sizeof(float4) * m_uniforms.ssaoKernelSize,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "ssao_samples"
  });

  ssaoNoise = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size        = sizeof(float4) * m_uniforms.ssaoNoiseSize * m_uniforms.ssaoNoiseSize,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "ssao_noise"
  });

  void *ssaoSamplesMappedMem = ssaoSamples.map();
  std::vector<float4> ssaoSamplesVec;
  ssaoSamplesVec.reserve(m_uniforms.ssaoKernelSize);
  for (uint32_t i = 0; i < m_uniforms.ssaoKernelSize; ++i)
  {
    float4 sample = {get_random_float() * 2.f - 1.f, get_random_float() * 2.f - 1.f, get_random_float(), 0.0};
    ssaoSamplesVec.push_back(LiteMath::normalize(sample) * get_random_float());
  }
  memcpy(ssaoSamplesMappedMem, ssaoSamplesVec.data(), ssaoSamplesVec.size() * sizeof(float4));
  ssaoSamples.unmap();

  void *ssaoNoiseMappedMem = ssaoNoise.map();
  std::vector<float4> ssaoNoiseVec(m_uniforms.ssaoNoiseSize*m_uniforms.ssaoNoiseSize,
                                   {get_random_float() * 2.f - 1.f, get_random_float() * 2.f - 1.f, 0.f, 0.0f});
  memcpy(ssaoNoiseMappedMem, ssaoNoiseVec.data(), m_uniforms.ssaoNoiseSize*m_uniforms.ssaoNoiseSize*sizeof(float4));
  ssaoNoise.unmap();

  m_gaussian_kernel.resize(m_gaussian_kernel_length);
  float kernel_radius = (static_cast<float>(m_gaussian_kernel_length) - 1.f) / 2.f;
  float sigma = kernel_radius / 3.f;
  float doubled_sigma2 = 2.f * sigma * sigma;
  float sum = 0.f;

  for (uint32_t i = 0; i < m_gaussian_kernel_length; i++) {
    float delta = float(i) - kernel_radius;
    m_gaussian_kernel[i] = std::exp(- delta * delta / doubled_sigma2);
    sum += m_gaussian_kernel[i];
  }  
  for (uint32_t i = 0; i < m_gaussian_kernel_length; i++) {
    m_gaussian_kernel[i] /= sum;
  }

  gaussianKernel = m_context->createBuffer(etna::Buffer::CreateInfo
  {
    .size = sizeof(float) * m_gaussian_kernel_length,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "gaussian_kernel"
  });

  void *gaussianKernelMappedMem = gaussianKernel.map();
  memcpy(gaussianKernelMappedMem, m_gaussian_kernel.data(), sizeof(float) * m_gaussian_kernel_length);
  gaussianKernel.unmap();
}

void SimpleShadowmapRender::loadBackgroundTexture()
{
  int width, height, channels;
  uint8_t* pixels = loadImageLDR(VK_GRAPHICS_BASIC_ROOT"/resources/textures/shrek.jpg", width, height, channels);

  backgroundTexture = etna::create_image_from_bytes(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
    .name = "shrek",
    .format = vk::Format::eR8G8B8A8Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eSampled
  }, m_textureCmdBuffer, pixels);

  freeImageMemLDR(pixels);
}

void SimpleShadowmapRender::loadEnvironmentMap()
{
  int width, height, channels;
  constexpr int FACES_NUM = 6;
  uint8_t *pixels[FACES_NUM];
  std::array<std::string, FACES_NUM> filenames = {
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/posx.jpg",
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/negx.jpg",
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/posy.jpg",
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/negy.jpg",
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/posz.jpg",
    VK_GRAPHICS_BASIC_ROOT"/resources/textures/skybox/negz.jpg"
  };
  for (int i = 0; i < FACES_NUM; ++i)
    pixels[i] = loadImageLDR(filenames[i].c_str(), width, height, channels);

  int imageSize = width * height * 4;
  void *bytes = malloc(imageSize * FACES_NUM);
  for (int i = 0; i < FACES_NUM; ++i)
    memcpy((uint8_t *)bytes + i * imageSize, (void *)pixels[i], imageSize);

  environmentMap = etna::create_image_from_bytes(etna::Image::CreateInfo
  {
    .extent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
    .name = "skybox",
    .flags = vk::ImageCreateFlagBits() | vk::ImageCreateFlagBits::eCubeCompatible,
    .format = vk::Format::eR8G8B8A8Unorm,
    .imageUsage = vk::ImageUsageFlagBits::eSampled,
    .layers = FACES_NUM,
  }, m_textureCmdBuffer, bytes);

  for (int i = 0; i < FACES_NUM; ++i)
    freeImageMemLDR(pixels[i]);
  free(bytes);
}

void SimpleShadowmapRender::LoadScene(const char* path, bool transpose_inst_matrices)
{
  m_pScnMgr->LoadSceneXML(path, transpose_inst_matrices);
  // loadBackgroundTexture();
  loadEnvironmentMap();
  makeAssets();
  transparencyScene = std::make_unique<TransparencyScene>();

  // TODO: Make a separate stage
  loadShaders();
  PreparePipelines();

  auto loadedCam = m_pScnMgr->GetCamera(0);
  m_cam.fov = loadedCam.fov;
  m_cam.pos = float3(loadedCam.pos);
  m_cam.up  = float3(loadedCam.up);
  m_cam.lookAt = float3(loadedCam.lookAt);
  m_cam.tdist  = loadedCam.farPlane;
}

void SimpleShadowmapRender::DeallocateResources()
{
  gBuffer.mainViewDepth.reset(); // TODO: Make an etna method to reset all the resources
  gBuffer.shadowMap.reset();
  gBuffer.position.reset();
  gBuffer.normal.reset();
  gBuffer.albedo.reset();
  gBuffer.ssao.reset();
  gBuffer.blurredSsao.reset();
  frameBeforeTransparency.reset();
  frameTransparencyOnly.reset();
  m_swapchain.Cleanup();
  vkDestroySurfaceKHR(GetVkInstance(), m_surface, nullptr);  

  constants = etna::Buffer();
  ssaoSamples = etna::Buffer();
  ssaoNoise = etna::Buffer();
  gaussianKernel = etna::Buffer();
}





/// PIPELINES CREATION

void SimpleShadowmapRender::PreparePipelines()
{
  SetupSimplePipeline();
}

void SimpleShadowmapRender::loadShaders()
{
  etna::create_program("shadowmap_producer", {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/render_scene.vert.spv"});
  etna::create_program("prepare_gbuffer",
    {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/prepare_gbuffer.frag.spv", VK_GRAPHICS_BASIC_ROOT"/resources/shaders/render_scene.vert.spv"});
  etna::create_program("calculate_ssao",
    { VK_GRAPHICS_BASIC_ROOT"/resources/shaders/ssao.frag.spv", VK_GRAPHICS_BASIC_ROOT "/resources/shaders/fullscreen_quad.vert.spv" });
  etna::create_program("gaussian_blur", {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/gaussian_blur.comp.spv"});
  etna::create_program("resolve_gbuffer",
    {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/resolve_gbuffer.frag.spv", VK_GRAPHICS_BASIC_ROOT"/resources/shaders/resolve_gbuffer.vert.spv"});
  etna::create_program("screen_space_transparency",
    {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/transparency.frag.spv", VK_GRAPHICS_BASIC_ROOT"/resources/shaders/transparency.vert.spv"});
  etna::create_program("resolve_transparency",
    {VK_GRAPHICS_BASIC_ROOT"/resources/shaders/resolve_transparency.frag.spv", VK_GRAPHICS_BASIC_ROOT"/resources/shaders/resolve_transparency.vert.spv"});
}

void SimpleShadowmapRender::SetupSimplePipeline()
{
  etna::VertexShaderInputDescription sceneVertexInputDesc
    {
      .bindings = {etna::VertexShaderInputDescription::Binding
        {
          .byteStreamDescription = m_pScnMgr->GetVertexStreamDescription()
        }}
    };

  etna::VertexShaderInputDescription transparencyVertexInputDesc
    {
      .bindings = {etna::VertexShaderInputDescription::Binding
        {
          .byteStreamDescription = transparencyMeshes->getTransparencyVertexAttributeDescriptions()
        }}
    };

  auto blendAttachment = vk::PipelineColorBlendAttachmentState
  {
    .blendEnable = false,
    .colorWriteMask = vk::ColorComponentFlagBits::eR
      | vk::ColorComponentFlagBits::eG
      | vk::ColorComponentFlagBits::eB
      | vk::ColorComponentFlagBits::eA
  };

  auto& pipelineManager = etna::get_context().getPipelineManager();
  m_shadowPipeline = pipelineManager.createGraphicsPipeline("shadowmap_producer",
    {
      .vertexShaderInput = sceneVertexInputDesc,
      .fragmentShaderOutput =
        {
          .depthAttachmentFormat = vk::Format::eD16Unorm
        }
    });
  m_prepareGbufferPipeline = pipelineManager.createGraphicsPipeline("prepare_gbuffer",
    {
      .vertexShaderInput = sceneVertexInputDesc,
      .blendingConfig =
        {
          .attachments = {blendAttachment, blendAttachment, blendAttachment}
        },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Sfloat, vk::Format::eR8G8B8A8Srgb},
          .depthAttachmentFormat = vk::Format::eD32Sfloat
        }
    });
  m_resolveGbufferPipeline = pipelineManager.createGraphicsPipeline("resolve_gbuffer",
    {
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = { vk::Format::eR32G32B32A32Sfloat },
        }
    });
  m_ssaoPipeline = pipelineManager.createGraphicsPipeline("calculate_ssao",
    {
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vk::Format::eR32Sfloat},
        }
    });
  m_gaussianBlurPipeline = pipelineManager.createComputePipeline("gaussian_blur", {});
  m_screenSpaceTransparencyPipeline = pipelineManager.createGraphicsPipeline("screen_space_transparency",
    {
      .vertexShaderInput = transparencyVertexInputDesc,
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = { vk::Format::eR32G32B32A32Sfloat },
          .depthAttachmentFormat = vk::Format::eD32Sfloat
        }
    });
  m_resolveTransparencyPipeline = pipelineManager.createGraphicsPipeline("resolve_transparency",
    {
      .vertexShaderInput = transparencyVertexInputDesc,
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = { static_cast<vk::Format>(m_swapchain.GetFormat()) },
        }
    });
}


/// COMMAND BUFFER FILLING

void SimpleShadowmapRender::DrawSceneCmd(VkCommandBuffer a_cmdBuff, const float4x4& a_wvp, VkPipelineLayout a_pipelineLayout,
  VkShaderStageFlags stageFlags)
{
  VkDeviceSize zero_offset = 0u;
  VkBuffer vertexBuf = m_pScnMgr->GetVertexBuffer();
  VkBuffer indexBuf  = m_pScnMgr->GetIndexBuffer();
  
  vkCmdBindVertexBuffers(a_cmdBuff, 0, 1, &vertexBuf, &zero_offset);
  vkCmdBindIndexBuffer(a_cmdBuff, indexBuf, 0, VK_INDEX_TYPE_UINT32);

  pushConst2M.projView = a_wvp;
  for (uint32_t i = 0; i < m_pScnMgr->InstancesNum(); ++i)
  {
    auto inst         = m_pScnMgr->GetInstanceInfo(i);
    pushConst2M.model = m_pScnMgr->GetInstanceMatrix(i);
    pushConst2M.colorNo = i;
    vkCmdPushConstants(a_cmdBuff, a_pipelineLayout,
      stageFlags, 0, sizeof(pushConst2M), &pushConst2M);

    auto mesh_info = m_pScnMgr->GetMeshInfo(inst.mesh_id);
    vkCmdDrawIndexed(a_cmdBuff, mesh_info.m_indNum, 1, mesh_info.m_indexOffset, mesh_info.m_vertexOffset, 0);
  }
}

void SimpleShadowmapRender::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  //// draw scene to shadowmap
  //
  {
    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, 2048, 2048}, {}, gBuffer.shadowMap);

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline.getVkPipeline());
    DrawSceneCmd(a_cmdBuff, m_lightMatrix, m_shadowPipeline.getVkPipelineLayout());
  }

  //// prepare gbuffer
  //
  {
    auto prepareGbufferInfo = etna::get_shader_program("prepare_gbuffer");
    auto set = etna::create_descriptor_set(prepareGbufferInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, constants.genBinding()}
    });

    VkDescriptorSet vkSet = set.getVkSet();

    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, {{gBuffer.position}, {gBuffer.normal}, {gBuffer.albedo}}, gBuffer.mainViewDepth);

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_prepareGbufferPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_prepareGbufferPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    DrawSceneCmd(a_cmdBuff, m_worldViewProj, m_prepareGbufferPipeline.getVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  //// calculate SSAO
  //
  {
    auto ssaoInfo = etna::get_shader_program("calculate_ssao");
    auto set = etna::create_descriptor_set(ssaoInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, constants.genBinding()},
      etna::Binding {1, gBuffer.position.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {2, gBuffer.normal.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {3, ssaoSamples.genBinding()},
      etna::Binding {4, ssaoNoise.genBinding()}
    });

    VkDescriptorSet vkSet = set.getVkSet();

    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, {{gBuffer.ssao}}, {});

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ssaoPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_ssaoPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    vkCmdDraw(a_cmdBuff, 6, 1, 0, 0); // 6 vertices for 2 triangles in a quad
  }

  //// blur SSAO texture
  //
  if (m_uniforms.ssaoEnabled)
  {
    auto gaussianBlurInfo = etna::get_shader_program("gaussian_blur");
    auto set = etna::create_descriptor_set(gaussianBlurInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, gBuffer.ssao.genBinding(defaultSampler.get(), vk::ImageLayout::eGeneral)},
      etna::Binding {1, gBuffer.blurredSsao.genBinding(defaultSampler.get(), vk::ImageLayout::eGeneral)},
      etna::Binding {2, gBuffer.position.genBinding(defaultSampler.get(), vk::ImageLayout::eGeneral)},
      etna::Binding {3, gaussianKernel.genBinding()},
    });
    VkDescriptorSet vkSet = set.getVkSet();
    etna::flush_barriers(a_cmdBuff);

    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE,
      m_gaussianBlurPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);
    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_gaussianBlurPipeline.getVkPipeline());
    vkCmdDispatch(a_cmdBuff, m_width / 32 + 1, m_height / 32 + 1, 1);
  }

  //// resolve gbuffer
  //
  {
    auto resolveGbufferInfo = etna::get_shader_program("resolve_gbuffer");
    auto set = etna::create_descriptor_set(resolveGbufferInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, constants.genBinding()},
      etna::Binding {1, gBuffer.shadowMap.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {2, gBuffer.position.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {3, gBuffer.normal.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {4, gBuffer.albedo.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {5, gBuffer.blurredSsao.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      // etna::Binding {6, backgroundTexture.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {0, 1, 1, vk::ImageViewType::e2D})},
      etna::Binding {7, environmentMap.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {0, 1, 6, vk::ImageViewType::eCube})},
    });
    VkDescriptorSet vkSet = set.getVkSet();

    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, {frameBeforeTransparency}, {});

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolveGbufferPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_resolveGbufferPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    vkCmdDraw(a_cmdBuff, 6, 1, 0, 0); // 6 vertices for 2 triangles in a quad
  }

  //// render transparency
  //
  {
    auto screenSpaceTransparencyInfo = etna::get_shader_program("screen_space_transparency");
    auto set = etna::create_descriptor_set(screenSpaceTransparencyInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, constants.genBinding()},
      etna::Binding {1, frameBeforeTransparency.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {2, gBuffer.position.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {3, gBuffer.albedo.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {4, environmentMap.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {0, 1, 6, vk::ImageViewType::eCube})},
    });
    VkDescriptorSet vkSet = set.getVkSet();

    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, {frameTransparencyOnly}, gBuffer.mainViewDepth);

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_screenSpaceTransparencyPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_screenSpaceTransparencyPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    prepareTransparency(a_cmdBuff);
    uint32_t startInstance = 0;
		for (std::pair<meshTypes, std::vector<glm::vec3>> pair : transparencyScene->positions)
			renderTransparency(
				a_cmdBuff, pair.first, startInstance, static_cast<uint32_t>(pair.second.size())
			);
  }

  //// resolve transparency
  //
  {
    auto resolveTransparencyInfo = etna::get_shader_program("resolve_transparency");
    auto set = etna::create_descriptor_set(resolveTransparencyInfo.getDescriptorLayoutId(0), a_cmdBuff,
    {
      etna::Binding {0, frameBeforeTransparency.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding {1, frameTransparencyOnly.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
    });
    VkDescriptorSet vkSet = set.getVkSet();

    etna::RenderTargetState renderTargets(a_cmdBuff, {0, 0, m_width, m_height}, {{a_targetImage, a_targetImageView}}, {});

    vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, m_resolveTransparencyPipeline.getVkPipeline());
    vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_resolveTransparencyPipeline.getVkPipelineLayout(), 0, 1, &vkSet, 0, VK_NULL_HANDLE);

    vkCmdDraw(a_cmdBuff, 6, 1, 0, 0); // 6 vertices for 2 triangles in a quad
  }

  if(m_input.drawFSQuad)
    m_pQuad->RecordCommands(a_cmdBuff, a_targetImage, a_targetImageView, gBuffer.shadowMap, defaultSampler);

  etna::set_state(a_cmdBuff, a_targetImage, vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::AccessFlags2(), vk::ImageLayout::ePresentSrcKHR,
    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

  etna::finish_frame(a_cmdBuff);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}

void SimpleShadowmapRender::makeAssets()
{
  std::string modelsPath = "resources/models/";
  std::string modelName = read_model_name(modelsPath + "model_to_load.txt");
	std::unordered_map<meshTypes, std::string> model_filenames = {
		{meshTypes::CUBE, modelsPath + modelName},
	};
	std::unordered_map<meshTypes, glm::mat4> preTransforms = {
		{meshTypes::CUBE, glm::mat4(1.f)}
		// {meshTypes::GIRL, glm::rotate(
		// 	glm::mat4(1.f), 
		// 	glm::radians(180.f), 
		// 	glm::vec3(0.f, 0.f, 1.f)
		// )},
	};
	std::unordered_map<meshTypes, ObjectMesh> loaded_models;

	std::vector<meshTypes> mesh_types = {
		meshTypes::CUBE,
	};
	for (meshTypes type : mesh_types)
	{
		loaded_models[type] = ObjectMesh();
		loaded_models[type].load(model_filenames[type] + ".obj", preTransforms[type]);
	}

  transparencyMeshes = std::make_unique<TransparencyMeshes>(m_context->getDevice(), m_context->getPhysicalDevice(),
    m_context->getQueueFamilyIdx(), m_context->getQueueFamilyIdx());

  for (std::pair<meshTypes, ObjectMesh> pair : loaded_models)
		transparencyMeshes->consume(pair.first, pair.second.vertices, pair.second.indices, model_filenames[pair.first] + ".sph");

	transparencyMeshes->finalize();
}

void SimpleShadowmapRender::prepareTransparency(vk::CommandBuffer commandBuffer)
{
	VkDeviceSize zero_offset = 0u;
  VkBuffer vertexBuf = transparencyMeshes->GetVertexBuffer();
  VkBuffer indexBuf  = transparencyMeshes->GetIndexBuffer();
  
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuf, &zero_offset);
	commandBuffer.bindIndexBuffer(indexBuf, 0, vk::IndexType::eUint32);
}

void SimpleShadowmapRender::renderTransparency(vk::CommandBuffer commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount)
{
	int indexCount = transparencyMeshes->indexCounts.find(objectType)->second;
	int firstIndex = transparencyMeshes->firstIndices.find(objectType)->second;
	commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, 0, startInstance);
	startInstance += instanceCount;
}

