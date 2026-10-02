#include "Primitive.h"

#include <utility>

#include "meshoptimizer.h"
#include "tiny_gltf_v3.h"

#include "GLTFScene.h"

namespace Scene
{
	bool ReadAccessorAsFloat(const tg3_model &model,
							 int32_t accessorIndex,
							 int32_t expectedNumComponents,
							 std::vector<float> &out)
	{
		if (accessorIndex < 0 || (uint32_t) accessorIndex >= model.accessors_count)
			return false;

		const tg3_accessor &acc = model.accessors[accessorIndex];
		if (acc.buffer_view < 0 || (uint32_t) acc.buffer_view >= model.buffer_views_count)
			return false;

		const tg3_buffer_view &bv = model.buffer_views[acc.buffer_view];
		if (bv.buffer < 0 || (uint32_t) bv.buffer >= model.buffers_count)
			return false;

		const tg3_buffer &buf = model.buffers[bv.buffer];

		int32_t numComponents = tg3_num_components(acc.type);
		int32_t componentSize = tg3_component_size(acc.component_type);
		int32_t stride = tg3_accessor_byte_stride(&acc, &bv);

		if (numComponents != expectedNumComponents || componentSize < 0 || stride <= 0)
			return false;

		const uint8_t *base = buf.data.data + bv.byte_offset + acc.byte_offset;

		out = std::vector<float>((size_t) acc.count * numComponents);

		for (uint64_t i = 0; i < acc.count; ++i)
		{
			const uint8_t *elem = base + i * stride;
			for (int32_t c = 0; c < numComponents; ++c)
			{
				const uint8_t *compPtr = elem + c * componentSize;
				float value = 0.0f;

				switch (acc.component_type)
				{
					case TG3_COMPONENT_TYPE_FLOAT:
						value = *reinterpret_cast<const float *>(compPtr);
						break;
					case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
						value = *compPtr / 255.0f;
						break;
					case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
						value = *reinterpret_cast<const uint16_t *>(compPtr) / 65535.0f;
						break;
					default:
						return false;
				}

				out[(size_t) i * numComponents + c] = value;
			}
		}

		return true;
	}

	bool ReadAccessorAsIndices(const tg3_model &model,
							   int32_t accessorIndex,
							   std::vector<uint32_t> &out)
	{
		if (accessorIndex < 0 || (uint32_t) accessorIndex >= model.accessors_count)
			return false;

		const tg3_accessor &acc = model.accessors[accessorIndex];
		if (acc.buffer_view < 0 || (uint32_t) acc.buffer_view >= model.buffer_views_count)
			return false;

		const tg3_buffer_view &bv = model.buffer_views[acc.buffer_view];
		if (bv.buffer < 0 || (uint32_t) bv.buffer >= model.buffers_count)
			return false;

		const tg3_buffer &buf = model.buffers[bv.buffer];

		int32_t componentSize = tg3_component_size(acc.component_type);
		int32_t stride = tg3_accessor_byte_stride(&acc, &bv);
		if (componentSize < 0 || stride <= 0)
			return false;

		const uint8_t *base = buf.data.data + bv.byte_offset + acc.byte_offset;

		out = std::vector<uint32_t>((size_t) acc.count);

		for (uint64_t i = 0; i < acc.count; ++i)
		{
			const uint8_t *elem = base + i * stride;

			switch (acc.component_type)
			{
				case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
					out[i] = *elem;
					break;
				case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
					out[i] = *reinterpret_cast<const uint16_t *>(elem);
					break;
				case TG3_COMPONENT_TYPE_UNSIGNED_INT:
					out[i] = *reinterpret_cast<const uint32_t *>(elem);
					break;
				default:
					return false;
			}
		}

		return true;
	}

	static int32_t FindAttribute(const tg3_primitive &prim, const char *attrName)
	{
		for (uint32_t i = 0; i < prim.attributes_count; ++i)
		{
			if (tg3_str_equals_cstr(prim.attributes[i].key, attrName))
				return prim.attributes[i].value;
		}
		return -1;
	}

