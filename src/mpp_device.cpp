#include "mpp_device.hpp"


#include <iostream>
#include <vector>
#include <cassert>
#include <filesystem>

namespace mpp
{

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}



MppDevice::MppDevice(MppWindow& window): mppWindow{window}
{
    checkAssets();// TODO: Trzeba to przenieśc gdzies poza to
    createInstance();
    setupDebugMessenger();
    createSurface();
    detectingDevice();
    deviceSetup();
    vmaSetup();
    createCommandPool();
}

MppDevice::~MppDevice()
{
    vkDestroyCommandPool(device_, renderingCommandPool, nullptr);
    vkDestroyCommandPool(device_, textureLoadingCommandPool, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device_, nullptr);

    if (enableValidationLayers) {
        vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr); //zakomentowac dla fun'u
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    
}

void MppDevice::createSurface() { mppWindow.createWindowSurface(instance, surface); }

void MppDevice::setupDebugMessenger() 
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void MppDevice::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();
    // COMMAND BUFFERS ==============================================================

    // buffery komend musza być zalokowane z command pool'a
    VkCommandPoolCreateInfo renderCommandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily
    };

    if(vkCreateCommandPool(device_, &renderCommandPoolCI, nullptr, &renderingCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create rendering command pool!");
    }

    VkCommandPoolCreateInfo loadingCommandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily
    };

    if(vkCreateCommandPool(device_, &loadingCommandPoolCI, nullptr, &textureLoadingCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture loading command pool!");
    }
}

void MppDevice::checkAssets()
{
    // Make sure asset folder is present from the current working directory
    if (!std::filesystem::is_directory("assets")) {
        throw std::runtime_error("Could not locate assets folder from current working directory\n");
    }
}

void MppDevice::vmaSetup()
{
    // VMA SETUP ================================

    //struct z wskaźnikami do funkcji vulkana
    VmaVulkanFunctions vkFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };
    //struct z info
    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = device_,
        .pVulkanFunctions = &vkFunctions,
        .instance = instance
    };
    //tworzenie alokatora i sprawdzanie
    if(vmaCreateAllocator(&allocatorCI, &allocator) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create allocator");
    }
}

void MppDevice::deviceSetup()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    // QUEUES ===============================================

    

    std::vector<VkDeviceQueueCreateInfo> queueCIs;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    


    // zapisujemy info tej rodziny kolejek
    // jest możliwe posiadania wielu kolejek dlatego specyfikuje 
    // się priorytet, ale tu jest ino jedna
    // const float qfpriorities{ 1.0f };
    // VkDeviceQueueCreateInfo queueCI{
    //     .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    //     .queueFamilyIndex = queueFamily,
    //     .queueCount =1,
    //     .pQueuePriorities = &qfpriorities
    // };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCIs.push_back(queueCreateInfo);
    }

    // DEVICE SETUP ======================================

    // pobieranie rozszerzeń
    // tylko swapchain żeby wyświetlić coś

    /*
    The Vulkan header defines constants for all extensions like 
    VK_KHR_SWAPCHAIN_EXTENSION_NAME. You can use these instead of 
    writing their name as a string. This helps to avoid typos in 
    extension names.
    */

    // używanie feature'ów bez rzszerzeń
    // włączenie feature'ów
    VkPhysicalDeviceVulkan12Features enabledVk12Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,// coś o indeksowaniu descriptorów
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,// Pozwala aktualizować deskryptory w locie
        .descriptorBindingPartiallyBound = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,// coś o indeksowaniu descriptorów
        .runtimeDescriptorArray = VK_TRUE,// coś o indeksowaniu descriptorów
        .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE // to to ten hehe :D
    };
    
    VkPhysicalDeviceFeatures2 enabledFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &enabledVk13Features,
        .features = {//enabledVk10Features
            .samplerAnisotropy = VK_TRUE // Filtrowanie anizotropowe (ostrzejsze tekstury pod kątem)
        }
    };

    /*
    Another Vulkan struct member you're going to see often is 
    pNext. This can be used to create a linked list of structures 
    that are passed into a function call. The driver then uses the 
    sType member of each structure in that list to identify said 
    structure's type.
    */

    // tworzenie logical device
    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabledFeatures2,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size()),
        .pQueueCreateInfos = queueCIs.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = nullptr
    };

    // might not really be necessary anymore because device specific validation layers
    // have been deprecated
    // if (enableValidationLayers) {
    //     deviceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     deviceCI.ppEnabledLayerNames = validationLayers.data();
    // } else {
    //     deviceCI.enabledLayerCount = 0;
    //     deviceCI.ppEnabledLayerNames = nullptr;
    // }

    //tworzenie i sprawdzanie od razu wyniku
    //już mnie denerwuje że nazwy nie są do końca opisujące dobrze
    // przyjmuje: physical device, create info structa, alokator, zmienną do której zapisze wynik
    if (vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    // proszenie logical device o kolejke komend
    // przyjmuje: logical device, rodzine kolejek, index kolejki, zmienną do której zapsize wskaźnik do kolejki
    vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue);
}

