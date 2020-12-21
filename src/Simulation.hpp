#pragma once


#include <box2d/b2_api.h>
#include <box2d/b2_math.h>
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_chain_shape.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_distance_joint.h>
#include <box2d/b2_revolute_joint.h>
#include <box2d/b2_circle_shape.h>
#include <algorithm>
#include <thread>
#include <mutex>

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
    const ParticleType smoke(6, glm::vec3{ 0.1, 0.1, 0.1 }, .99999999, 0, 0, 0, true, false, "Smoke");
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

    class QueryAABBCallback : public b2QueryCallback {
    public:
        QueryAABBCallback(bool* called) : called(called) {};
        bool* called;
        bool ReportFixture(b2Fixture* fixture) {
            *called = true;
            return true;
        }
    };

    bool BodiesWithinAABB(const b2World& world, const b2AABB& aabb) {
        // only do this if there is a rigid body in this chunk
        bool called = false;
        QueryAABBCallback callback(&called);
        world.QueryAABB(&callback, aabb);
        return called;
    }

    class Simulation {
    public:
        std::string name;
        ui64 width, height;
        const ParticleType* currentParticleType;

        /** RIGID BODIES **/
#ifdef SIMULATE_RIGID_BODIES
        std::vector<RigidBody> rigidBodies;
        b2Vec2 gravity;
        b2World world;

        std::mutex triangles_mutex;
        TPPLPolyList triangles;

#ifdef DEBUG_DRAW
        std::mutex contours_mutex;
        std::vector<MarchingSquares::Contour> contours;
#endif
#endif


        Grid grid;
        ui8* solidBuffer;

        /** UI STATE **/
        bool tabPressed;
        bool paused;
        float radius;

        Simulation(std::string name, ui64 width, ui64 height) :
            name(name), width(width), height(height),
            currentParticleType(SAND),
            grid(width, height),
            paused(true),
            radius(5.0),
            tabPressed(false)
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

            // we give an extra pixel here because of the marching squares optimization
            b2Vec2 groundBoxVerts[4] = { {-1, -1}, {-1, (float)height + 1}, {(float)width + 1, (float)height + 1}, {(float)width + 1, -1} };
            groundBox.CreateLoop(groundBoxVerts, 4);
            groundBody->CreateFixture(&groundBox, 1);

#if SPAWN_BODY==PENDULUM
            for (int i = 0; i < 20; i++) {
                i64 thisX = 100 + i * 10;
                b2BodyDef ropeBodyDef;
                ropeBodyDef.type = (i == 0) ? b2_staticBody : b2_dynamicBody;
                ropeBodyDef.position.Set(thisX, 200);
                b2Body* ropeBody = world.CreateBody(&ropeBodyDef);
                b2PolygonShape ropeShape;
                ropeShape.SetAsBox(5, i == 19 ? 5 : 0.5);
                ropeBody->CreateFixture(&ropeShape, 1);
                rigidBodies.push_back({ ropeBody });
                if (i > 0) {
                    b2RevoluteJointDef jd;
                    // b2DistanceJointDef jd;
                     //jd.stiffness = 10000;
                    jd.Initialize(rigidBodies[i].body, rigidBodies[i - 1].body, b2Vec2(thisX - 5, 200));
                    world.CreateJoint(&jd);
                }
            }
#elif SPAWN_BODY==BOUNCE
            for (int i = 0; i < 20; i++) {
                i64 thisX = 100 + i * 10;
                b2BodyDef ropeBodyDef;
                ropeBodyDef.type = (i == 0 || i == 19) ? b2_staticBody : b2_dynamicBody;
                ropeBodyDef.position.Set(thisX, 100);
                b2Body* ropeBody = world.CreateBody(&ropeBodyDef);
                b2PolygonShape ropeShape;
                ropeShape.SetAsBox(5, 0.5);
                ropeBody->CreateFixture(&ropeShape, 1);
                rigidBodies.push_back({ ropeBody });
                if (i > 0) {
                    b2RevoluteJointDef jd;
                    // b2DistanceJointDef jd;
                     //jd.stiffness = 10000;
                    jd.Initialize(rigidBodies[i].body, rigidBodies[i - 1].body, b2Vec2(thisX - 5, 100));
                    world.CreateJoint(&jd);
                }
                if (i == 19) {
                    //rigidBodies[i].body->SetTransform(b2Vec2(thisX - 5, 160), 0);
                }
            }
#elif SPAWN_BODY==CAR

            // create car body
            b2BodyDef carbodyDef, w1def, w2def;
            carbodyDef.type = b2_dynamicBody;
            w1def.type = b2_dynamicBody;
            w2def.type = b2_dynamicBody;
            carbodyDef.position.Set(100, 200);
            w1def.position.Set(82, 180);
            w2def.position.Set(128, 180);

            b2Body *carBody, *w1Body, *w2Body;
            carBody = world.CreateBody(&carbodyDef);
            w1Body = world.CreateBody(&w1def);
            w2Body = world.CreateBody(&w2def);

            b2PolygonShape bodyShape;
            b2CircleShape w1Shape, w2Shape;

            bodyShape.SetAsBox(25, 12.5);
            w1Shape.m_radius = 7;
            w2Shape.m_radius = 7;

            carBody->CreateFixture(&bodyShape, 1);
            b2Fixture* wheelFixture = w1Body->CreateFixture(&w1Shape, 1);
            wheelFixture->SetFriction(1);
            w2Body->CreateFixture(&w2Shape, 1);

            b2RevoluteJointDef axle1, axle2;
            axle1.Initialize(carBody, w1Body, w1def.position);
            axle1.enableMotor = true;
            axle1.motorSpeed = -3;
            axle1.maxMotorTorque = 100000000;
            axle2.Initialize(carBody, w2Body, w2def.position);
            world.CreateJoint(&axle1);
            world.CreateJoint(&axle2);
            

            
            rigidBodies.push_back({ carBody });
            rigidBodies.push_back({ w1Body });
            rigidBodies.push_back({ w2Body });
#endif
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

        void TickChunk(i64 i, i64 j, ui8 dir) {
            i64 xStart = i * CHUNK_SIZE;
            i64 yStart = j * CHUNK_SIZE;
            i64 xEnd = std::min<i64>(xStart + CHUNK_SIZE, width);
            i64 yEnd = std::min<i64>(yStart + CHUNK_SIZE, height);
            i64 xStride = xEnd - xStart;
            i64 yStride = yEnd - yStart;

            // tick here
            /*
                So why are we altering the update direction every tick?
                Because we don't want to bias the update in a particular direction, since the
                simulation should be symmetric.

                For example, if we were to set dir = 0 on every tick, water will move
                more readily leftwards, and falling sand will move more readily rightwards.
            */
            if (dir == 0) {
                for (i64 y = yStart; y < yEnd; y++) {
                    for (i64 x = xStart; x < xEnd; x++) {
                        TickParticle(x, y);
                    }
                }
            }
            else if (dir == 1) {
                for (i64 y = yStart; y < yEnd; y++) {
                    for (i64 x = xEnd - 1; x >= xStart; x--) {
                        TickParticle(x, y);
                    }
                }
            }
            else if (dir == 2) {
                for (i64 y = yEnd - 1; y >= yStart; y--) {
                    for (i64 x = xEnd - 1; x >= xStart; x--) {
                        TickParticle(x, y);
                    }
                }
            }
            else {
                for (i64 y = yEnd - 1; y >= yStart; y--) {
                    for (i64 x = xStart; x < xEnd; x++) {
                        TickParticle(x, y);
                    }
                }
            }
        }

        void Tick(i64 tick) {
            for (i64 i = 0; i < width * height; i++) {
                grid(i).updated = false;
            }

            // find number of chunks
            i64 xChunks = (i64)((width + (CHUNK_SIZE - 1)) / CHUNK_SIZE);
            i64 yChunks = (i64)((height + (CHUNK_SIZE - 1)) / CHUNK_SIZE);

            ui8 dir = tick % 4;
            // round one of four
#pragma omp parallel for
            for (int i = 0; i < xChunks; i += 2) {
#pragma omp parallel for
                for (int j = 0; j < yChunks; j += 2) {
                    TickChunk(i, j, dir);
                }
            }

#pragma omp parallel for
            for (int i = 1; i < xChunks; i += 2) {
#pragma omp parallel for
                for (int j = 0; j < yChunks; j += 2) {
                    TickChunk(i, j, dir);
                }
            }

#pragma omp parallel for
            for (int i = 1; i < xChunks; i += 2) {
#pragma omp parallel for
                for (int j = 1; j < yChunks; j += 2) {
                    TickChunk(i, j, dir);
                }
            }

#pragma omp parallel for
            for (int i = 0; i < xChunks; i += 2) {
#pragma omp parallel for
                for (int j = 1; j < yChunks; j += 2) {
                    TickChunk(i, j, dir);
                }
            }

            // flush the data into the solid buffer
            for (i64 y = 0; y < height; y++) {
                for (i64 x = 0; x < width; x++) {
                    Particle& p = grid(x, y);
                    solidBuffer[y * width + x] = p.t == FIRE ? p.secondary_t->isSolid : p.t->isSolid;
                    //solidBuffer[y * width + x] = p.t != AIR;
                }
            }

            // reset triangles and contours
            triangles.clear();
#ifdef DEBUG_DRAW
            contours.clear();
#endif


#ifdef SIMULATE_RIGID_BODIES
#pragma omp parallel for collapse(2) schedule(dynamic)
            for (int i = 0; i < xChunks; i++) {
                for (int j = 0; j < yChunks; j++) {


                    i64 xStart = i * CHUNK_SIZE;
                    i64 yStart = j * CHUNK_SIZE;
                    i64 xEnd = std::min<i64>(xStart + CHUNK_SIZE, width);
                    i64 yEnd = std::min<i64>(yStart + CHUNK_SIZE, height);
                    i64 xStride = xEnd - xStart;
                    i64 yStride = yEnd - yStart;

                    b2AABB aabb = b2AABB{ b2Vec2((float)xStart, (float)yStart), b2Vec2((float)xEnd, (float)yEnd) };

                    if (!BodiesWithinAABB(world, aabb)) continue;

                    // do marching squares
                    std::vector<MarchingSquares::Contour> chunkContours;
                    TPPLPolyList chunkTriangles;
                    TPPLPartition partition;
                    TPPLPolyList polyList;

                    MarchingSquares::MarchingSquares(xStart, yStart, xStride, yStride, width, height, solidBuffer, chunkContours);


                    // convert contours to polygons using polypartition
                    for (auto contour : chunkContours) {
                        TPPLPoly poly;
                        i64 numPoints = contour.vertices.size();
                        poly.Init(numPoints);
                        for (int i = 0; i < numPoints; i++) {
                            TPPLPoint& p = poly.GetPoint(i);
                            p.x = contour.vertices[i].x;
                            p.y = contour.vertices[i].y;
                        }

                        if (poly.GetOrientation() == TPPL_CW) {
                            poly.SetHole(true);
                        }

                        polyList.push_back(poly);
                    }

                    TPPLPolyList tmpPolys;
                    partition.RemoveHoles(&polyList, &tmpPolys);
                    partition.Triangulate_EC(&tmpPolys, &chunkTriangles);

                    // flush triangles to global list
                    {
                        const std::lock_guard<std::mutex> lock(triangles_mutex);
                        triangles.insert(triangles.end(), chunkTriangles.begin(), chunkTriangles.end());
                    }

#ifdef DEBUG_DRAW
                    {
                        const std::lock_guard<std::mutex> lock(contours_mutex);
                        contours.insert(contours.end(), chunkContours.begin(), chunkContours.end());
                    }
#endif
                }
            }

            std::vector<b2Body*> staticBodies;

            // add these constraints to the world!
            b2BodyDef posDef;
            posDef.position.Set(0, 0);

            b2Vec2 triBuffer[3];
            for (auto triangle : triangles) {
                triBuffer[0] = { (float)triangle.GetPoint(0).x, (float)triangle.GetPoint(0).y };
                triBuffer[1] = { (float)triangle.GetPoint(1).x, (float)triangle.GetPoint(1).y };
                triBuffer[2] = { (float)triangle.GetPoint(2).x, (float)triangle.GetPoint(2).y };

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