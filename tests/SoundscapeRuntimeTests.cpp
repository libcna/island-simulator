#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/CampfireLayout.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void Exercise(bool startNearStream) {
    island::Soundscape sound;
    const float nearX = -90.0f;
    const float nearZ = island::StreamRoute::CentreZ(nearX);
    const float startX = startNearStream ? nearX : 0.0f;
    const float startZ = startNearStream ? nearZ : -225.0f;
    Check(sound.Start(30, false, 0, 0, 0, .5f, startX, startZ),
          "Dummy audio startup failed: " + sound.Error());
    for (int cycle = 0; cycle < 4; ++cycle) {
        // Enter the stream biome, leave it, swim (the fourth voice changes), return.
        // Run this while audible, muted, and unmuted. All calls use real CNA instances.
        sound.SetMuted(cycle == 1);
        for (int region = 0; region < 4; ++region) {
            const bool near = region != 1;
            const bool swimming = region == 2;
            island::SoundscapeListenerState listener;
            listener.x = near ? nearX : 0.0f;
            listener.z = near ? nearZ : -225.0f;
            listener.y = swimming ? .58f : 4.0f;
            for (int tick = 0; tick < 90; ++tick) {
                sound.Update(near ? 30 : 2, swimming, swimming ? 1.0f : 0.0f,
                    1.0f, 0, 1.0f/60, .5f, listener, {}, {}, {}, {}, {});
                Check(sound.IsAvailable(), "Stream transition disabled all audio: " + sound.Error());
                Check(sound.Error().empty(), "Audio retained an unexpected failure");
                const auto sources = sound.DebugState().playingSourceCount;
                Check(sources >= sound.PersistentSourceCount,
                      "A persistent ambient track stopped after a stream transition");
                Check(sources <= sound.MaximumPlayingSourceCount,
                      "Stream arbitration exceeded the shared voice budget");
            }
        }
    }
}
void ExerciseCampfire(bool nearStartup) {
    using namespace island;
    Soundscape sound;
    Check(sound.Start(80,false,0,0,0,.5f,nearStartup?CampfireSite::X:0,nearStartup?CampfireSite::Z:-225),
        "Campfire dummy audio startup failed: "+sound.Error());
    for(int cycle=0;cycle<3;++cycle) {
        sound.SetMuted(cycle==1);
        for(int region=0;region<6;++region) {
            const bool nearFire=region==0||region==5,swimming=region==3;
            SoundscapeListenerState listener;
            listener.x=nearFire||region==4?CampfireSite::X:region==1?-90:0;
            listener.z=nearFire||region==4?CampfireSite::Z:region==1?StreamRoute::CentreZ(-90):-225;
            listener.y=CampfireLayout::GroundY()+(region==4?50:1.72f);
            for(int tick=0;tick<60;++tick) {
                sound.Update(80,swimming,swimming?1:0,0,0,1.0f/60,.5f,listener,{},{},{},{},{});
                Check(sound.IsAvailable()&&sound.Error().empty(),"Fire/stream/water transition disabled audio: "+sound.Error());
                const auto debug=sound.DebugState();
                Check(debug.campfirePlaying==nearFire,"Actual campfire loop state disagrees with listener region");
                Check(debug.playingSourceCount>=4&&debug.playingSourceCount<=6,"Campfire exceeded shared source budget");
            }
        }
    }
}
void ExerciseBearVoice() {
    using namespace island;
    Soundscape sound;
    BearState bear;bear.x=0;bear.y=4;bear.z=0;bear.behavior=BearBehavior::Forage;
    SoundscapeListenerState listener;listener.x=2;listener.y=5.72f;listener.z=0;
    Check(sound.Start(80,false,0,0,0,.5f,listener.x,listener.z),
        "Bear dummy audio startup failed: "+sound.Error());
    bool active=false;
    for(int tick=0;tick<720;++tick) {
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},&bear);
        active|=sound.IsWildlifeCallActive();
        Check(sound.IsAvailable()&&sound.Error().empty(),"Bear call disabled the soundscape");
        Check(sound.DebugState().playingSourceCount>=4&&sound.DebugState().playingSourceCount<=6,
            "Bear call exceeded the shared six-source budget");
    }
    Check(active&&sound.PlayedBearCallCount()>0,"Nearby peaceful bear never entered actual playback");
    const int audible=sound.PlayedBearCallCount();sound.SetMuted(true);
    for(int tick=0;tick<180;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},&bear);
    Check(sound.PlayedBearCallCount()==audible&&!sound.IsWildlifeCallActive(),
        "Muted bear voice retained or created transient playback");
    sound.SetMuted(false);listener.x=100;
    for(int tick=0;tick<720;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},&bear);
    Check(sound.PlayedBearCallCount()==audible,"Out-of-range bear created actual playback");
}
void ExerciseGoatVoice() {
    using namespace island;
    Soundscape sound;
    std::array<GoatState,1> goats{};
    goats[0].x=0;goats[0].y=4;goats[0].z=0;goats[0].behavior=GoatBehavior::Graze;
    SoundscapeListenerState listener;listener.x=3;listener.y=5.72f;listener.z=0;
    Check(sound.Start(80,false,0,0,0,.5f,listener.x,listener.z),
        "Goat dummy audio startup failed: "+sound.Error());
    bool active=false;
    for(int tick=0;tick<1200;++tick) {
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},goats,{});
        active|=sound.IsWildlifeCallActive();
        Check(sound.IsAvailable()&&sound.Error().empty(),"Goat call disabled the soundscape");
        Check(sound.DebugState().playingSourceCount>=4&&sound.DebugState().playingSourceCount<=6,
            "Goat call exceeded the shared six-source budget");
    }
    Check(active&&sound.PlayedGoatCallCount()>0,"Nearby goat never entered actual playback");
    const int audible=sound.PlayedGoatCallCount();sound.SetMuted(true);
    for(int tick=0;tick<180;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},goats,{});
    Check(sound.PlayedGoatCallCount()==audible&&!sound.IsWildlifeCallActive(),
        "Muted goat voice retained or created transient playback");
    sound.SetMuted(false);listener.x=100;
    for(int tick=0;tick<1200;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},goats,{});
    Check(sound.PlayedGoatCallCount()==audible,"Out-of-range goat created actual playback");
}
}

int main(int argc, char** argv) {
    const char* driver = std::getenv("SDL_AUDIODRIVER");
    Check(driver && std::string_view(driver) == "dummy", "Test requires the dummy audio driver");
    island::InitializeRuntimeDiagnostics(argc, argv);
    try {
        Exercise(false);
        Exercise(true);
        ExerciseCampfire(false);
        ExerciseCampfire(true);
        ExerciseBearVoice();
        ExerciseGoatVoice();
        std::cout << "Soundscape stream startup/entry/swim/mute runtime regression passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
