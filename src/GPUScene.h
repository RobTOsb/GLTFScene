#pragma once

#include <stdint.h>

#include "FixedVector.h"

// One row per Mesh, describing where its primitives live in the PrimitiveGPUDesc table.
struct MeshGPUDesc
{
	uint32_t primitiveOffset;
	uint32_t primitiveCount;
};

// One row per Primitive, describing where its data lives in the scene's
// shared global buffers (see GPUScene::GetStreamBufferIndex). Every
// *Offset/*Count field here is an ELEMENT offset/count into the
// corresponding shared buffer -- not a byte offset, and not a separate
// per-primitive descriptor. All primitives in a GPUScene share the same
// small set of buffers/descriptors; only the offsets differ.
//
// Position, TexCoord0, Normal and Tangent are all required attributes (see
// Primitive::IsValid) and therefore share one offset/count: every vertex
// stream for a primitive is the same length and starts at the same index
// in its respective shared buffer, so there's nothing to flag as present
// or absent.
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
// Unlike a per-primitive-descriptor layout, there is exactly one of each of
// these per GPUScene, regardless of how many primitives/meshes it holds --
// PrimitiveGPUDesc offsets pick out each primitive's slice.
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

	// RHI-agnostic entry point for turning a loaded GLTFScene into whatever
	// GPU-resident form a given renderer backend needs. Callers only see
	// this interface; how (and whether) an implementation actually talks to
	// a device is entirely up to it.
	//
	// The MeshGPUDesc/PrimitiveGPUDesc/InstanceGPU tables live here rather
	// than in each backend, since every backend needs the same CPU-side
	// bookkeeping (which mesh owns which primitives, which primitive owns
	// which meshlets, per-instance transforms) regardless of how it turns
	// that into actual GPU resources/descriptors.
	class GPUScene
	{
	public:
		virtual ~GPUScene() = default;

		// Builds (or rebuilds) all GPU-resident resources for `scene`.
		// Implementations own the lifetime of whatever they allocate, and
		// are expected to populate meshDescs/primitiveDescs/instances as
		// part of doing so.
		virtual void BuildGPUScene(const GLTFScene &scene) = 0;

		// Pushes updated per-instance transforms for the current frame.
		// Cheap relative to BuildGPUScene: no new resources, no descriptor
		// allocation, just refreshing InstanceGPU data that already has a
		// home.
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

		// The bindless descriptor index for one of the scene's shared
		// global buffers (see GPUStream). A mesh/task shader combines this
		// with a PrimitiveGPUDesc offset to reach a specific primitive's
		// data: e.g. ResourceDescriptorHeap[GetStreamBufferIndex(Position)]
		// indexed at desc.vertexOffset + localVertexIndex.
		uint32_t GetStreamBufferIndex(GPUStream stream) const
		{
			return streamBufferIndices[(uint32_t) stream];
		}

	protected:
		DSA::FixedVector<MeshGPUDesc> meshDescs;
		DSA::FixedVector<PrimitiveGPUDesc> primitiveDescs;
		DSA::FixedVector<InstanceGPU> instances;
		uint32_t streamBufferIndices[(uint32_t) GPUStream::Count] = {};
	};

}; // namespace Scene
