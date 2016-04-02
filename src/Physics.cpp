#ifndef QOR_NO_PHYSICS

#include "Physics.h"
#include <glm/glm.hpp>
#include "Node.h"
//#include "IMeshContainer.h"
//#include "NodeAttributes.h"
#include "Node.h"
#include "Graphics.h"
#include "PhysicsObject.h"
#include "Mesh.h"
#include <iostream>
#include <memory>
#include "kit/math/vectorops.h"
using namespace std;
using namespace glm;

Physics::Physics(Node* root, void* userdata):
    m_pRoot(root)
{
    m_pCollisionConfig = kit::make_unique<btDefaultCollisionConfiguration>();
    m_pDispatcher = kit::make_unique<btCollisionDispatcher>(m_pCollisionConfig.get());
    //m_pBroadphase = kit::make_unique<btDbvtBroadphase>();
    btVector3 worldMin(-1000,-1000,-1000);
    btVector3 worldMax(1000,1000,1000);
    m_pBroadphase = kit::make_unique<btDbvtBroadphase>();
    //m_pBroadphase = kit::make_unique<btAxisSweep3>(worldMin, worldMax);
    m_pSolver = kit::make_unique<btSequentialImpulseConstraintSolver>();
    m_pWorld = kit::make_unique<btDiscreteDynamicsWorld>(
        m_pDispatcher.get(),
        m_pBroadphase.get(),
        m_pSolver.get(),
        m_pCollisionConfig.get()
    );
    m_pWorld->setGravity(btVector3(0.0, -9.8, 0.0));
}

Physics :: ~Physics() {
    if(m_pWorld)
    {
        //NewtonDestroyAllBodies(m_pWorld);
        //NewtonDestroy(m_pWorld);
    }
}

void Physics :: logic(Freq::Time advance)
{
    static float accum = 0.0f;
    const float fixed_step = 1/60.0f;
    float timestep = advance.s();
    m_pWorld->stepSimulation(timestep, NUM_SUBSTEPS, fixed_step);

//    accum += timestep;
//    float tick = fixed_step / NUM_SUBSTEPS;
//    while(accum >= fixed_step / NUM_SUBSTEPS)
//    {
//        m_pWorld->stepSimulation(tick, NUM_SUBSTEPS, fixed_step);
//        //NewtonUpdate(m_pWorld, fixed_step);
//        //sync(m_pRoot, SYNC_RECURSIVE);
////#ifdef _NEWTON_VISUAL_DEBUGGER
////        NewtonDebuggerServe(m_pDebugger, m_pWorld);
////#endif
//        accum -= fixed_step;
//    }
}

void Physics :: generate(Node* node, unsigned flags, std::unique_ptr<mat4> transform)
{
    bool root = false;
    if(!node)
        return;

    // TODO: If no transform is given, derive world space transform from node
    if(!transform){
        transform = kit::make_unique<mat4>();
        root = true;
    }

    // apply transformation of node so the mesh vertices are correct
    *transform *= *node->matrix_c();
    //assert(transform->isIdentity());
    
    // Are there physics instructions?
    if(node->physics())
    {
        if(not node->body())
        {
            // Check if there's static geometry in this node, if so let's process it
            switch(node->physics())
            {
                case Node::Physics::STATIC:
                    if(node->physics_shape() == Node::MESH)
                        generate_tree(node, flags, transform.get());
                    else
                        generate_generic(node, flags, transform.get());
                    break;
                case Node::Physics::ACTOR:
                    generate_actor(node, flags, transform.get());
                    break;
                case Node::Physics::DYNAMIC:
                case Node::Physics::KINEMATIC:
                    generate_generic(node, flags, transform.get());
                    break;
                default:
                    //assert(false);
                    break;
            }
        }
    }

    // generate children
    if(node->has_children() && (flags & GEN_RECURSIVE))
    {
        for(auto&& child: node->subnodes())
        {
            // copy current node's transform so it can be modified by child
            std::unique_ptr<mat4> transform_copy =
                kit::make_unique<mat4>(*transform);
            generate(child.get(), flags, std::move(transform_copy));
        }
    }
    
    // delete generated identity matrix for those who passed in null matrix pointers
    //if(created_transform)
    //    delete transform;

    if(root){
        m_onGenerate();
        m_onGenerate = boost::signals2::signal<void()>();
    }
}

