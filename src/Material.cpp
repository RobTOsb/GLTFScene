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

		bool HasExtension(const tg3_material &material, const char *extension)
		{
			for (uint32_t i = 0; i < material.ext.extensions_count; ++i)
			{
				if (tg3_str_equals_cstr(material.ext.extensions[i].name, extension))
					return true;
			}
			return false;
		}

		// Reads an RGB array from an extension's JSON object into out, leaving out as it is when absent.
		void ExtensionColour(const tg3_material &material, const char *extension, const char *key, float out[3])
		{
			const tg3_value *colour = ExtensionValue(material, extension, key);
			if (!colour || colour->type != TG3_VALUE_ARRAY)
				return;
			for (uint32_t c = 0; c < 3 && c < colour->array_count; ++c)
			{
				double value;
				if (ToNumber(&colour->array_data[c], value))
					out[c] = (float) value;
			}
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
		diffuseTransmission = 0.0f;
		diffuseTransmissionColour[0] = diffuseTransmissionColour[1] = diffuseTransmissionColour[2] = 1.0f;
		anisotropyStrength = 0.0f;
		anisotropyRotation = 0.0f;
		subsurfaceWeight = 0.0f;
		subsurfaceColour[0] = subsurfaceColour[1] = subsurfaceColour[2] = 0.8f;
		subsurfaceRadius = 1.0f;
		subsurfaceRadiusScale[0] = 1.0f;
		subsurfaceRadiusScale[1] = 0.5f;
		subsurfaceRadiusScale[2] = 0.25f;
		subsurfaceScatterAnisotropy = 0.0f;
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
		// Present means thin-walled unless it says "thinWalled": false.
		thinWalled = HasExtension(srcMaterial, "EXT_materials_thin_wall");
		if (const tg3_value *flag = ExtensionValue(srcMaterial, "EXT_materials_thin_wall", "thinWalled");
			flag && flag->type == TG3_VALUE_BOOL)
			thinWalled = flag->bool_val != 0;
		diffuseTransmission = (float) ExtensionNumber(srcMaterial,
													  "KHR_materials_diffuse_transmission",
													  "diffuseTransmissionFactor",
													  0.0);
		diffuseTransmissionColour[0] = diffuseTransmissionColour[1] = diffuseTransmissionColour[2] = 1.0f;
		ExtensionColour(srcMaterial,
						"KHR_materials_diffuse_transmission",
						"diffuseTransmissionColorFactor",
						diffuseTransmissionColour);
		// OpenPBR's defaults, except that the extension being there means a weight of 1.
		const char *subsurface = "EXT_materials_subsurface";
		subsurfaceWeight =
				HasExtension(srcMaterial, subsurface)
						? (float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceWeight", 1.0)
						: 0.0f;
		subsurfaceColour[0] = subsurfaceColour[1] = subsurfaceColour[2] = 0.8f;
		ExtensionColour(srcMaterial, subsurface, "subsurfaceColor", subsurfaceColour);
		subsurfaceRadius = (float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceRadius", 1.0);
		subsurfaceRadiusScale[0] = 1.0f;
		subsurfaceRadiusScale[1] = 0.5f;
		subsurfaceRadiusScale[2] = 0.25f;
		ExtensionColour(srcMaterial, subsurface, "subsurfaceRadiusScale", subsurfaceRadiusScale);
		subsurfaceScatterAnisotropy =
				(float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceScatterAnisotropy", 0.0);
		anisotropyStrength =
				(float) ExtensionNumber(srcMaterial, "KHR_materials_anisotropy", "anisotropyStrength", 0.0);
		anisotropyRotation =
				(float) ExtensionNumber(srcMaterial, "KHR_materials_anisotropy", "anisotropyRotation", 0.0);
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

		if (diffuseTransmission > 0.0f)
		{
			const int32_t factorTextureIndex =
					ExtensionTexture(srcMaterial, "KHR_materials_diffuse_transmission", "diffuseTransmissionTexture");
			if (factorTextureIndex >= 0)
				diffuseTransmissionTexture = LoadSharedTexture(model, gltfDir, factorTextureIndex, images);
			const int32_t colourTextureIndex = ExtensionTexture(srcMaterial,
																"KHR_materials_diffuse_transmission",
																"diffuseTransmissionColorTexture");
			if (colourTextureIndex >= 0)
				diffuseTransmissionColourTexture = LoadSharedTexture(model, gltfDir, colourTextureIndex, images);
		}

		// The texture's B channel multiplies the strength, so without strength it changes nothing.
		const int32_t anisotropyTextureIndex =
				ExtensionTexture(srcMaterial, "KHR_materials_anisotropy", "anisotropyTexture");
		if (anisotropyTextureIndex >= 0 && anisotropyStrength > 0.0f)
		{
			anisotropyTexture = LoadSharedTexture(model, gltfDir, anisotropyTextureIndex, images);
		}

		const int32_t subsurfaceColourTextureIndex =
				ExtensionTexture(srcMaterial, subsurface, "subsurfaceColorTexture");
		if (subsurfaceColourTextureIndex >= 0 && subsurfaceWeight > 0.0f)
		{
			subsurfaceColourTexture = LoadSharedTexture(model, gltfDir, subsurfaceColourTextureIndex, images);
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
