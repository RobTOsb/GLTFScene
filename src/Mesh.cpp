#include "Mesh.h"

#include "tiny_gltf_v3.h"

namespace Scene
{
	Mesh::Mesh(const tg3_model &model, const tg3_mesh &srcMesh)
	{
		primitives.reserve(srcMesh.primitives_count);
		for (size_t p = 0; p < srcMesh.primitives_count; ++p)
			primitives.emplace_back(model, srcMesh.primitives[p]);

		if (srcMesh.name.data)
			name.assign(srcMesh.name.data, srcMesh.name.len);
	}
}; // namespace Scene
