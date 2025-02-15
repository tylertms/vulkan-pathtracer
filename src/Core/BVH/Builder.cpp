#include "Builder.h"

#include "../Constants.h"
#include "Bounds.h"
#include "Split.h"
#include "Stats.h"

#include <algorithm>
#include <chrono>
#include <future>

namespace BVH {

void createBVH(VKPT::Mesh& mesh, VKPT::BVH& rootBVH, Vulkan::SceneManager& sceneManager) {
    computeBounds(sceneManager, rootBVH);

    uint32_t rootIndex;
    {
        std::lock_guard<std::mutex> lock(bvhAllocationMutex);
        rootIndex = sceneManager.sceneData.numBVHs++;
    }
    sceneManager.sceneStorage->bvhs[rootIndex] = rootBVH;

    auto start = std::chrono::high_resolution_clock::now();

    splitBVH(sceneManager, rootIndex, 0);

    auto end = std::chrono::high_resolution_clock::now();
    float buildTime = std::chrono::duration<float, std::milli>(end - start).count();

    collectAndPrintStats(sceneManager, rootIndex, buildTime);
}

void splitBVH(Vulkan::SceneManager& sceneManager, uint32_t bvhIndex, uint8_t depth) {
    VKPT::BVH& bvh = sceneManager.sceneStorage->bvhs[bvhIndex];

    if (bvh.triangleCount <= BVH_TRIANGLE_MIN || depth > BVH_MAX_DEPTH)
        return;

    SplitData split = chooseSplit(sceneManager, bvh);
    if (split.cost >= nodeCost(bvh.boundsMin, bvh.boundsMax, bvh.triangleCount))
        return;

    uint32_t start = bvh.index;
    uint32_t end = start + bvh.triangleCount;
    uint32_t leftCount = 0;

    for (uint32_t i = start; i < end; ++i) {
        uint32_t triIndex = sceneManager.triIndices[i];
        const glm::vec3& centroid = sceneManager.triCentroid[triIndex];

        if (centroid[split.axis] < split.pos) {
            std::swap(sceneManager.triIndices[i], sceneManager.triIndices[start + leftCount]);
            leftCount++;
        }
    }

    if (leftCount == 0 || leftCount == bvh.triangleCount)
        return;

    uint32_t leftIndex, rightIndex;
    {
        std::lock_guard<std::mutex> lock(bvhAllocationMutex);
        leftIndex = sceneManager.sceneData.numBVHs++;
        rightIndex = sceneManager.sceneData.numBVHs++;
    }

    VKPT::BVH& left = sceneManager.sceneStorage->bvhs[leftIndex];
    VKPT::BVH& right = sceneManager.sceneStorage->bvhs[rightIndex];

    left.index = start;
    left.triangleCount = leftCount;
    right.index = start + leftCount;
    right.triangleCount = bvh.triangleCount - leftCount;

    computeBounds(sceneManager, left);
    computeBounds(sceneManager, right);

    bvh.index = leftIndex;
    bvh.triangleCount = 0;

    const int taskDepthThreshold = 4;
    if (depth < taskDepthThreshold) {
        auto leftFuture = std::async(std::launch::async, [&sceneManager, leftIndex, depth]() {
            splitBVH(sceneManager, leftIndex, depth + 1);
        });
        auto rightFuture = std::async(std::launch::async, [&sceneManager, rightIndex, depth]() {
            splitBVH(sceneManager, rightIndex, depth + 1);
        });

        leftFuture.get();
        rightFuture.get();
    } else {
        splitBVH(sceneManager, leftIndex, depth + 1);
        splitBVH(sceneManager, rightIndex, depth + 1);
    }
}

} // namespace BVH
