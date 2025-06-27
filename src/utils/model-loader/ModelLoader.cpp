#include "ModelLoader.hpp"

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/material.h"

// 确保包含GLM主头文件
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <functional> // For std::function

// The return type is changed to std::optional<ModelAttributes>
std::optional<ModelAttributes> ModelLoader::loadModelFromPath(const std::string &filePath,
                                                              Logger *logger) {
    Assimp::Importer importer;
    // Added more robust post-processing flags, common for real-time rendering.

    // Check for errors: scene is null, root node is null, or scene is marked incomplete.
    const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate | 
                                             aiProcess_FixInfacingNormals | 
                                             aiProcess_GenSmoothNormals | 
                                             aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        // Log the specific error message from Assimp.
        logger->error("Failed to load model from path: {}. Assimp error: {}", filePath,
                      importer.GetErrorString());
        return std::nullopt; // Return an empty optional to indicate failure.
    }

    ModelAttributes model;
    ModelAttributes modelAttributes;
    std::function<void(aiNode *)> processNode = [&](aiNode *node) {
        // Process all the node's meshes
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            // // Reserve space in advance to avoid multiple reallocations
            // model.vertices.reserve(model.vertices.size() + mesh->mNumVertices);
            // model.indices.reserve(model.indices.size() + mesh->mNumFaces * 3);

            SubModel subModel;

            // 提取材质和各种纹理路径
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texturePath;

            // baseColor 贴图
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                std::string basePath = texturePath.C_Str();
                if (basePath.find("textures/") == 0) basePath = basePath.substr(9);
                subModel.baseColorTexturePath = basePath;
                logger->info("Mesh {}: BaseColor Texture Path = {}", i, subModel.baseColorTexturePath);
            } else {
                subModel.baseColorTexturePath = "";
                logger->warn("Mesh {}: No BaseColor Texture found", i);
            }

            // emissive 贴图
            if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == AI_SUCCESS) {
                std::string basePath = texturePath.C_Str();
                if (basePath.find("textures/") == 0) basePath = basePath.substr(9);
                subModel.emissiveTexturePath = basePath;
                logger->info("Mesh {}: Emissive Texture Path = {}", i, subModel.emissiveTexturePath);
            } else {
                subModel.emissiveTexturePath = "";
                logger->warn("Mesh {}: No Emissive Texture found", i);
            }

            // metallicRoughness 贴图
            if (material->GetTexture(aiTextureType_UNKNOWN, 0, &texturePath) == AI_SUCCESS) {
                std::string basePath = texturePath.C_Str();
                if (basePath.find("textures/") == 0) basePath = basePath.substr(9);
                subModel.metallicRoughnessTexturePath = basePath;
                logger->info("Mesh {}: MetallicRoughness Texture Path = {}", i, subModel.metallicRoughnessTexturePath);
            } else {
                subModel.metallicRoughnessTexturePath = "";
                logger->warn("Mesh {}: No MetallicRoughness Texture found", i);
            }

            // normal 贴图
            if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
                std::string basePath = texturePath.C_Str();
                if (basePath.find("textures/") == 0) basePath = basePath.substr(9);
                subModel.normalTexturePath = basePath;
                logger->info("Mesh {}: Normal Texture Path = {}", i, subModel.normalTexturePath);
            } else {
                subModel.normalTexturePath = "";
                logger->warn("Mesh {}: No Normal Texture found", i);
            }

            // 加载顶点
            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex{};
                vertex.pos = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};

                if (mesh->HasNormals()) {
                    vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
                    // logger->info("Mesh {} Vertex {}: Normal = ({}, {}, {})", i, j, vertex.normal.x, vertex.normal.y, vertex.normal.z);
                } else {
                    vertex.normal = glm::vec3(0, 0, 1); // 默认向上
                    // logger->warn("Mesh {} Vertex {}: No normals, defaulting to (0, 0, 1)", i, j);
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
                // model.vertices.push_back(vertex);
                //     vertex.texCoord = {0, 0};
                //     logger->warn("Mesh {} Vertex {}: No UV coords, defaulting to (0, 0)", i, j);
                // }
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z, 1.0f};
                    // 手动计算手性
                    glm::vec3 bitangent = glm::vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
                    glm::vec3 crossCheck = glm::cross(vertex.normal, glm::vec3(vertex.tangent));
                    float handedness = glm::dot(crossCheck, bitangent) >= 0.0f ? 1.0f : -1.0f;
                    vertex.tangent.w = handedness;
                    // logger->info("Mesh {} Vertex {}: Tangent = ({}, {}, {}, {}), Handedness = {}", i, j, 
                                //   vertex.tangent.x, vertex.tangent.y, vertex.tangent.z, vertex.tangent.w, handedness);
                } else {
                    vertex.tangent = {1, 0, 0, 1}; // 默认X轴，手性1
                    // logger->warn("Mesh {} Vertex {}: No tangents, defaulting to (1, 0, 0, 1)", i, j);
                }
                subModel.vertices.emplace_back(vertex);
            }

            // Process indices

            // 加载索引
            for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (uint32_t k = 0; k < face.mNumIndices; k++) {
                    // model.indices.push_back(face.mIndices[k]);
                    subModel.indices.push_back(face.mIndices[k]);
                }
            }
            modelAttributes.subModels.push_back(subModel);
            logger->info("Mesh {}: Vertices = {}, Indices = {}", i, subModel.vertices.size(), subModel.indices.size());
        }
        // Recursively process each of the children nodes
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i]);
        }
    };

    processNode(scene->mRootNode);

    logger->info("New Model Loaded: {}", filePath);
    // logger->info("Vertices size: {}", model.vertices.size());
    // logger->info("Indices size: {}", model.indices.size());

    // On success, return the populated model attributes, automatically wrapped in std::optional.
    
    if (modelAttributes.subModels.empty()) {
        logger->error("No submodels loaded for model: {}", filePath);
        return std::nullopt;
    }
    logger->info("Total SubModels: {}", modelAttributes.subModels.size());
    return modelAttributes;
}