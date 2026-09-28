#pragma once

#include "Texture.h"

struct tg3_model;
struct tg3_material;

namespace Scene
{
	class Material
	{
	public:
		Material();

		Material(Allocator::Arena &arena, const tg3_model &model, const char *gltfDir, const tg3_material &srcMaterial);

		const Texture &GetAlbedoTexture() const
		{
			return albedoTexture;
		}
		const Texture &GetRoughMetalTexture() const
		{
			return roughMetalTexture;
		}

		const float *GetBaseColour() const
		{
			return baseColour;
		}
		float GetRoughness() const
		{
			return roughness;
		}
		float GetMetal() const
		{
			return metal;
		}

	private:
		static int32_t ResolveTextureImage(const tg3_model &model, int32_t textureIndex);

		float baseColour[3];

		// Will get packed on the gpu
		float roughness;
		float metal;

		bool materialFactorsSet;

		Texture albedoTexture;
		Texture roughMetalTexture;
	};
}; // namespace Scene
