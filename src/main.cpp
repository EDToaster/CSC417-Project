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

/***** USER SETTINGS *****/
#define SIMULATE_RIGID_BODIES   /* Simulate using rigid body system */
#define DOUGLAS_PEUCKER         /* Approximate world particle's Rigid body boundaries using Douglas Peucker Algorithm */
//#define DEBUG_DRAW              /* Draw rigid body lines */
#define LOAD_FROM_FILE          /* Load binary file as initial simulation state */ 

#define SIM_WIDTH 400
#define SIM_HEIGHT 300
#define RENDER_WIDTH 1200
#define RENDER_HEIGHT 900

/***** END SETTINGS  *****/

#include "Shader.hpp"
#include "Types.hpp"
#include "UI.hpp"
#include "Simulation.hpp"
#include "Marching.hpp"

#define SHADER_DIR "../shader/"
#define TEXTURES_DIR "../assets/textures/"


GLFWwindow* InitializeAndCreateWindow(std::string title, glm::ivec2 renderResolution) {
    std::cout << "Initializing OpenGL" << std::endl;

    // initialize the GLFW library
    if (!glfwInit()) {
    throw std::runtime_error("Couldn't init GLFW");
    }

    // create the window
    GLFWwindow* window = glfwCreateWindow(renderResolution.x, renderResolution.y, title.c_str(), NULL, NULL);
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
    glEnable(GL_TEXTURE_2D);
    std::cout << "Window Created" << std::endl;
    return window;
}

ShaderProgram CreateBaseShader() {
    Shader baseFrag(SHADER_DIR "base.frag", GL_FRAGMENT_SHADER);
    return ShaderProgram({ baseFrag });
}

ShaderProgram CreateRigidShader() {
    Shader rigidFrag(SHADER_DIR "rigid.frag", GL_FRAGMENT_SHADER);
    return ShaderProgram({ rigidFrag });
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

    glm::ivec2 simResolution{ SIM_WIDTH, SIM_HEIGHT };
    glm::ivec2 renderResolution{ RENDER_WIDTH, RENDER_HEIGHT };
    glm::vec2 renderScale{ float(RENDER_WIDTH) / SIM_WIDTH, float(RENDER_HEIGHT) / SIM_HEIGHT };

    // initialize simulation from file
    Simulation::Simulation sim("Powder Sim", simResolution.x, simResolution.y);


#ifdef LOAD_FROM_FILE
    std::vector<char> fc = readFile(TEXTURES_DIR "noita.b");
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
    UI::UIRenderer ui(renderResolution);
    ui.AddText(UI::Text("text"));

    i64 x = 10, y = 10, w = 20, h = 20;
    for (auto t : Simulation::types) {
        if (t == Simulation::AIR) continue;
        std::cout << t->name << std::endl;
        ui.AddDisplay(UI::Display(t->id, x, y, w, h, 5, t->col));
        x += 10 + w;
    }

    GLFWwindow* window = InitializeAndCreateWindow(std::string("CSC417 Project: ") + sim.name, renderResolution);
    ShaderProgram baseShader = CreateBaseShader(), rigidShader = CreateRigidShader();

    glfwSetWindowUserPointer(window, &sim);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);


    // setup ssbo   
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);

    Rendering::Particle* render_data = new Rendering::Particle[simResolution.x * simResolution.y];

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
                    render_data[i * simResolution.x + j].lifetime_ratio = std::clamp(double(p.lifetime) / p.secondary_t->burntime, 0.0, 1.0);
                }
                else {
                    render_data[i * simResolution.x + j].lifetime_ratio = 0;
                }
            }
        }
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Rendering::Particle) * simResolution.x * simResolution.y, render_data, GL_DYNAMIC_DRAW);

        // render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        baseShader.use();
        baseShader.setUniform("simResolution", glm::vec2{ simResolution });
        baseShader.setUniform("renderScale", renderScale);
        glRectf(-1, -1, 1, 1);
        baseShader.unuse();

#ifdef SIMULATE_RIGID_BODIES
#ifdef DEBUG_DRAW
        // draw contours
        if (sim.paused) {
            glLineWidth(3);
            for (auto contour : sim.contours) {
                float sx, sy, ox, oy;
                glColor3f(1, 0, 0);
                glBegin(GL_LINE_LOOP);
                {
                    bool firstSeg = false;
                    for (auto vertex : contour.vertices) {
                        UI::SimToScreen(renderResolution, renderScale, vertex.x, vertex.y, sx, sy);
                        UI::ScreenToOpenGL(renderResolution, sx, sy, ox, oy);
                        glVertex2f(ox, oy);
                        if (firstSeg) {
                            glColor3f(1, 1, 1);
                        }
                        firstSeg = true;
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

#endif

        // draw rigid bodies
        rigidShader.use();

        glColor3f(1, 1, 1);
        for (auto rbody : sim.rigidBodies) {
            // get rigid body position
            b2Body* body = rbody.body;
            const b2Vec2& pos = body->GetPosition();
            const b2Transform& transform = body->GetTransform();

            // for each fixture get polygons
            for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext())
            {
                // assume this fixture is of e_polygon type
                b2PolygonShape* shape = (b2PolygonShape*)f->GetShape();
                int v_count = shape->m_count;


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
            // draw a dot at the current position
            //UI::DrawCircle(renres, sx, sy, 3, 3);
        }
        rigidShader.unuse();
#endif

        // draw selectors
        ui.Render(sim.currentParticleType->id);

        // draw circle around 
        glColor3f(1, 1, 1);
        glLineWidth(1);
        UI::DrawCircle(renderResolution, mx, my, sim.radius * renderScale.x, sim.radius * renderScale.y);

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
        //std::cout << "Tick" << std::endl;
    }

    baseShader.unuse();
    glfwDestroyWindow(window);


}
