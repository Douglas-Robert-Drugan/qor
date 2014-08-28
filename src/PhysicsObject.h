#ifndef _IPHYSICSOBJECT_H
#define _IPHYSICSOBJECT_H

#include <memory>
#include <vector>

#include "Physics.h"
#include "kit/math/common.h"
#include "Node.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

class PhysicsObject:
    public btMotionState
{
public:

    PhysicsObject(){}
    virtual ~PhysicsObject() {}
    
    Physics* physics() { return m_pPhysics; }
    const Physics* physics() const { return m_pPhysics; }
    //NewtonBody* getPhysicsBody() { return m_pBody; }
    void* body() { return nullptr; } //placeholder
    void set_physics(Physics* sys) {
        m_pPhysics = sys;
    }
    
    virtual float radius() const { return 0.0f; }
    virtual float height() const { return 0.0f; }

    virtual void sync(glm::mat4* m) {}
    
    virtual Node::Physics physics_type() const { return m_Type; }
    void physics_type(Node::Physics t) {
        m_Type = t;
    }
    //virtual unsigned int physicsLogic(float timestep, float mass, glm::vec3& force, glm::vec3& omega, glm::vec3& torque, glm::vec3& velocity);
    virtual float mass() const { return 0.0f; }
    
    virtual void set_world_transform(const btTransform& worldTrans) {
        Node* node = dynamic_cast<Node*>(this);
        glm::mat4 matrix = Physics::fromBulletTransform(worldTrans);
        if(!sync(matrix))
            *node->matrix() = matrix;
        node->pend();
    }
    virtual void get_world_transform(btTransform& worldTrans) const {
        const Node* node = dynamic_cast<const Node*>(this);
        worldTrans = Physics::toBulletTransform(*node->matrix_c());
    }
    
    // returns true if the object will sync its own properties, otherwise false to autosync
    virtual bool sync(glm::mat4& m) {
        return false;
    }

    virtual void set_kinematic_pos(btTransform &currentPos) {}

    void add_striding_mesh_interface(std::unique_ptr<btStridingMeshInterface>& a) {
        m_StridingMeshInterfaces.push_back(std::move(a));
    }
    void add_collision_shape(std::unique_ptr<btCollisionShape>& a) {
        m_CollisionShapes.push_back(std::move(a));
    }
    void action(std::unique_ptr<btActionInterface>& a) {
        m_ActionInterface = std::move(a);
    }
    btActionInterface* action() {
        return m_ActionInterface.get();
    }
    btKinematicCharacterController* character() {
        return dynamic_cast<btKinematicCharacterController*>(m_ActionInterface.get());
    }
    void ghost_pair_callback(std::unique_ptr<btGhostPairCallback>& a) {
        m_GhostPairCallback = std::move(a);
    }
protected:
    std::unique_ptr<btCollisionObject> m_pBody;
    std::unique_ptr<btActionInterface> m_ActionInterface;
    std::unique_ptr<btGhostPairCallback> m_GhostPairCallback;
    std::vector<std::unique_ptr<btStridingMeshInterface>> m_StridingMeshInterfaces;
    std::vector<std::unique_ptr<btCollisionShape>> m_CollisionShapes;
    
    //NewtonWorld* m_pWorld; // weak
    //std::unique_ptr<NewtonBody> m_pBody;
    //NewtonBody* m_pBody;
    Physics* m_pPhysics = nullptr;
    Node::Physics m_Type = Node::Physics::NONE;
};

#endif

