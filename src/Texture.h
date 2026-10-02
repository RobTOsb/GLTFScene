#pragma once

#include <stdint.h>
#include <vector>

struct tg3_model;

namespace Scene
{
	class Texture
	{
	public:
		Texture() : width(0), height(0) {}
		Texture(const uint8_t *rgba, int32_t w, int32_t h);
		bool IsValid() const
		{
			return width > 0 && height > 0;
		}

		int32_t GetWidth() const
		{
			return width;
		}
		int32_t GetHeight() const
		{
			return height;
		}
		const uint8_t *GetPixels() const
		{
			return pixels.data();
		}

	private:
		std::vector<uint8_t> pixels; // RGBA8, width * height * 4 bytes
		int32_t width;
		int32_t height;
	};

	// Reads and decodes the pixel data for model.images[imageIndex] into
	// `out`. Resolves the image's bytes either from an external file (URI,
	// read relative to gltfDir) or from an embedded buffer_view (GLB).
	// Leaves `out` empty (IsValid() == false) if the image can't be
	// resolved or decoded - callers treat that as "no texture."
	void LoadTexture(
			const tg3_model &model, const char *gltfDir, int32_t imageIndex, Texture &out);


}; // namespace Scene
