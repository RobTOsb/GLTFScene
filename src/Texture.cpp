#include "Texture.h"

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

#include "tiny_gltf_v3.h"

namespace Scene
{

	Texture::Texture(const uint8_t *rgba, int32_t w, int32_t h) :
		pixels(std::make_shared<const std::vector<uint8_t>>(rgba, rgba + (size_t) w * h * 4)), width(w), height(h)
	{
	}

	void LoadTexture(const tg3_model &model, const char *gltfDir, int32_t imageIndex, Texture &out)
	{
		if (imageIndex < 0 || (uint32_t) imageIndex >= model.images_count)
			return;

		const tg3_image &img = model.images[imageIndex];

		const uint8_t *encoded = nullptr;
		size_t encodedSize = 0;

		// Owns file bytes read from disk, if that path is taken, so they
		// outlive the stbi_load_from_memory call below.
		std::vector<uint8_t> fileBytes;

		if (img.buffer_view >= 0)
		{
			if ((uint32_t) img.buffer_view >= model.buffer_views_count)
				return;

			const tg3_buffer_view &bv = model.buffer_views[img.buffer_view];
			if (bv.buffer < 0 || (uint32_t) bv.buffer >= model.buffers_count)
				return;

			const tg3_buffer &buf = model.buffers[bv.buffer];
			encoded = buf.data.data + bv.byte_offset;
			encodedSize = (size_t) bv.byte_length;
		}
		else if (img.uri.data && img.uri.len > 0)
		{
			if (tg3_is_data_uri(img.uri.data, img.uri.len))
			{
				fprintf(stderr, "LoadTexture: embedded data URIs are not supported\n");
				return;
			}

			char path[1024];
			int written = snprintf(path, sizeof(path), "%s%.*s", gltfDir, (int) img.uri.len, img.uri.data);
			if (written < 0 || (size_t) written >= sizeof(path))
			{
				fprintf(stderr, "LoadTexture: image path too long\n");
				return;
			}

			FILE *f = fopen(path, "rb");
			if (!f)
			{
				fprintf(stderr, "LoadTexture: failed to open '%s'\n", path);
				return;
			}

			fseek(f, 0, SEEK_END);
			long size = ftell(f);
			fseek(f, 0, SEEK_SET);
			if (size <= 0)
			{
				fclose(f);
				return;
			}

			fileBytes = std::vector<uint8_t>((size_t) size);
			size_t readBytes = fread(fileBytes.data(), 1, (size_t) size, f);
			fclose(f);

			if (readBytes != (size_t) size)
				return;

			encoded = fileBytes.data();
			encodedSize = (size_t) size;
		}
		else
		{
			return;
		}

		int w = 0, h = 0, channelsInFile = 0;
		stbi_uc *decoded = stbi_load_from_memory(encoded, (int) encodedSize, &w, &h, &channelsInFile, 4);
		if (!decoded)
		{
			fprintf(stderr, "LoadTexture: decode failed (%s)\n", stbi_failure_reason());
			return;
		}

		out = Texture(decoded, w, h);
		stbi_image_free(decoded);
	}

}; // namespace Scene
