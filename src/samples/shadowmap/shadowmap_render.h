#ifndef SIMPLE_SHADOWMAP_RENDER_H
#define SIMPLE_SHADOWMAP_RENDER_H

#include "../../render/scene_mgr.h"
#include "../../render/render_common.h"
#include "../../render/quad_renderer.h"
#include "../../../resources/shaders/common.h"
#include "etna/GraphicsPipeline.hpp"
#include <geom/vk_mesh.h>
#include "transparency_scene.h"
#include "transparency_meshes.h"
#include <vk_descriptor_sets.h>
#include <vk_fbuf_attachment.h>
#include <vk_images.h>
#include <vk_swapchain.h>

#include <string>
#include <iostream>

#include <etna/GlobalContext.hpp>
#include <etna/Sampler.hpp>


class IRenderGUI;

class SimpleShadowmapRender : public IRender
{
public:
  SimpleShadowmapRender(uint32_t a_width, uint32_t a_height);
  ~SimpleShadowmapRender();

  uint32_t     GetWidth()      const override { return m_width; }
  uint32_t     GetHeight()     const override { return m_height; }
  VkInstance   GetVkInstance() const override { return m_context->getInstance(); }

  void InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId) override;

  void InitPresentation(VkSurfaceKHR &a_surface, bool initGUI) override;

  void ProcessInput(const AppInput& input) override;
  void UpdateCamera(const Camera* cams, uint32_t a_camsNumber) override;
  Camera GetCurrentCamera() override {return m_cam;}
  void UpdateView();

  void LoadScene(const char *path, bool transpose_inst_matrices) override;
  void DrawFrame(float a_time, DrawMode a_mode) override;

private:
  etna::GlobalContext* m_context;
  struct {
    etna::Image position;
    etna::Image albedo;
    etna::Image normal;
    etna::Image mainViewDepth;
    etna::Image shadowMap;
    etna::Image ssao;
    etna::Image blurredSsao;
  } gBuffer;
  etna::Image frameBeforeTransparency;
  etna::Image frameTransparencyOnly;
  etna::Image backgroundTexture;
  etna::Image environmentMap;
  etna::Sampler defaultSampler;
  etna::Buffer constants;
  etna::Buffer ssaoSamples;
  etna::Buffer ssaoNoise;
  etna::Buffer gaussianKernel;

  VkCommandPool    m_commandPool    = VK_NULL_HANDLE;

  struct
  {
    uint32_t    currentFrame      = 0u;
    VkQueue     queue             = VK_NULL_HANDLE;
    VkSemaphore imageAvailable    = VK_NULL_HANDLE;
    VkSemaphore renderingFinished = VK_NULL_HANDLE;
  } m_presentationResources;

  std::vector<VkFence> m_frameFences;
  std::vector<VkCommandBuffer> m_cmdBuffersDrawMain;
  VkCommandBuffer m_textureCmdBuffer; // a command buffer specifically to load textures

  struct
  {
    float4x4 projView;
    float4x4 model;
    uint colorNo;
  } pushConst2M;

  float4x4 m_worldViewProj;
  float4x4 m_lightMatrix;    

  UniformParams m_uniforms {};
  void* m_uboMappedMem = nullptr;

  etna::GraphicsPipeline m_shadowPipeline {};
  etna::GraphicsPipeline m_prepareGbufferPipeline {};
  etna::GraphicsPipeline m_ssaoPipeline {};
  etna::ComputePipeline  m_gaussianBlurPipeline {};
  etna::GraphicsPipeline m_resolveGbufferPipeline {};
  etna::GraphicsPipeline m_screenSpaceTransparencyPipeline {};
  etna::GraphicsPipeline m_resolveTransparencyPipeline {};

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  VulkanSwapChain m_swapchain;

  Camera   m_cam;
  uint32_t m_width  = 1024u;
  uint32_t m_height = 1024u;
  uint32_t m_framesInFlight = 2u;
  uint32_t m_gaussian_kernel_length = 23;
  std::vector<float> m_gaussian_kernel;
  uint m_gauss_window = 21;
  bool m_vsync = false;

  vk::PhysicalDeviceFeatures m_enabledDeviceFeatures = {};
  std::vector<const char*> m_deviceExtensions;
  std::vector<const char*> m_instanceExtensions;

  std::shared_ptr<SceneManager> m_pScnMgr;
  std::shared_ptr<IRenderGUI> m_pGUIRender;

  std::unique_ptr<TransparencyMeshes> transparencyMeshes;
  std::unique_ptr<TransparencyScene> transparencyScene;
  
  std::unique_ptr<QuadRenderer> m_pQuad;

  struct InputControlMouseEtc
  {
    bool drawFSQuad = false;
  } m_input;

  /**
  \brief basic parameters that you usually need for shadow mapping
  */
  struct ShadowMapCam
  {
    ShadowMapCam() 
    {  
      cam.pos    = float3(4.0f, 4.0f, 4.0f);
      cam.lookAt = float3(0, 0, 0);
      cam.up     = float3(0, 1, 0);
  
      radius          = 5.0f;
      lightTargetDist = 20.0f;
      usePerspectiveM = true;
    }

    float  radius;           ///!< ignored when usePerspectiveM == true 
    float  lightTargetDist;  ///!< identify depth range
    Camera cam;              ///!< user control for light to later get light worldViewProj matrix
    bool   usePerspectiveM;  ///!< use perspective matrix if true and ortographics otherwise
  
  } m_light;
 
  void DrawFrameSimple(bool draw_gui);

  void BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkImage a_targetImage, VkImageView a_targetImageView);

  void DrawSceneCmd(VkCommandBuffer a_cmdBuff, const float4x4& a_wvp, VkPipelineLayout a_pipelineLayout,
    VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT);

  void prepareTransparency(vk::CommandBuffer commandBuffer);
  void renderTransparency(vk::CommandBuffer commandBuffer, meshTypes objectType, uint32_t& startInstance, uint32_t instanceCount);

  void makeAssets();
  void loadShaders();
  void loadBackgroundTexture();
  void loadEnvironmentMap();

  void SetupSimplePipeline();
  void RecreateSwapChain();

  void UpdateUniformBuffer(float a_time);


  void SetupDeviceExtensions();

  void AllocateResources();
  void PreparePipelines();

  void DeallocateResources();

  void InitPresentStuff();
  void ResetPresentStuff();
  void SetupGUIElements();
};


#endif //CHIMERA_SIMPLE_RENDER_H
