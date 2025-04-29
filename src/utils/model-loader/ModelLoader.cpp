#include "ModelLoader.hpp"
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


ModelAttributes ModelLoader::loadModelFromPath(const std::string &filePath, Logger *logger) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    logger->error("failed to load model from: {}", filePath);
    exit(0);
  }
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::function<void(aiNode*)> processNode = [&](aiNode *node) {
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      for (uint32_t j = 0; j < mesh->mNumVertices; j++) {
        Vertex vertex{};
        vertex.pos = { mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z };
        if (mesh->HasNormals()) {
          vertex.normal = { mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z };
        }
        if (mesh->mTextureCoords[0]) {
          vertex.texCoord = { mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y };
          vertex.tangent = { mesh->mTangents[j].x,  mesh->mTangents[j].y,  mesh->mTangents[j].z, 1 };
        } else {
          vertex.texCoord = {0, 0 };
        }
        vertices.emplace_back(vertex);
      }
      for (uint32_t j = 0; j < mesh->mNumFaces; j++) {
        aiFace face = mesh->mFaces[j];
        for (uint32_t k = 0; k < face.mNumIndices; k++) {
          indices.push_back(face.mIndices[k]);
        }
      }
    }
    for (uint32_t i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i]);
    }
  };
  processNode(scene->mRootNode);
  logger->info( "New Model Loaded: {}", filePath);
  logger->info("Vertices size: {}", vertices.size());
  return { vertices, indices };
}

