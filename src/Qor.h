#ifndef _ENGINE_H_HVSZVL8
#define _ENGINE_H_HVSZVL8

#include <memory>
#include <functional>
#include <deque>
#include <vector>
#include "Nodes.h"
#include "State.h"
#include "StateManager.h"
#include "Window.h"
#include "Input.h"
#include "IResource.h"
#include "Interpreter.h"
#include "Texture.h"
#include "IPhysics.h"
#include "Session.h"
#include "Audio.h"
#include "GUI.h"
#include "TaskHandler.h"
#include "kit/args/args.h"
#include "kit/meta/meta.h"
#include "kit/cache/cache.h"

class Qor:
    public StateManager<State>,
    public TaskHandler
    //public StateManager<State, unsigned, Qor*>
{
    public:
                
        Qor(int argc, const char** argv);
        virtual ~Qor();
        void run(unsigned state_id = 1);
        void logic();
        void render();

        void quit() { m_bQuit = true; }
        bool quit_flag() const { return m_bQuit; }

        virtual std::shared_ptr<State> new_state(unsigned id) override;

        Input* input() { return m_pInput.get(); }
        const Input* input() const { return m_pInput.get(); }
        Freq* timer() { return m_pTimer.get(); }
        const Freq* timer() const { return m_pTimer.get(); }
        Window* window() { return m_pWindow.get(); }
        const Window* window() const { return m_pWindow.get(); }
        GUI* gui() { return m_pGUI.get(); }
        const GUI* gui() const { return m_pGUI.get(); }
        IPhysics* physics() { return m_pPhysics.get(); }
        const IPhysics* physics() const { return m_pPhysics.get(); }
        Audio* audio() { return m_pAudio.get(); }
        const Audio* audio() const { return m_pAudio.get(); }
        Session* session() { return m_pSession.get(); }
        const Session* session() const { return m_pSession.get(); }
        Interpreter* interpreter() { return m_pInterpreter.get(); }
        const Interpreter* interpreter() const { return m_pInterpreter.get(); }

        //const std::shared_ptr<ResourceLocator>& locator() {
        //    return m_pLocator;
        //}
        //const std::shared_ptr<ResourceCache<Texture>>& textures() const {
        //    return m_pTextures;
        //}
        
        Cache<IResource, std::string>* resources() {
            return &m_Resources;
        }
        const Cache<IResource, std::string>* resources() const {
            return &m_Resources;
        }

        Factory<State, Qor*>& states() {
            return m_StateFactory;
        }
        virtual std::shared_ptr<State> current_state() {
            std::shared_ptr<State> s;
            unsigned id = 0;
            do{
                s = state(id);
                ++id;
            }while(s && s->is_loader());
            return s;
        }
        bool is_loading() const {
            return state()->is_loader();
        }
        Factory<Node, std::string>& nodes() {
            return m_NodeFactory;
        }

        virtual void clear_tasks() {
            auto l = std::unique_lock<std::mutex>(m_TasksMutex);
            m_Tasks.clear();
        }
        virtual void wait_task(std::function<void()> func) override {
            auto l = std::unique_lock<std::mutex>(m_TasksMutex);
            m_Tasks.push_front(func);
            l.unlock();
            while(true) {
                auto l2 = std::unique_lock<std::mutex>(m_TasksMutex);
                if(m_Tasks.empty())
                    break;
                l2.unlock();
                std::this_thread::yield();
            }
        }
        virtual void add_task(std::function<void()> func) override {
            auto l = std::unique_lock<std::mutex>(m_TasksMutex);
            m_Tasks.push_front(func);
        }
        virtual void do_tasks() override {
            auto l = std::unique_lock<std::mutex>(m_TasksMutex);
            assert(std::this_thread::get_id() == m_HandlerThreadID);
            while(!m_Tasks.empty()) {
                m_Tasks.back()();
                m_Tasks.pop_back();
            }
        }
        virtual bool is_handler() const override {
            return std::this_thread::get_id() == m_HandlerThreadID;
        }
        virtual bool has_tasks() const override {
            auto l = std::unique_lock<std::mutex>(m_TasksMutex);
            if(std::this_thread::get_id() != m_HandlerThreadID)
                return false;
            return !m_Tasks.empty();
        }
        
        Args& args() {return m_Args;}
        const Args& args() const {return m_Args;}
        
        unsigned resolve_resource(
            const std::tuple<
                std::string,
                ICache*
            >& args
        );
        
        std::string resource_path(
            const std::string&
        );

    private:

        mutable std::mutex m_TasksMutex;
        std::thread::id m_HandlerThreadID = std::this_thread::get_id();
        std::deque<std::function<void()>> m_Tasks;
        
        static void async_load(State* s) {
            s->preload();
            s->finish_loading();
        }
        
        Factory<State, Qor*> m_StateFactory;
        Factory<Node, std::string> m_NodeFactory;
        //Factory<Resource, std::string> m_ResourceFactory;
        Cache<IResource, std::string> m_Resources;
        
        unsigned m_LoadingState = 0;
        std::atomic<bool> m_bQuit = ATOMIC_VAR_INIT(false);
        std::shared_ptr<Window> m_pWindow;
        std::shared_ptr<GUI> m_pGUI;
        std::shared_ptr<Input> m_pInput;
        std::shared_ptr<Freq> m_pTimer;
        std::shared_ptr<IPhysics> m_pPhysics;
        std::shared_ptr<Session> m_pSession;
        std::shared_ptr<Interpreter> m_pInterpreter;
        std::shared_ptr<Audio> m_pAudio;
        std::shared_ptr<Meta> m_pConfig;

        //std::shared_ptr<ResourceLocator> m_pLocator;
        //std::shared_ptr<ResourceCache<Texture>> m_pTextures;

        Args m_Args;
        std::string m_Filename;

        std::vector<std::string> m_SearchPaths;
};

#endif

