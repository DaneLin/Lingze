#pragma once

#include "LzMesh.h"
#include <memory>
#include <string>

namespace lz
{
/**
 * @brief LzMesh loader abstract class
 */
class LzMeshLoader
{
  public:
	virtual ~LzMeshLoader() = default;

	// load mesh file
	virtual LzMesh load(const std::string &file_name) = 0;

	// check if can load the file
	virtual bool can_load(const std::string &file_name) = 0;

	// get the loader for the specified file
	static std::shared_ptr<LzMeshLoader> get_loader(const std::string &file_name);
};

/**
 * @brief OBJ loader
 */
class LzObjLoader : public LzMeshLoader
{
  public:
	LzMesh load(const std::string &file_name) override;
	bool   can_load(const std::string &file_name) override;
};

/**
 * @brief GLTF loader
 */
class LzGltfLoader : public LzMeshLoader
{
  public:
	LzMesh load(const std::string &file_name) override;
	bool   can_load(const std::string &file_name) override;
};
}        // namespace lz
