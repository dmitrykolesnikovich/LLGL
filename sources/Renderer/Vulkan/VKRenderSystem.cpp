/*
 * VKRenderSystem.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include <LLGL/Platform/Platform.h>
#include "VKRenderSystem.h"
#include "Ext/VKExtensionLoader.h"
#include "Ext/VKExtensions.h"
#include "Buffer/VKVertexBuffer.h"
#include "../CheckedCast.h"
#include "../../Core/Helper.h"
#include "../../Core/Vendor.h"
#include "../GLCommon/GLTypes.h"
#include "VKCore.h"
#include <LLGL/Log.h>


namespace LLGL
{


/* ----- Internal functions ----- */

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* createInfo, const VkAllocationCallbacks* allocator, VkDebugReportCallbackEXT* callback)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    if (func != nullptr)
        return func(instance, createInfo, allocator, callback);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* allocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
    if (func != nullptr)
        func(instance, callback, allocator);
}


/* ----- Common ----- */

static const std::vector<const char*> g_deviceExtensions
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VKRenderSystem::VKRenderSystem() :
    instance_            { vkDestroyInstance                        },
    device_              { vkDestroyDevice                          },
    debugReportCallback_ { instance_, DestroyDebugReportCallbackEXT }
{
    //TODO: get application descriptor from client programmer
    ApplicationDescriptor appDesc;
    appDesc.applicationName     = "LLGL-App";
    appDesc.applicationVersion  = 1;
    appDesc.engineName          = "LLGL";
    appDesc.engineVersion       = 1;

    #ifdef LLGL_DEBUG
    debugLayerEnabled_ = true;
    #endif

    CreateInstance(appDesc);
    LoadExtensions();

    if (!PickPhysicalDevice())
        throw std::runtime_error("failed to find physical device with Vulkan support");

    QueryDeviceProperties();
    CreateLogicalDevice();
}

VKRenderSystem::~VKRenderSystem()
{
}

/* ----- Render Context ----- */

RenderContext* VKRenderSystem::CreateRenderContext(const RenderContextDescriptor& desc, const std::shared_ptr<Surface>& surface)
{
    return TakeOwnership(renderContexts_, MakeUnique<VKRenderContext>(instance_, physicalDevice_, device_, desc, surface));
}

void VKRenderSystem::Release(RenderContext& renderContext)
{
    RemoveFromUniqueSet(renderContexts_, &renderContext);
}

/* ----- Command buffers ----- */

CommandBuffer* VKRenderSystem::CreateCommandBuffer()
{
    auto mainContext = renderContexts_.begin()->get();
    return TakeOwnership(
        commandBuffers_,
        MakeUnique<VKCommandBuffer>(device_, mainContext->GetSwapChainSize(), queueFamilyIndices_)
    );
}

void VKRenderSystem::Release(CommandBuffer& commandBuffer)
{
    RemoveFromUniqueSet(commandBuffers_, &commandBuffer);
}

/* ----- Buffers ------ */

Buffer* VKRenderSystem::CreateBuffer(const BufferDescriptor& desc, const void* initialData)
{
    AssertCreateBuffer(desc, static_cast<uint64_t>(std::numeric_limits<VkDeviceSize>::max()));

    switch (desc.type)
    {
        case BufferType::Vertex:
        {
            /* Create vertex buffer */
            VkBufferCreateInfo createInfo;
            {
                createInfo.sType                    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                createInfo.pNext                    = nullptr;
                createInfo.flags                    = 0;
                createInfo.size                     = static_cast<VkDeviceSize>(desc.size);
                createInfo.usage                    = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                createInfo.sharingMode              = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount    = 0;
                createInfo.pQueueFamilyIndices      = nullptr;
            }
            return TakeOwnership(buffers_, MakeUnique<VKVertexBuffer>(device_, createInfo));
        }
        break;

        default:
        break;
    }
    return nullptr;
}

BufferArray* VKRenderSystem::CreateBufferArray(unsigned int numBuffers, Buffer* const * bufferArray)
{
    return nullptr;//todo
}

void VKRenderSystem::Release(Buffer& buffer)
{
    //todo
}

void VKRenderSystem::Release(BufferArray& bufferArray)
{
    //todo
}

void VKRenderSystem::WriteBuffer(Buffer& buffer, const void* data, std::size_t dataSize, std::size_t offset)
{
    //todo
}

void* VKRenderSystem::MapBuffer(Buffer& buffer, const BufferCPUAccess access)
{
    return nullptr;//todo
}

