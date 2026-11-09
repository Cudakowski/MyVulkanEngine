#pragma once

#include "mpp_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace mpp
{

class MppDescriptorSetLayout {
public:
    class Builder {
    public:
        Builder(MppDevice &device) : mppDevice{device} {}

        Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1, VkDescriptorBindingFlags bindingFlags = 0, const VkSampler* pImmutableSamplers = nullptr);
        std::unique_ptr<MppDescriptorSetLayout> build() const;

    private:
        MppDevice &mppDevice;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindingsMap{};
        std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlagsMap{};
    };

    MppDescriptorSetLayout(MppDevice &device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindingsMap, std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlagsMap);
    ~MppDescriptorSetLayout();

    MppDescriptorSetLayout(const MppDescriptorSetLayout &) = delete;
    MppDescriptorSetLayout &operator=(const MppDescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    MppDevice &mppDevice;
    VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindingsMap;

    friend class MppDescriptorWriter;
};

class MppDescriptorPool 
{
public:
    class Builder 
    {
    public:
        Builder(MppDevice &device) : mppDevice{device} {}

        Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
        Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder &setMaxSets(uint32_t count);
        std::unique_ptr<MppDescriptorPool> build() const;

    private:
        MppDevice &mppDevice;
        std::vector<VkDescriptorPoolSize> poolSizes{};
        uint32_t maxSets = 1000;
        VkDescriptorPoolCreateFlags poolFlags = 0;
    };

    MppDescriptorPool(MppDevice &device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,  const std::vector<VkDescriptorPoolSize> &poolSizes);
    ~MppDescriptorPool();

    MppDescriptorPool(const MppDescriptorPool &) = delete;
    MppDescriptorPool &operator=(const MppDescriptorPool &) = delete;

    // variableCount: 0 oznacza zwykły set; >0 alokuje set z flagą variable descriptor count
    bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor, uint32_t variableDescCount = 0) const;
    void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;
    void resetPool()const;

private:
    MppDevice &mppDevice;
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

    friend class MppDescriptorWriter;
};

class MppDescriptorWriter 
{
public:
    MppDescriptorWriter(MppDescriptorSetLayout &setLayout, MppDescriptorPool &pool);

    MppDescriptorWriter &writeBuffer(uint32_t binding, const VkDescriptorBufferInfo *bufferInfo);
    MppDescriptorWriter &writeImage(uint32_t binding, const VkDescriptorImageInfo *imageInfo);

    MppDescriptorWriter &writeBufferArray(uint32_t binding, const VkDescriptorBufferInfo *bufferInfos, uint32_t count, uint32_t dstArrayElement = 0);
    MppDescriptorWriter &writeImageArray(uint32_t binding, const VkDescriptorImageInfo *imageInfos, uint32_t count, uint32_t dstArrayElement = 0);

    bool build(VkDescriptorSet &set, uint32_t variableCount = 0);
    void overwrite(VkDescriptorSet &set);

private:
    MppDescriptorSetLayout &setLayout;
    MppDescriptorPool &pool;
    std::vector<VkWriteDescriptorSet> writes;
};


} // namespace mpp
