#include "Instance.h"
#include "DebugMessenger.h"
#include "vulkan/vulkan_core.h"

#include <stdexcept>
#include <vector>

namespace Vulkan {

VkInstance Instance::init() {
    if (g_EnabledValidationLayers && !validationLayersSupported()) {
        throw std::runtime_error("ERROR: Validation layers not supported.");
    }

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    populateDebugMessengerCreateInfo(debugCreateInfo);

    VkApplicationInfo appInfo = getAppInfo();
    VkInstanceCreateInfo createInfo = getCreateInfo(appInfo, debugCreateInfo);

    if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
        throw std::runtime_error("ERROR: Failed to create instance.");
    }

    setupDebugMessenger(m_Instance, m_DebugMessenger);

    return m_Instance;
}

void Instance::deinit() {
    if (g_EnabledValidationLayers) {
        destroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
    }

    vkDestroyInstance(m_Instance, nullptr);
}

VkInstanceCreateInfo Instance::getCreateInfo(VkApplicationInfo appInfo, VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo) {
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    attachExtensions(createInfo);

    if (g_EnabledValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = g_ValidationLayers.data();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

#if __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    return createInfo;
}

VkApplicationInfo Instance::getAppInfo() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vk-pathtracer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NONE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    return appInfo;
}

void Instance::attachExtensions(VkInstanceCreateInfo &createInfo) {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        m_Extensions.emplace_back(glfwExtensions[i]);
    }

#if __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    m_Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    m_Extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    if (g_EnabledValidationLayers) {
        m_Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_Extensions.size());
    createInfo.ppEnabledExtensionNames = m_Extensions.data();
}

} // namespace Vulkan
