#include "GpuMeshCache.h"
#include "BufferUtils.h"

#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <array>

namespace SurfaceVertexAttribs
{
	VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.stride = sizeof(GpuSurfaceVertex);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding;
	}

	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attrs{};
		attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuSurfaceVertex, pos) };
		attrs[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuSurfaceVertex, normal) };
		attrs[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(GpuSurfaceVertex, uv) };
		std::vector<VkVertexInputAttributeDescription> out(attrs.begin(), attrs.end());
		return out;
	}
}

namespace PointVertexAttribs
{
	VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription binding{};
		binding.binding = 0;
		binding.stride = sizeof(glm::vec3);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding;
	}

	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 1> attrs{};
		attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
		std::vector<VkVertexInputAttributeDescription> out(attrs.begin(), attrs.end());
		return out;
	}
}

namespace
{
	//I kept these functions outside of GpuMeshCache class in case i need an intermediate cache for different listeners
	//aka so theyre reusable 

	struct CornerTri { uint32_t c0, c1, c2, face; };

	struct SurfaceExtract
	{
		std::vector<GpuSurfaceVertex> vertices;	 //deduplicated by (position, normal, uv)
		std::vector<uint32_t> indices;	//3 per triangle
	};

	struct CornerKey
	{
		uint32_t vert;
		glm::vec3 normal;
		glm::vec2 uv;
		bool operator==(const CornerKey& o) const
		{
			return vert == o.vert && normal == o.normal && uv == o.uv;
		}
	};
	struct CornerKeyHash
	{
		size_t operator()(const CornerKey& k) const
		{
			size_t h = std::hash<uint32_t>()(k.vert);
			auto mix = [&](float f) { h ^= std::hash<float>()(f) + 0x9e3779b9 + (h << 6) + (h >> 2); };
			mix(k.normal.x); mix(k.normal.y); mix(k.normal.z);
			mix(k.uv.x); mix(k.uv.y);
			return h;
		}
	};

	///////////////////////
	//Cached Data Builders

	std::vector<CornerTri> buildCornerTris(const DMesh& mesh)
	{
		std::vector<CornerTri> tris;
		tris.reserve(mesh.cornerVerts.size());

		for (uint32_t f = 0; f < mesh.getFaceCount(); f++)
		{
			uint32_t start = mesh.faceOffsets[f];
			uint32_t n = mesh.getFaceSize(f);
			//fan triangulation from corner 0 -- fine for the convex faces we build (tris/quads)
			for (uint32_t i = 1; i + 1 < n; i++)
				tris.push_back({ start, start + i, start + i + 1, f });
		}
		return tris;
	}

	std::vector<glm::vec3> buildCornerNormals(const DMesh& mesh, const std::vector<CornerTri>& tris)
	{
		std::vector<glm::vec3> faceNormal(mesh.getFaceCount(), glm::vec3(0.0f));
		for (const auto& t : tris)
		{
			glm::vec3 a = mesh.positions[mesh.cornerVerts[t.c0]];
			glm::vec3 b = mesh.positions[mesh.cornerVerts[t.c1]];
			glm::vec3 c = mesh.positions[mesh.cornerVerts[t.c2]];
			faceNormal[t.face] += glm::cross(b - a, c - a); //accumulate, normalize once
		}
		for (auto& n : faceNormal)
			if (glm::length(n) > 1e-8f) n = glm::normalize(n);

		//smooth vertex normals, only used where the owning face isn't flat
		std::vector<glm::vec3> vertAccum(mesh.positions.size(), glm::vec3(0.0f));
		for (uint32_t f = 0; f < mesh.getFaceCount(); f++)
		{
			if (mesh.sharpFaces[f]) continue;
			for (uint32_t c = mesh.faceOffsets[f]; c < mesh.faceOffsets[f + 1]; c++)
				vertAccum[mesh.cornerVerts[c]] += faceNormal[f];
		}
		for (auto& v : vertAccum)
			if (glm::length(v) > 1e-8f) v = glm::normalize(v);

		std::vector<glm::vec3> cornerNormal(mesh.cornerVerts.size());
		for (uint32_t f = 0; f < mesh.getFaceCount(); f++)
			for (uint32_t c = mesh.faceOffsets[f]; c < mesh.faceOffsets[f + 1]; c++)
				cornerNormal[c] = mesh.sharpFaces[f] ? faceNormal[f] : vertAccum[mesh.cornerVerts[c]];

		return cornerNormal;
	}

