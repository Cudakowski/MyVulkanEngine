#include "mpp_descriptors.hpp"

namespace mpp
{

// MppDescriptorSetLayout Builder =================================================================================================================
MppDescriptorSetLayout::Builder &MppDescriptorSetLayout::Builder::addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count, VkDescriptorBindingFlags bindingFlags, const VkSampler* pImmutableSamplers)
{
    assert(bindingsMap.count(binding) == 0 && "Binding already in use");
    VkDescriptorSetLayoutBinding layoutBinding{
        .binding = binding,
        .descriptorType = descriptorType,
        .descriptorCount = count,
        .stageFlags = stageFlags,
        .pImmutableSamplers = pImmutableSamplers,
    };
    bindingsMap[binding] = layoutBinding;
    bindingFlagsMap[binding] = bindingFlags;
    return *this;
}

std::unique_ptr<MppDescriptorSetLayout> MppDescriptorSetLayout::Builder::build() const
{
    return std::make_unique<MppDescriptorSetLayout>(mppDevice, bindingsMap, bindingFlagsMap);
}

// MppDescriptorSetLayout =================================================================================================================
MppDescriptorSetLayout::MppDescriptorSetLayout(MppDevice &device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings, std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlagsMap): mppDevice{device}, bindingsMap{bindings}
{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
    std::vector<VkDescriptorBindingFlags> setLayoutBindingFlags{};
    bool hasNonZeroFlags{false};
    // for (auto kv : bindings) {
    //     setLayoutBindings.push_back(kv.second);
    // }

    // for (uint32_t i = 0; i < bindings.size(); ++i) {
    //     if (bindings.count(i)) {
    //         setLayoutBindings.push_back(bindings[i]);
    //         VkDescriptorBindingFlags flags = bindingFlagsMap.count(i) ? bindingFlagsMap[i] : 0;
    //         setLayoutBindingFlags.push_back(flags);
    //         if (flags != 0) hasNonZeroFlags = true;
    //     }
    // }

    //jakoś nie podobają mi się te pętle ale zgaduję że nie potrzeba tu aż takiej wydajności, prawda?
    std::vector<uint32_t> bindingIndices{};
    bindingIndices.reserve(bindings.size());
    for (const auto& [idx, _] : bindings) {
        bindingIndices.push_back(idx);
    }
    std::sort(bindingIndices.begin(), bindingIndices.end());

    for (uint32_t bIndex : bindingIndices) {
        setLayoutBindings.push_back(bindings[bIndex]);
        VkDescriptorBindingFlags flags = bindingFlagsMap.count(bIndex) ? bindingFlagsMap[bIndex] : 0;
        setLayoutBindingFlags.push_back(flags);
        if (flags != 0) hasNonZeroFlags = true;
    }

    // VkDescriptorBindingFlags descVariableFlag{ 
    //     VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT|
    //     VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
    // }; // TODO: dodać VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT kiedy będzie się robić ładowanie tekstur w tracie działania programu
    
    VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlagsInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(setLayoutBindingFlags.size()),
        .pBindingFlags = setLayoutBindingFlags.data()
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = hasNonZeroFlags ? &descBindingFlagsInfo : nullptr,
        .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
        .pBindings = setLayoutBindings.data(),
    };

    if (vkCreateDescriptorSetLayout(mppDevice.getLogicalDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

MppDescriptorSetLayout::~MppDescriptorSetLayout()
{
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(mppDevice.getLogicalDevice(), descriptorSetLayout, nullptr);
    }
}


// MppDescriptorPool Builder =================================================================================================================
MppDescriptorPool::Builder &MppDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType, uint32_t count)
{
    poolSizes.push_back({descriptorType, count});
    return *this;
}

MppDescriptorPool::Builder &MppDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags)
{
    poolFlags = flags;
    return *this;
}

MppDescriptorPool::Builder &MppDescriptorPool::Builder::setMaxSets(uint32_t count)
{
    maxSets = count;
    return *this;
}

std::unique_ptr<MppDescriptorPool> MppDescriptorPool::Builder::build() const
{
    return std::make_unique<MppDescriptorPool>(mppDevice, maxSets, poolFlags, poolSizes);
}

