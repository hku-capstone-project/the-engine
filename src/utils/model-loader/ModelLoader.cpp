#include "ModelLoader.hpp"

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include <functional> // For std::function

// The return type is changed to std::optional<ModelAttributes>
std::optional<ModelAttributes> ModelLoader::loadModelFromPath(const std::string &filePath,
                                                              Logger *logger) {
    Assimp::Importer importer;
    // Added more robust post-processing flags, common for real-time rendering.
    const unsigned int flags = aiProcess_Triangulate | aiProcess_GenNormals |
                               aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
                               aiProcess_SortByPType;

    const aiScene *scene = importer.ReadFile(filePath, flags);

    // Check for errors: scene is null, root node is null, or scene is marked incomplete.
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        // Log the specific error message from Assimp.
        logger->error("Failed to load model from path: {}. Assimp error: {}", filePath,
                      importer.GetErrorString());
        return std::nullopt; // Return an empty optional to indicate failure.
    }

    ModelAttributes model;
    std::function<void(aiNode *)> processNode = [&](aiNode *node) {
        // Process all the node's meshes
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            // Reserve space in advance to avoid multiple reallocations
            model.vertices.reserve(model.vertices.size() + mesh->mNumVertices);
            model.indices.reserve(model.indices.size() + mesh->mNumFaces * 3);

            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex{};
                vertex.pos = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};

                if (mesh->HasNormals()) {
                    vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
                }

                // Check if the mesh has texture coordinates
                if (mesh->mTextureCoords[0]) {
                    vertex.texCoord = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
                    // Also check for tangents, which are needed for normal mapping
                    if (mesh->HasTangentsAndBitangents()) {
                        vertex.tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y,
                                          mesh->mTangents[j].z, 1.0f};
                    }
                } else {
                    vertex.texCoord = {0.0f, 0.0f};
                }
                model.vertices.push_back(vertex);
            }

            // Process indices
            for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (uint32_t k = 0; k < face.mNumIndices; k++) {
                    model.indices.push_back(face.mIndices[k]);
                }
            }
        }
        // Recursively process each of the children nodes
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i]);
        }
    };

    processNode(scene->mRootNode);

    logger->info("New Model Loaded: {}", filePath);
    logger->info("Vertices size: {}", model.vertices.size());
    logger->info("Indices size: {}", model.indices.size());

    // On success, return the populated model attributes, automatically wrapped in std::optional.
    return model;
}
