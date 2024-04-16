#include <memory>
#include <unordered_map>

#include <etna/VertexInput.hpp>
#include <glm/glm.hpp>
#include <vk_buffers.h>
#include <vk_copy.h>

#include "object.h"
#include "preprocessing_common.h"
#include "transparency_meshes.h"

TransparencyMeshes::TransparencyMeshes(VkDevice a_device, VkPhysicalDevice a_physDevice, uint32_t a_transferQId, uint32_t a_graphicsQId)
	: indexOffset(0)
	, m_device(a_device)
	, m_physDevice(a_physDevice)
	, m_transferQId(a_transferQId)
	, m_graphicsQId(a_graphicsQId)
{
	vkGetDeviceQueue(a_device, a_transferQId, 0, &m_transferQ);
	vkGetDeviceQueue(a_device, a_graphicsQId, 0, &m_graphicsQ);
	VkDeviceSize scratchMemSize = 64 * 1024 * 1024;
	m_pCopyHelper = std::make_shared<vk_utils::PingPongCopyHelper>(a_physDevice, a_device, m_transferQ, m_transferQId, scratchMemSize);
}

// Möller–Trumbore ray-triangle intersection algorithm:
static bool ray_intersects_triangle(const glm::vec3 &rayOrigin, const glm::dvec3 &rayVector, 
	const glm::vec3 &vertex0, const glm::vec3 &vertex1, const glm::vec3 &vertex2, double &distToIntersectionPoint)
{
	constexpr float EPSILON = 1.e-6f;
	glm::dvec3 edge1, edge2, rayVecXe2, s, sXe1;
	float det, invDet, u, v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	rayVecXe2 = glm::cross(rayVector, edge2);
	det = static_cast<float>(glm::dot(edge1, rayVecXe2));
	if (det > -EPSILON && det < EPSILON)
		return false; // This ray is parallel to this triangle.

	invDet = 1.f / det;
	s = rayOrigin - vertex0;
	u = invDet * static_cast<float>(glm::dot(s, rayVecXe2));
	if (u < 0.f || u > 1.f)
		return false;

	sXe1 = glm::cross(s, edge1);
	v = invDet * static_cast<float>(glm::dot(rayVector, sXe1));
	if (v < 0.f || u + v > 1.f)
		return false;

	// At this stage we can compute t to find out the distance to the intesection point:
	double t = double(invDet) * glm::dot(edge2, sXe1);
	if (t > EPSILON) // ray intersection
	{
		distToIntersectionPoint = t;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection:
		return false;
}
	
void TransparencyMeshes::consume(meshTypes type, std::vector<float>& vertexData, std::vector<uint32_t>& indexData)
{
	int indexCount = static_cast<int>(indexData.size());
	int vertexCount = static_cast<int>(vertexData.size() / SINGLE_VERTEX_FLOAT_NUM);
	int lastIndex = static_cast<int>(indexLump.size());

	firstIndices.insert(std::make_pair(type, lastIndex));
	indexCounts.insert(std::make_pair(type, indexCount));

	static std::vector<glm::dvec3> hammersleySequence = construct_hemisphere_hammersley_sequence(500);
	for (int vertexNo = 0; vertexNo < vertexCount; vertexNo++)
	{
		glm::vec3 vertexPos = {vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_POSITION_START + 0],
													 vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_POSITION_START + 1],
													 vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_POSITION_START + 2]};
		glm::vec3 inVertexNormal = {-vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_NORMAL_START + 0],
																-vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_NORMAL_START + 1],
																-vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + VERTEX_NORMAL_START + 2]};

		// Constructing right-handed orthonormal basis
		static constexpr glm::vec3 UP = glm::vec3(0.f, 1.f, 0.f);
		glm::vec3 x_axis = (abs(glm::dot(UP, inVertexNormal)) == 1.f) ? glm::vec3(1.f, 0.f, 0.f) : glm::normalize(glm::cross(UP, inVertexNormal));
		glm::vec3 y_axis = glm::normalize(cross(inVertexNormal, x_axis));
		glm::mat3 transform = glm::mat3(x_axis, y_axis, inVertexNormal);

		std::function<DataToEncode(glm::dvec3)> getDataToEncode = [&vertexCount, &vertexData, &vertexPos, &vertexNo, &transform, &indexCount, &indexData](glm::dvec3 direction)
		{
			double width = 0;
			double maxWidth = 0;
			glm::vec3 refractedDirection = {0.f, 0.f, 0.f};
			for (int triangleIndexNo = 0; triangleIndexNo + 2 < indexCount; triangleIndexNo += 3)
			{
				glm::vec3 triangleVertex0 = {
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_POSITION_START + 0], // x
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_POSITION_START + 1],  // y
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_POSITION_START + 2]}; // z

				glm::vec3 triangleVertex1 = {
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_POSITION_START + 0],
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_POSITION_START + 1],
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_POSITION_START + 2]};

				glm::vec3 triangleVertex2 = {
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_POSITION_START + 0],
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_POSITION_START + 1],
					vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_POSITION_START + 2]};

				// Here we go from vertex reference frame to object reference frame
				glm::vec3 globalDirection = transform * direction;
				if (ray_intersects_triangle(vertexPos, globalDirection,
						triangleVertex0, triangleVertex1, triangleVertex2, width)) [[unlikely]]
					if (width > maxWidth)
					{
						maxWidth = width;

						glm::vec3 triangleNormal0 = {
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_NORMAL_START + 0],  // x
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_NORMAL_START + 1],  // y
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 0] + VERTEX_NORMAL_START + 2]}; // z

						glm::vec3 triangleNormal1 = {
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_NORMAL_START + 0],
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_NORMAL_START + 1],
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 1] + VERTEX_NORMAL_START + 2]};

						glm::vec3 triangleNormal2 = {
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_NORMAL_START + 0],
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_NORMAL_START + 1],
							vertexData[SINGLE_VERTEX_FLOAT_NUM * indexData[triangleIndexNo + 2] + VERTEX_NORMAL_START + 2]};

						glm::vec3 triangleNormalAvg = glm::normalize((triangleNormal0 + triangleNormal1 + triangleNormal2) / 3.f);

						// Normal is directed inward, eta = IOR of glass since we go from glass to air
						refractedDirection = glm::refract(globalDirection, -triangleNormalAvg, IOR);
						if (glm::dot(refractedDirection, refractedDirection) > FLT_EPSILON)
							refractedDirection = glm::normalize(refractedDirection);
						else
							refractedDirection = glm::vec3(0.f);
					}
			}
			return DataToEncode(static_cast<float>(maxWidth), refractedDirection.x, refractedDirection.y, refractedDirection.z);
		};
		std::vector<float> sphCoeffs = calculate_sh_terms(hammersleySequence, getDataToEncode);

		for (int i = 0; i < SH_COEEFS_NUM * SH_ENCODED_VALUES; i++)
			vertexData[SINGLE_VERTEX_FLOAT_NUM * vertexNo + SH_COEFFS_START + i] = sphCoeffs[i];

		if (vertexNo % 100 == 0)
			std::cout << "Vertex: " << vertexNo << "/" << vertexCount << '\n';
	}

	for (float attribute : vertexData)
		vertexLump.push_back(attribute);

	for (uint32_t index : indexData)
		indexLump.push_back(index + indexOffset);

	indexOffset += vertexCount;
}

