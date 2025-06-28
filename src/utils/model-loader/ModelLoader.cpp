#include "ModelLoader.hpp"
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/material.h"

// 确保包含GLM主头文件
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

ModelAttributes ModelLoader::loadModelFromPath(const std::string &filePath, Logger *logger) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate | 
                                             aiProcess_FixInfacingNormals | 
                                             aiProcess_GenSmoothNormals | 
                                             aiProcess_CalcTangentSpace | 
                                             aiProcess_SplitLargeMeshes | 
                                             aiProcess_SortByPType);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        logger->error("failed to load model from: {}", filePath);
        exit(0);
    }
    ModelAttributes modelAttributes;
    std::function<void(aiNode *)> processNode = [&](aiNode *node) {
        logger->info("Node {} Meshes: {}", node->mName.C_Str(), node->mNumMeshes);
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
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
            if (material->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
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
                } else {
                    vertex.normal = glm::vec3(0, 0, 1); // 默认向上
                    logger->warn("Mesh {} Vertex {}: No normals, defaulting to (0, 0, 1)", i, j);
                }
                if (mesh->mTextureCoords[0]) {
                    vertex.texCoord = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
                } else {
                    vertex.texCoord = {0, 0};
                    logger->warn("Mesh {} Vertex {}: No UV coords, defaulting to (0, 0)", i, j);
                }
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z, 1.0f};
                    glm::vec3 bitangent = glm::vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
                    glm::vec3 crossCheck = glm::cross(vertex.normal, glm::vec3(vertex.tangent));
                    float handedness = glm::dot(crossCheck, bitangent) >= 0.0f ? 1.0f : -1.0f;
                    vertex.tangent.w = handedness;
                } else {
                    vertex.tangent = {1, 0, 0, 1}; // 默认X轴，手性1
                    logger->warn("Mesh {} Vertex {}: No tangents, defaulting to (1, 0, 0, 1)", i, j);
                }
                subModel.vertices.emplace_back(vertex);
            }

            // 加载索引
            for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (uint32_t k = 0; k < face.mNumIndices; k++) {
                    subModel.indices.push_back(face.mIndices[k]);
                }
            }
            modelAttributes.subModels.push_back(subModel);
            logger->info("Mesh {}: Vertices = {}, Indices = {}", i, subModel.vertices.size(), subModel.indices.size());
        }
        for (uint32_t i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i]);
        }
    };
    processNode(scene->mRootNode);
    logger->info("New Model Loaded: {}", filePath);
    logger->info("Total SubModels: {}", modelAttributes.subModels.size());
    return modelAttributes;
}