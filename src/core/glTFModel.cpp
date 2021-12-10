//
// Created by magnus on 12/10/21.
//

#include "glTFModel.h"



void glTFModel::Model::drawNode(Node *node, VkCommandBuffer commandBuffer) {
    if (node->mesh) {
        for (Primitive *primitive: node->mesh->primitives) {
            vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
        }
    }
    for (auto &child: node->children) {
        drawNode(child, commandBuffer);
    }
}

void glTFModel::Model::draw(VkCommandBuffer commandBuffer) {
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    for (auto &node: nodes) {
        drawNode(node, commandBuffer);
    }
}

glTFModel::Node *glTFModel::Model::findNode(Node *parent, uint32_t index) {
    Node *nodeFound = nullptr;
    if (parent->index == index) {
        return parent;
    }
    for (auto &child: parent->children) {
        nodeFound = findNode(child, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}

glTFModel::Node *glTFModel::Model::nodeFromIndex(uint32_t index) {
    Node *nodeFound = nullptr;
    for (auto &node: nodes) {
        nodeFound = findNode(node, index);
        if (nodeFound) {
            break;
        }
    }
    return nodeFound;
}