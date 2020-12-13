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

#define SHADER_DIR "../shader/"
#define TEXTURES_DIR "../assets/textures/"


inline double noise() {
    ui64 r = rand();
    // adjust because RAND_MAX is small and inclusive.
    if (r == RAND_MAX) r--;
    return r / double(RAND_MAX);
}

namespace Simulation {

    class ParticleType {
    public:
        ParticleType(i64 id, double dens, double flammability, i64 burntime, double acidability, bool movable, bool isSolid, std::string name) :
            id(id), dens(dens), flammability(flammability), burntime(burntime), acidability(acidability),
            movable(movable), isSolid(isSolid), name(name) {}
        const i64 id;
        const double dens;
        const double flammability;
        const i64 burntime;
        const double acidability;
        const bool movable;
        const bool isSolid;
        const std::string name;
    };

    const ParticleType air(0, 1,        0, 0,           0,  true, false, "Air");
    const ParticleType sand(1, 60,      0, 0,           .2, true, true, "Sand");
    const ParticleType water(2, 5,      0, 0,           0, true, false, "Water");
    const ParticleType oil(3, 2,        .04, 3000,      0, true, false, "Oil");
    const ParticleType wood(4, -1,      .001, 10000,    .02, false, true, "Wood");
    const ParticleType fire(5, -1,      0, 0,           0, false, false, "Fire");
    const ParticleType smoke(6, .9999,     0, 0,           0, true, false, "Smoke");
    const ParticleType gunpowder(7, 40, 1, 50,          .2, true, true, "Gunpowder");
    const ParticleType acid(8, 5.001,   0, 0,           0, true, false, "Acid");
    const ParticleType cotton(9, -1,    .05, 1000,      .5, false, true, "Cotton");
    const ParticleType fuse(10, -1,     .3, 200,        .5, false, true, "Fuse");

    const ParticleType* AIR = &air;
    const ParticleType* SAND = &sand;
    const ParticleType* WATER = &water;
    const ParticleType* OIL = &oil;
    const ParticleType* WOOD = &wood;
    const ParticleType* FIRE = &fire;
    const ParticleType* SMOKE = &smoke;
    const ParticleType* GUNPOWDER = &gunpowder;
    const ParticleType* ACID = &acid;
    const ParticleType* COTTON = &cotton;
    const ParticleType* FUSE = &fuse;

    const ParticleType* types[] = {AIR, SAND, WATER, OIL, WOOD, FIRE, SMOKE, GUNPOWDER, ACID, COTTON, FUSE};


    struct Particle {
        const ParticleType* t;
        const ParticleType* secondary_t;
        //glm::vec2 vel;
        i64 lifetime;
        bool updated;
    };


    void InitializeNormal(Particle& p, const ParticleType* t) {
        p.t = t;
        p.secondary_t = nullptr;
        p.lifetime = 0;
    }

    void InitializeFire(Particle& p, const ParticleType* secondary_t) {
        InitializeNormal(p, FIRE);
        p.secondary_t = secondary_t;
    }

    class Grid {
    public:
        ui64 width, height;
        Particle* grid;
        Grid(ui64 width, ui64 height): width(width), height(height) {
            grid = new Particle[width * height];
            Reset();
        }

        ~Grid() {
            delete[] grid;
        }

        void Reset() {
            for (int i = 0; i < width * height; i++) {
                InitializeNormal(grid[i], AIR);
            }
        }

        Particle& operator()(int x, int y) {
            return grid[x + y * width];
        }

        Particle& operator()(int i) {
            return grid[i];
        }

        inline bool InBounds(i64 x, i64 y) {
            return x >= 0 && x < width&& y >= 0 && y < height;
        }
    };

    class Simulation {
    public:
        std::string name;
        ui64 width, height;
        const ParticleType* currentParticleType;

        //bool grid1_current;
        Grid grid;

        bool paused;
        float radius;

        Simulation(std::string name, ui64 width, ui64 height) :
                name(name), width(width), height(height),
                currentParticleType(SAND),
                grid(width, height), 
                paused(true),
                radius(5.0) {
        }
        
        //Grid& GetCurrentGrid() {
        //    return grid1_current ? grid1 : grid2;
        //}

        //Grid& GetNextGrid() {
        //    return grid1_current ? grid2 : grid1;
        //}

        std::vector<glm::ivec2> SAND_UPDATE_ORDER = { {0, -1}, {1, -1}, {-1, -1} };
        //std::vector<glm::ivec2> WATER_UPDATE_ORDER = { {0, -1}, {1, -1}, {-1, -1}, {1, 0}, {-1, 0} };
        std::vector<glm::ivec2> WATER_UPDATE_ORDER = { {0, -1}, {2, -1}, {-2, -1}, {1, -1}, {-1, -1}, {2, 0}, {-2, 0}, {1, 0}, {-1, 0} };
        std::vector<glm::ivec2> SMOKE_UPDATE_ORDER = { {0, 1}, {1, 1}, {-1, 1}, {1, 0}, {-1, 0} };
        //std::vector<glm::ivec2> SMOKE_UPDATE_ORDER = { {0, 1}, {2, 1}, {-2, 1}, {1, 1}, {-1, 1}, {2, 0}, {-2, 0}, {1, 0}, {-1, 0} };
        std::vector<glm::ivec2> FIRE_UPDATE_NEIGHBOURS = { {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0} };

