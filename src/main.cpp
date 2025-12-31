#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <thread>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

#include <omp.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "VulkanContext.hpp"
#include "VulkanSwapchain.hpp"

// SPAWN BODY IDs
#define NONE 0
#define PENDULUM 1
#define BOUNCE 2
#define CAR 3

/***** USER SETTINGS *****/
//#define SIMULATE_RIGID_BODIES   /* Simulate using rigid body system */
#define SPAWN_BODY NONE            /* Change this value to 1, 2 or 3 to spawn rigid bodies */
#define DOUGLAS_PEUCKER         /* Approximate world particle's Rigid body boundaries using Douglas Peucker Algorithm */
//#define DEBUG_DRAW              /* Draw rigid body boundaries */
#define LOAD_FROM_FILE          /* Load binary file as initial simulation state */ 
#define TEXTURE_FILE "oct.b"    /* Filename, stored in assets/ */

#define SIM_WIDTH 400
#define SIM_HEIGHT 300
#define RENDER_WIDTH 2400
#define RENDER_HEIGHT 1800

/***** END USER SETTINGS  *****/

// for multithreading
#define CHUNK_SIZE 16

// TODO: Phase 2 - Migrate shaders to Vulkan
// #include "Shader.hpp"
#include "Types.hpp"
#include "UI.hpp"
#include "Simulation.hpp"
#include "Marching.hpp"

#define SHADER_DIR "../shader/"
#define TEXTURES_DIR "../assets/textures/"


struct VulkanWindow {
    GLFWwindow* window;
    VkSurfaceKHR surface;
    Vulkan::VulkanContext* context;
    Vulkan::VulkanSwapchain* swapchain;
};

VulkanWindow* InitializeAndCreateWindow(std::string title, glm::ivec2 renderResolution) {
    std::cout << "Initializing Vulkan" << std::endl;

    // initialize the GLFW library
    if (!glfwInit()) {
        throw std::runtime_error("Couldn't init GLFW");
    }

    // Don't create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // create the window
    GLFWwindow* window = glfwCreateWindow(renderResolution.x, renderResolution.y, title.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Couldn't create a window");
    }

    // Initialize Vulkan context (creates instance in constructor)
    Vulkan::VulkanContext* context = new Vulkan::VulkanContext();
    
    // Create surface using GLFW (instance already created)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkInstance instance = context->GetInstance();
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    if (result != VK_SUCCESS) {
        context->Cleanup();
        delete context;
        glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Failed to create window surface!");
    }

    // Initialize context with surface (creates device, queues)
    context->Initialize(surface);

    // Create swapchain
    Vulkan::VulkanSwapchain* swapchain = new Vulkan::VulkanSwapchain(
        context, surface, renderResolution.x, renderResolution.y);

    VulkanWindow* vkWindow = new VulkanWindow();
    vkWindow->window = window;
    vkWindow->surface = surface;
    vkWindow->context = context;
    vkWindow->swapchain = swapchain;

    std::cout << "Vulkan window created successfully" << std::endl;
    return vkWindow;
}

// TODO: Phase 2 - Migrate shaders to Vulkan
/*
ShaderProgram CreateBaseShader() {
    Shader baseFrag(SHADER_DIR "base.frag", GL_FRAGMENT_SHADER);
    return ShaderProgram({ baseFrag });
}

ShaderProgram CreateRigidShader() {
    Shader rigidFrag(SHADER_DIR "rigid.frag", GL_FRAGMENT_SHADER);
    return ShaderProgram({ rigidFrag });
}

ShaderProgram CreateScreenShader() {
    Shader rigidFrag(SHADER_DIR "screen.frag", GL_FRAGMENT_SHADER);
    return ShaderProgram({ rigidFrag });
}
*/

