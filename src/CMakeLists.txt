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
Camera.cpp
Timer.cpp
Font.cpp
)

target_link_libraries(MyVulkan vulkan glfw freetype tinymuduo)

# add_executable(Main main.cpp)
# target_link_libraries(Main MyVulkan vulkan glfw freetype tinymuduo)

add_executable(Server Server.cpp)
target_link_libraries(Server tinymuduo)

add_executable(Client1 client1.cpp)
target_link_libraries(Client1 MyVulkan vulkan glfw freetype tinymuduo)

add_executable(Client2 client2.cpp)
target_link_libraries(Client2 MyVulkan vulkan glfw freetype tinymuduo)