void VKRenderSystem::UnmapBuffer(Buffer& buffer)
{
    //todo
}

/* ----- Textures ----- */

Texture* VKRenderSystem::CreateTexture(const TextureDescriptor& textureDesc, const ImageDescriptor* imageDesc)
{
    return nullptr;//todo
}

TextureArray* VKRenderSystem::CreateTextureArray(unsigned int numTextures, Texture* const * textureArray)
{
    return nullptr;//todo
}

void VKRenderSystem::Release(Texture& texture)
{
    //todo
}

void VKRenderSystem::Release(TextureArray& textureArray)
{
    //todo
}

TextureDescriptor VKRenderSystem::QueryTextureDescriptor(const Texture& texture)
{
    return {};//todo
}

void VKRenderSystem::WriteTexture(Texture& texture, const SubTextureDescriptor& subTextureDesc, const ImageDescriptor& imageDesc)
{
    //todo
}

void VKRenderSystem::ReadTexture(const Texture& texture, int mipLevel, ImageFormat imageFormat, DataType dataType, void* buffer)
{
    //todo
}

void VKRenderSystem::GenerateMips(Texture& texture)
{
    //todo
}

/* ----- Sampler States ---- */

Sampler* VKRenderSystem::CreateSampler(const SamplerDescriptor& desc)
{
    return TakeOwnership(samplers_, MakeUnique<VKSampler>(device_, desc));
}

SamplerArray* VKRenderSystem::CreateSamplerArray(unsigned int numSamplers, Sampler* const * samplerArray)
{
    return TakeOwnership(samplerArrays_, MakeUnique<VKSamplerArray>(numSamplers, samplerArray));
}

void VKRenderSystem::Release(Sampler& sampler)
{
    RemoveFromUniqueSet(samplers_, &sampler);
}

void VKRenderSystem::Release(SamplerArray& samplerArray)
{
    RemoveFromUniqueSet(samplerArrays_, &samplerArray);
}

/* ----- Render Targets ----- */

RenderTarget* VKRenderSystem::CreateRenderTarget(const RenderTargetDescriptor& desc)
{
    return nullptr;//todo
}

void VKRenderSystem::Release(RenderTarget& renderTarget)
{
    //todo
}

/* ----- Shader ----- */

Shader* VKRenderSystem::CreateShader(const ShaderType type)
{
    return TakeOwnership(shaders_, MakeUnique<VKShader>(device_, type));
}

ShaderProgram* VKRenderSystem::CreateShaderProgram()
{
    return TakeOwnership(shaderPrograms_, MakeUnique<VKShaderProgram>());
}

void VKRenderSystem::Release(Shader& shader)
{
    RemoveFromUniqueSet(shaders_, &shader);
}

void VKRenderSystem::Release(ShaderProgram& shaderProgram)
{
    RemoveFromUniqueSet(shaderPrograms_, &shaderProgram);
}

/* ----- Pipeline States ----- */

GraphicsPipeline* VKRenderSystem::CreateGraphicsPipeline(const GraphicsPipelineDescriptor& desc)
{
    if (renderContexts_.empty())
        throw std::runtime_error("cannot create graphics pipeline without a render context");

    auto renderContext = renderContexts_.begin()->get();
    auto renderPassVK = renderContext->GetSwapChainRenderPass().Get();

    return TakeOwnership(graphicsPipelines_, MakeUnique<VKGraphicsPipeline>(device_, renderPassVK, desc));
}

ComputePipeline* VKRenderSystem::CreateComputePipeline(const ComputePipelineDescriptor& desc)
{
    return nullptr;//todo
}

void VKRenderSystem::Release(GraphicsPipeline& graphicsPipeline)
{
    RemoveFromUniqueSet(graphicsPipelines_, &graphicsPipeline);
}

void VKRenderSystem::Release(ComputePipeline& computePipeline)
{
    //todo
}

/* ----- Queries ----- */

Query* VKRenderSystem::CreateQuery(const QueryDescriptor& desc)
{
    return nullptr;//todo
}

void VKRenderSystem::Release(Query& query)
{
    //todo
}


/*
 * ======= Private: =======
 */

