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

		// KHR_materials_anisotropy: linear, RG = direction in tangent space (* 2 - 1), B multiplies the strength.
		const Texture &GetAnisotropyTexture() const
		{
			return anisotropyTexture;
		}

		// EXT_materials_subsurface: sRGB, RGB multiplies the subsurface colour.
		const Texture &GetSubsurfaceColourTexture() const
		{
			return subsurfaceColourTexture;
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

		// EXT_materials_subsurface (our own): OpenPBR's subsurface parameters for a solid object, a
		// scattering medium under the surface. The weight is 0 when absent and 1 by default when present.
		float GetSubsurfaceWeight() const
		{
			return subsurfaceWeight;
		}

		// Linear RGB albedo the medium shows after multiple scattering (OpenPBR subsurface_color).
		const float *GetSubsurfaceColour() const
		{
			return subsurfaceColour;
		}

		// Mean free path in scene units, multiplied per channel by GetSubsurfaceRadiusScale.
		float GetSubsurfaceRadius() const
		{
			return subsurfaceRadius;
		}

		const float *GetSubsurfaceRadiusScale() const
		{
			return subsurfaceRadiusScale;
		}

		// Henyey-Greenstein phase function g, -1 back to 1 forward scattering.
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
		bool thinWalled;
		float diffuseTransmission;
		float diffuseTransmissionColour[3];
		float anisotropyStrength;
		float anisotropyRotation;
		float subsurfaceWeight;
		float subsurfaceColour[3];
		float subsurfaceRadius;
		float subsurfaceRadiusScale[3];
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
		Texture diffuseTransmissionTexture;
		Texture diffuseTransmissionColourTexture;
		Texture anisotropyTexture;
		Texture subsurfaceColourTexture;
	};
}; // namespace Scene
