#pragma once

#include <stdint.h>
#include <string>

struct tg3_node;

namespace Scene
{
	// Composes translation/rotation(quat xyzw)/scale into a row-major
	// float3x4, i.e. out[row*4+col] for row in [0,3) and col in [0,4),
	// with translation as the 4th column of each row so the GPU can
	// add it directly (row-vector convention: v' = v * M, translation
	// appended per-row rather than as a separate 4th row).
	static void ComposeTRS(const float translation[3], const float rotation[4], const float scale[3], float out[12])
	{
		const float x = rotation[0], y = rotation[1], z = rotation[2], w = rotation[3];

		const float xx = x * x, yy = y * y, zz = z * z;
		const float xy = x * y, xz = x * z, yz = y * z;
		const float wx = w * x, wy = w * y, wz = w * z;

		float rot[3][3] = {
			{ 1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy) },
			{ 2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx) },
			{ 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy) },
		};

		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 3; ++c)
				out[r * 4 + c] = rot[r][c] * scale[r];

		out[0 * 4 + 3] = translation[0];
		out[1 * 4 + 3] = translation[1];
		out[2 * 4 + 3] = translation[2];
	}

	// Converts a row-major float3x4 built for row-vector convention
	// (v' = v * M, as produced by ComposeTRS) into the row-major float3x4
	// column-vector convention (v' = M * v) expected by
	// D3D12_RAYTRACING_INSTANCE_DESC::Transform / VkTransformMatrixKHR
	static void RowMajorToBLASTransform(const float in[12], float out[12])
	{
		float rot[3][3];
		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 3; ++c)
				rot[r][c] = in[r * 4 + c];

		for (int r = 0; r < 3; ++r)
			for (int c = 0; c < 3; ++c)
				out[r * 4 + c] = rot[c][r];

		out[0 * 4 + 3] = in[0 * 4 + 3];
		out[1 * 4 + 3] = in[1 * 4 + 3];
		out[2 * 4 + 3] = in[2 * 4 + 3];
	}

	class alignas(64) Instance
	{
	public:
		Instance(int32_t meshIndex, std::string name, const tg3_node &srcNode);

		int32_t GetMeshIndex() const
		{
			return meshIndex;
		}

		const float *GetWorldMatrix() const
		{
			return worldMatrix;
		}

		void SetTranslation(const float t[3])
		{
			translation[0] = t[0];
			translation[1] = t[1];
			translation[2] = t[2];
			ComposeTRS(translation, rotation, scale, worldMatrix);
		}

		void SetRotation(const float q[4])
		{
			rotation[0] = q[0];
			rotation[1] = q[1];
			rotation[2] = q[2];
			rotation[3] = q[3];
			ComposeTRS(translation, rotation, scale, worldMatrix);
		}

		void SetScale(const float s[3])
		{
			scale[0] = s[0];
			scale[1] = s[1];
			scale[2] = s[2];
			ComposeTRS(translation, rotation, scale, worldMatrix);
		}

	private:
		std::string name;
		int32_t meshIndex;

		float rotation[4];
		float translation[3];
		float scale[3];
		float worldMatrix[12]; // float3x4
	};

	static_assert(alignof(Instance) == 64, "scene node is not aligned to cache size.");
    
}; // namespace Scene
