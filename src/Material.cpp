#include "Material.h"

#include "tiny_gltf_v3.h"

namespace Scene
{
	namespace
	{
		// The member key of a JSON object, or null when it is absent.
		const tg3_value *ObjectMember(const tg3_value &object, const char *key)
		{
			if (object.type != TG3_VALUE_OBJECT)
				return nullptr;
			for (uint32_t j = 0; j < object.object_count; ++j)
			{
				if (tg3_str_equals_cstr(object.object_data[j].key, key))
					return &object.object_data[j].value;
			}
			return nullptr;
		}

		// The member key of an extension's JSON object, or null when either is absent.
		const tg3_value *ExtensionValue(const tg3_material &material, const char *extension, const char *key)
		{
			for (uint32_t i = 0; i < material.ext.extensions_count; ++i)
			{
				const tg3_extension &ext = material.ext.extensions[i];
				if (tg3_str_equals_cstr(ext.name, extension))
					return ObjectMember(ext.value, key);
			}
			return nullptr;
		}

		bool ToNumber(const tg3_value *value, double &out)
		{
			if (value && value->type == TG3_VALUE_REAL)
				out = value->real_val;
			else if (value && value->type == TG3_VALUE_INT)
				out = (double) value->int_val;
			else
				return false;
			return true;
		}

		// Reads a number from an extension's JSON object, falling back when it is absent.
		double ExtensionNumber(const tg3_material &material, const char *extension, const char *key, double fallback)
		{
			double value;
			return ToNumber(ExtensionValue(material, extension, key), value) ? value : fallback;
		}

		// The texture index of an extension's textureInfo member, -1 when absent.
		int32_t ExtensionTexture(const tg3_material &material, const char *extension, const char *key)
		{
			const tg3_value *info = ExtensionValue(material, extension, key);
			double index;
			return info && ToNumber(ObjectMember(*info, "index"), index) ? (int32_t) index : -1;
		}
	} // namespace

	Material::Material() : materialFactorsSet(false)
	{
		baseColour[0] = baseColour[1] = baseColour[2] = baseColour[3] = 1.0f;
		alphaMode = AlphaMode::Opaque;
		alphaCutoff = 0.5f;
		transmission = 0.0f;
		ior = 1.5f;
		thinWalled = false;
		roughness = 1.0f;
		metal = 1.0f;
		normalScale = 1.0f;
		emissive[0] = emissive[1] = emissive[2] = 0.0f;
		emissiveStrength = 1.0f;
	}

	Material::Material(const tg3_model &model,
					   const char *gltfDir,
					   const tg3_material &srcMaterial,
					   std::vector<Texture> &images) : materialFactorsSet(true)
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
		ior = (float) ExtensionNumber(srcMaterial, "KHR_materials_ior", "ior", 1.5);
		thinWalled = false;
		roughness = (float) pbr.roughness_factor;
		metal = (float) pbr.metallic_factor;
		normalScale = (float) srcMaterial.normal_texture.scale;
		emissive[0] = (float) srcMaterial.emissive_factor[0];
		emissive[1] = (float) srcMaterial.emissive_factor[1];
		emissive[2] = (float) srcMaterial.emissive_factor[2];
		emissiveStrength = (float) ExtensionNumber(srcMaterial, "KHR_materials_emissive_strength", "emissiveStrength", 1.0);

		if (pbr.base_color_texture.index >= 0)
		{
			albedoTexture = LoadSharedTexture(model, gltfDir, pbr.base_color_texture.index, images);
		}

		if (pbr.metallic_roughness_texture.index >= 0)
		{
			roughMetalTexture = LoadSharedTexture(model, gltfDir, pbr.metallic_roughness_texture.index, images);
		}

		if (srcMaterial.normal_texture.index >= 0)
		{
			normalTexture = LoadSharedTexture(model, gltfDir, srcMaterial.normal_texture.index, images);
		}

		// A texture without a non-zero factor emits nothing, so it is not worth decoding.
		if (srcMaterial.emissive_texture.index >= 0 && IsEmissive())
		{
			emissiveTexture = LoadSharedTexture(model, gltfDir, srcMaterial.emissive_texture.index, images);
		}

		// Likewise, without transmission the texture has nothing to scale.
		const int32_t transmissionTextureIndex =
				ExtensionTexture(srcMaterial, "KHR_materials_transmission", "transmissionTexture");
		if (transmissionTextureIndex >= 0 && transmission > 0.0f)
		{
			transmissionTexture = LoadSharedTexture(model, gltfDir, transmissionTextureIndex, images);
		}
	}

	int32_t Material::ResolveTextureImage(const tg3_model &model, int32_t textureIndex)
	{
		if (textureIndex < 0 || (uint32_t) textureIndex >= model.textures_count)
			return -1;
		return model.textures[textureIndex].source;
	}

	Texture Material::LoadSharedTexture(const tg3_model &model,
										const char *gltfDir,
										int32_t textureIndex,
										std::vector<Texture> &images)
	{
		const int32_t imageIndex = ResolveTextureImage(model, textureIndex);
		if (imageIndex < 0 || (size_t) imageIndex >= images.size())
			return Texture();
		if (!images[imageIndex].IsValid())
			LoadTexture(model, gltfDir, imageIndex, images[imageIndex]);
		return images[imageIndex];
	}

}; // namespace Scene
