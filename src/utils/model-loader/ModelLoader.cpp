// ModelLoader.cpp
#include "ModelLoader.hpp"

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <functional> // For std::function

std::optional<ModelAttributes> ModelLoader::loadModelFromPath(const std::string &filePath,
                                                              Logger *logger) {
    Assimp::Importer importer;
    const unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals |
                               aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                               aiProcess_SortByPType;

    const aiScene *scene = importer.ReadFile(filePath, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        logger->error("Failed to load model from path: {}. Assimp error: {}", filePath,
                      importer.GetErrorString());
        return std::nullopt;
    }

    ModelAttributes model;
    std::function<void(aiNode *)> processNode = [&](aiNode *node) {
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            MeshAttribute meshAttr;
            uint32_t vertexStart = static_cast<uint32_t>(meshAttr.vertices.size());
            meshAttr.vertices.reserve(meshAttr.vertices.size() + mesh->mNumVertices);
            meshAttr.indices.reserve(meshAttr.indices.size() + mesh->mNumFaces * 3);

            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex{};
                vertex.pos = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};

                if (mesh->HasNormals()) {
                    vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
                }

                if (mesh->mTextureCoords[0]) {
                    vertex.texCoord = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
                    if (mesh->HasTangentsAndBitangents()) {
                        glm::vec3 t = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z};
                        glm::vec3 b = {mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z};
                        glm::vec3 n = vertex.normal;
                        float handedness = glm::dot(glm::cross(n, t), b) < 0.0f ? -1.0f : 1.0f;
                        vertex.tangent = {t.x, t.y, t.z, handedness};
                    }
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }
                meshAttr.vertices.push_back(vertex);
            }

            for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (uint32_t k = 0; k < face.mNumIndices; k++) {
                    meshAttr.indices.push_back(face.mIndices[k] + vertexStart);
                }
            }

            // Extract texture paths for this mesh's material
            aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
            std::string directory = filePath.substr(0, filePath.find_last_of('/')) + "/";
            aiString aiPath;

            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath) == AI_SUCCESS) {
                meshAttr.baseColorTexturePath = directory + aiPath.data;
            }

            if (mat->GetTexture(aiTextureType_NORMALS, 0, &aiPath) == AI_SUCCESS) {
                meshAttr.normalTexturePath = directory + aiPath.data;
            }

            if (mat->GetTexture(aiTextureType_UNKNOWN, 0, &aiPath) == AI_SUCCESS) {
                meshAttr.metallicRoughnessTexturePath = directory + aiPath.data;
            }

            if (mat->GetTexture(aiTextureType_EMISSIVE, 0, &aiPath) == AI_SUCCESS) {
                meshAttr.emissiveTexturePath = directory + aiPath.data;
            }

            model.meshes.push_back(meshAttr);
        }
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i]);
        }
    };

    processNode(scene->mRootNode);

    logger->info("New Scene Model Loaded: {}", filePath);
    logger->info("Meshes count: {}", model.meshes.size());

    return model;
}