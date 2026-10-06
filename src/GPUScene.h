#pragma once

#include <stdint.h>

#include <vector>

struct MeshGPUDesc
{
	uint32_t primitiveOffset;
	uint32_t primitiveCount;
};

struct PrimitiveGPUDesc
{
	uint32_t meshletOffset;
	uint32_t meshletVerticesOffset;
	uint32_t meshletTrianglesOffset;
	uint32_t meshletCount;
	uint32_t materialIndex;
	uint32_t vertexOffset;  // shared element offset into positions/texcoord0/normals/tangents (same offset, one per vertex)
	uint32_t vertexCount;
	uint32_t indexOffset;  // element offset into the shared index buffer
	uint32_t indexCount;
};

// One row per Instance, refreshed every frame it moves.
struct InstanceGPU
{
	float worldMatrix[12];
	uint32_t meshIndex;
};

// The scene-wide shared buffers every primitive's data is packed into.
enum class GPUStream : uint32_t
{
	Position,
	TexCoord0,
	Normal,
	Tangent,
	Index,
	Meshlet,
	MeshletVertices,
	MeshletTriangles,
	Count
};

namespace Scene
{
	class GLTFScene;

	class GPUScene
	{
	public:
		virtual ~GPUScene() = default;

		virtual void BuildGPUScene(const GLTFScene &scene) = 0;

		virtual void UpdateInstances(const GLTFScene &scene) = 0;

		size_t GetMeshDescCount() const
		{
			return meshDescs.size();
		}
		size_t GetPrimitiveDescCount() const
		{
			return primitiveDescs.size();
		}
		size_t GetInstanceCount() const
		{
			return instances.size();
		}

		const MeshGPUDesc &GetMeshDesc(size_t index) const
		{
			return meshDescs[index];
		}
		const PrimitiveGPUDesc &GetPrimitiveDesc(size_t index) const
		{
			return primitiveDescs[index];
		}
		const InstanceGPU &GetInstanceGPU(size_t index) const
		{
			return instances[index];
		}

		uint32_t GetStreamBufferIndex(GPUStream stream) const
		{
			return streamBufferIndices[(uint32_t) stream];
		}

	protected:
		std::vector<MeshGPUDesc> meshDescs;
		std::vector<PrimitiveGPUDesc> primitiveDescs;
		std::vector<InstanceGPU> instances;
		uint32_t streamBufferIndices[(uint32_t) GPUStream::Count] = {};
	};

}; // namespace Scene