void Physics :: generate_actor(Node* node, unsigned int flags, mat4* transform)
{
    assert(node);
    assert(transform);
    assert(node->physics());

    //Actor* actor = dynamic_cast<Actor*>(node);
    
    // TODO: generate code
}

void Physics :: generate_tree(Node* node, unsigned int flags, mat4* transform)
{
    assert(node);
    assert(transform);
    if(node->physics() != Node::STATIC)
        return;
    if(node->physics_shape() != Node::MESH)
        return;
    //assert(node->physics() == Node::STATIC);
    //assert(node->physics_shape() == Node::MESH);

    Mesh* mesh = dynamic_cast<Mesh*>(node);
    if(not mesh or mesh->empty())
        return;
    
    auto triangles = kit::make_unique<btTriangleMesh>();
    auto verts = mesh->internals()->geometry->ordered_verts();
    for(int i = 0; i < verts.size(); i += 3)
    {
        triangles->addTriangle(
            btVector3(verts[i].x, verts[i].y,  verts[i].z),
            btVector3(verts[i+1].x, verts[i+1].y,  verts[i+1].z),
            btVector3(verts[i+2].x, verts[i+2].y,  verts[i+2].z)
        );
    }
    
    reset_body(node);
    auto physics_object = node->body();
    assert(physics_object.get());
    unique_ptr<btCollisionShape> shape = kit::make_unique<btBvhTriangleMeshShape>(
        triangles.get(), true, true
    );
    btRigidBody::btRigidBodyConstructionInfo info(
        0.0f,
        physics_object.get(), // inherits btMotionState
        shape.get()
    );
    auto body = kit::make_unique<btRigidBody>(info);
    body->setWorldTransform(toBulletTransform(*node->matrix(Space::WORLD)));
    body->setUserPointer((void*)node);
    auto interface = unique_ptr<btStridingMeshInterface>(std::move(triangles));
    physics_object->add_striding_mesh_interface(interface);
    physics_object->add_collision_shape(shape);
    physics_object->body(std::move(body));
    physics_object->system(this);
    m_pWorld->addRigidBody((btRigidBody*)physics_object->body());
}

void Physics :: reset_body(Node* node)
{
    if(node->body())
        node->clear_body();
    node->reset_body();
}

unique_ptr<btCollisionShape> Physics :: generate_shape(Node* node)
{
    unique_ptr<btCollisionShape> shape;
    switch(node->physics_shape())
    {
        case Node::HULL:
        {
            auto mesh = dynamic_cast<Mesh*>(node);
            if(mesh->composite())
            {
                shape = kit::make_unique<btConvexHullShape>();
                node->each([&shape](Node* n){
                    auto mesh = dynamic_cast<Mesh*>(n);
                    if(not mesh || not mesh->geometry())
                        return;
                    for(auto&& v: mesh->geometry()->verts())
                        ((btConvexHullShape*)shape.get())->addPoint(Physics::toBulletVector(
                            vec3(*mesh->matrix() * vec4(v,1.0))
                        ));
                });
            }else{
                if(not mesh || not mesh->geometry())
                    return nullptr;
                shape = kit::make_unique<btConvexHullShape>();
                for(auto&& v: mesh->geometry()->verts())
                    ((btConvexHullShape*)shape.get())->addPoint(Physics::toBulletVector(
                        v
                    ));
            }
            break;
        }
        case Node::BOX:
            shape = kit::make_unique<btBoxShape>(
                toBulletVector(node->box().size() / 2.0f)
            );
            break;
        case Node::CAPSULE:
            shape = kit::make_unique<btCapsuleShape>(
                node->box().size().y / 2.0f,
                std::max(node->box().size().x, node->box().size().z) / 2.0f
            );
            break;
        case Node::CYLINDER:
            shape = kit::make_unique<btCylinderShape>(
                Physics::toBulletVector(node->box().size() / 2.0f)
            );
            break;
        default:
            assert(false);
    };
    return shape;
}

