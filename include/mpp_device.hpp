#pragma once

#include <vector>

namespace mpp
{
    
    

class MppDevice 
{
private:
    void createInstance();
    void detectingDevice();

    VkInstance instance{ VK_NULL_HANDLE }; //instancja, po najechaniu myszką można zobaczyć że to wskaźnik inicjalizowany nullptr'em
    std::vector<VkPhysicalDevice> devices;
    
public:
    MppDevice();
    ~MppDevice();
    
    MppDevice(const MppDevice&) = delete;
    MppDevice& operator=(const MppDevice&) = delete;
    MppDevice(MppDevice&&) = delete;
    MppDevice& operator=(MppDevice&&) = delete;
};
    
} // namespace mpp