	Primitive::Primitive(const tg3_model &model, const tg3_primitive &prim) :
		materialIndex(prim.material)
	{
		// POSITION, NORMAL and TEXCOORD_0 are required; every primitive in
		// this pipeline is expected to carry them, so a missing one is a
		// load error rather than an optional stream. TANGENT is not read
		// from the source -- glTF's tangent data is frequently missing or
		// low quality, so it's always generated (see GenerateTangents).
		int32_t posAccessor = FindAttribute(prim, "POSITION");
		if (!ReadAccessorAsFloat(model, posAccessor, 3, positions))
		{
			fprintf(stderr, "Primitive missing/invalid POSITION attribute\n");
			return;
		}

		int32_t normalAccessor = FindAttribute(prim, "NORMAL");
		if (!ReadAccessorAsFloat(model, normalAccessor, 3, normals))
		{
			fprintf(stderr, "Primitive missing/invalid NORMAL attribute\n");
			return;
		}

		int32_t uvAccessor = FindAttribute(prim, "TEXCOORD_0");
		if (!ReadAccessorAsFloat(model, uvAccessor, 2, texCoord0))
		{
			fprintf(stderr, "Primitive missing/invalid TEXCOORD_0 attribute\n");
			return;
		}

		if (prim.indices >= 0)
			ReadAccessorAsIndices(model, prim.indices, indices);

		GenerateTangents();
		BuildMeshlets();
	}

	void Primitive::GenerateTangents()
	{
		if (indices.empty())
			return;

		size_t cornerCount = indices.size();

		// Deindex position/normal/uv to one entry per triangle corner --
		// meshopt_generateTangents' output is per-corner regardless of
		// whether its input is indexed, so building the remap input this
		// way lets each corner carry its own tangent independently.
		std::vector<float> cornerPositions(cornerCount * 3);
		std::vector<float> cornerNormals(cornerCount * 3);
		std::vector<float> cornerUVs(cornerCount * 2);

		for (size_t i = 0; i < cornerCount; ++i)
		{
			uint32_t v = indices[i];

			cornerPositions[i * 3 + 0] = positions[v * 3 + 0];
			cornerPositions[i * 3 + 1] = positions[v * 3 + 1];
			cornerPositions[i * 3 + 2] = positions[v * 3 + 2];

			cornerNormals[i * 3 + 0] = normals[v * 3 + 0];
			cornerNormals[i * 3 + 1] = normals[v * 3 + 1];
			cornerNormals[i * 3 + 2] = normals[v * 3 + 2];

			cornerUVs[i * 2 + 0] = texCoord0[v * 2 + 0];
			cornerUVs[i * 2 + 1] = texCoord0[v * 2 + 1];
		}

		std::vector<float> cornerTangents(cornerCount * 4);
		meshopt_generateTangents(cornerTangents.data(),
								 indices.data(),
								 cornerCount,
								 positions.data(),
								 GetVertexCount(),
								 3 * sizeof(float),
								 normals.data(),
								 3 * sizeof(float),
								 texCoord0.data(),
								 2 * sizeof(float),
								 0);

		// Re-weld: any corner whose position/normal/uv/tangent all match
		// another corner collapses back to one vertex; corners that only
		// match on position/normal/uv but disagree on tangent (UV seams)
		// become distinct vertices, and `indices` is rebuilt to reference
		// them correctly.
		meshopt_Stream streams[4] = {
				{ cornerPositions.data(), 3 * sizeof(float), 3 * sizeof(float) },
				{ cornerNormals.data(), 3 * sizeof(float), 3 * sizeof(float) },
				{ cornerUVs.data(), 2 * sizeof(float), 2 * sizeof(float) },
				{ cornerTangents.data(), 4 * sizeof(float), 4 * sizeof(float) },
		};

		std::vector<uint32_t> remap(cornerCount);
		size_t uniqueVertexCount =
				meshopt_generateVertexRemapMulti(remap.data(), nullptr, cornerCount, cornerCount, streams, 4);

		std::vector<float> newPositions(uniqueVertexCount * 3);
		std::vector<float> newNormals(uniqueVertexCount * 3);
		std::vector<float> newUVs(uniqueVertexCount * 2);
		std::vector<float> newTangents(uniqueVertexCount * 4);
		std::vector<uint32_t> newIndices(cornerCount);

		meshopt_remapVertexBuffer(newPositions.data(), cornerPositions.data(), cornerCount, 3 * sizeof(float), remap.data());
		meshopt_remapVertexBuffer(newNormals.data(), cornerNormals.data(), cornerCount, 3 * sizeof(float), remap.data());
		meshopt_remapVertexBuffer(newUVs.data(), cornerUVs.data(), cornerCount, 2 * sizeof(float), remap.data());
		meshopt_remapVertexBuffer(newTangents.data(), cornerTangents.data(), cornerCount, 4 * sizeof(float), remap.data());
		meshopt_remapIndexBuffer(newIndices.data(), nullptr, cornerCount, remap.data());

		positions = std::move(newPositions);
		normals = std::move(newNormals);
		texCoord0 = std::move(newUVs);
		tangents = std::move(newTangents);
		indices = std::move(newIndices);
	}