void Physics :: generate_generic(Node* node, unsigned int flags, mat4* transform)
{
    assert(node);
    assert(transform);
    assert(node->physics());
    //assert(node->physics() == Node::DYNAMIC);

    Mesh* mesh = dynamic_cast<Mesh*>(node);
    if(not mesh)
        return;

    mesh->each([](Node* n){
        auto m = std::dynamic_pointer_cast<Mesh>(n->as_node());
        if(m)
            m->set_physics(Node::NO_PHYSICS);
    });
    node->reset_body();
    auto physics_object = node->body();
    assert(physics_object.get());
    auto b = node->box();
    
    unique_ptr<btCollisionShape> shape = generate_shape(node);
    if(not shape)
        return;
    
    btVector3 inertia;
    inertia.setZero();
    if(node->has_inertia())
        shape->calculateLocalInertia(node->mass(), inertia);
    btRigidBody::btRigidBodyConstructionInfo info(
        mesh->mass(),
        physics_object.get(), // inherits btMotionState
        shape.get(),
        inertia
    );
    auto body = kit::make_unique<btRigidBody>(info);
    //body->setCcdMotionThreshold(0.001f);
    body->setUserPointer((void*)node);
    if(node->physics() == Node::KINEMATIC)
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    if(node->friction() >= 0.0f - K_EPSILON) // negative values (like -1) have default friction
        body->setFriction(node->friction());
    auto boxsize = node->box().size();
    auto minsz = std::min(std::min(boxsize.x, boxsize.y), boxsize.z);
    //LOGf("minsz: %s", minsz);
    //body->setCcdSweptSphereRadius(2.0f * minsz);
    //auto interface = unique_ptr<btStridingMeshInterface>(std::move(triangles));
    //physics_object->add_striding_mesh_interface(interface);
    physics_object->add_collision_shape(shape);
    physics_object->body(std::move(body));
    physics_object->system(this);
    m_pWorld->addRigidBody((btRigidBody*)physics_object->body());
    //std::vector<shared_ptr<Mesh>> meshes = node->children<Mesh>();
    //if(meshes.empty())
    //    return;
    //Node* physics_object = dynamic_cast<Node*>(node);
}

// syncBody gets the data out of physics subsystem, reports it back to each node
void Physics :: sync(Node* node, unsigned int flags)
{
    if(!node)
        return;
    if(!node->physics())
        return;
    
    if(node->physics() != Node::Physics::STATIC)
    {
        mat4 body_matrix;
        //NewtonBodyGetMatrix((NewtonBody*)node->body(), value_ptr(body_matrix));
        node->sync(body_matrix);

        // NOTE: Remember to update the transform from the object side afterwards.
    }

    if(flags & SYNC_RECURSIVE)
        for(auto&& child: *node)
            sync(child.get(), flags);
}

//NewtonBody* Physics :: add_body(NewtonCollision* nc, Node* node, mat4* transform)
//{
//    return nullptr;
//}

//btRigidBody* Physics :: add_body(btCollisionObject* obj, Node* node, mat4* transform, btVector3* inertia)
//{
    //float mass = node->mass();

    //vec3 inertia, origin;
    //NewtonBody* body = NewtonCreateBody(m_pWorld, nc, value_ptr(*transform));
    //NewtonBodySetUserData(body, node);
    
    //btTransform btt;
    //btt.setFromOpenGLMatrix(value_ptr(transform));
    //btRigidBody* body = new bt

    //node->setPhysicsBody(this, (void*)body, (void*)motion);
    //m_pWorld->addRigidBody(body);
    
    //if(mass > EPSILON)
    //{
    //    NewtonConvexCollisionCalculateInertialMatrix(nc, value_ptr(inertia), value_ptr(origin));
    //    NewtonBodySetMassMatrix(body, mass, inertia.x * mass, inertia.y * mass, inertia.z * mass);
    //    NewtonBodySetCentreOfMass(body, value_ptr(origin));
    //    NewtonBodySetForceAndTorqueCallback(body, (NewtonApplyForceAndTorque)&cbForceTorque);
    //    NewtonBodySetTransformCallback(body, (NewtonSetTransform)&cbTransform);
    //}

