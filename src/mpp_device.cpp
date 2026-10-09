#include "mpp_device.hpp"

#include "mpp_checks.hpp"

#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <iostream>
#include <vector>
#include <cassert>
#include <filesystem>

namespace mpp
{

MppDevice::MppDevice()
{
    initialize();
    createInstance();
    detectingDevice();
    deviceSetup();
    vmaSetup();
    createCommandPool();
}

MppDevice::~MppDevice()
{
    
}

void MppDevice::createCommandPool()
{
    // COMMAND BUFFERS ==============================================================

    // buffery komend musza być zalokowane z command pool'a
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily
    };
    chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
}

void MppDevice::initialize()
{
    // Make sure asset folder is present from the current working directory
	if (!std::filesystem::is_directory("assets")) {
		std::cerr << "Could not locate assets folder from current working directory\n";
		exit(-1);
	}
	chk(SDL_Init(SDL_INIT_VIDEO));
	chk(SDL_Vulkan_LoadLibrary(NULL));
	volkInitialize();
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
        .physicalDevice = devices[deviceIndex],
        .device = device,
        .pVulkanFunctions = &vkFunctions,
        .instance = instance
    };
    //tworzenie alokatora i sprawdzanie
    chk(vmaCreateAllocator(&allocatorCI, &allocator));
}

void MppDevice::deviceSetup()
{
    // QUEUES ===============================================

    // pobieranie najpierw liczby rodzin kolejek a potem pobiranie kolejek
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
    
    // szukamy kolejki graficznej
    for (size_t i{0}; i < queueFamilies.size(); i++) {
        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            queueFamily = i;
            break;
        }
    }
    //sprawdzamy czy ta kolejka wspiera prezentowanie
    chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex], queueFamily));

    // zapisujemy info tej rodziny kolejek
    // jest możliwe posiadania wielu kolejek dlatego specyfikuje 
    // się priorytet, ale tu jest ino jedna
    const float qfpriorities{ 1.0f };
    VkDeviceQueueCreateInfo queueCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount =1,
        .pQueuePriorities = &qfpriorities
    };

    // DEVICE SETUP ======================================

    // pobieranie rozszerzeń
    // tylko swapchain żeby wyświetlić coś
    const std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,// coś o indeksowaniu descriptorów
        .descriptorBindingVariableDescriptorCount = true,// coś o indeksowaniu descriptorów
        .runtimeDescriptorArray = true,// coś o indeksowaniu descriptorów
        .bufferDeviceAddress = true
    };

    VkPhysicalDeviceVulkan13Features enabledVk13Features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabledVk12Features,
        .synchronization2 = true,
        .dynamicRendering = true // to to ten hehe :D
    };
    
    VkPhysicalDeviceFeatures enabledVk10Features{
        .samplerAnisotropy = VK_TRUE // lepsze filterowanie tekstur(cokolwiek to znaczy)
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
        .pNext = &enabledVk13Features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &enabledVk10Features
    };
    //tworzenie i sprawdzanie od razu wyniku
    //już mnie denerwuje że nazwy nie są do końca opisujące dobrze
    // przyjmuje: physical device, create info structa, alokator, zmienną do której zapisze wynik
    chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));

    // proszenie logical device o kolejke komend
    // przyjmuje: logical device, rodzine kolejek, index kolejki, zmienną do której zapsize wskaźnik do kolejki
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
}

void MppDevice::detectingDevice()
{
    // WYKRYWANIE DEVICE =========================================


    // najpierw wykrywanie ilości urządzeń a potem samych urządzeń
    uint32_t deviceCount{0};
    chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    devices.resize(deviceCount);
    chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

    /*
    Having to call functions that return some sort of list twice 
    is common in the Vulkan C-API. The first call will return the 
    number of elements, which is then used to properly size the 
    result list. The second call then fills the actual result list.
    */

    int argc = 1;
    char** argv = nullptr;

    // wybieranie urządzenia
    // uint32_t deviceIndex{0};// defaultowo 0
    if (argc > 1){// można podać przy uruchamianiu
        deviceIndex =std::stoi(argv[1]);
        assert(deviceIndex < deviceCount);
    }

//     void SspDevice::pickPhysicalDevice() {
//   uint32_t deviceCount = 0;
//   vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
//   if (deviceCount == 0) {
//     throw std::runtime_error("failed to find GPUs with Vulkan support!");
//   }
//   std::cout << "Device count: " << deviceCount << std::endl;
//   std::vector<VkPhysicalDevice> devices(deviceCount);
//   vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

//   for (const auto &device : devices) {
//     if (isDeviceSuitable(device)) {
//       physicalDevice = device;
//       break;
//     }
//   }

//   if (physicalDevice == VK_NULL_HANDLE) {
//     throw std::runtime_error("failed to find a suitable GPU!");
//   }

//   vkGetPhysicalDeviceProperties(physicalDevice, &properties);
//   std::cout << "physical device: " << properties.deviceName << std::endl;
// }

    // pobieranie właściwości urządzenia i printowanie nazwy
    VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
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
    uint32_t instanceExtensionsCount{0};
    // to niżej to wskaźnik do tablicy c-style stringów
    char const* const* instanceExtensions{SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};
    
    //info o instancji
    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,// typ structa
        .pApplicationInfo = &appInfo,//wskaźnik do informacji o aplikacji
        .enabledExtensionCount = instanceExtensionsCount,//ilość rozszerzeń
        .ppEnabledExtensionNames = instanceExtensions,//nazwy rozszerzeń
    };
    // tworzenie instancji od razu ze sprawdzaniem
    // argumenty funkcji to: struct z infem, alokator, wskaźnik do instancji
    chk(vkCreateInstance(&instanceCI, nullptr, &instance));

	volkLoadInstance(instance);//co
}

} // namespace mpp