void VKRenderSystem::CreateInstance(const ApplicationDescriptor& appDesc)
{
    /* Setup application descriptor */
    VkApplicationInfo appInfo;
    
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext               = nullptr;
    appInfo.pApplicationName    = appDesc.applicationName.c_str();
    appInfo.applicationVersion  = appDesc.applicationVersion;
    appInfo.pEngineName         = appDesc.engineName.c_str();
    appInfo.engineVersion       = appDesc.engineVersion;
    appInfo.apiVersion          = VK_API_VERSION_1_0;

    /* Query instance layer properties */
    auto layerProperties = VKQueryInstanceLayerProperties();
    std::vector<const char*> layerNames;

    for (const auto& prop : layerProperties)
    {
        if (IsLayerRequired(prop.layerName))
            layerNames.push_back(prop.layerName);
    }

    /* Query instance extension properties */
    auto extensionProperties = VKQueryInstanceExtensionProperties();
    std::vector<const char*> extensionNames;

    for (const auto& prop : extensionProperties)
    {
        if (IsExtensionRequired(prop.extensionName))
            extensionNames.push_back(prop.extensionName);
    }

    /* Setup Vulkan instance descriptor */
    VkInstanceCreateInfo instanceInfo;
    
    instanceInfo.sType                          = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext                          = nullptr;
    instanceInfo.flags                          = 0;
    instanceInfo.pApplicationInfo               = (&appInfo);

    if (layerNames.empty())
    {
        instanceInfo.enabledLayerCount          = 0;
        instanceInfo.ppEnabledLayerNames        = nullptr;
    }
    else
    {
        instanceInfo.enabledLayerCount          = static_cast<std::uint32_t>(layerNames.size());
        instanceInfo.ppEnabledLayerNames        = layerNames.data();
    }

    if (extensionNames.empty())
    {
        instanceInfo.enabledExtensionCount      = 0;
        instanceInfo.ppEnabledExtensionNames    = nullptr;
    }
    else
    {
        instanceInfo.enabledExtensionCount      = static_cast<std::uint32_t>(extensionNames.size());
        instanceInfo.ppEnabledExtensionNames    = extensionNames.data();
    }

    /* Create Vulkan instance */
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, instance_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan instance");

    if (debugLayerEnabled_)
        CreateDebugReportCallback();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VKDebugCallback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
    uint64_t object, size_t location, int32_t messageCode,
    const char* layerPrefix, const char* message, void* userData)
{
    //auto renderSystemVK = reinterpret_cast<VKRenderSystem*>(userData);
    Log::StdErr() << message << std::endl;
    return VK_FALSE;
}

void VKRenderSystem::CreateDebugReportCallback()
{
    /* Initialize debug report callback descriptor */
    VkDebugReportCallbackCreateInfoEXT createInfo;

    createInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createInfo.pNext        = nullptr;
    createInfo.flags        = 0;
    createInfo.pfnCallback  = VKDebugCallback;
    createInfo.pUserData    = reinterpret_cast<void*>(this);

    /* Create report callback */
    VkResult result = CreateDebugReportCallbackEXT(instance_, &createInfo, nullptr, debugReportCallback_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan debug report callback");
}

void VKRenderSystem::LoadExtensions()
{
    LoadAllExtensions(instance_);
}

bool VKRenderSystem::PickPhysicalDevice()
{
    /* Query all physical devices and pick suitable */
    auto devices = VKQueryPhysicalDevices(instance_);

    for (const auto& dev : devices)
    {
        if (IsPhysicalDeviceSuitable(dev))
        {
            physicalDevice_ = dev;
            return true;
        }
    }

    return false;
}

void VKRenderSystem::QueryDeviceProperties()
{
    /* Query physical memory propertiers */
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryPropertiers_);

    /* Query properties of selected physical device */
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

    /* Map properties to output renderer info */
    RendererInfo info;

    info.rendererName           = "Vulkan " + VKApiVersionToString(properties.apiVersion);
    info.deviceName             = std::string(properties.deviceName);
    info.vendorName             = GetVendorByID(properties.vendorID);
    info.shadingLanguageName    = "SPIR-V";

    SetRendererInfo(info);

    /* Map limits to output rendering capabilites */
    const auto& limits = properties.limits;

    RenderingCaps caps;

    caps.screenOrigin                       = ScreenOrigin::UpperLeft;
    caps.clippingRange                      = ClippingRange::ZeroToOne;
    caps.shadingLanguage                    = ShadingLanguage::SPIRV_100;
    caps.hasRenderTargets                   = true;
    caps.has3DTextures                      = true;
    caps.hasCubeTextures                    = true;
    caps.hasTextureArrays                   = true;
    caps.hasCubeTextureArrays               = true;
    caps.hasMultiSampleTextures             = true;
    caps.hasSamplers                        = true;
    caps.hasConstantBuffers                 = true;
    caps.hasStorageBuffers                  = true;
    caps.hasUniforms                        = true;
    caps.hasGeometryShaders                 = true;
    caps.hasTessellationShaders             = true;
    caps.hasComputeShaders                  = true;
    caps.hasInstancing                      = true;
    caps.hasOffsetInstancing                = true;
    caps.hasViewportArrays                  = true;
    caps.hasConservativeRasterization       = true;
    caps.hasStreamOutputs                   = true;
    caps.maxNumTextureArrayLayers           = limits.maxImageArrayLayers;
    caps.maxNumRenderTargetAttachments      = static_cast<std::uint32_t>(limits.framebufferColorSampleCounts);
    caps.maxConstantBufferSize              = 0; //???
    caps.maxPatchVertices                   = limits.maxTessellationPatchSize;
    caps.max1DTextureSize                   = limits.maxImageDimension1D;
    caps.max2DTextureSize                   = limits.maxImageDimension2D;
    caps.max3DTextureSize                   = limits.maxImageDimension3D;
    caps.maxCubeTextureSize                 = limits.maxImageDimensionCube;
    caps.maxAnisotropy                      = static_cast<std::uint32_t>(limits.maxSamplerAnisotropy);
    caps.maxNumComputeShaderWorkGroups[0]   = limits.maxComputeWorkGroupCount[0];
    caps.maxNumComputeShaderWorkGroups[1]   = limits.maxComputeWorkGroupCount[1];
    caps.maxNumComputeShaderWorkGroups[2]   = limits.maxComputeWorkGroupCount[2];
    caps.maxComputeShaderWorkGroupSize[0]   = limits.maxComputeWorkGroupSize[0];
    caps.maxComputeShaderWorkGroupSize[1]   = limits.maxComputeWorkGroupSize[1];
    caps.maxComputeShaderWorkGroupSize[2]   = limits.maxComputeWorkGroupSize[2];

    SetRenderingCaps(caps);
}

