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

		// KHR_materials_diffuse_transmission: A multiplies the factor.
		const Texture &GetDiffuseTransmissionTexture() const
		{
			return diffuseTransmissionTexture;
		}

		// KHR_materials_diffuse_transmission: sRGB, RGB multiplies the colour factor.
		const Texture &GetDiffuseTransmissionColourTexture() const
		{
			return diffuseTransmissionColourTexture;
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

		// Both sides are outside: light crosses without refracting or entering a medium. Set by the
		// EXT_materials_thin_wall extension (our own, {"thinWalled": true} or just {}).
		bool IsThinWalled() const
		{
			return thinWalled;
		}

		// KHR_materials_diffuse_transmission factor: the share of the diffuse base that is transmitted
		// rather than reflected, 0 when absent.
		float GetDiffuseTransmission() const
		{
			return diffuseTransmission;
		}

		// KHR_materials_diffuse_transmission colour factor, linear RGB, white when absent.
		const float *GetDiffuseTransmissionColour() const
		{
			return diffuseTransmissionColour;
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
		bool thinWalled;
		float diffuseTransmission;
		float diffuseTransmissionColour[3];

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
		Texture diffuseTransmissionTexture;
		Texture diffuseTransmissionColourTexture;
	};
}; // namespace Scene