// MppDescriptorPool =================================================================================================================
MppDescriptorPool::MppDescriptorPool(MppDevice &device, uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,  const std::vector<VkDescriptorPoolSize> &poolSizes): mppDevice{device}
{
    VkDescriptorPoolCreateInfo descriptorPoolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = poolFlags,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    if (vkCreateDescriptorPool(mppDevice.getLogicalDevice(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

MppDescriptorPool::~MppDescriptorPool()
{
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(mppDevice.getLogicalDevice(), descriptorPool, nullptr);
    }
}

bool MppDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor, uint32_t variableDescCount) const
{
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount =1,
        .pDescriptorCounts = &variableDescCount
    };

    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = (variableDescCount > 0) ? &variableDescCountAI : nullptr, // Jeśli 0, nie podpinamy struktury,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout,
    };

    // TODO: Might want to create a "DescriptorPoolManager" class that handles this case, and builds
    // a new pool whenever an old pool fills up. But this is beyond our current scope
    if (vkAllocateDescriptorSets(mppDevice.getLogicalDevice(), &allocInfo, &descriptor) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void MppDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const
{
    vkFreeDescriptorSets(mppDevice.getLogicalDevice(), descriptorPool, static_cast<uint32_t>(descriptors.size()), descriptors.data());
}

void MppDescriptorPool::resetPool()
{
    vkResetDescriptorPool(mppDevice.getLogicalDevice(), descriptorPool, 0);
}

// MppDescriptorWriter =================================================================================================================
MppDescriptorWriter::MppDescriptorWriter(MppDescriptorSetLayout &setLayout, MppDescriptorPool &pool): setLayout{setLayout}, pool{pool}
{

}

MppDescriptorWriter &MppDescriptorWriter::writeBuffer(uint32_t binding, const VkDescriptorBufferInfo *bufferInfo)
{
    return writeBufferArray(binding, bufferInfo, 1, 0);
}

MppDescriptorWriter &MppDescriptorWriter::writeImage(uint32_t binding, const VkDescriptorImageInfo *imageInfo)
{
    return writeImageArray(binding, imageInfo, 1, 0);
}

MppDescriptorWriter &MppDescriptorWriter::writeBufferArray(uint32_t binding, const VkDescriptorBufferInfo *bufferInfos, uint32_t count, uint32_t dstArrayElement)
{
    assert(setLayout.bindingsMap.count(binding) == 1 && "Layout does not contain specified binding");
    auto &bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // Zostanie uzupełnione w overwrite()
        .dstBinding = binding,
        .dstArrayElement = dstArrayElement,
        .descriptorCount = count,
        .descriptorType = bindingDescription.descriptorType,
        .pBufferInfo = bufferInfos,
    };

    writes.push_back(write);
    return *this;

    /* Odnośnie .dstArrayElement :
    
    Gdy masz tablicę w shaderze: Sampler2D textures[100];
    Jeśli ładujesz pierwsze 5 tekstur przy starcie gry: dstArrayElement = 0, descriptorCount = 5.
    Jeśli w trakcie gry doładujesz szóstą teksturę: nie musisz nadpisywać całej tablicy! Podajesz 
    pojedynczą nową teksturę z dstArrayElement = 5, descriptorCount = 1. To pozwala na punktowe aktualizowanie tablic bez dotykania istniejących slotów.
    */
}

//UWAGA: to na co wskazuje imageInfos musi istnieć w momencie wywołania .build(najlepiej wykonywać wszystko łańcuchem wywołań)
MppDescriptorWriter &MppDescriptorWriter::writeImageArray(uint32_t binding, const VkDescriptorImageInfo *imageInfos, uint32_t count, uint32_t dstArrayElement)
{
    assert(setLayout.bindingsMap.count(binding) == 1 && "Layout does not contain specified binding");
    auto &bindingDescription = setLayout.bindingsMap[binding];

    VkWriteDescriptorSet write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // Zostanie uzupełnione w overwrite()
        .dstBinding = binding,
        .dstArrayElement = dstArrayElement,
        .descriptorCount = count,
        .descriptorType = bindingDescription.descriptorType,
        .pImageInfo = imageInfos,
    };

    writes.push_back(write);
    return *this;
}

bool MppDescriptorWriter::build(VkDescriptorSet &set, uint32_t variableCount)
{
    bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set, variableCount);
    if (!success) {
        return false;
    }
    overwrite(set);
    return true;
}

void MppDescriptorWriter::overwrite(VkDescriptorSet &set)
{
    for (auto &write : writes) {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(pool.mppDevice.getLogicalDevice(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

}

} // namespace mpp
