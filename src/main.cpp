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

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Shader.hpp"
#include "Types.hpp"
#include "UI.hpp"
#include "Simulation.hpp"

#define SHADER_DIR "../shader/"
#define TEXTURES_DIR "../assets/textures/"

GLFWwindow* InitializeAndCreateWindow(std::string title, i64 width, i64 height) {
    std::cout << "Initializing OpenGL" << std::endl;

    // initialize the GLFW library
    if (!glfwInit()) {
    throw std::runtime_error("Couldn't init GLFW");
    }

    // setting the opengl version
    //int major = 3;
    //int minor = 2;
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create the window
    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Couldn't create a window");
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();

    if (err != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error(std::string("Could initialize GLEW, error = ") +
                                (const char*)glewGetErrorString(err));
    }

    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported " << version << std::endl;

    // opengl configuration
    //glEnable(GL_DEPTH_TEST);  // enable depth-testing
    //glDepthFunc(GL_LESS);  // depth-testing interprets a smaller value as "closer"

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // vsync
    glfwSwapInterval(false);
    std::cout << "Window Created" << std::endl;
    return window;
}

ShaderProgram InitializeAndCreateShaders() {
    //Shader basevert(SHADER_DIR "base.vert", GL_VERTEX_SHADER);
    Shader basefrag(SHADER_DIR "base.frag", GL_FRAGMENT_SHADER);

    return ShaderProgram({ basefrag });
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
    i64 simwidth = 400, simheight = 300, renwidth = 1200, renheight = 900;

    glm::ivec2 renres{renwidth, renheight};

    // initialize simulation from file
    Simulation::Simulation sim("Powder Sim", simwidth, simheight);

    UI::UIRenderer ui(renres);
    ui.AddText(UI::Text("text"));

    int64_t x = 10, y = 10, w = 20, h = 20;
    for (auto t : Simulation::types) {
        if (t == Simulation::AIR) continue;
        std::cout << t->name << std::endl;
        ui.AddDisplay(UI::Display(t->id, x, y, w, h, 5, glm::vec3(0.5, 0.5, 0.5)));
        x += 10 + w;
    }


    /*std::vector<char> fc = readFile(TEXTURES_DIR "spiral.b");
    if (fc.size() != simwidth * simheight) {
        std::cerr << "Simulation requires binary file of size " << simwidth * simheight << " bytes, one of " << fc.size() << " bytes was provided." << std::endl;
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
    }*/


    GLFWwindow* window = InitializeAndCreateWindow(std::string("CSC417 Project: ") + sim.name, renwidth, renheight);
    ShaderProgram shader = InitializeAndCreateShaders();
    glfwSetWindowUserPointer(window, &sim);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glm::vec2 simResolution(sim.width, sim.height);
    glm::vec2 renderScale(float(renwidth) / sim.width, float(renheight) / sim.height);

    // setup ssbo   
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

    Rendering::Particle* render_data = new Rendering::Particle[sim.width * sim.height];

    i64 tick = 0;
    while (!glfwWindowShouldClose(window)) {
        // RENDER

        double mx, my, x, y;
        glfwGetCursorPos(window, &mx, &my);
        x = mx / renderScale.x;
        y = my / renderScale.y;
        y = sim.height - y;

        Simulation::Grid& currentGrid = sim.grid;

        // setup data for render
        for (i64 i = 0; i < sim.height; i++) {
            for (i64 j = 0; j < sim.width; j++) {
                Simulation::Particle& p = currentGrid(j, i);
                render_data[i * sim.width + j].id = p.t->id;
                if (p.t == Simulation::FIRE) {
                    render_data[i * sim.width + j].lifetime_ratio = std::clamp(double(p.lifetime) / p.secondary_t->burntime, 0.0, 1.0);
                }
                else {
                    render_data[i * sim.width + j].lifetime_ratio = 0;
                }
            }
        }
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Rendering::Particle) * sim.width * sim.height, render_data, GL_DYNAMIC_DRAW);

        // render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setUniform("simResolution", simResolution);
        shader.setUniform("renderScale", renderScale);
        glRectf(-1, -1, 1, 1);
        shader.unuse();

        // draw selectors
        ui.Render(sim.currentParticleType->id);

        // draw circle around 
        glColor3f(1, 1, 1);
        glLineWidth(5);
        UI::DrawCircle(renres, mx, my, sim.radius * renderScale.x, sim.radius * renderScale.y);


        glfwSwapBuffers(window);
        glfwPollEvents();


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

        if (sim.paused) {
            //if (currentGrid.InBounds(x, y)) {
            //    std::cout << currentGrid(x, y).t->id << std::endl;
            //}
        }
        else {
            sim.Tick(tick);
            tick++;
        }
    }

    shader.unuse();
    glfwDestroyWindow(window);


}
