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
Pipeline.cpp 
PipelineLayout.cpp 
RenderPass.cpp 
Semaphore.cpp 
ShaderModule.cpp 
SwapChain.cpp 
Vertex.cpp 
Sampler.cpp
Camera.cpp
Timer.cpp
Font.cpp
Editor.cpp
)

target_link_libraries(MyVulkan vulkan-1 glfw3dll ktx freetype)

add_executable(Main main.cpp)
target_link_libraries(Main MyVulkan vulkan-1 glfw3dll ktx freetype)

add_executable(Test test.cpp)
target_link_libraries(Test MyVulkan)