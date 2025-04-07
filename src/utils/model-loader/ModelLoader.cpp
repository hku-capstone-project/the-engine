#include "ModelLoader.hpp"
#include "utils/logger/Logger.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

VkVertexInputBindingDescription Vertex::GetBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> Vertex::GetAttributeDescriptions() {
  std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, normal);

  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(Vertex, tangent);

  return attributeDescriptions;
}

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

