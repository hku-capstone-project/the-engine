#include "ModelLoader.hpp"
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/material.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

ModelAttributes ModelLoader::loadModelFromPath(const std::string &filePath, Logger *logger) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        logger->error("failed to load model from: {}", filePath);
        exit(0);
    }
    ModelAttributes modelAttributes;
    std::function<void(aiNode *)> processNode = [&](aiNode *node) {
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            SubModel subModel;
            
            // 提取材质和baseColor纹理
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                subModel.baseColorTexturePath = texturePath.C_Str();
                logger->info("Mesh {}: BaseColor Texture Path = {}", i, subModel.baseColorTexturePath);
            } else {
                subModel.baseColorTexturePath = "";
                logger->warn("Mesh {}: No BaseColor Texture found", i);
            }

            // 加载顶点
            for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
                Vertex vertex{};
                vertex.pos = {mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z};
                if (mesh->HasNormals()) {
                    vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z};
                }
                if (mesh->mTextureCoords[0]) {
                    vertex.texCoord = {mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y};
                } else {
                    vertex.texCoord = {0, 0};
                    logger->warn("Mesh {} Vertex {}: No UV coords, defaulting to (0, 0)", i, j);
                }
                if (mesh->HasTangentsAndBitangents()) {
                    vertex.tangent = {mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z, 1};
                } else {
                    vertex.tangent = {0, 0, 0, 1};
                    logger->warn("Mesh {} Vertex {}: No tangents, defaulting to (0, 0, 0, 1)", i, j);
                }
                subModel.vertices.emplace_back(vertex);
            }
            // 加载索引（无需偏移，因为每个SubModel独立）
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