void TransparencyMeshes::finalize()
{
	VkDeviceSize vertexBufSize = sizeof(float) * vertexLump.size();
  VkDeviceSize indexBufSize  = sizeof(uint32_t) * indexLump.size();

  m_geoVertBuf  = vk_utils::createBuffer(m_device, vertexBufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  m_geoIdxBuf   = vk_utils::createBuffer(m_device, indexBufSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  VkMemoryAllocateFlags allocFlags {};
  m_geoMemAlloc = vk_utils::allocateAndBindWithPadding(m_device, m_physDevice, {m_geoVertBuf, m_geoIdxBuf}, allocFlags);

  m_pCopyHelper->UpdateBuffer(m_geoVertBuf, 0, vertexLump.data(), vertexBufSize);
  m_pCopyHelper->UpdateBuffer(m_geoIdxBuf,  0, indexLump.data(), indexBufSize);

	vertexLump.clear();
	indexLump.clear();
}

TransparencyMeshes::~TransparencyMeshes()
{
  if(m_geoVertBuf != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_geoVertBuf, nullptr);
    m_geoVertBuf = VK_NULL_HANDLE;
  }

  if(m_geoIdxBuf != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_geoIdxBuf, nullptr);
    m_geoIdxBuf = VK_NULL_HANDLE;
  }

  if(m_geoMemAlloc != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_geoMemAlloc, nullptr);
    m_geoMemAlloc = VK_NULL_HANDLE;
  }
}

etna::VertexByteStreamFormatDescription TransparencyMeshes::getTransparencyVertexAttributeDescriptions()
{
	std::vector<vk::Format> shFormats = {
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32B32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
		vk::Format::eR32G32B32A32Sfloat,
	};
	std::vector<uint32_t> shFutureOffsets = { 3, 3, 3, 4, 4, 4, 4, };

  etna::VertexByteStreamFormatDescription result;
  result.stride = SINGLE_VERTEX_FLOAT_NUM * sizeof(float);
  result.attributes.reserve(30);

	// Position
	result.attributes.push_back(
		etna::VertexByteStreamFormatDescription::Attribute
		{
			.format = vk::Format::eR32G32B32Sfloat,
			.offset = VERTEX_POSITION_START * sizeof(float)
		});

	// Normal
	result.attributes.push_back(
		etna::VertexByteStreamFormatDescription::Attribute
		{
			.format = vk::Format::eR32G32B32Sfloat,
			.offset = VERTEX_NORMAL_START * sizeof(float)
		});

	// Spherical harmonics expansion coefficients

	uint32_t shOffset = SH_COEFFS_START;

	// Width
	for (int i = 0; i < shFormats.size(); i++)
	{
		result.attributes.push_back(
			etna::VertexByteStreamFormatDescription::Attribute
			{
				.format = shFormats[i],
				.offset = shOffset * (uint32_t)(sizeof(float))
			});
		shOffset += shFutureOffsets[i];
	}

	// X-coordinate of a refracted vector
	for (int i = 0; i < shFormats.size(); i++)
	{
		result.attributes.push_back(
			etna::VertexByteStreamFormatDescription::Attribute
			{
				.format = shFormats[i],
				.offset = shOffset * (uint32_t)(sizeof(float))
			});
		shOffset += shFutureOffsets[i];
	}

	// Y-coordinate of a refracted vector
	for (int i = 0; i < shFormats.size(); i++)
	{
		result.attributes.push_back(
			etna::VertexByteStreamFormatDescription::Attribute
			{
				.format = shFormats[i],
				.offset = shOffset * (uint32_t)(sizeof(float))
			});
		shOffset += shFutureOffsets[i];
	}

	// Z-coordinate of a refracted vector
	for (int i = 0; i < shFormats.size(); i++)
	{
		result.attributes.push_back(
			etna::VertexByteStreamFormatDescription::Attribute
			{
				.format = shFormats[i],
				.offset = shOffset * (uint32_t)(sizeof(float))
			});
		shOffset += shFutureOffsets[i];
	}

  return result;
}

