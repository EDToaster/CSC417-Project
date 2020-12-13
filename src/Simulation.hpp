#pragma once

#include "Types.hpp"

#include <box2d/b2_api.h>
#include <box2d/b2_math.h>
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>

#define PIXELS_PER_METRE 200

namespace Simulation {

    // PARTICLE STUFF
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

    const ParticleType air(0, 1, 0, 0, 0, true, false, "Air");
    const ParticleType sand(1, 60, 0, 0, .2, true, true, "Sand");
    const ParticleType water(2, 5, 0, 0, 0, true, false, "Water");
    const ParticleType oil(3, 2, .04, 3000, 0, true, false, "Oil");
    const ParticleType wood(4, -1, .001, 10000, .02, false, true, "Wood");
    const ParticleType fire(5, -1, 0, 0, 0, false, false, "Fire");
    const ParticleType smoke(6, .9999, 0, 0, 0, true, false, "Smoke");
    const ParticleType gunpowder(7, 40, 1, 50, .2, true, true, "Gunpowder");
    const ParticleType acid(8, 5.001, 0, 0, 0, true, false, "Acid");
    const ParticleType cotton(9, -1, .05, 1000, .5, false, true, "Cotton");
    const ParticleType fuse(10, -1, .3, 200, .5, false, true, "Fuse");

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

    const ParticleType* types[] = { AIR, SAND, WATER, OIL, WOOD, FIRE, SMOKE, GUNPOWDER, ACID, COTTON, FUSE };


    struct Particle {
        const ParticleType* t;
        const ParticleType* secondary_t;
        //glm::vec2 vel;
        i64 lifetime;
        bool updated;
    };

    void InitializeNormal(Particle & p, const ParticleType * t) {
        p.t = t;
        p.secondary_t = nullptr;
        p.lifetime = 0;
    }

    void InitializeFire(Particle & p, const ParticleType * secondary_t) {
        InitializeNormal(p, FIRE);
        p.secondary_t = secondary_t;
    }

    // RIGID STUFF
    // Only support one type of rigid body material at this time

    struct RigidBody {
        b2Body* body;
    };
    
    // GRID STUFF
    class Grid {
    public:
        ui64 width, height;
        Particle* grid;
        Grid(ui64 width, ui64 height) : width(width), height(height) {
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

        std::vector<RigidBody> rigidBodies;
        b2Vec2 gravity;
        b2World world;

        Grid grid;

        // game states, should probably put this somewhere else
        bool paused;
        float radius;

        Simulation(std::string name, ui64 width, ui64 height) :
            name(name), width(width), height(height),
            currentParticleType(SAND),
            grid(width, height),
            paused(true),
            radius(5.0), gravity(0, -10),
            world(gravity)
        {

            // setup box2d

            // create bounding box
            b2BodyDef groundBodyDef;
            groundBodyDef.position.Set(0, -10);
            b2Body* groundBody = world.CreateBody(&groundBodyDef);
            b2PolygonShape groundBox;
            groundBox.SetAsBox(50, 10);
            groundBody->CreateFixture(&groundBox, 0);

            // create dynamic body
            b2BodyDef bodyDef;
            bodyDef.type = b2_dynamicBody;
            bodyDef.position.Set(0, 4);
            b2Body* body = world.CreateBody(&bodyDef);
            b2PolygonShape dynamicBox;
            dynamicBox.SetAsBox(1, 1);
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &dynamicBox;
            fixtureDef.density = 1.0f;
            fixtureDef.friction = 0.3f;
            body->CreateFixture(&fixtureDef);

            rigidBodies.push_back({ body });
        }

        std::vector<glm::ivec2> SAND_UPDATE_ORDER = { {0, -1}, {1, -1}, {-1, -1} };
        std::vector<glm::ivec2> WATER_UPDATE_ORDER = { {0, -1}, {2, -1}, {-2, -1}, {1, -1}, {-1, -1}, {2, 0}, {-2, 0}, {1, 0}, {-1, 0} };
        std::vector<glm::ivec2> SMOKE_UPDATE_ORDER = { {0, 1}, {1, 1}, {-1, 1}, {1, 0}, {-1, 0} };
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
            int choice = (int)(n * FIRE_UPDATE_NEIGHBOURS.size());
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
            }
            else if (dir == 1) {
                for (i64 y = 0; y < height; y++) {
                    for (i64 x = width - 1; x >= 0; x--) {
                        TickParticle(x, y);
                    }
                }
            }
            else if (dir == 2) {
                for (i64 y = height - 1; y >= 0; y--) {
                    for (i64 x = width - 1; x >= 0; x--) {
                        TickParticle(x, y);
                    }
                }
            }
            else {
                for (i64 y = height - 1; y >= 0; y--) {
                    for (i64 x = 0; x < width; x++) {
                        TickParticle(x, y);
                    }
                }
            }
            
            // simulate rigid bodies
            float timestep = 1.0 / 60;
            i32 velIters = 6, posIters = 2;
            world.Step(timestep, velIters, posIters);

            for (auto rbody : rigidBodies) {
                b2Body* body = rbody.body;

                b2Vec2 pos = body->GetPosition();
                float angle = body->GetAngle();

                // what is this filthy printf statement doing here!
                printf("%4.2f %4.2f %4.2f\n", pos.x, pos.y, angle);
            }
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