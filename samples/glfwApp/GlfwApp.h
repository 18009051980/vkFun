#ifndef __GlfwApp_H__
#define __GlfwApp_H__

#include <VulkanEnv.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <chrono>   
#include <thread>   


#define KEYPRESS(win, key) glfwGetKey(win, key) == GLFW_PRESS
#define MOUSEPRESS(win, mousebutton) glfwGetMouseButton(win, mousebutton) == GLFW_PRESS

class GlfwApp : public VulkanEnv
{
public:
    GlfwApp()
    {
        //初始化glfw
        glfwInit(); 
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE); 
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        //创建窗口
        int framebufferWidth, framebufferHeight;
        window = glfwCreateWindow(m_width, m_height, "base", nullptr, nullptr);
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    }
    virtual ~GlfwApp()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void run()
    {
        // 初始化Vulkan
        VulkanEnv::init(glfwGetWin32Window(window));
        init();
        while (true) 
        {
            static auto lastStartTime = std::chrono::high_resolution_clock::now();
            static auto p = 0.f;
            static auto frames = 0;
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastStartTime).count();
            lastStartTime = currentTime;
            
            bool shouldQuitLoop = tick(deltaTime);
            drawFrame();
            if(shouldQuitLoop)
            {
                break;
            }
            glfwPollEvents();

            p += deltaTime;
            ++frames;
            if (p >= 1.f)
            {
                std::cout << "FPS: " << frames / p << std::endl;
                p = 0.f;
                frames = 0;
            }
            
        }

        // 清理Vulkan资源
        clean();
        VulkanEnv::clean();
    
    }
protected:
    virtual bool tick(float delta); 
    virtual void drawFrame() = 0;
    virtual void init() = 0;
    virtual void clean() = 0;
private:
    GLFWwindow* window;
    const uint32_t m_width = 800;
    const uint32_t m_height = 600;
};



#endif
