#ifndef _PHYSICS_H
#define _PHYSICS_H

#include <memory>
//#include <newton/Newton.h>
//#include <PxPhysicsAPI.h>
#include "kit/math/common.h"
//#include <btBulletCollisionCommon.h>
//#include <btBulletDynamicsCommon.h>
//#include <BulletCollision/CollisionDispatch/btGhostObject.h>
//#include "extra/KinematicCharacterController.h"
//#include <PxPhysicsAPI.h>
#include "kit/log/log.h"
#include "kit/kit.h"
#include "IPhysics.h"

class IPhysicsObject;
//class Node;
//class Scene;

class Node;

class Physics:
    public IPhysics
{
//protected:
    //virtual void failsafe();
private:

    static const int NUM_SUBSTEPS = 7;

    //physx::PxFoundation* m_pFoundation;
    //physx::PxPhysics* m_pPhysics;
    //physx::PxProfileZoneManager* m_pProfileZoneManager;
    //physx::PxCooking* m_pCooking;
    //physx::PxDefaultAllocator* m_DefaultAllocatorCallback;
    //physx::PxDefaultErrorCallback* m_DefaultErrorCallback;

    //NewtonWorld* m_pWorld;
    ////std::unique_ptr<NewtonWorld> m_pWorld;
    //std::unique_ptr<btDefaultCollisionConfiguration> m_pCollisionConfig;
    //std::unique_ptr<btCollisionDispatcher> m_pDispatcher;
    //std::unique_ptr<btBroadphaseInterface> m_pBroadphase;
    //std::unique_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
    //std::unique_ptr<btDiscreteDynamicsWorld> m_pWorld;

//#ifndef _NEWTON_VISUAL_DEBUGGER
//    void* m_pDebugger;
//#endif

    void generate_actor(Node* node, unsigned flags, glm::mat4* transform);
    void generate_tree(Node* node, unsigned flags, glm::mat4* transform);
    void generate_dynamic(Node* node, unsigned flags, glm::mat4* transform);

public:

    //static btVector3 to_bullet(const glm::vec3& v) {
    //    return btVector3(v.x,v.y,v.z);
    //}
    //static glm::vec3 from_vector(const btVector3& v){
    //    return glm::vec3(v.x(),v.y(),v.z());
    //}
    //static btTransform to_bullet(const glm::mat4& m) {
    //    btTransform t;
    //    t.setFromOpenGLMatrix(glm::value_ptr(m));
    //    return t;
    //}
    //static glm::mat4 from_bullet(const btTransform& t) {
    //    glm::mat4 m;
    //    t.getOpenGLMatrix(glm::value_ptr(m));
    //    return m;
    //}

    Physics();
    virtual ~Physics() {}

    /*! Physics Logic
     * \param advance Ticks (in ms) to advanced simulation.
     * \param root Root of physics node which can be automatically synced
     */
    virtual void logic(Freq::Time t);

    enum class GenerateFlag: unsigned {
        RECURSIVE = kit::bit(0)
    };
    /*! Generate Physics
     *  \param node Generates physics for a specific node.
     *  \param flags GenerationFlags
     *  \param matrix Current transformation matrix
     */
    void generate(Node* node, unsigned flags = 0, std::unique_ptr<glm::mat4> transform = std::unique_ptr<glm::mat4>());
    
    enum SyncFlags {
        SYNC_RECURSIVE = kit::bit(0)
    };
    /*! Sync Body
     *  Syncronizes a single nodes' transformation matrix with the equivalent physics body.
     *  \param node The node to be syncronized
     *  \param flags Syncronization options (use SyncFlags)
     *  \param matrix Current transformation matrix
     */
    void sync(Node* node, unsigned flags = 0);

    // IPhysicsObject param is required to also be of type Node*
    //btRigidBody* add_body(btCollisionObject* obj, IPhysicsObject* pud, glm::mat4* transform);
    //bool deleteBody(PhysicsBody* obj);

    //enum {
    //    USER_FORCE = kit::bit(0),
    //    USER_OMEGA = kit::bit(1),
    //    USER_TORQUE = kit::bit(2),
    //    USER_VELOCITY = kit::bit(3)
    //};
    //static void cbForceTorque(const NewtonBody* body, float timestep, int threadIndex);
    //static void cbTransform(const NewtonBody* body);
    //btCollisionWorld* world() { return m_pWorld.get(); }
    //NewtonWorld* getWorld() { return m_pWorld; }
};

#endif