UI::UIRenderer CreateUI(const glm::ivec2 renderResolution) {
    // Initialize UI
    UI::UIRenderer ui(renderResolution);
    ui.AddText(UI::Text("text"));

    i64 x = 10, y = 10, w = 20, h = 20;
    for (auto t : Simulation::types) {
        if (t == Simulation::AIR) continue;
        std::cout << t->name << std::endl;
        ui.AddDisplay(UI::Display(t->id, x, y, w, h, 5, t->col));
        x += 10 + w;
    }
    return ui;
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
    Simulation::Simulation* simptr = (Simulation::Simulation*)glfwGetWindowUserPointer(window);

    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_1:
            simptr->currentParticleType = Simulation::SAND; break;
        case GLFW_KEY_2:
            simptr->currentParticleType = Simulation::WATER; break;
        case GLFW_KEY_3:
            simptr->currentParticleType = Simulation::OIL; break;
        case GLFW_KEY_4:
            simptr->currentParticleType = Simulation::WOOD; break;
        case GLFW_KEY_5:
            simptr->currentParticleType = Simulation::FIRE; break;
        case GLFW_KEY_6:
            simptr->currentParticleType = Simulation::SMOKE; break;
        case GLFW_KEY_7:
            simptr->currentParticleType = Simulation::GUNPOWDER; break;
        case GLFW_KEY_8:
            simptr->currentParticleType = Simulation::ACID; break;
        case GLFW_KEY_9:
            simptr->currentParticleType = Simulation::COTTON; break;
        case GLFW_KEY_0:
            simptr->currentParticleType = Simulation::FUSE; break;
        case GLFW_KEY_SPACE:
            simptr->paused = !simptr->paused; break;
        case GLFW_KEY_TAB:
            simptr->tabPressed = true; break;
        default:
            break;
        }
        std::cout << "Current particle type: " << simptr->currentParticleType->name << ". Paused " << simptr->paused << std::endl;
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        default:
            break;
        }
    }

}

void scroll_callback(GLFWwindow* window, double xoff, double yoff) {
    Simulation::Simulation* simptr = (Simulation::Simulation*)glfwGetWindowUserPointer(window);
    simptr->radius = std::clamp(simptr->radius + yoff, 1.0, 100.0);
}

std::vector<char> readFile(const char* filename)
{
    std::ifstream file(filename);
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string& s = ss.str();
    std::vector<char> vec(s.begin(), s.end());
    std::cout << vec.size() << std::endl;
    return vec;
}

