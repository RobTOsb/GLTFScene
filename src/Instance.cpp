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
			// glTF matrix[] is column-major and built for column vectors
			// (v' = M * v). Store it in the same layout ComposeTRS produces:
			// the upper 3x3 for row vectors (v' = v * M), which is the
			// column-major array read row by row, with the translation
			// (matrix[12..14]) in the 4th column of each row. TRS fields are
			// meaningless when has_matrix is set, so leave them at identity.
			for (int r = 0; r < 3; ++r)
			{
				for (int c = 0; c < 3; ++c)
					localMatrix[r * 4 + c] = (float) srcNode.matrix[r * 4 + c];
				localMatrix[r * 4 + 3] = (float) srcNode.matrix[12 + r];
			}

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

			ComposeTRS(translation, rotation, scale, localMatrix);
		}

		// Root until GLTFScene walks the hierarchy and hands down the parent's world matrix.
		static const float identity[12] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };
		SetParentMatrix(identity);
	}
}; // namespace Scene