void VKRenderSystem::CreateLogicalDevice()
{
    /* Initialize queue create description */
    queueFamilyIndices_ = VKFindQueueFamilies(physicalDevice_, (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT));

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices_.graphicsFamily, queueFamilyIndices_.presentFamily };

    float queuePriority = 1.0f;
    for (auto family : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo info;
        {
            info.sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext              = nullptr;
            info.flags              = 0;
            info.queueFamilyIndex   = family;
            info.queueCount         = 1;
            info.pQueuePriorities   = &queuePriority;
        }
        queueCreateInfos.push_back(info);
    }

    /* Initialize device features */
    VkPhysicalDeviceFeatures deviceFeatures;
    InitMemory(deviceFeatures);

    /* Initialize create descriptor for logical device */
    VkDeviceCreateInfo createInfo;

    createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext                    = nullptr;
    createInfo.flags                    = 0;
    createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos        = queueCreateInfos.data();
    createInfo.enabledLayerCount        = 0;
    createInfo.ppEnabledLayerNames      = nullptr;
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(g_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames  = g_deviceExtensions.data();
    createInfo.pEnabledFeatures         = &deviceFeatures;

    /* Create logical device */
    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, device_.ReleaseAndGetAddressOf());
    VKThrowIfFailed(result, "failed to create Vulkan logical device");
}

bool VKRenderSystem::IsLayerRequired(const std::string& name) const
{
    return (name == "VK_LAYER_NV_optimus");
}

bool VKRenderSystem::IsExtensionRequired(const std::string& name) const
{
    return
    (
        name == VK_KHR_SURFACE_EXTENSION_NAME
        #ifdef LLGL_OS_WIN32
        || name == VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        #endif
        #ifdef LLGL_OS_LINUX
        || name == VK_KHR_XLIB_SURFACE_EXTENSION_NAME
        #endif
        || (debugLayerEnabled_ && name == VK_EXT_DEBUG_REPORT_EXTENSION_NAME)
    );
}

bool VKRenderSystem::IsPhysicalDeviceSuitable(VkPhysicalDevice device) const
{
    if (CheckDeviceExtensionSupport(device, g_deviceExtensions))
    {
        //TODO...
        return true;
    }
    return false;
}

bool VKRenderSystem::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensionNames) const
{
    /* Check if device supports all required extensions */
    auto availableExtensions = VKQueryDeviceExtensionProperties(device);

    std::set<std::string> requiredExtensions(extensionNames.begin(), extensionNames.end());

    for (const auto& ext : availableExtensions)
        requiredExtensions.erase(ext.extensionName);

    return requiredExtensions.empty();
}


} // /namespace LLGL



// ================================================================================