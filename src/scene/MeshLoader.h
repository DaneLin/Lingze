#pragma once

#include "Mesh.h"
#include <memory>
#include <string>
#include <tiny_gltf.h>

namespace lz
{
/**
 * @brief Mesh loader abstract class
 */
class MeshLoader
{
  protected:
	std::string file_path;

  public:
	virtual ~MeshLoader() = default;

	// load mesh file
	virtual Mesh load() = 0;

	virtual Mesh load(const std::string &file_name) {
		file_path = file_name;
		return load();
	}

	// check if can load the file
	virtual bool can_load(const std::string &file_name) = 0;

	// set file path
	void set_file_path(const std::string &file_name) {
		file_path = file_name;
	}

	// get stored file path
	const std::string& get_file_path() const {
		return file_path;
	}
};

/**
 * @brief OBJ loader
 */
class ObjMeshLoader : public MeshLoader
{
  public:
	Mesh load() override;
	bool can_load(const std::string &file_name) override;
};

/**
 * @brief GLTF loader
 */
class GltfMeshLoader : public MeshLoader
{
  public:
	Mesh load() override;
	bool can_load(const std::string &file_name) override;

private:
	void load_materials_and_textures(const tinygltf::Model& model, Mesh& mesh);

	uint32_t material_count_{0};
};

// manager for mesh loaders
// singleton
class MeshLoaderManager
{
  public:
	static MeshLoaderManager& get_instance();

	Mesh load(const std::string &file_name);

  private:
	MeshLoaderManager();
	~MeshLoaderManager();

	std::shared_ptr<MeshLoader> get_loader(const std::string &file_name);

	std::vector<std::shared_ptr<MeshLoader>> loaders_;

};

}        // namespace lz