int main(int argc, const char* argv[]) {

#pragma omp parallel
    {
        printf("Thread %d of %d reporting\n",
            omp_get_thread_num(), omp_get_num_threads());
    }

    glm::ivec2 simResolution{ SIM_WIDTH, SIM_HEIGHT };
    glm::ivec2 renderResolution{ RENDER_WIDTH, RENDER_HEIGHT };
    glm::vec2 renderScale{ float(RENDER_WIDTH) / SIM_WIDTH, float(RENDER_HEIGHT) / SIM_HEIGHT };

    // initialize simulation from file
    Simulation::Simulation sim("Powder Sim", simResolution.x, simResolution.y);

#ifdef LOAD_FROM_FILE
    std::vector<char> fc = readFile(TEXTURES_DIR TEXTURE_FILE);
    if (fc.size() != simResolution.x * simResolution.y) {
        std::cerr << "Simulation requires binary file of size " << simResolution.x * simResolution.y << " bytes, one of " << fc.size() << " bytes was provided." << std::endl;
        return 1;
    }
    
    for (i64 i = 0; i < sim.height; i++) {
        for (i64 j = 0; j < sim.width; j++) {
            char id = fc[(sim.height - i - 1) * sim.width + j];
            const Simulation::ParticleType* t = Simulation::types[id];
    
            if (t == Simulation::FIRE) {
                InitializeFire(sim.grid(j, i), Simulation::OIL);
            }
            else {
                InitializeNormal(sim.grid(j, i), t);
            }
        }
    }
#endif

    // Initialize UI
    UI::UIRenderer ui = CreateUI(renderResolution);

    // Setup render pipeline
    VulkanWindow* vkWindow = InitializeAndCreateWindow(std::string("CSC417 Project: ") + sim.name, renderResolution);
    GLFWwindow* window = vkWindow->window;
    
    // TODO: Migrate shaders to Vulkan (keeping OpenGL shaders for now during migration)
    // ShaderProgram baseShader = CreateBaseShader(), rigidShader = CreateRigidShader(), screenShader = CreateScreenShader();

    glfwSetWindowUserPointer(window, &sim);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Cache mouse position to avoid querying every frame (which can be expensive on Windows)
    // Use static variables so the callback can access them
    static double cachedMouseX = 0.0, cachedMouseY = 0.0;
    glfwGetCursorPos(window, &cachedMouseX, &cachedMouseY); // Initialize
    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        cachedMouseX = x;
        cachedMouseY = y;
    });

    // TODO: Phase 2 - Migrate to Vulkan framebuffers and buffers
    // setup render to texture (OpenGL - commented out for Phase 1)
    /*
    GLuint frameBuffer = 0;
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    GLuint renderedTexture;
    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, simResolution.x, simResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER != GL_FRAMEBUFFER_COMPLETE)) {
        std::cerr << "There was an error in setting up the frame buffer to render" << std::endl;
        return 1;
    }

    // setup ssbo   
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
    */

    // TODO: Phase 2 - Migrate to Vulkan buffers
    // Rendering::Particle* render_data = new Rendering::Particle[simResolution.x * simResolution.y];

    i64 tick = 0;
    // Throttle event polling to reduce CPU usage with high mouse polling rates
    // Use time-based throttling for better responsiveness than frame-based
    double lastEventPollTime = glfwGetTime();
    const double EVENT_POLL_INTERVAL = 0.005; // Poll events every 5ms (200Hz max event processing)
    
    std::cout << "[INFO] Phase 1 Complete: Vulkan initialized. Window should be visible (no rendering yet)." << std::endl;
    std::cout << "[INFO] Press ESC or close window to exit." << std::endl;
    
    while (!glfwWindowShouldClose(window)) {
        // Poll events at the start of frame with time-based throttling
        // This prevents processing too many mouse events per second while maintaining responsiveness
        double currentTime = glfwGetTime();
        if (currentTime - lastEventPollTime >= EVENT_POLL_INTERVAL) {
            glfwPollEvents();
            lastEventPollTime = currentTime;
        }
        // Use cached mouse position instead of querying every frame
        double mx = cachedMouseX, my = cachedMouseY;
        double x, y;
        x = mx / renderScale.x;
        y = my / renderScale.y;
        y = sim.height - y;

        Simulation::Grid& currentGrid = sim.grid;

        // TODO: Phase 2 - Setup data for Vulkan rendering
        /*
        // setup data for render
        for (i64 i = 0; i < sim.height; i++) {
            for (i64 j = 0; j < sim.width; j++) {
                Simulation::Particle& p = currentGrid(j, i);
                render_data[i * sim.width + j].id = p.t->id;
                if (p.t == Simulation::FIRE) {
                    render_data[i * simResolution.x + j].lifetime_ratio = std::clamp(double(p.lifetime) / p.secondary_t->burntime, 0.0, 1.0);
                }
                else {
                    render_data[i * simResolution.x + j].lifetime_ratio = 0;
                }
            }
        }
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Rendering::Particle)* simResolution.x* simResolution.y, render_data, GL_DYNAMIC_DRAW);
        */

        // TODO: Phase 2 - Migrate rendering to Vulkan
        // render to texture (OpenGL - commented out for Phase 1)
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glViewport(0, 0, simResolution.x, simResolution.y);

        // render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        baseShader.use();
        baseShader.setUniform("simResolution", glm::vec2{ simResolution });
        glRectf(-1, -1, 1, 1);
        baseShader.unuse();
        */

#ifdef SIMULATE_RIGID_BODIES
        // TODO: Phase 2 - Migrate rigid body rendering to Vulkan
        // draw rigid bodies (OpenGL - commented out for Phase 1)
        /*
        rigidShader.use();
        rigidShader.setUniform("viewportSize", glm::vec2{ simResolution });

        glColor3f(1, 1, 1);
        for (auto rbody : sim.rigidBodies) {
            // get rigid body position
            b2Body* body = rbody.body;
            const b2Vec2& pos = body->GetPosition();
            const b2Transform& transform = body->GetTransform();

            // for each fixture get polygons
            for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext())
            {
                if (f->GetShape()->GetType() == b2Shape::Type::e_polygon) {
                    b2PolygonShape* shape = (b2PolygonShape*)f->GetShape();
                    int v_count = shape->m_count;

                    glColor3f(0.3, 0.1, 0.8);
                    glBegin(GL_POLYGON);
                    {
                        for (int i = 0; i < v_count; i++) {
                            b2Vec2& vpos = body->GetWorldPoint(shape->m_vertices[i]);

                            // transform from sim space to openGL space
                            float sx, sy, ox, oy;
                            UI::SimToScreen(renderResolution, renderScale, vpos.x, vpos.y, sx, sy);
                            UI::ScreenToOpenGL(renderResolution, sx, sy, ox, oy);

                            // apply texture and draw vertex
                            glTexCoord2f(shape->m_vertices[i].x / 10.0, shape->m_vertices[i].y / 10.0);
                            glVertex2f(ox, oy);
                        }
                    }
                    glEnd();
                }
                else if (f->GetShape()->GetType() == b2Shape::Type::e_circle) {
                    b2CircleShape* shape = (b2CircleShape*)f->GetShape();
                    b2Vec2& pos = body->GetWorldPoint(shape->m_p);
                    float angle = body->GetAngle();
                    float radius = shape->m_radius;

                    float sx, sy, srx, sry;
                    UI::SimToScreen(renderResolution, renderScale, pos.x, pos.y, sx, sy);
                    UI::SimToScreen(renderResolution, renderScale, radius, radius, srx, sry);
                    UI::DrawCircle(renderResolution, sx, sy, srx, srx, true);
                }
            }
            // draw a dot at the current position
            //UI::DrawCircle(renres, sx, sy, 3, 3);
        }
        rigidShader.unuse();
        */
#endif

        // TODO: Phase 2 - Migrate screen rendering to Vulkan
        // render to screen (OpenGL - commented out for Phase 1)
        /*
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, renderResolution.x, renderResolution.y);
        screenShader.use();
        screenShader.setUniform("viewportSize", glm::vec2{ renderResolution });
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 0);
            glVertex2f(-1, -1);
            glTexCoord2f(1, 0);
            glVertex2f(1, -1);
            glTexCoord2f(1, 1);
            glVertex2f(1, 1);
            glTexCoord2f(0, 1);
            glVertex2f(-1, 1);
        }
        glEnd();
        screenShader.unuse(); 
        glDisable(GL_TEXTURE_2D);
        */

#ifdef SIMULATE_RIGID_BODIES
#ifdef DEBUG_DRAW
        // TODO: Phase 2 - Migrate debug drawing to Vulkan
        // draw contours (OpenGL - commented out for Phase 1)
        /*
        if (!sim.paused) {
            glLineWidth(1);
            for (auto contour : sim.contours) {
                float sx, sy, ox, oy;
                glBegin(GL_LINE_LOOP);
                {
                    int seg = 0;
                    for (auto vertex : contour.vertices) {
                        if (seg == 0) {
                            glColor3f(1, 0, 0);

                        }
                        else if (seg == 1) {
                            glColor3f(0, 1, 0);
                        }
                        else {
                            glColor3f(0, 0, 1);
                        }
                        UI::SimToScreen(renderResolution, renderScale, vertex.x, vertex.y, sx, sy);
                        UI::ScreenToOpenGL(renderResolution, sx, sy, ox, oy);
                        glVertex2f(ox, oy);
                        seg++;
                    }
                }
                glEnd();
            }
        }
        else {
            // Turn on wireframe mode
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
            glBegin(GL_TRIANGLES);
            float sx, sy, ox, oy;
            for (auto triangle : sim.triangles) {
                for (int i = 0; i < 3; i++) {
                    TPPLPoint& p = triangle.GetPoint(i);
                    UI::SimToScreen(renderResolution, renderScale, p.x, p.y, sx, sy);
                    UI::ScreenToOpenGL(renderResolution, sx, sy, ox, oy);
                    glVertex2f(ox, oy);
                }
            }
            glEnd();
            // Turn off wireframe mode
            glPolygonMode(GL_FRONT, GL_FILL);
            glPolygonMode(GL_BACK, GL_FILL);

        }
        */
#endif
#endif

        // TODO: Phase 4 - Migrate UI rendering to Vulkan
        // draw selectors (OpenGL - commented out for Phase 1)
        // ui.Render(sim.currentParticleType->id);

        // draw circle around (OpenGL - commented out for Phase 1)
        // glColor3f(1, 1, 1);
        // glLineWidth(1);
        // UI::DrawCircle(renderResolution, mx, my, sim.radius * renderScale.x, sim.radius * renderScale.y, false);

        // TODO: Phase 2 - Implement Vulkan presentation
        // glfwSwapBuffers(window); // OpenGL swap - need Vulkan presentation

        // GAME LOGIC
        // set mouse pos to current block
        bool m1 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
        bool m2 = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2);

        float radius = sim.radius;

        // spawn
        if (m1 || m2) {
            const Simulation::ParticleType* t = m1 ? sim.currentParticleType : Simulation::AIR;
            for (i64 off_x = -radius - 1; off_x < radius + 1; off_x++) {
                for (i64 off_y = -radius - 1; off_y < radius + 1; off_y++) {
                    if ((off_x * off_x) + (off_y * off_y) >= radius * radius) continue;

                    i64 px = x + off_x;
                    i64 py = y + off_y;
                    // check if px, py is within radius of current mouse

                    float noiseThresh =  (t == Simulation::WOOD || t == Simulation::AIR || t == Simulation::COTTON || t == Simulation::FUSE) ? 0 : 0.99;
                    if (currentGrid.InBounds(px, py) && noise() > noiseThresh) {
                        if (t == Simulation::FIRE) {
                            InitializeFire(currentGrid(px, py), Simulation::OIL);
                        } else {
                            InitializeNormal(currentGrid(px, py), t);
                        }
                    }
                }
            }
        }

        // spawn rigid bodies
