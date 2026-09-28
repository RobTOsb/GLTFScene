#include "Instance.h"
#include <utility>
#include "tiny_gltf_v3.h"

namespace Scene
{
	Instance::Instance(int32_t meshIndex, std::string name, const tg3_node &srcNode) :
		meshIndex(meshIndex), name(std::move(name))
	{
		if (srcNode.has_matrix)
		{
			// glTF matrix[] is column-major; transpose into our
			// row-major worldMatrix. TRS fields are meaningless
			// when has_matrix is set, so leave them at identity.
			for (int r = 0; r < 3; ++r)
				for (int c = 0; c < 4; ++c)
					worldMatrix[r * 4 + c] = (float) srcNode.matrix[c * 4 + r];

			translation[0] = translation[1] = translation[2] = 0.0f;
			rotation[0] = rotation[1] = rotation[2] = 0.0f;
			rotation[3] = 1.0f;
			scale[0] = scale[1] = scale[2] = 1.0f;
		}
		else
		{
			translation[0] = (float) srcNode.translation[0];
			translation[1] = (float) srcNode.translation[1];
			translation[2] = (float) srcNode.translation[2];

			rotation[0] = (float) srcNode.rotation[0];
			rotation[1] = (float) srcNode.rotation[1];
			rotation[2] = (float) srcNode.rotation[2];
			rotation[3] = (float) srcNode.rotation[3];

			scale[0] = (float) srcNode.scale[0];
			scale[1] = (float) srcNode.scale[1];
			scale[2] = (float) srcNode.scale[2];

			ComposeTRS(translation, rotation, scale, worldMatrix);
		}
	}
}; // namespace Scene
