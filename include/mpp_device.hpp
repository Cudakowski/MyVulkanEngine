#pragma once

#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <vector>

namespace mpp
{
    
    

class MppDevice 
{
private:
    void initialize();
    void createInstance();
    void detectingDevice();
    void deviceSetup();
    void vmaSetup();
    void createCommandPool();

    VkInstance instance{ VK_NULL_HANDLE }; //instancja, po najechaniu myszką można zobaczyć że to wskaźnik inicjalizowany nullptr'em
    std::vector<VkPhysicalDevice> devices;
    uint32_t deviceIndex{0};// defaultowo 0
    uint32_t queueFamily{0};

    VkDevice device{ VK_NULL_HANDLE };// logical device
    VkQueue queue{ VK_NULL_HANDLE};// kolejka z komendami(cammand buffer)
    VmaAllocator allocator{ VK_NULL_HANDLE };//alokator
    VkCommandPool commandPool{ VK_NULL_HANDLE };

public:
    MppDevice();
    ~MppDevice();
    
    MppDevice(const MppDevice&) = delete;
    MppDevice& operator=(const MppDevice&) = delete;
    MppDevice(MppDevice&&) = delete;
    MppDevice& operator=(MppDevice&&) = delete;

    VkInstance getInstance(){return instance;}
    VkPhysicalDevice getPhysicalDevice(){return devices[deviceIndex];}
    VkDevice getLogicalDevice(){return device;}
    VmaAllocator getAllocator(){return allocator;}
    uint32_t getQueueFamily(){return queueFamily;}
    VkCommandPool getCommandPool(){return commandPool;}
    VkQueue getQueue(){ return queue;};

};
    
} // namespace mpp

