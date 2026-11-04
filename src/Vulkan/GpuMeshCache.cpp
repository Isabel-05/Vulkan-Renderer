#include "GpuMeshCache.h"
#include "BufferUtils.h"

#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <array>
#include <iostream>

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
		binding.stride = sizeof(GpuPointVertex);
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding;
	}

	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attrs{};
		attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GpuPointVertex, pos) };
		attrs[1] = { 1, 0, VK_FORMAT_R32_UINT, offsetof(GpuPointVertex, selected) };
		std::vector<VkVertexInputAttributeDescription> out(attrs.begin(), attrs.end());
		return out;
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

/////////////////
//GPU MESH CACHE

//GPU Mesh Cache is a per-object cache of GPU buffers for rendering.
//Its responsible for coverting DMesh data into Gpu friendly formats and uploading to GPU buffers.

void GpuMeshCache::init(VulkanContext& context, CommandPool& cmdPool, std::shared_ptr<DMesh> dmesh)
{
	dataMesh = dmesh;
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
	bool selectionChanged = synced.selection != dataMesh->version.selection;

	//The dedup key is (position, normal, uv) together, so any of the three
	//invalidates the whole surface batch, not just one attribute stream.
	if (topoChanged || posChanged || nrmChanged || uvChanged)
	{
		SurfaceExtract surf = extractSurface(*dataMesh);
		surfaceVertBuffer.uploadOrResize(context, cmdPool, surf.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		surfaceIdxBuffer.uploadOrResize(context, cmdPool, surf.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

	//first upload should include overlays
	//so its not stale when you switch to edit mode for the first time
	if (wantOverlay || synced == MeshVersion{ 0,0,0,0,0 })
	{
		//no dedup needed here. safe and cheap to reupload on every drag
		if (posChanged || topoChanged || selectionChanged)
		{
			std::vector<GpuPointVertex> points = extractPoints(*dataMesh);
			pointBuffer.uploadOrResize(context, cmdPool, points, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}

		//pure connectivity, only rebuilt when topology actually changes
		if (topoChanged)
		{
			auto edgeIdx = extractEdgeIndices(*dataMesh);
			edgeIdxBuffer.uploadOrResize(context, cmdPool, edgeIdx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		}
	}

	synced = dataMesh->version;
}

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

std::vector<CornerTri> GpuMeshCache::buildCornerTris(const DMesh& mesh)
{
	std::vector<CornerTri> tris;
	tris.reserve(mesh.cornerVerts.size());

	for (uint32_t f = 0; f < mesh.getFaceCount(); f++)
	{
		uint32_t start = mesh.faceOffsets[f];
		uint32_t n = mesh.getFaceSize(f);
		//fan triangulation from corner 0
		for (uint32_t i = 1; i + 1 < n; i++)
			tris.push_back({ start, start + i, start + i + 1, f });
	}
	return tris;
}

std::vector<glm::vec3> GpuMeshCache::buildCornerNormals(const DMesh& mesh, const std::vector<CornerTri>& tris)
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


SurfaceExtract GpuMeshCache::extractSurface(const DMesh& mesh)
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

std::vector<uint32_t> GpuMeshCache::extractEdgeIndices(const DMesh& mesh)
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

std::vector<GpuPointVertex> GpuMeshCache::extractPoints(const DMesh& mesh)
{
	std::vector<GpuPointVertex> out(mesh.positions.size());
	for (size_t i = 0; i < out.size(); i++)
	{
		out[i].pos = mesh.positions[i];
		out[i].selected = mesh.vertSelected[i];
	}
	return out;
}



