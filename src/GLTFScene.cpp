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
		for (size_t m = 0; m < model.materials_count; ++m)
		{
			weightedProgress((uint32_t) m, "Materials");
			scene.materials.emplace_back(model, gltfDir.c_str(), model.materials[m]);
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

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);

		reportProgress(1.0f, "Done");

		return SCENE_OK;
	}
}; // namespace Scene
