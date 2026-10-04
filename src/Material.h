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

		Material(const tg3_model &model, const char *gltfDir, const tg3_material &srcMaterial);

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

	private:
		static int32_t ResolveTextureImage(const tg3_model &model, int32_t textureIndex);

		float baseColour[4];
		AlphaMode alphaMode;
		float alphaCutoff;
		float transmission;

		// Will get packed on the gpu
		float roughness;
		float metal;
		float normalScale;

		bool materialFactorsSet;

		Texture albedoTexture;
		Texture roughMetalTexture;
		Texture normalTexture;
	};
}; // namespace Scene
