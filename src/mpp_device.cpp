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

namespace mpp
{

MppDevice::MppDevice()
{
    createInstance();
}

MppDevice::~MppDevice()
{
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
    uint32_t deviceIndex{0};// defaultowo 0
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
