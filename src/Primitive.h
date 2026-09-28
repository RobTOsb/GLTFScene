#pragma once

#include "FixedVector.h"

struct tg3_model;
struct tg3_primitive;

namespace Scene
{
	bool ReadAccessorAsFloat(Allocator::Arena &arena,
							 const tg3_model &model,
							 int32_t accessorIndex,
							 int32_t expectedNumComponents,
							 DSA::FixedVector<float> &out);

	bool ReadAccessorAsIndices(Allocator::Arena &arena,
							   const tg3_model &model,
							   int32_t accessorIndex,
							   DSA::FixedVector<uint32_t> &out);

	struct Meshlet
	{
		uint32_t vertexOffset;
		uint32_t triangleOffset;
		uint32_t vertexCount;
		uint32_t triangleCount;
		float boundingSphere[4];
	};

	class Primitive
	{
	public:
		Primitive(Allocator::Arena &arena, const tg3_model &model, const tg3_primitive &prim);

		bool IsValid() const
		{
			return positions.size() > 0 && texCoord0.size() > 0 && normals.size() > 0 && tangents.size() > 0;
		}

		int32_t GetMaterialIndex() const
		{
			return materialIndex;
		}

		bool HasIndices() const
		{
			return !indices.empty();
		}

		size_t GetVertexCount() const
		{
			return positions.size() / 3;
		}

		size_t GetIndexCount() const
		{
			return indices.size();
		}

		const DSA::FixedVector<float> &GetPositions() const
		{
			return positions;
		}

		const DSA::FixedVector<float> &GetTexCoord0() const
		{
			return texCoord0;
		}

		const DSA::FixedVector<float> &GetNormals() const
		{
			return normals;
		}

		const DSA::FixedVector<float> &GetTangents() const
		{
			return tangents;
		}

		const DSA::FixedVector<uint32_t> &GetIndices() const
		{
			return indices;
		}

		const DSA::FixedVector<Meshlet> &GetMeshlets() const
		{
			return meshlets;
		}

		const DSA::FixedVector<uint32_t> &GetMeshletVertices() const
		{
			return meshletVertices;
		}

		const DSA::FixedVector<uint32_t> &GetMeshletTriangles() const
		{
			return meshletTriangles;
		}

		void BuildMeshlets(Allocator::Arena &arena);

	private:
		// Generates tangents with meshoptimizer, which produces one tangent
		// per triangle CORNER rather than per vertex -- the same vertex can
		// legitimately need different tangents on either side of a UV seam.
		// To keep a single indexed vertex stream, this deindexes
		// position/normal/uv/tangent to per-corner arrays and re-welds them
		// with meshopt_generateVertexRemapMulti, which splits any vertex
		// whose corners disagree into separate vertices and rebuilds
		// `indices` to match. Called before BuildMeshlets, since meshlet
		// generation needs the final vertex/index counts.
		void GenerateTangents(Allocator::Arena &arena);


		DSA::FixedVector<float> positions;
		DSA::FixedVector<float> texCoord0;
		DSA::FixedVector<float> normals;
		DSA::FixedVector<float> tangents;  // vec4: xyz + w handedness sign
		DSA::FixedVector<uint32_t> indices;

		DSA::FixedVector<Meshlet> meshlets;
		DSA::FixedVector<uint32_t> meshletVertices;
		DSA::FixedVector<uint32_t> meshletTriangles;

		int32_t materialIndex;
	};


}; // namespace Scene
