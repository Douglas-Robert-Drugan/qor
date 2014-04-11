#ifndef _BASICSTATE_H_VZ3QNB09
#define _BASICSTATE_H_VZ3QNB09

#include "State.h"
#include "Input.h"
#include "TileMap.h"
#include "Camera.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "Sprite.h"
#include "PlayerInterface2D.h"

class Qor;

class BasicState:
    public State
{
    public:
        BasicState(Qor* engine);
        virtual ~BasicState();

        virtual void preload() override;
        virtual void logic(Freq::Time t) override;
        virtual void render() const override;
        virtual bool needs_load() const override {
            return false;
        }

    private:
        Qor* m_pQor = nullptr;
        Input* m_pInput = nullptr;

        std::shared_ptr<Node> m_pRoot;
        //std::shared_ptr<Node> m_pTemp;
        std::shared_ptr<Sprite> m_pSprite;
        std::shared_ptr<PlayerInterface2D> m_pPlayer;
        std::shared_ptr<TileMap> m_pMap;
        std::shared_ptr<Camera> m_pCamera;
        std::shared_ptr<Pipeline> m_pPipeline;
        //std::shared_ptr<BasicPhysics> m_pPhysics;
};

#endif

