#pragma once

#include <stdint.h>
#include <stdio.h>

#include "Instance.h"
#include "Material.h"
#include "Mesh.h"

namespace Scene
{
	static constexpr size_t kMaxVerts = 64;
	static constexpr size_t kMaxTris = 126;
	static constexpr uint32_t kAsGroupSize = 32;

	enum SceneError : uint32_t
	{
		SCENE_OK = 0,
		SCENE_LOAD_FAILED = 1024
	};

	class GLTFScene
	{
	public:
		GLTFScene() = default;

		using ProgressCallback = void (*)(float progress, const char *stage, void *userData);

		static SceneError MakeGLTFScene(const char *filename,
										GLTFScene &scene,
										ProgressCallback onProgress = nullptr,
										void *userData = nullptr);

		const Mesh &GetMesh(size_t index) const
		{
			return meshes[index];
		}
		const Material &GetMaterial(size_t index) const
		{
			return materials[index];
		}
		const Instance &GetInstance(size_t index) const
		{
			return nodes[index];
		}
		size_t GetMeshCount() const
		{
			return meshes.size();
		}
		size_t GetMaterialCount() const
		{
			return materials.size();
		}
		size_t GetInstanceCount() const
		{
			return nodes.size();
		}

		void Print() const
		{
			printf("Scene: %zu mesh(es)\n", meshes.size());
			for (size_t i = 0; i < meshes.size(); ++i)
			{
				const Mesh &mesh = meshes[i];
				printf("  [%zu] %s -- %zu primitive(s)\n",
					   i,
					   mesh.GetName().empty() ? "(unnamed)" : mesh.GetName().c_str(),
					   mesh.GetPrimitiveCount());

				const std::vector<Primitive> &prims = mesh.GetPrimitives();
				for (size_t p = 0; p < prims.size(); ++p)
				{
					const Primitive &prim = prims[p];
					printf("      [%zu] POSITION|NORMAL|TEXCOORD_0|TANGENT%s, %zu vert(s), %zu index(es), material %d\n",
						   p,
						   prim.HasIndices() ? "|INDICES" : "",
						   prim.GetVertexCount(),
						   prim.GetIndexCount(),
						   prim.GetMaterialIndex());
				}
			}

			printf("Materials: %zu\n", materials.size());
			for (size_t i = 0; i < materials.size(); ++i)
			{
				const Material &mat = materials[i];
				const float *baseColour = mat.GetBaseColour();
				printf("  [%zu] baseColour(%.2f, %.2f, %.2f) roughness=%.2f metal=%.2f "
					   "albedo=%s(%dx%d) roughMetal=%s(%dx%d)\n",
					   i,
					   baseColour[0],
					   baseColour[1],
					   baseColour[2],
					   mat.GetRoughness(),
					   mat.GetMetal(),
					   mat.GetAlbedoTexture().IsValid() ? "yes" : "no",
					   mat.GetAlbedoTexture().GetWidth(),
					   mat.GetAlbedoTexture().GetHeight(),
					   mat.GetRoughMetalTexture().IsValid() ? "yes" : "no",
					   mat.GetRoughMetalTexture().GetWidth(),
					   mat.GetRoughMetalTexture().GetHeight());
			}
		}

	private:
		std::vector<Mesh> meshes;
		std::vector<Material> materials;
		std::vector<Instance> nodes;
	};


}; // namespace Scene
