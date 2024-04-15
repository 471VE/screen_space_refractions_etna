#pragma once

#include <etna/Buffer.hpp>
#include <vk_utils.h>

#include "transparency_scene.h"

class TransparencyMeshes {
	public:
		TransparencyMeshes(VkDevice a_device, VkPhysicalDevice a_physDevice, uint32_t a_transferQId, uint32_t a_graphicsQId);
		~TransparencyMeshes();
		void consume(meshTypes type, std::vector<float>& vertexData, std::vector<uint32_t>& indexData);
		void finalize();

		std::unordered_map<meshTypes, int> firstIndices;
		std::unordered_map<meshTypes, int> indexCounts;

		VkBuffer GetVertexBuffer() const { return m_geoVertBuf; }
  	VkBuffer GetIndexBuffer()  const { return m_geoIdxBuf; }

		etna::VertexByteStreamFormatDescription getTransparencyVertexAttributeDescriptions();
		
	private:
		int indexOffset;
		std::vector<float> vertexLump;
		std::vector<uint32_t> indexLump;

		VkDeviceMemory m_geoMemAlloc = VK_NULL_HANDLE;
		VkBuffer m_geoVertBuf = VK_NULL_HANDLE;
  	VkBuffer m_geoIdxBuf  = VK_NULL_HANDLE;

		VkDevice m_device = VK_NULL_HANDLE;
		VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
		uint32_t m_transferQId = UINT32_MAX;
		VkQueue m_transferQ = VK_NULL_HANDLE;

		uint32_t m_graphicsQId = UINT32_MAX;
		VkQueue m_graphicsQ = VK_NULL_HANDLE;
		std::shared_ptr<vk_utils::ICopyEngine> m_pCopyHelper;
};