        inline double getDensity(Particle& p) {
            return p.t == FIRE ? p.secondary_t->dens : p.t->dens;
        }

        inline bool getMovable(Particle& p) {
            return p.t == FIRE ? p.secondary_t->movable : p.t->movable;
        }

        void UpdateNormalParticle(const ParticleType* t, i64 x, i64 y, std::vector<glm::ivec2>& updateOrder) {
            // apply gravity 
            Particle& p = grid(x, y);
            bool preferDown = t->dens > AIR->dens;

            Particle* swap = nullptr;
            double density = preferDown ? INFINITY : 0.0;

            // pick which air pocket to swap with
            if (!swap) {
                bool inverted = noise() > 0.5;
                for (auto off : updateOrder) {
                    i64 sx = x + (inverted ? -off.x : off.x);
                    i64 sy = y + off.y;

                    if (grid.InBounds(sx, sy)) {
                        Particle& candidate = grid(sx, sy);
                        if (!getMovable(candidate) || (t->isSolid && candidate.t->isSolid)) continue;
                        // find most preferred direction
                        double candidateDensity = getDensity(candidate);
                        // solids cannot swap
                        if ((preferDown && candidateDensity < density) || (!preferDown && candidateDensity > density)) {
                            density = candidateDensity;
                            swap = &candidate;
                        }
                    }
                }
            }

            bool doSwap = false;
            if (swap) {
                double relDensity = t->dens / density;

                double n = noise();
                
                if (relDensity <= 1.0) {
                    doSwap = n > (relDensity / 2.0);
                }
                else {
                    double invDensity = 1.0 / relDensity;
                    doSwap = n > (invDensity / 2.0);
                }
            }

            if (doSwap) {
                Particle tmp = *swap;
                *swap = p;
                p = tmp;
                p.updated = true;
            }
        }

        void UpdateAcid(i64 x, i64 y) {
            Particle& p = grid(x, y);
            int choice = (int)(noise() * FIRE_UPDATE_NEIGHBOURS.size());
            glm::ivec2& offset = FIRE_UPDATE_NEIGHBOURS[choice];

            int px = x + offset.x, py = y + offset.y;
            if (grid.InBounds(px, py)) {
                Particle& n = grid(px, py);
                // has n.flammibility chance to turn into fire
                if (noise() < n.t->acidability) {
                    // spread
                    n.updated = true;
                    InitializeNormal(n, AIR);
                }
            }
        }

        void UpdateFire(i64 x, i64 y) {
            Particle& p = grid(x, y);
            p.lifetime++;
            
            // choise neighbour
            float n = noise();
            int choice = (int) (n * FIRE_UPDATE_NEIGHBOURS.size());
            glm::ivec2& offset = FIRE_UPDATE_NEIGHBOURS[choice];

            int px = x + offset.x, py = y + offset.y;
            if (grid.InBounds(px, py)) {
                Particle& n = grid(px, py);
                // has n.flammibility chance to turn into fire
                if (noise() < n.t->flammability) {
                    // spread
                    InitializeFire(n, n.t);
                    // don't let the neighbour spread this tick
                    n.updated = true;
                }
                else if (n.t == AIR && noise() < 0.001) {
                    InitializeNormal(n, SMOKE);
                }
            }

            if (p.lifetime > p.secondary_t->burntime) {
                InitializeNormal(p, AIR);
                //p.updated = false;
            }
        }


        inline void TickParticle(i64 x, i64 y) {
            Particle& p = grid(x, y);
            if (p.updated) return;
            p.updated = true;

            const ParticleType* t = p.t;
            // spread fire
            if (t == FIRE) {
                t = p.secondary_t;
                UpdateFire(x, y);
            }
            else if (t == ACID) {
                UpdateAcid(x, y);
            }

            // so physics
            if (t == SAND || t == GUNPOWDER) {
                UpdateNormalParticle(t, x, y, SAND_UPDATE_ORDER);
            }
            else if (t == WATER || t == OIL || t == ACID) {
                UpdateNormalParticle(t, x, y, WATER_UPDATE_ORDER);
            }
            else if (t == SMOKE) {
                UpdateNormalParticle(t, x, y, SMOKE_UPDATE_ORDER);
            }
        }

        void Tick(i64 tick) {
            for (i64 i = 0; i < width * height; i++) {
                grid(i).updated = false;
            }

            // 
            int dir = tick % 4;
            dir = 0;
            if (dir == 0) {
                for (i64 y = 0; y < height; y++) {
                    for (i64 x = 0; x < width; x++) {
                        TickParticle(x, y);
                    }
                }
            } else if (dir == 1) {
                for (i64 y = 0; y < height; y++) {
                    for (i64 x = width - 1; x >= 0; x--) {
                        TickParticle(x, y);
                    }
                }
            } else if (dir == 2) {
                for (i64 y = height - 1; y >= 0; y--) {
                    for (i64 x = width - 1; x >= 0; x--) {
                        TickParticle(x, y);
                    }
                }
            } else {
                for (i64 y = height - 1; y >= 0; y--) {
                    for (i64 x = 0; x < width; x++) {
                        TickParticle(x, y);
                    }
                }
            }


            //grid1_current = !grid1_current;
        }
    };
};

namespace Rendering {
    struct Particle {
        GLuint id;
        // how much of the lifetime of this particle is over?
        GLfloat lifetime_ratio;
    };
};

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
    i64 simwidth = 40, simheight = 30, renwidth = 1200, renheight = 900;
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
