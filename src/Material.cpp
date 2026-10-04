#include "Material.h"

#include "tiny_gltf_v3.h"

namespace Scene
{
	namespace
	{
		// Reads a number from an extension's JSON object, falling back when it is absent.
		double ExtensionNumber(const tg3_material &material, const char *extension, const char *key, double fallback)
		{
			for (uint32_t i = 0; i < material.ext.extensions_count; ++i)
			{
				const tg3_extension &ext = material.ext.extensions[i];
				if (!tg3_str_equals_cstr(ext.name, extension) || ext.value.type != TG3_VALUE_OBJECT)
					continue;

				for (uint32_t j = 0; j < ext.value.object_count; ++j)
				{
					const tg3_kv_pair &pair = ext.value.object_data[j];
					if (!tg3_str_equals_cstr(pair.key, key))
						continue;
					if (pair.value.type == TG3_VALUE_REAL)
						return pair.value.real_val;
					if (pair.value.type == TG3_VALUE_INT)
						return (double) pair.value.int_val;
				}
			}
			return fallback;
		}
	} // namespace

	Material::Material() : materialFactorsSet(false)
	{
		baseColour[0] = baseColour[1] = baseColour[2] = baseColour[3] = 1.0f;
		alphaMode = AlphaMode::Opaque;
		alphaCutoff = 0.5f;
		transmission = 0.0f;
		roughness = 1.0f;
		metal = 1.0f;
		normalScale = 1.0f;
	}

	Material::Material(const tg3_model &model,
					   const char *gltfDir,
					   const tg3_material &srcMaterial) : materialFactorsSet(true)
	{
		const tg3_pbr_metallic_roughness &pbr = srcMaterial.pbr_metallic_roughness;

		baseColour[0] = (float) pbr.base_color_factor[0];
		baseColour[1] = (float) pbr.base_color_factor[1];
		baseColour[2] = (float) pbr.base_color_factor[2];
		baseColour[3] = (float) pbr.base_color_factor[3];

		alphaMode = AlphaMode::Opaque;
		if (srcMaterial.alpha_mode.data && tg3_str_equals_cstr(srcMaterial.alpha_mode, "MASK"))
			alphaMode = AlphaMode::Mask;
		else if (srcMaterial.alpha_mode.data && tg3_str_equals_cstr(srcMaterial.alpha_mode, "BLEND"))
			alphaMode = AlphaMode::Blend;
		alphaCutoff = (float) srcMaterial.alpha_cutoff;
		transmission = (float) ExtensionNumber(srcMaterial, "KHR_materials_transmission", "transmissionFactor", 0.0);
		roughness = (float) pbr.roughness_factor;
		metal = (float) pbr.metallic_factor;
		normalScale = (float) srcMaterial.normal_texture.scale;

		if (pbr.base_color_texture.index >= 0)
		{
			int32_t imageIndex = ResolveTextureImage(model, pbr.base_color_texture.index);
			LoadTexture(model, gltfDir, imageIndex, albedoTexture);
		}

		if (pbr.metallic_roughness_texture.index >= 0)
		{
			int32_t imageIndex = ResolveTextureImage(model, pbr.metallic_roughness_texture.index);
			LoadTexture(model, gltfDir, imageIndex, roughMetalTexture);
		}

		if (srcMaterial.normal_texture.index >= 0)
		{
			int32_t imageIndex = ResolveTextureImage(model, srcMaterial.normal_texture.index);
			LoadTexture(model, gltfDir, imageIndex, normalTexture);
		}
	}

	int32_t Material::ResolveTextureImage(const tg3_model &model, int32_t textureIndex)
	{
		if (textureIndex < 0 || (uint32_t) textureIndex >= model.textures_count)
			return -1;
		return model.textures[textureIndex].source;
	}

}; // namespace Scene
