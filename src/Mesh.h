#pragma once

#include <string>
#include <vector>
#include "Primitive.h"

struct tg3_model;
struct tg3_mesh;

namespace Scene
{
	class Mesh
	{
	public:
		Mesh(const tg3_model &model, const tg3_mesh &srcMesh);

		const std::string &GetName() const
		{
			return name;
		}
		const std::vector<Primitive> &GetPrimitives() const
		{
			return primitives;
		}
		size_t GetPrimitiveCount() const
		{
			return primitives.size();
		}

	private:
		std::string name;
		std::vector<Primitive> primitives;
	};
}; // namespace Scene
