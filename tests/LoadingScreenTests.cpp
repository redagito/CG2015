#include "engine/CEngine.h"
#include "graphics/system/CGraphicsSystem.h"
#include "graphics/window/CGlfwWindow.h"
#include "resource/Resource.h"
#include "game/CGameSystem.h"
#include "game/state/CLoadState.h"
#include "game/state/CTitleState.h"
#include "game/state/CGamePlayState.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>
#include <thread>

namespace {
class Input : public IInputProvider {
public:
    bool enter=false;
    void addInputListener(IInputListener*) override {}
    void removeInputListener(IInputListener*) override {}
    bool isKeyPressed(int key) override {return key==GLFW_KEY_ENTER && enter;}
};
void require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
}
bool runLoadingScreenTests(CGlfwWindow& window)
{
    try {
        std::unique_ptr<IResourceManager> resources(createResourceManager());
        const auto boot=resources->loadManifestAsync("data/bootstrap-assets.json");
        auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        for(;;) {
            resources->pumpAsyncLoads(2.0);const auto status=resources->getLoadStatus(boot);
            require(status.phase!=LoadPhase::Failed,status.error.c_str());
            if(status.phase==LoadPhase::Ready) break;
            require(std::chrono::steady_clock::now()<deadline,"Bootstrap timed out");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        resources->setCacheOnly(true);
        CGraphicsSystem graphics;require(graphics.init(*resources),"Cached renderer bootstrap failed");
        Input input;CGameSystem game;
        auto* loading=new CLoadState("data/world/load_1.json","data/game/defenders_of_cthedra/assets.json");
        game.addState("load",loading);
        game.addState("title",new CTitleState("data/world/intro_1.json"));game.addState("game",new CGamePlayState());
        require(game.init("load",&graphics,&input,resources.get()),"Loading state initialization failed");
        input.enter=true; // A press during loading must not skip the ready prompt.
        unsigned loadingFrames=0;
        deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        while(!loading->readyToContinue()) {
            require(game.update(0.016f),"Loading state failed");
            graphics.draw(window);++loadingFrames;
            require(game.activeStateName()=="load","Enter skipped the loading screen before readiness");
            require(std::chrono::steady_clock::now()<deadline,"Loading screen did not finish loading");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(loadingFrames>1 && game.activeStateName()=="load","Loading screen transitioned automatically");
        for(int i=0;i<3;++i) require(game.update(0.016f)&&game.activeStateName()=="load","Held Enter dismissed the ready prompt");
        input.enter=false;require(game.update(0.016f)&&game.activeStateName()=="load","Releasing Enter dismissed the ready prompt");
        input.enter=true;require(game.update(0.016f)&&game.activeStateName()=="title","Fresh Enter did not continue to title");
        require(game.update(0.016f)&&game.activeStateName()=="title","The same Enter press skipped the title");
        input.enter=false;require(game.update(0.016f),"Title Enter release failed");
        input.enter=true;require(game.update(0.016f),"Gameplay initialization failed after loading");
        require(game.activeStateName()=="game","Title Enter did not enter gameplay");
        for(int i=0;i<3;++i) {require(game.update(0.016f),"Gameplay update failed");graphics.draw(window);}
        std::printf("Loading screen rendered %u frames, then title and gameplay initialized from cache only.\n",loadingFrames);
        return true;
    }catch(const std::exception& error) {std::fprintf(stderr,"Loading integration: %s\n",error.what());return false;}
}
