#include "Scene/SceneObject.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/RenderData.h"
#include <stdexcept>
#include <utility>
#include <cmath>

namespace VulkanRenderer {
SceneObject::SceneObject(std::shared_ptr<const Mesh> mesh, uint32_t materialIndex) : _mesh(std::move(mesh)) {
	if (!_mesh || !_mesh->IsInitialized()) {
		throw std::invalid_argument("scene object requires an initialized mesh");
	}
	SetMaterialIndex(materialIndex);
}

void SceneObject::SetNodeTransform(const glm::mat4 &matrix) {
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) {
				throw std::invalid_argument("node matrix must be finite");
			}
		}
	}
	if (matrix[0][3] != 0 || matrix[1][3] != 0 || matrix[2][3] != 0 || matrix[3][3] != 1
		|| glm::determinant(glm::mat3(matrix)) == 0) {
		throw std::invalid_argument("node matrix must be affine and nonsingular");
	}
	_nodeTransform = matrix;
}

void SceneObject::SetMaterialIndex(uint32_t materialIndex) {
	if (materialIndex >= MATERIAL_CAPACITY) {
		throw std::out_of_range("material index exceeds descriptor table capacity");
	}
	_materialIndex = materialIndex;
}
} // namespace VulkanRenderer
