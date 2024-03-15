add_library(MyVulkan 
Vulkan.cpp 
Buffer.cpp 
CommandBuffer.cpp 
CommandPool.cpp 
DescriptorPool.cpp 
DescriptorSetLayout.cpp 
Fence.cpp 
FrameBuffer.cpp 
Image.cpp 
ImageView.cpp 
Memory.cpp 
Pipeline.cpp 
PipelineLayout.cpp 
RenderPass.cpp 
Semaphore.cpp 
ShaderModule.cpp 
SwapChain.cpp 
Vertex.cpp 
Sampler.cpp
)

target_link_libraries(MyVulkan vulkan-1 glfw3dll ktx)

add_executable(Main main.cpp)
target_link_libraries(Main MyVulkan vulkan-1 glfw3dll ktx)