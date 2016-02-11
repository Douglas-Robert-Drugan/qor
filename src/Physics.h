#ifndef QOR_NO_PHYSICS
#ifndef _PHYSICS_H
#define _PHYSICS_H

#include <memory>
#include "kit/math/common.h"
#include "Graphics.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include "kit/log/log.h"
#include "kit/kit.h"
#include "IRealtime.h"

class Node;

class Physics:
    public IRealtime
{
public:

    // Original: http://bulletphysics.org/mediawiki-1.5.8/index.php/Collision_Callbacks_and_Triggers
    struct ContactSensorCallback : public btCollisionWorld::ContactResultCallback {
        ContactSensorCallback(btRigidBody& tgtBody)
            : btCollisionWorld::ContactResultCallback(), body(tgtBody){ }

        btRigidBody& body;

        virtual bool needsCollision(btBroadphaseProxy* proxy) const {
            if(!btCollisionWorld::ContactResultCallback::needsCollision(proxy))
                return false;
            return body.checkCollideWithOverride(static_cast<btCollisionObject*>(proxy->m_clientObject));
        }
        virtual btScalar addSingleResult(btManifoldPoint& cp,
            const btCollisionObjectWrapper* colObj0,int partId0,int index0,
            const btCollisionObjectWrapper* colObj1,int partId1,int index1)
        {
            btVector3 pt;
            if(colObj0->m_collisionObject==&body) {
                pt = cp.m_localPointA;
            } else {
                assert(colObj1->m_collisionObject==&body && "body does not match either collision object");
                pt = cp.m_localPointB;
            }
            return 0;
        }
    };
    // End

    static btVector3 toBulletVector(const glm::vec3& v) {
        return btVector3(v.x,v.y,v.z);
    }
    static glm::vec3 fromBulletVector(const btVector3& v){
        return glm::vec3(v.x(),v.y(),v.z());
    }
    static btTransform toBulletTransform(const glm::mat4& m) {
        btTransform t;
        t.setFromOpenGLMatrix(glm::value_ptr(m));
        return t;
    }
    static glm::mat4 fromBulletTransform(const btTransform& t) {
        glm::mat4 m;
        t.getOpenGLMatrix(glm::value_ptr(m));
        return m;
    }

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

    Physics(Node* root, void* userdata = nullptr);
    virtual ~Physics();

    /*! Physics Logic
     * \param advance Ticks (in ms) to advanced simulation.
     * \param root Root of physics node which can be automatically synced
     */
    virtual void logic(Freq::Time t) override;
    //virtual root(Node* root) {
    //    m_pRoot = root;
    //}
    //Node* root() {
    //    return m_pRoot;
    //}
    //const Node* root() const {
    //    return m_pRoot;
    //}

    enum class GenerateFlag: unsigned {
        RECURSIVE = kit::bit(0)
    };
    /*! Generate Physics
     *  \param node Generates physics for a specific node.
     *  \param flags GenerationFlags
     *  \param matrix Current transformation matrix (internal use)
     */
    void generate(Node* node, unsigned flags = 0, std::unique_ptr<glm::mat4> transform = std::unique_ptr<glm::mat4>());
    std::unique_ptr<btCollisionShape> generate_shape(Node* node);
    
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

    //btRigidBody* add_body(btCollisionObject* obj, Node* pud, glm::mat4* transform);
    //NewtonBody* add_body(NewtonCollision* nc, Node* node, glm::mat4* transform);
    //bool delete_body(void* obj);
    void reset_body(Node* n);

    enum {
        USER_FORCE = kit::bit(0),
        USER_OMEGA = kit::bit(1),
        USER_TORQUE = kit::bit(2),
        USER_VELOCITY = kit::bit(3)
    };
    //static void cb_force_torque(const NewtonBody* body, float timestep, int threadIndex);
    //static void cb_transform(const NewtonBody* body);
    //btCollisionWorld* world() { return m_pWorld.get(); }
    btDiscreteDynamicsWorld* world() { return m_pWorld.get(); }
    //NewtonWorld* world() { return m_pWorld; }
    //virtual void failsafe();
    
    std::tuple<Node*, glm::vec3, glm::vec3> first_hit(
        glm::vec3 start, glm::vec3 end
    );
    
    //std::tuple<Node*, glm::vec3, glm::vec3> first_other_hit(
    //    Node* me, glm::vec3 start, glm::vec3 end
    //);
    
private:

    static const int NUM_SUBSTEPS = 7;

    std::unique_ptr<btDefaultCollisionConfiguration> m_pCollisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_pDispatcher;
    std::unique_ptr<btBroadphaseInterface> m_pBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_pWorld;
    
    Node* m_pRoot = nullptr;
    
    void generate_actor(Node* node, unsigned flags, glm::mat4* transform);
    void generate_tree(Node* node, unsigned flags, glm::mat4* transform);
    void generate_dynamic(Node* node, unsigned flags, glm::mat4* transform);
};

#endif
#endif