void MppDevice::detectingDevice()
{
    // WYKRYWANIE DEVICE =========================================


    // najpierw wykrywanie ilości urządzeń a potem samych urządzeń
    std::vector<VkPhysicalDevice> devices;
    uint32_t deviceCount{0};
    chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    if (deviceCount == 0) 
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    devices.resize(deviceCount);
    chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

    /*
    Having to call functions that return some sort of list twice 
    is common in the Vulkan C-API. The first call will return the 
    number of elements, which is then used to properly size the 
    result list. The second call then fills the actual result list.
    */

    // int argc = 1;
    // char** argv = nullptr;

    // // wybieranie urządzenia
    // // uint32_t deviceIndex{0};// defaultowo 0
    // if (argc > 1){// można podać przy uruchamianiu
    //     deviceIndex =std::stoi(argv[1]);
    //     assert(deviceIndex < deviceCount);
    // }


    for (const auto &device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }


    // pobieranie właściwości urządzenia i printowanie nazwy
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);
    std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";

    /*
    You might have noticed that VkPhysicalDeviceProperties2 and 
    vkGetPhysicalDeviceProperties2 are suffixed with a 2. 
    This is done to address shortcomings from previous versions. 
    Fixing the original functions and structures isn't an option, 
    as that would break API compatibility.
    */
}

void MppDevice::createInstance()
{
    if(volkInitialize() != VK_SUCCESS)
    {
        throw std::runtime_error("failed to initialize volk");
    }

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    //INSTANCJA ==================================================


    // uzupełnianie informacji o aplikacji
    VkApplicationInfo appInfo{// struktura z informacjami
        .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,// typ struktury
        .pApplicationName = "Ale jak to Vulkan",//nazwa aplikacji
        .apiVersion = VK_API_VERSION_1_3 // info o tym że chcemy użyć wersji vulkana 1.3
    };

    /*
    There are two extension types in Vulkan. Instance and device 
    extensions. The former are mostly global, often platform specific 
    extensions independent of your GPU, the latter are based on your 
    GPU's capabilities.
    */

    //pobieranie ilości i nazw rozszerzeń z SDL'a
    uint32_t sdlExtensionCount{0};
    // to niżej to wskaźnik do tablicy c-style stringów
    char const* const* sdlExtensions{SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount)};
    if (!sdlExtensions) {
        throw std::runtime_error("failed to get SDL Vulkan instance extensions!");
    }

    std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //info o instancji
    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,// typ structa
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,//wskaźnik do informacji o aplikacji
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),//ilość rozszerzeń
        .ppEnabledExtensionNames = extensions.data(),//nazwy rozszerzeń
    };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        instanceCI.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCI.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        //instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        instanceCI.pNext = &debugCreateInfo;
    } 

    // tworzenie instancji od razu ze sprawdzaniem
    // argumenty funkcji to: struct z infem, alokator, wskaźnik do instancji
    if(vkCreateInstance(&instanceCI, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    volkLoadInstance(instance);//co
}

bool MppDevice::checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void MppDevice::populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //added verbose
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |//added general
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;  // Optional
}

bool MppDevice::isDeviceSuitable(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices MppDevice::findQueueFamilies(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices;

    // pobieranie najpierw liczby rodzin kolejek a potem pobiranie kolejek
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    // // szukamy kolejki graficznej
    // for (size_t i{0}; i < queueFamilies.size(); i++) {
    //     if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
    //         queueFamily = i;
    //         break;
    //     }
    // }

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
            indices.graphicsFamilyHasValue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        
        if (queueFamily.queueCount > 0 && presentSupport) 
        {
            indices.presentFamily = i;
            indices.presentFamilyHasValue = true;
        }

        if (indices.isComplete()) 
        {
            break;
        }

        i++;
    }

    //sprawdzamy czy ta kolejka wspiera prezentowanie
    // if(!SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamily))
    // {
    //     throw std::runtime_error("presentation support not supported");
    // }
    /*
    Kiedy warto rozważyć SDL_Vulkan_GetPresentationSupport?
    Jedyny sensowny przypadek to sytuacja, w której z powodów 
    architektonicznych chcesz wybrać fizyczne GPU i logiczne 
    urządzenie zanim w ogóle otworzysz jakiekolwiek okno (np. 
    silnik inicjalizuje podsystem renderujący w tle, a okno 
    tworzone jest później). W praktyce gier i aplikacji okienkowych 
    zdarza się to bardzo rzadko.
    */

    return indices;
}

bool MppDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) 
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VkSurfaceCapabilitiesKHR MppDevice::getDeviceSurfaceCapabilities()
{
    VkSurfaceCapabilitiesKHR capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    return capabilities;
}

SwapChainSupportDetails MppDevice::querySwapChainSupport(VkPhysicalDevice device) 
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,surface,&presentModeCount,details.presentModes.data());
    }
    return details;
}

VkFormat MppDevice::findSupportedFormat( const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
{
    for (VkFormat format : candidates) 
    {
        VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, &formatProperties);


        if  (tiling == VK_IMAGE_TILING_LINEAR && 
            (formatProperties.formatProperties.linearTilingFeatures & features) == features) 
        {
        return format;
        } 
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && 
                (formatProperties.formatProperties.optimalTilingFeatures & features) == features) 
        {
        return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

} // namespace mpp