//    return nullptr;
//}

//bool Physics :: delete_body(void* obj)
//{
//    if(!obj)
//        return false;
//    m_pWorld->removeCollisionObject((btRigidBody*)obj);
//    delete (btRigidBody*)obj;
//    return true;
//}

//void Physics :: cb_force_torque(const NewtonBody* body, float timestep, int threadIndex)
//{
//    float mass, ix, iy, iz;
//    //NewtonBodyGetMassMatrix(body, &mass, &ix, &iy, &iz);
//    vec3 force(0.0f, mass * -9.8f, 0.0f);
//    vec3 omega(0.0f);
//    vec3 velocity(0.0f);
//    vec3 torque(0.0f);
//    //NewtonBodyGetVelocity(body, value_ptr(velocity));

//    //Node* node = (Node*)NewtonBodyGetUserData(body);
//    unsigned int userflags = 0;
//    //node->on_physics_tick(
//    //    Freq::Time::seconds(timestep), mass, force, omega, torque, velocity, &userflags
//    //);
    
//    //if(userflags & USER_FORCE)
//    //    NewtonBodyAddForce(body, value_ptr(force));
//    //if(userflags & USER_OMEGA)
//    //    NewtonBodySetOmega(body, value_ptr(omega));
//    //if(userflags & USER_TORQUE)
//    //    NewtonBodySetTorque(body, value_ptr(torque));
//    //if(userflags & USER_VELOCITY)
//    //    NewtonBodySetVelocity(body, value_ptr(velocity));
//}

//void Physics :: cb_transform(const NewtonBody* body)
//{
//    //Node* node = (Node*)NewtonBodyGetUserData(body);
    
//    //float marray[16];
//    //NewtonBodyGetMatrix(body, &marray[0]);
    
//    //mat4 m = Matrix::from_array(marray);
//    //node->sync(m);
//}

tuple<Node*, vec3, vec3> Physics :: first_hit(vec3 start, vec3 end)
{
    auto s = toBulletVector(start);
    auto e = toBulletVector(end);
    btCollisionWorld::ClosestRayResultCallback ray(s,e);
    m_pWorld->rayTest(s,e,ray);
    bool b = ray.hasHit();
    return std::tuple<Node*,vec3,vec3>(
        b ?
            (Node*)ray.m_collisionObject->getUserPointer() :
            nullptr,
        b ? Physics::fromBulletVector(ray.m_hitPointWorld) : vec3(0.0f),
        b ? Physics::fromBulletVector(ray.m_hitNormalWorld) : vec3(0.0f)
    );
}

vector<tuple<Node*, vec3, vec3>> Physics :: hits(vec3 start, vec3 end)
{
    vector<tuple<Node*, vec3, vec3>> r;
    auto s = toBulletVector(start);
    auto e = toBulletVector(end);
    btCollisionWorld::AllHitsRayResultCallback ray(s,e);
    m_pWorld->rayTest(s,e,ray);
    bool b = ray.hasHit();
    if(b)
    {
        r.reserve(ray.m_collisionObjects.size());
        for(int i=0; i<ray.m_collisionObjects.size(); ++i)
        {
            r.push_back(std::make_tuple(
                (Node*)ray.m_collisionObjects[i]->getUserPointer(),
                Physics::fromBulletVector(ray.m_hitPointWorld[i]),
                Physics::fromBulletVector(ray.m_hitNormalWorld[i])
            ));
        }
    }
    else
    {
        return vector<tuple<Node*, vec3, vec3>>();
    }
    return r;
}

//std::tuple<Node*, vec3, vec3> Physics :: first_other_hit(
//    Node* me, vec3 start, vec3 end
//){
//    auto s = toBulletVector(start);
//    auto e = toBulletVector(end);
//    btCollisionWorld::btKinematicClosestNotMeRayResultCallback ray(s,e);
//    m_pWorld->rayTest(s,e,ray);
    
//}

void Physics :: gravity(glm::vec3 v)
{
    m_pWorld->setGravity(Physics::toBulletVector(v));
}

glm::vec3 Physics :: gravity()
{
    return Physics::fromBulletVector(m_pWorld->getGravity());
}

#endif

