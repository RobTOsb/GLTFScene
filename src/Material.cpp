#include "Material.h"

#include "tiny_gltf_v3.h"

namespace Scene
{
	Material::Material() : materialFactorsSet(false)
	{
		baseColour[0] = baseColour[1] = baseColour[2] = 1.0f;
		roughness = 1.0f;
		metal = 1.0f;
	}

	Material::Material(Allocator::Arena &arena,
					   const tg3_model &model,
					   const char *gltfDir,
					   const tg3_material &srcMaterial) : materialFactorsSet(true)
	{
		const tg3_pbr_metallic_roughness &pbr = srcMaterial.pbr_metallic_roughness;

		baseColour[0] = (float) pbr.base_color_factor[0];
		baseColour[1] = (float) pbr.base_color_factor[1];
		baseColour[2] = (float) pbr.base_color_factor[2];
		roughness = (float) pbr.roughness_factor;
		metal = (float) pbr.metallic_factor;

		if (pbr.base_color_texture.index >= 0)
		{
			int32_t imageIndex = ResolveTextureImage(model, pbr.base_color_texture.index);
			LoadTexture(arena, model, gltfDir, imageIndex, albedoTexture);
		}

		if (pbr.metallic_roughness_texture.index >= 0)
		{
			int32_t imageIndex = ResolveTextureImage(model, pbr.metallic_roughness_texture.index);
			LoadTexture(arena, model, gltfDir, imageIndex, roughMetalTexture);
		}
	}

	int32_t Material::ResolveTextureImage(const tg3_model &model, int32_t textureIndex)
	{
		if (textureIndex < 0 || (uint32_t) textureIndex >= model.textures_count)
			return -1;
		return model.textures[textureIndex].source;
	}

}; // namespace Scene
