#include "GLTFScene.h"
#include "tiny_gltf_v3.h"

namespace Scene
{
	static std::string GetDirectory(const char *filename)
	{
		std::string path(filename);
		size_t slash = path.find_last_of("/\\");
		if (slash == std::string::npos)
			return "";
		return path.substr(0, slash + 1);
	}

	// glTF node transforms are relative to the parent node, so hand each node its parent's world
	// matrix, top down from every node that is no other node's child. Also fills each node's parent.
	static void PropagateTransforms(const tg3_model &model, std::vector<Instance> &nodes, std::vector<int32_t> &parents)
	{
		const size_t count = nodes.size();
		parents.assign(count, -1);
		std::vector<uint8_t> hasParent(count, 0);
		for (size_t n = 0; n < count; ++n)
		{
			const tg3_node &srcNode = model.nodes[n];
			for (uint32_t c = 0; c < srcNode.children_count; ++c)
			{
				const int32_t child = srcNode.children[c];
				if (child >= 0 && (size_t) child < count)
					hasParent[child] = 1;
			}
		}

		// Iterative, as hierarchies can be deep. visited guards against malformed files with cycles.
		std::vector<uint8_t> visited(count, 0);
		std::vector<int32_t> stack;
		for (size_t root = 0; root < count; ++root)
		{
			if (hasParent[root])
				continue;

			visited[root] = 1;
			stack.push_back((int32_t) root);
			while (!stack.empty())
			{
				const int32_t n = stack.back();
				stack.pop_back();

				const tg3_node &srcNode = model.nodes[n];
				for (uint32_t c = 0; c < srcNode.children_count; ++c)
				{
					const int32_t child = srcNode.children[c];
					if (child < 0 || (size_t) child >= count || visited[child])
						continue;

					visited[child] = 1;
					parents[child] = n;
					nodes[child].SetParentMatrix(nodes[n].GetWorldMatrix());
					stack.push_back(child);
				}
			}
		}
	}

	SceneError GLTFScene::MakeGLTFScene(const char *filename,
										GLTFScene &scene,
										ProgressCallback onProgress,
										void *userData)
	{
		auto reportProgress = [&](float progress, const char *stage)
		{
			if (onProgress)
				onProgress(progress, stage, userData);
		};

		tg3_parse_options opts;
		tg3_error_stack errors;
		tg3_model model;

		tg3_parse_options_init(&opts);
		tg3_error_stack_init(&errors);

		opts.memory.memory_budget = 4ull * 1024 * 1024 * 1024;

		reportProgress(0.0f, "Parsing");
		tg3_error_code err = tg3_parse_file(&model, &errors, filename, (uint32_t) strlen(filename), &opts);

		if (err != TG3_OK)
		{
			for (uint32_t i = 0; i < errors.count; i++)
			{
				fprintf(stderr,
						"[%d] %s\n",
						(int) errors.entries[i].severity,
						errors.entries[i].message ? errors.entries[i].message : "(null)");
			}

			tg3_model_free(&model);
			tg3_error_stack_free(&errors);

			return SCENE_LOAD_FAILED;
		}

		uint32_t totalItems = model.meshes_count + model.materials_count + model.nodes_count;
		uint32_t itemsDone = 0;

		reportProgress(0.1f, "Parsing");

		auto weightedProgress = [&](uint32_t doneInStage, const char *stage)
		{
			float fraction = totalItems > 0 ? (float) (itemsDone + doneInStage) / (float) totalItems : 1.0f;
			reportProgress(0.1f + fraction * 0.9f, stage);
		};

		scene.meshes.clear();
		scene.meshes.reserve(model.meshes_count);
		for (size_t m = 0; m < model.meshes_count; ++m)
		{
			weightedProgress((uint32_t) m, "Meshes");
			scene.meshes.emplace_back(model, model.meshes[m]);
		}
		itemsDone += model.meshes_count;

		std::string gltfDir = GetDirectory(filename);
		scene.materials.clear();
		scene.materials.reserve(model.materials_count);
		// Decoded once each, whatever number of materials use them.
		std::vector<Texture> images(model.images_count);
		for (size_t m = 0; m < model.materials_count; ++m)
		{
			weightedProgress((uint32_t) m, "Materials");
			scene.materials.emplace_back(model, gltfDir.c_str(), model.materials[m], images);
			// The primitives baked the first texture's mapping into their UVs (Primitive's constructor).
			if (!ReadUVTransform(model, (int32_t) m).consistent)
				fprintf(stderr,
						"Material %zu's textures use different UV sets or KHR_texture_transforms, all get the first one's\n",
						m);
		}
		itemsDone += model.materials_count;

		scene.nodes.clear();
		scene.nodes.reserve(model.nodes_count);
		for (size_t n = 0; n < model.nodes_count; ++n)
		{
			weightedProgress((uint32_t) n, "Nodes");

			const tg3_node &srcNode = model.nodes[n];
			std::string nodeName;
			if (srcNode.name.data)
				nodeName.assign(srcNode.name.data, srcNode.name.len);

			scene.nodes.emplace_back(srcNode.mesh, std::move(nodeName), srcNode);
		}
		itemsDone += model.nodes_count;

		PropagateTransforms(model, scene.nodes, scene.parents);

		scene.animations.clear();
		scene.animations.reserve(model.animations_count);
		for (size_t a = 0; a < model.animations_count; ++a)
			scene.animations.emplace_back(model, model.animations[a]);
		if (model.skins_count > 0)
			fprintf(stderr, "%u skin(s) ignored: skinning is not supported, so skinned meshes stay in their bind pose\n",
					model.skins_count);

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);

		reportProgress(1.0f, "Done");

		return SCENE_OK;
	}
}; // namespace Scene
