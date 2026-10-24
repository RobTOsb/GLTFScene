#pragma once

#include "Texture.h"

struct tg3_model;
struct tg3_material;

namespace Scene
{
	// glTF alphaMode.
	enum class AlphaMode : uint32_t
	{
		Opaque = 0,
		Mask = 1,
		Blend = 2,
	};
	
	struct UVTransform
	{
		int32_t texCoord = 0;
		float m[6] = { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
		bool consistent = true;

		bool IsIdentity() const
		{
			return m[0] == 1.0f && m[1] == 0.0f && m[2] == 0.0f && m[3] == 1.0f && m[4] == 0.0f && m[5] == 0.0f;
		}
	};

	UVTransform ReadUVTransform(const tg3_model &model, int32_t materialIndex);

	class Material
	{
	public:
		Material();
		Material(const tg3_model &model, const char *gltfDir, const tg3_material &srcMaterial, std::vector<Texture> &images);

		const Texture &GetAlbedoTexture() const
		{
			return albedoTexture;
		}

		const Texture &GetRoughMetalTexture() const
		{
			return roughMetalTexture;
		}

		const Texture &GetNormalTexture() const
		{
			return normalTexture;
		}

		const Texture &GetEmissiveTexture() const
		{
			return emissiveTexture;
		}

		const Texture &GetTransmissionTexture() const
		{
			return transmissionTexture;
		}

		// KHR_materials_anisotropy: linear, RG = direction in tangent space (* 2 - 1), B multiplies the strength.
		const Texture &GetAnisotropyTexture() const
		{
			return anisotropyTexture;
		}

		// KHR_materials_sheen: sRGB, RGB multiplies the sheen colour.
		const Texture &GetSheenColourTexture() const
		{
			return sheenColourTexture;
		}

		// KHR_materials_specular: linear, A multiplies the specular factor.
		const Texture &GetSpecularTexture() const
		{
			return specularTexture;
		}

		// KHR_materials_specular: sRGB, RGB multiplies the specular colour.
		const Texture &GetSpecularColourTexture() const
		{
			return specularColourTexture;
		}

		// KHR_materials_sheen: linear, A multiplies the sheen roughness.
		const Texture &GetSheenRoughnessTexture() const
		{
			return sheenRoughnessTexture;
		}

		// KHR_materials_clearcoat: linear, R multiplies the clearcoat factor.
		const Texture &GetClearcoatTexture() const
		{
			return clearcoatTexture;
		}

		// KHR_materials_clearcoat: linear, G multiplies the clearcoat roughness.
		const Texture &GetClearcoatRoughnessTexture() const
		{
			return clearcoatRoughnessTexture;
		}

		// KHR_materials_clearcoat: tangent-space normal map of the coat. Without one the coat follows the
		// vertex normal, not the base's normal map.
		const Texture &GetClearcoatNormalTexture() const
		{
			return clearcoatNormalTexture;
		}

		// EXT_materials_subsurface: linear, GetSubsurfaceWeightChannel multiplies the weight.
		const Texture &GetSubsurfaceWeightTexture() const
		{
			return subsurfaceWeightTexture;
		}

		// EXT_materials_subsurface: linear, RGB multiplies the radius.
		const Texture &GetSubsurfaceRadiusTexture() const
		{
			return subsurfaceRadiusTexture;
		}

		// EXT_materials_subsurface: linear, GetSubsurfaceScaleChannel multiplies the scale.
		const Texture &GetSubsurfaceScaleTexture() const
		{
			return subsurfaceScaleTexture;
		}

		const float *GetBaseColour() const
		{
			return baseColour;
		}

		AlphaMode GetAlphaMode() const
		{
			return alphaMode;
		}

		float GetAlphaCutoff() const
		{
			return alphaCutoff;
		}

		float GetTransmission() const
		{
			return transmission;
		}

		// KHR_materials_ior, 1.5 when absent.
		float GetIor() const
		{
			return ior;
		}

		// KHR_materials_specular factor, scales the dielectric F0, 1 when absent.
		float GetSpecular() const
		{
			return specular;
		}

		// KHR_materials_specular colour factor, linear RGB that tints the dielectric F0 and may exceed 1,
		// white when absent.
		const float *GetSpecularColour() const
		{
			return specularColour;
		}

		// KHR_materials_volume: white light inside the volume comes out as this colour after
		// GetAttenuationDistance, linear RGB, white when absent.
		const float *GetAttenuationColour() const
		{
			return attenuationColour;
		}

		// KHR_materials_volume, infinite (no absorption) when absent or not positive.
		float GetAttenuationDistance() const
		{
			return attenuationDistance;
		}

		// Both sides are outside: light crosses without refracting or entering a medium. Set by the
		// EXT_materials_thin_wall extension (our own, {"thinWalled": true} or just {}).
		bool IsThinWalled() const
		{
			return thinWalled;
		}

