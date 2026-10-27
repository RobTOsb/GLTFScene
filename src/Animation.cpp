#include "Animation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "tiny_gltf_v3.h"

#include "Primitive.h"

namespace Scene
{
	namespace
	{
		std::string ToString(const tg3_str &s)
		{
			return s.data ? std::string(s.data, s.len) : std::string();
		}

		// Spherical interpolation along the shorter arc, normalised.
		void Slerp(const float *a, const float *b, float s, float out[4])
		{
			float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
			const float sign = d < 0.0f ? -1.0f : 1.0f;
			d *= sign;
			float wa = 1.0f - s;
			float wb = s;
			// Nearly parallel quaternions divide by a vanishing sine, where lerping is as good.
			if (d < 0.9995f)
			{
				const float angle = acosf(d);
				const float invSin = 1.0f / sinf(angle);
				wa = sinf(wa * angle) * invSin;
				wb = sinf(wb * angle) * invSin;
			}
			for (int c = 0; c < 4; ++c)
				out[c] = wa * a[c] + sign * wb * b[c];
		}

		void Normalise4(float q[4])
		{
			const float length = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
			const float scale = length > 0.0f ? 1.0f / length : 0.0f;
			for (int c = 0; c < 4; ++c)
				q[c] *= scale;
			if (length == 0.0f)
				q[3] = 1.0f;
		}
	} // namespace

	void SampleAnimation(AnimationPath path,
						 AnimationInterpolation interpolation,
						 const float *times,
						 const float *values,
						 uint32_t keyCount,
						 float time,
						 float out[4])
	{
		const uint32_t n = AnimationPathComponents(path);
		const uint32_t stride = AnimationValuesPerKey(interpolation) * n;
		// A cubic spline's value sits between its tangents.
		const uint32_t valueOffset = interpolation == AnimationInterpolation::CubicSpline ? n : 0;
		auto value = [&](uint32_t key) { return values + key * stride + valueOffset; };
		auto copy = [&](const float *v) { std::copy(v, v + n, out); };

		if (keyCount <= 1 || time <= times[0])
			copy(value(0));
		else if (time >= times[keyCount - 1])
			copy(value(keyCount - 1));
		else
		{
			// times[k] <= time < times[k + 1]
			const uint32_t k = (uint32_t) (std::upper_bound(times, times + keyCount, time) - times) - 1;
			const float dt = times[k + 1] - times[k];
			const float s = dt > 0.0f ? (time - times[k]) / dt : 0.0f;
			const float *v0 = value(k);
			const float *v1 = value(k + 1);
			switch (interpolation)
			{
				case AnimationInterpolation::Step:
					copy(v0);
					break;
				case AnimationInterpolation::Linear:
					if (path == AnimationPath::Rotation)
						Slerp(v0, v1, s, out);
					else
						for (uint32_t c = 0; c < n; ++c)
							out[c] = v0[c] + (v1[c] - v0[c]) * s;
					break;
				case AnimationInterpolation::CubicSpline:
				{
					// Hermite, with the tangents per unit time scaled to the key interval.
					const float *outTangent0 = v0 + n;
					const float *inTangent1 = v1 - n;
					const float s2 = s * s;
					const float s3 = s2 * s;
					const float h00 = 2.0f * s3 - 3.0f * s2 + 1.0f;
					const float h10 = (s3 - 2.0f * s2 + s) * dt;
					const float h01 = -2.0f * s3 + 3.0f * s2;
					const float h11 = (s3 - s2) * dt;
					for (uint32_t c = 0; c < n; ++c)
						out[c] = h00 * v0[c] + h10 * outTangent0[c] + h01 * v1[c] + h11 * inTangent1[c];
					break;
				}
			}
		}

		if (path == AnimationPath::Rotation)
			Normalise4(out);
	}

	Animation::Animation(const tg3_model &model, const tg3_animation &srcAnimation) : name(ToString(srcAnimation.name))
	{
		for (uint32_t c = 0; c < srcAnimation.channels_count; ++c)
		{
			const tg3_animation_channel &srcChannel = srcAnimation.channels[c];
			const std::string pathName = ToString(srcChannel.target.path);
			AnimationChannel channel;
			channel.node = srcChannel.target.node;
			if (pathName == "translation")
				channel.path = AnimationPath::Translation;
			else if (pathName == "rotation")
				channel.path = AnimationPath::Rotation;
			else if (pathName == "scale")
				channel.path = AnimationPath::Scale;
			else
			{
				fprintf(stderr, "Animation '%s': channel %u animates '%s', which is not supported\n", name.c_str(), c,
						pathName.c_str());
				continue;
			}
			if (channel.node < 0 || (uint32_t) channel.node >= model.nodes_count || srcChannel.sampler < 0 ||
				(uint32_t) srcChannel.sampler >= srcAnimation.samplers_count)
			{
				fprintf(stderr, "Animation '%s': channel %u has no valid node or sampler\n", name.c_str(), c);
				continue;
			}

			const tg3_animation_sampler &sampler = srcAnimation.samplers[srcChannel.sampler];
			const std::string interpolation = ToString(sampler.interpolation);
			channel.interpolation = interpolation == "STEP"			 ? AnimationInterpolation::Step
									: interpolation == "CUBICSPLINE" ? AnimationInterpolation::CubicSpline
																	 : AnimationInterpolation::Linear;

			const uint32_t n = AnimationPathComponents(channel.path);
			if (!ReadAccessorAsFloat(model, sampler.input, 1, channel.times) ||
				!ReadAccessorAsFloat(model, sampler.output, (int32_t) n, channel.values) || channel.times.empty() ||
				channel.values.size() != channel.times.size() * n * AnimationValuesPerKey(channel.interpolation))
			{
				fprintf(stderr, "Animation '%s': channel %u's keys could not be read\n", name.c_str(), c);
				continue;
			}

			duration = std::max(duration, channel.times.back());
			channels.push_back(std::move(channel));
		}
	}
}; // namespace Scene