	///////////////
	// Extractors

	SurfaceExtract extractSurface(const DMesh& mesh)
	{
		std::vector<CornerTri> tris = buildCornerTris(mesh);
		std::vector<glm::vec3> cornerNormal = buildCornerNormals(mesh, tris);

		SurfaceExtract out;
		std::unordered_map<CornerKey, uint32_t, CornerKeyHash> unique;
		unique.reserve(mesh.cornerVerts.size());

		auto emit = [&](uint32_t c) -> uint32_t
			{
				CornerKey key{ mesh.cornerVerts[c], cornerNormal[c], mesh.cornerUv[c] };
				auto it = unique.find(key);
				if (it != unique.end()) return it->second;

				uint32_t idx = (uint32_t)out.vertices.size();
				out.vertices.push_back({ mesh.positions[key.vert], key.normal, key.uv });
				unique.emplace(key, idx);
				return idx;
			};

		out.indices.reserve(tris.size() * 3);
		for (const auto& t : tris)
		{
			out.indices.push_back(emit(t.c0));
			out.indices.push_back(emit(t.c1));
			out.indices.push_back(emit(t.c2));
		}
		return out;
	}

	std::vector<uint32_t> extractEdgeIndices(const DMesh& mesh)
	{
		std::vector<uint32_t> indices;
		indices.reserve(mesh.edges.size() * 2);
		for (const auto& e : mesh.edges)
		{
			indices.push_back(e.x);
			indices.push_back(e.y);
		}
		return indices;
	}
}

void GpuBuffer::cleanup(VulkanContext& context)
{
	if (buffer) vkDestroyBuffer(context.logicalDevice, buffer, nullptr);
	if (memory) vkFreeMemory(context.logicalDevice, memory, nullptr);
	buffer = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
	capacity = 0;
	count = 0;
}


void GpuMeshCache::init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh)
{
	dataMesh = std::make_shared<DMesh>(dmesh);
}

void GpuMeshCache::cleanup(VulkanContext& context)
{
	surfaceVertBuffer.cleanup(context);
	surfaceIdxBuffer.cleanup(context);
	pointBuffer.cleanup(context);
	edgeIdxBuffer.cleanup(context);
}

void GpuMeshCache::sync(VulkanContext& context, CommandPool& cmdPool, bool wantOverlay)
{
	bool topoChanged = synced.topology != dataMesh->version.topology;
	bool posChanged = synced.positions != dataMesh->version.positions;
	bool nrmChanged = synced.normals != dataMesh->version.normals;
	bool uvChanged = synced.uvs != dataMesh->version.uvs;

	//The dedup key is (position, normal, uv) together, so any of the three
	//invalidates the whole surface batch, not just one attribute stream.
	if (topoChanged || posChanged || nrmChanged || uvChanged)
	{
		SurfaceExtract surf = extractSurface(*dataMesh);
		surfaceVertBuffer.uploadOrResize(context, cmdPool, surf.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		surfaceIdxBuffer.uploadOrResize(context, cmdPool, surf.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	if (wantOverlay)
	{
		//no dedup needed here. safe and cheap to reupload on every drag
		if (posChanged || topoChanged)
			pointBuffer.uploadOrResize(context, cmdPool, dataMesh->positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

		//pure connectivity, only rebuilt when topology actually changes
		if (topoChanged)
		{
			auto edgeIdx = extractEdgeIndices(*dataMesh);
			edgeIdxBuffer.uploadOrResize(context, cmdPool, edgeIdx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		}
	}

	synced = dataMesh->version;
}