	void Primitive::BuildMeshlets()
	{
		size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), kMaxVerts, kMaxTris);

		std::vector<meshopt_Meshlet> rawMeshlets(maxMeshlets);
		std::vector<uint32_t> rawMeshletVertices(maxMeshlets * kMaxVerts);
		std::vector<uint8_t> rawMeshletTriangles(maxMeshlets * kMaxTris * 3);

		size_t meshletCount = meshopt_buildMeshlets(rawMeshlets.data(),
													rawMeshletVertices.data(),
													rawMeshletTriangles.data(),
													indices.data(),
													indices.size(),
													positions.data(),
													GetVertexCount(),
													3 * sizeof(float),
													kMaxVerts,
													kMaxTris,
													0.0f);

		const meshopt_Meshlet &last = rawMeshlets[meshletCount - 1];
		size_t usedMeshletVerts = last.vertex_offset + last.vertex_count;

		meshlets = std::vector<Meshlet>(meshletCount);
		meshletVertices = std::vector<uint32_t>(usedMeshletVerts);

		for (size_t i = 0; i < usedMeshletVerts; ++i)
			meshletVertices[i] = rawMeshletVertices[i];

		// Count total triangles across the meshlets we're keeping so the
		// packed-triangle buffer can be a single allocation.
		size_t totalTriangles = 0;
		for (size_t i = 0; i < meshletCount; ++i)
			totalTriangles += rawMeshlets[i].triangle_count;

		meshletTriangles = std::vector<uint32_t>(totalTriangles);

		size_t packedTriangleCursor = 0;
		for (size_t i = 0; i < meshletCount; ++i)
		{
			const meshopt_Meshlet &m = rawMeshlets[i];

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(&rawMeshletVertices[m.vertex_offset],
																 &rawMeshletTriangles[m.triangle_offset],
																 m.triangle_count,
																 positions.data(),
																 GetVertexCount(),
																 3 * sizeof(float));

			Meshlet &g = meshlets[i];
			g.vertexOffset = m.vertex_offset;
			g.triangleOffset = static_cast<uint32_t>(packedTriangleCursor);
			g.vertexCount = m.vertex_count;
			g.triangleCount = m.triangle_count;
			g.boundingSphere[0] = bounds.center[0];
			g.boundingSphere[1] = bounds.center[1];
			g.boundingSphere[2] = bounds.center[2];
			g.boundingSphere[3] = bounds.radius;

			for (uint32_t t = 0; t < m.triangle_count; ++t)
			{
				const uint8_t *tri = &rawMeshletTriangles[m.triangle_offset + t * 3];
				meshletTriangles[packedTriangleCursor++] =
						uint32_t(tri[0]) | (uint32_t(tri[1]) << 8) | (uint32_t(tri[2]) << 16);
			}
		}
	}
}; // namespace Scene
