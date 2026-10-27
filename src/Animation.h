#pragma once

#include <stdint.h>
#include <string>
#include <vector>

struct tg3_model;
struct tg3_animation;

namespace Scene
{
	// The node property a channel animates. Morph target weights are not supported.
	enum class AnimationPath : uint32_t
	{
		Translation,
		Rotation, // quaternion xyzw
		Scale
	};

	enum class AnimationInterpolation : uint32_t
	{
		Linear, // rotations are slerped
		Step,
		CubicSpline
	};

	// Components per key of a path's values: 4 for a rotation, 3 otherwise.
	inline uint32_t AnimationPathComponents(AnimationPath path)
	{
		return path == AnimationPath::Rotation ? 4u : 3u;
	}

	// A cubic spline has an in-tangent, the value and an out-tangent per key, the others the value only.
	inline uint32_t AnimationValuesPerKey(AnimationInterpolation interpolation)
	{
		return interpolation == AnimationInterpolation::CubicSpline ? 3u : 1u;
	}

	// The value of a channel's keys at time, held at the first and last key outside them. values holds
	// AnimationValuesPerKey * AnimationPathComponents floats per key, times one per key in increasing order.
	// Rotations come out normalised.
	void SampleAnimation(AnimationPath path,
						 AnimationInterpolation interpolation,
						 const float *times,
						 const float *values,
						 uint32_t keyCount,
						 float time,
						 float out[4]);

	// One glTF animation channel with its sampler's keys.
	struct AnimationChannel
	{
		int32_t node;
		AnimationPath path;
		AnimationInterpolation interpolation;
		std::vector<float> times;
		std::vector<float> values;
	};

	class Animation
	{
	public:
		// Keeps the channels on nodes' translation, rotation and scale whose keys read correctly; the rest
		// are reported to stderr and dropped.
		Animation(const tg3_model &model, const tg3_animation &srcAnimation);

		const std::string &GetName() const
		{
			return name;
		}

		const std::vector<AnimationChannel> &GetChannels() const
		{
			return channels;
		}

		// The time of the last key of any channel.
		float GetDuration() const
		{
			return duration;
		}

	private:
		std::string name;
		std::vector<AnimationChannel> channels;
		float duration = 0.0f;
	};
}; // namespace Scene
