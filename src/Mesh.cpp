#include "Mesh.h"

#include "tiny_gltf_v3.h"

namespace Scene
{
	Mesh::Mesh(Allocator::Arena &arena, const tg3_model &model, const tg3_mesh &srcMesh) :
		primitives(arena,
				   srcMesh.primitives_count,
				   [&](size_t p) { return Primitive(arena, model, srcMesh.primitives[p]); })
	{
		if (srcMesh.name.data)
			name.assign(srcMesh.name.data, srcMesh.name.len);
	}
}; // namespace Scene