		// KHR_materials_anisotropy strength, 0 (isotropic) when absent.
		float GetAnisotropyStrength() const
		{
			return anisotropyStrength;
		}

		// KHR_materials_anisotropy rotation in radians, counter-clockwise from the tangent in tangent space.
		float GetAnisotropyRotation() const
		{
			return anisotropyRotation;
		}

		// KHR_materials_sheen colour factor, linear RGB, black (no sheen) when absent.
		const float *GetSheenColour() const
		{
			return sheenColour;
		}

		// KHR_materials_sheen roughness factor, 0 when absent.
		float GetSheenRoughness() const
		{
			return sheenRoughness;
		}

		bool HasSheen() const
		{
			return sheenColour[0] > 0.0f || sheenColour[1] > 0.0f || sheenColour[2] > 0.0f;
		}

		// KHR_materials_clearcoat factor, 0 (no coat) when absent.
		float GetClearcoat() const
		{
			return clearcoat;
		}

		// KHR_materials_clearcoat roughness factor, 0 when absent.
		float GetClearcoatRoughness() const
		{
			return clearcoatRoughness;
		}

		// KHR_materials_clearcoat normal texture's scale, 1 when absent.
		float GetClearcoatNormalScale() const
		{
			return clearcoatNormalScale;
		}

		bool HasClearcoat() const
		{
			return clearcoat > 0.0f;
		}

		// EXT_materials_subsurface
		float GetSubsurfaceWeight() const
		{
			return subsurfaceWeight;
		}

		uint32_t GetSubsurfaceWeightChannel() const
		{
			return subsurfaceWeightChannel;
		}

		const float *GetSubsurfaceRadius() const
		{
			return subsurfaceRadius;
		}

		float GetSubsurfaceScale() const
		{
			return subsurfaceScale;
		}

		uint32_t GetSubsurfaceScaleChannel() const
		{
			return subsurfaceScaleChannel;
		}

		// Solid: the medium's Henyey-Greenstein phase function g, -1 back to 1 forward scattering.
		// Thin-walled: (1 - g) / 2 of the subsurface is reflected and (1 + g) / 2 transmitted.
		float GetSubsurfaceScatterAnisotropy() const
		{
			return subsurfaceScatterAnisotropy;
		}

		float GetRoughness() const
		{
			return roughness;
		}

		float GetMetal() const
		{
			return metal;
		}

		float GetNormalScale() const
		{
			return normalScale;
		}

		// glTF emissiveFactor, linear RGB.
		const float *GetEmissive() const
		{
			return emissive;
		}

		// KHR_materials_emissive_strength, scales the emissive factor so it can exceed 1.
		float GetEmissiveStrength() const
		{
			return emissiveStrength;
		}

		bool IsEmissive() const
		{
			return emissiveStrength > 0.0f && (emissive[0] > 0.0f || emissive[1] > 0.0f || emissive[2] > 0.0f);
		}

	private:
		static int32_t ResolveTextureImage(const tg3_model &model, int32_t textureIndex);
		static Texture LoadSharedTexture(const tg3_model &model,
										 const char *gltfDir,
										 int32_t textureIndex,
										 std::vector<Texture> &images);

		float baseColour[4];
		AlphaMode alphaMode;
		float alphaCutoff;
		float transmission;
		float ior;
		float specular;
		float specularColour[3];
		float attenuationColour[3];
		float attenuationDistance;
		bool thinWalled;
		float anisotropyStrength;
		float anisotropyRotation;
		float sheenColour[3];
		float sheenRoughness;
		float clearcoat;
		float clearcoatRoughness;
		float clearcoatNormalScale;
		float subsurfaceWeight;
		uint32_t subsurfaceWeightChannel;
		float subsurfaceRadius[3];
		float subsurfaceScale;
		uint32_t subsurfaceScaleChannel;
		float subsurfaceScatterAnisotropy;

		// Will get packed on the gpu
		float roughness;
		float metal;
		float normalScale;
		float emissive[3];
		float emissiveStrength;

		bool materialFactorsSet;

		Texture albedoTexture;
		Texture roughMetalTexture;
		Texture normalTexture;
		Texture emissiveTexture;
		Texture transmissionTexture;
		Texture anisotropyTexture;
		Texture specularTexture;
		Texture specularColourTexture;
		Texture sheenColourTexture;
		Texture sheenRoughnessTexture;
		Texture clearcoatTexture;
		Texture clearcoatRoughnessTexture;
		Texture clearcoatNormalTexture;
		Texture subsurfaceWeightTexture;
		Texture subsurfaceRadiusTexture;
		Texture subsurfaceScaleTexture;
	};
}; // namespace Scene
