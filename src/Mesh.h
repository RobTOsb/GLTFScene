#pragma once

#include <string>
#include "FixedVector.h"
#include "Primitive.h"

struct tg3_model;
struct tg3_mesh;

namespace Scene
{
	class Mesh
	{
	public:
		Mesh(Allocator::Arena &arena, const tg3_model &model, const tg3_mesh &srcMesh);

		const std::string &GetName() const
		{
			return name;
		}
		const DSA::FixedVector<Primitive> &GetPrimitives() const
		{
			return primitives;
		}
		size_t GetPrimitiveCount() const
		{
			return primitives.size();
		}

	private:
		std::string name;
		DSA::FixedVector<Primitive> primitives;
	};
}; // namespace Scene
