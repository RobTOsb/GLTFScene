#include "Material.h"

#include <cmath>

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

		// An extension's JSON object, or null when it is absent.
		const tg3_value *FindExtension(const tg3_extras_ext &ext, const char *extension)
		{
			for (uint32_t i = 0; i < ext.extensions_count; ++i)
			{
				if (tg3_str_equals_cstr(ext.extensions[i].name, extension))
					return &ext.extensions[i].value;
			}
			return nullptr;
		}

		// The member key of an extension's JSON object, or null when either is absent.
		const tg3_value *ExtensionValue(const tg3_material &material, const char *extension, const char *key)
		{
			const tg3_value *object = FindExtension(material.ext, extension);
			return object ? ObjectMember(*object, key) : nullptr;
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

		// An extension's "R", "G", "B" or "A" member as 0 to 3, fallback when absent or anything else.
		uint32_t ExtensionChannel(const tg3_material &material, const char *extension, const char *key, uint32_t fallback)
		{
			const tg3_value *channel = ExtensionValue(material, extension, key);
			if (!channel || channel->type != TG3_VALUE_STRING || channel->string_val.len != 1)
				return fallback;
			switch (channel->string_val.data[0])
			{
				case 'R': return 0;
				case 'G': return 1;
				case 'B': return 2;
				case 'A': return 3;
				default: return fallback;
			}
		}

		// Reads up to count numbers of a JSON array into out, leaving the rest as they are.
		void ReadNumbers(const tg3_value *array, double *out, uint32_t count)
		{
			if (!array || array->type != TG3_VALUE_ARRAY)
				return;
			for (uint32_t i = 0; i < count && i < array->array_count; ++i)
				ToNumber(&array->array_data[i], out[i]);
		}

		// One texture's mapping from its textureInfo's texCoord and KHR_texture_transform object (null
		// when absent), whose own texCoord overrides the textureInfo's. The matrix is T * R * S.
		UVTransform TextureUVTransform(int32_t texCoord, const tg3_value *transform)
		{
			UVTransform t;
			t.texCoord = texCoord;
			if (!transform)
				return t;
			double value;
			if (ToNumber(ObjectMember(*transform, "texCoord"), value))
				t.texCoord = (int32_t) value;
			double offset[2] = { 0.0, 0.0 };
			double scale[2] = { 1.0, 1.0 };
			double rotation = 0.0;
			ReadNumbers(ObjectMember(*transform, "offset"), offset, 2);
			ReadNumbers(ObjectMember(*transform, "scale"), scale, 2);
			ToNumber(ObjectMember(*transform, "rotation"), rotation);
			const double c = std::cos(rotation);
			const double s = std::sin(rotation);
			t.m[0] = (float) (c * scale[0]);
			t.m[1] = (float) (-s * scale[0]);
			t.m[2] = (float) (s * scale[1]);
			t.m[3] = (float) (c * scale[1]);
			t.m[4] = (float) offset[0];
			t.m[5] = (float) offset[1];
			return t;
		}

		// A textureInfo of the material itself, -1 index when absent.
		UVTransform CoreTextureUVTransform(int32_t texCoord, const tg3_extras_ext &ext)
		{
			return TextureUVTransform(texCoord, FindExtension(ext, "KHR_texture_transform"));
		}

		bool SameMapping(const UVTransform &a, const UVTransform &b)
		{
			if (a.texCoord != b.texCoord)
				return false;
			for (int i = 0; i < 6; ++i)
			{
				if (std::abs(a.m[i] - b.m[i]) > 1.0e-6f)
					return false;
			}
			return true;
		}
	} // namespace

	UVTransform ReadUVTransform(const tg3_model &model, int32_t materialIndex)
	{
		UVTransform result;
		if (materialIndex < 0 || (uint32_t) materialIndex >= model.materials_count)
			return result;
		const tg3_material &material = model.materials[materialIndex];
		const tg3_pbr_metallic_roughness &pbr = material.pbr_metallic_roughness;

		bool found = false;
		auto add = [&](int32_t index, const UVTransform &t)
		{
			if (index < 0)
				return;
			if (!found)
			{
				result = t;
				found = true;
			}
			else if (!SameMapping(t, result))
			{
				result.consistent = false;
			}
		};

		add(pbr.base_color_texture.index, CoreTextureUVTransform(pbr.base_color_texture.tex_coord, pbr.base_color_texture.ext));
		add(material.normal_texture.index, CoreTextureUVTransform(material.normal_texture.tex_coord, material.normal_texture.ext));
		add(pbr.metallic_roughness_texture.index,
			CoreTextureUVTransform(pbr.metallic_roughness_texture.tex_coord, pbr.metallic_roughness_texture.ext));
		add(material.emissive_texture.index,
			CoreTextureUVTransform(material.emissive_texture.tex_coord, material.emissive_texture.ext));

		// The extensions' textures that Material loads.
		static const char *const kExtensionTextures[][2] = {
			{ "KHR_materials_transmission", "transmissionTexture" },
			{ "KHR_materials_anisotropy", "anisotropyTexture" },
			{ "KHR_materials_sheen", "sheenColorTexture" },
			{ "KHR_materials_sheen", "sheenRoughnessTexture" },
			{ "EXT_materials_subsurface", "subsurfaceTexture" },
			{ "EXT_materials_subsurface", "subsurfaceRadiusTexture" },
			{ "EXT_materials_subsurface", "subsurfaceScaleTexture" },
		};
		for (const auto &[extension, key]: kExtensionTextures)
		{
			const tg3_value *info = ExtensionValue(material, extension, key);
			double index;
			if (!info || !ToNumber(ObjectMember(*info, "index"), index))
				continue;
			double texCoord = 0.0;
			ToNumber(ObjectMember(*info, "texCoord"), texCoord);
			const tg3_value *extensions = ObjectMember(*info, "extensions");
			add((int32_t) index,
				TextureUVTransform((int32_t) texCoord,
								   extensions ? ObjectMember(*extensions, "KHR_texture_transform") : nullptr));
		}
		return result;
	}

	Material::Material() : materialFactorsSet(false)
	{
		baseColour[0] = baseColour[1] = baseColour[2] = baseColour[3] = 1.0f;
		alphaMode = AlphaMode::Opaque;
		alphaCutoff = 0.5f;
		transmission = 0.0f;
		ior = 1.5f;
		thinWalled = false;
		anisotropyStrength = 0.0f;
		anisotropyRotation = 0.0f;
		sheenColour[0] = sheenColour[1] = sheenColour[2] = 0.0f;
		sheenRoughness = 0.0f;
		subsurfaceWeight = 0.0f;
		subsurfaceWeightChannel = 0;
		subsurfaceRadius[0] = 1.0f;
		subsurfaceRadius[1] = 0.2f;
		subsurfaceRadius[2] = 0.1f;
		subsurfaceScale = 0.05f;
		subsurfaceScaleChannel = 0;
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
		// Blender's Principled BSDF defaults, except that the extension being there means a weight of 1.
		const char *subsurface = "EXT_materials_subsurface";
		subsurfaceWeight =
				HasExtension(srcMaterial, subsurface)
						? (float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceFactor", 1.0)
						: 0.0f;
		subsurfaceWeightChannel = ExtensionChannel(srcMaterial, subsurface, "subsurfaceTextureChannel", 0);
		subsurfaceRadius[0] = 1.0f;
		subsurfaceRadius[1] = 0.2f;
		subsurfaceRadius[2] = 0.1f;
		ExtensionColour(srcMaterial, subsurface, "subsurfaceRadius", subsurfaceRadius);
		subsurfaceScale = (float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceScale", 0.05);
		subsurfaceScaleChannel = ExtensionChannel(srcMaterial, subsurface, "subsurfaceScaleTextureChannel", 0);
		subsurfaceScatterAnisotropy =
				(float) ExtensionNumber(srcMaterial, subsurface, "subsurfaceAnisotropy", 0.0);
		anisotropyStrength =
				(float) ExtensionNumber(srcMaterial, "KHR_materials_anisotropy", "anisotropyStrength", 0.0);
		anisotropyRotation =
				(float) ExtensionNumber(srcMaterial, "KHR_materials_anisotropy", "anisotropyRotation", 0.0);
		sheenColour[0] = sheenColour[1] = sheenColour[2] = 0.0f;
		ExtensionColour(srcMaterial, "KHR_materials_sheen", "sheenColorFactor", sheenColour);
		sheenRoughness = (float) ExtensionNumber(srcMaterial, "KHR_materials_sheen", "sheenRoughnessFactor", 0.0);
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

		// The texture's B channel multiplies the strength, so without strength it changes nothing.
		const int32_t anisotropyTextureIndex =
				ExtensionTexture(srcMaterial, "KHR_materials_anisotropy", "anisotropyTexture");
		if (anisotropyTextureIndex >= 0 && anisotropyStrength > 0.0f)
		{
			anisotropyTexture = LoadSharedTexture(model, gltfDir, anisotropyTextureIndex, images);
		}

		// A black sheen colour factor turns the sheen off, whatever the textures hold.
		if (HasSheen())
		{
			const int32_t colourTextureIndex = ExtensionTexture(srcMaterial, "KHR_materials_sheen", "sheenColorTexture");
			if (colourTextureIndex >= 0)
				sheenColourTexture = LoadSharedTexture(model, gltfDir, colourTextureIndex, images);
			const int32_t roughnessTextureIndex =
					ExtensionTexture(srcMaterial, "KHR_materials_sheen", "sheenRoughnessTexture");
			if (roughnessTextureIndex >= 0)
				sheenRoughnessTexture = LoadSharedTexture(model, gltfDir, roughnessTextureIndex, images);
		}

		// Without weight the medium is not there for the textures to change. A thin wall has no medium,
		// so no radius either.
		if (subsurfaceWeight > 0.0f)
		{
			const int32_t weightTextureIndex = ExtensionTexture(srcMaterial, subsurface, "subsurfaceTexture");
			if (weightTextureIndex >= 0)
				subsurfaceWeightTexture = LoadSharedTexture(model, gltfDir, weightTextureIndex, images);
		}
		if (subsurfaceWeight > 0.0f && !thinWalled)
		{
			const int32_t radiusTextureIndex = ExtensionTexture(srcMaterial, subsurface, "subsurfaceRadiusTexture");
			if (radiusTextureIndex >= 0)
				subsurfaceRadiusTexture = LoadSharedTexture(model, gltfDir, radiusTextureIndex, images);
			const int32_t scaleTextureIndex = ExtensionTexture(srcMaterial, subsurface, "subsurfaceScaleTexture");
			if (scaleTextureIndex >= 0)
				subsurfaceScaleTexture = LoadSharedTexture(model, gltfDir, scaleTextureIndex, images);
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
