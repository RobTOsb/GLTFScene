#pragma once

#include <stdint.h>
#include <vector>

struct tg3_model;
struct tg3_primitive;

namespace Scene
{
	bool ReadAccessorAsFloat(const tg3_model &model,
							 int32_t accessorIndex,
							 int32_t expectedNumComponents,
							 std::vector<float> &out);

	bool ReadAccessorAsIndices(const tg3_model &model,
							   int32_t accessorIndex,
							   std::vector<uint32_t> &out);

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
		Primitive(const tg3_model &model, const tg3_primitive &prim);

		bool IsValid() const
		{
			return positions.size() > 0 && texCoord0.size() > 0 && normals.size() > 0 && tangents.size() > 0;
		}

		// The material's UV set came from the file; otherwise every vertex has UV (0, 0).
		bool HasTexCoord0() const
		{
			return hasTexCoord0;
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

		const std::vector<float> &GetPositions() const
		{
			return positions;
		}

		// The UV set the material's textures use (TEXCOORD_0 unless KHR_texture_transform or the
		// textureInfo picks another), with its KHR_texture_transform applied (see UVTransform).
		const std::vector<float> &GetTexCoord0() const
		{
			return texCoord0;
		}

		const std::vector<float> &GetNormals() const
		{
			return normals;
		}

		const std::vector<float> &GetTangents() const
		{
			return tangents;
		}

		const std::vector<uint32_t> &GetIndices() const
		{
			return indices;
		}

		const std::vector<Meshlet> &GetMeshlets() const
		{
			return meshlets;
		}

		const std::vector<uint32_t> &GetMeshletVertices() const
		{
			return meshletVertices;
		}

		const std::vector<uint32_t> &GetMeshletTriangles() const
		{
			return meshletTriangles;
		}

		void BuildMeshlets();

	private:
		void GenerateTangents();
		void GenerateTangentsFromNormals();

		std::vector<float> positions;
		std::vector<float> texCoord0;
		std::vector<float> normals;
		std::vector<float> tangents;  // vec4: xyz + w handedness sign
		std::vector<uint32_t> indices;

		std::vector<Meshlet> meshlets;
		std::vector<uint32_t> meshletVertices;
		std::vector<uint32_t> meshletTriangles;

		int32_t materialIndex;
		bool hasTexCoord0 = false;
	};


}; // namespace Scene