#ifdef SIMULATE_RIGID_BODIES 
        b2Vec2 dynamicBoxVerts[8] = { {3.3, 0}, {6.6, 0}, {10, 3.3 }, {10, 6.6}, {6.6, 10}, {3.3, 10}, {0, 6.6}, {0, 3.3} };
        if (sim.tabPressed) {
            sim.tabPressed = false;
            // create dynamic bodies
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(x, y);
            b2Body* body = sim.world.CreateBody(&bodyDef);
            b2PolygonShape dynamicBox;
            //dynamicBox.SetAsBox(5, 5);
            dynamicBox.Set(dynamicBoxVerts, 8);
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.3f;
            fixtureDef.restitution = 0.6;
            fixtureDef.restitutionThreshold = 0;
            body->CreateFixture(&fixtureDef);

            sim.rigidBodies.push_back({ body });
        }
#endif

        if (sim.paused) {
            //if (currentGrid.InBounds(x, y)) {
            //    std::cout << currentGrid(x, y).t->id << std::endl;
            //}
        }
        else {
            sim.Tick(tick);
            tick++;
        }
        //std::cout << "Tick" << std::endl;
    }

    // TODO: Cleanup Vulkan resources
    // baseShader.unuse();
    
    // Cleanup Vulkan resources
    delete vkWindow->swapchain;
    vkDestroySurfaceKHR(vkWindow->context->GetInstance(), vkWindow->surface, nullptr);
    vkWindow->context->Cleanup();
    delete vkWindow->context;
    glfwDestroyWindow(window);
    delete vkWindow;
    glfwTerminate();
}
