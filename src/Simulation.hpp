#pragma once


#include <box2d/b2_api.h>
#include <box2d/b2_math.h>
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_chain_shape.h>
#include <box2d/b2_fixture.h>
#include <algorithm>

#include "Types.hpp"
#include "Marching.hpp"

#include "polypartition.h"

namespace Simulation {

    // PARTICLE STUFF
    class ParticleType {
    public:
        ParticleType(i64 id, glm::vec3 col, double dens, double flammability, i64 burntime, double acidability, bool movable, bool isSolid, std::string name) :
            id(id), col(col), dens(dens), flammability(flammability), burntime(burntime), acidability(acidability),
            movable(movable), isSolid(isSolid), name(name) {}
        const i64 id;
        const glm::vec3 col;
        const double dens;
        const double flammability;
        const i64 burntime;
        const double acidability;
        const bool movable;
        const bool isSolid;
        const std::string name;
    };

    const ParticleType air(0, glm::vec3{0, 0, 0}, 1, 0, 0, 0, true, false, "Air");
    const ParticleType sand(1, glm::vec3{ .7, .5, 0.26 }, 60, 0, 0, .2, true, true, "Sand");
    const ParticleType water(2, glm::vec3{ 0.2, 0.3, 0.8 }, 5, 0, 0, 0, true, false, "Water");
    const ParticleType oil(3, glm::vec3{ 0.8, 0.6, 0.4 }, 2, .04, 3000, 0, true, false, "Oil");
    const ParticleType wood(4, glm::vec3{ 0.5, 0.2, 0.1 }, -1, .001, 10000, .02, false, true, "Wood");
    const ParticleType fire(5, glm::vec3{ 0.7, 0.1, 0.0 }, -1, 0, 0, 0, false, false, "Fire");
    const ParticleType smoke(6, glm::vec3{ 0.1, 0.1, 0.1 }, .9999, 0, 0, 0, true, false, "Smoke");
    const ParticleType gunpowder(7, glm::vec3{ 0.25, 0.25, 0.25 }, 40, 1, 50, .2, true, true, "Gunpowder");
    const ParticleType acid(8, glm::vec3{ 0.25, .9, .5 }, 5.001, 0, 0, 0, true, false, "Acid");
    const ParticleType cotton(9, glm::vec3{ .84, .84, .84 }, -1, .05, 1000, .5, false, true, "Cotton");
    const ParticleType fuse(10, glm::vec3{ .30, .30, .30 }, -1, .3, 200, .5, false, true, "Fuse");

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

#ifdef SIMULATE_RIGID_BODIES
        std::vector<RigidBody> rigidBodies;
        TPPLPolyList triangles;
        std::vector<MarchingSquares::Contour> contours;
        b2Vec2 gravity;
        b2World world;
#endif

        Grid grid;
        ui8* solidBuffer;

        // game states, should probably put this somewhere else
        bool paused;
        float radius;

        Simulation(std::string name, ui64 width, ui64 height) :
            name(name), width(width), height(height),
            currentParticleType(SAND),
            grid(width, height),
            paused(true),
            radius(5.0)
#ifdef SIMULATE_RIGID_BODIES
            , gravity(0, -10),
            world(gravity)
#endif
        {

            // setup box2d

            // create bounding box
#ifdef SIMULATE_RIGID_BODIES
            b2BodyDef groundBodyDef;
            groundBodyDef.position.Set(0, 0);
            b2Body* groundBody = world.CreateBody(&groundBodyDef);
            b2ChainShape groundBox;

            b2Vec2 groundBoxVerts[4] = { {0, 0}, {0, (float)height}, {(float)width, (float)height}, {(float)width, 0} };
            b2Vec2 dynamicBoxVerts[4] = { {0, 0}, {10, 5}, {5, 10}, {0, 10} };
            groundBox.CreateLoop(groundBoxVerts, 4);
            groundBody->CreateFixture(&groundBox, 0);

            // create dynamic bodies
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    float xpos = 100 + 22 * i;
                    float ypos = 150 + 22 * j;

                    b2BodyDef bodyDef;
                    bodyDef.type = b2_dynamicBody;
                    bodyDef.position.Set(xpos, ypos);
                    b2Body* body = world.CreateBody(&bodyDef);
                    b2PolygonShape dynamicBox;
                    dynamicBox.Set(dynamicBoxVerts, 4);
                    //dynamicBox.SetAsBox(10, 10);
                    b2FixtureDef fixtureDef;
                    fixtureDef.shape = &dynamicBox;
                    fixtureDef.density = 1.0f;
                    fixtureDef.friction = 0.3f;
                    body->CreateFixture(&fixtureDef);

                    rigidBodies.push_back({ body });
                }
            }
#endif

            // allocate solid buffer
            solidBuffer = new ui8[width * height];
        }

        ~Simulation() {
            delete[] solidBuffer;
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

            /*
                So why are we altering the update direction every tick?
                Because we don't want to bias the update in a particular direction, since the
                simulation should be symmetric.

                For example, if we were to set dir = 0 on every tick, water will move 
                more readily leftwards, and falling sand will move more readily rightwards. 
            */
            int dir = tick % 4;
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
            
            // flush the data into the solid buffer
            for (i64 y = 0; y < height; y++) {
                for (i64 x = 0; x < width; x++) {
                    solidBuffer[y * width + x] = grid(x, y).t == FIRE ? grid(x, y).secondary_t->isSolid : grid(x, y).t->isSolid;
                }
            }

#ifdef SIMULATE_RIGID_BODIES
            // do marching squares
            MarchingSquares::MarchingSquares(width, height, solidBuffer, contours);

            // convert contours to polygons using polypartition
            TPPLPartition partition;
            
            TPPLPolyList polyList;
            
            for (auto contour : contours) {
                std::reverse(contour.vertices.begin(), contour.vertices.end());

                TPPLPoly poly;
                i64 numPoints = contour.vertices.size();
                poly.Init(numPoints);
                for (int i = 0; i < numPoints; i++) {
                    TPPLPoint& p = poly.GetPoint(i);
                    p.x = contour.vertices[i].x;
                    p.y = contour.vertices[i].y;
                }

                polyList.push_back(poly);
            }

            triangles.clear();
            partition.Triangulate_MONO(&polyList, &triangles);

            std::vector<b2Body*> staticBodies;

            // add these constraints to the world!
            b2BodyDef posDef;
            posDef.position.Set(0, 0);

            b2Vec2 triBuffer[3];
            for (auto triangle : triangles) {
                triBuffer[0] = { (float) triangle.GetPoint(0).x, (float) triangle.GetPoint(0).y };
                triBuffer[1] = { (float) triangle.GetPoint(1).x, (float) triangle.GetPoint(1).y };
                triBuffer[2] = { (float) triangle.GetPoint(2).x, (float) triangle.GetPoint(2).y };

                b2Body* groundBody = world.CreateBody(&posDef);
                b2PolygonShape triangleShape;
                triangleShape.Set(triBuffer, 3);
                groundBody->CreateFixture(&triangleShape, 0);

                staticBodies.push_back(groundBody);
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
                // printf("%4.2f %4.2f %4.2f\n", pos.x, pos.y, angle);
            }

            for (auto staticBody : staticBodies) {
                world.DestroyBody(staticBody);
            }
#endif
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