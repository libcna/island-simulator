#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
#include "IslandSimulator/CampfireLayout.hpp"
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

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
                    1.0f, 0, 1.0f/60, .5f, listener, {}, {}, {}, {}, {},nullptr,
                    swimming?1.4f:0.0f);
                Check(sound.IsAvailable(), "Stream transition disabled all audio: " + sound.Error());
                Check(sound.Error().empty(), "Audio retained an unexpected failure");
                const auto sources = sound.DebugState().playingSourceCount;
                Check(sources >= sound.PersistentSourceCount,
                      "A persistent ambient track stopped after a stream transition");
                Check(sources <= sound.MaximumPlayingSourceCount,
                      "Stream arbitration exceeded the shared voice budget");
                if(swimming) {
                    const auto underwater=sound.DebugState().underwaterAudio;
                    Check(underwater.immersion==1&&underwater.airGain==.18f
                              &&underwater.waterGain==1.22f,
                          "real soundscape did not apply the deep-water mix");
                }
            }
        }
    }
}
void ExerciseCampfire(bool nearStartup) {
    using namespace island;
    constexpr float FarX=80.0f,FarZ=-120.0f;
    Soundscape sound;
    Check(sound.Start(80,false,0,0,0,.5f,nearStartup?CampfireSite::X:FarX,
                      nearStartup?CampfireSite::Z:FarZ),
        "Campfire dummy audio startup failed: "+sound.Error());
    for(int cycle=0;cycle<3;++cycle) {
        sound.SetMuted(cycle==1);
        for(int region=0;region<6;++region) {
            const bool nearFire=region==0||region==5,swimming=region==3;
            SoundscapeListenerState listener;
            listener.x=nearFire||region==4?CampfireSite::X:region==1?-90:FarX;
            listener.z=nearFire||region==4?CampfireSite::Z:region==1?StreamRoute::CentreZ(-90):FarZ;
            listener.y=CampfireLayout::GroundY()+(region==4?50:1.72f);
            for(int tick=0;tick<60;++tick) {
                sound.Update(80,swimming,swimming?1:0,0,0,1.0f/60,.5f,listener,{},{},{},{},{});
                Check(sound.IsAvailable()&&sound.Error().empty(),"Fire/stream/water transition disabled audio: "+sound.Error());
                const auto debug=sound.DebugState();
                Check(debug.campfirePlaying==nearFire,
                    "Actual campfire loop state disagrees with listener region: cycle="
                        +std::to_string(cycle)+" region="+std::to_string(region)
                        +" expected="+std::to_string(nearFire)
                        +" actual="+std::to_string(debug.campfirePlaying)
                        +" volume="+std::to_string(debug.campfireVolume));
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
void ExerciseFrogVoice() {
    using namespace island;
    Soundscape sound;
    std::array<FrogState,1> frogs{};
    frogs[0].x=0.0f;frogs[0].y=.08f;frogs[0].z=0.0f;
    frogs[0].behavior=FrogBehavior::Rest;
    frogs[0].simulationTier=WildlifeSimulationTier::Full;
    SoundscapeListenerState listener;listener.x=2.0f;listener.y=1.72f;listener.z=0.0f;
    Check(sound.Start(80,false,0,0,0,.5f,listener.x,listener.z),
        "Frog dummy audio startup failed: "+sound.Error());
    bool active=false;
    for(int tick=0;tick<900;++tick) {
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},nullptr,0.0f,frogs);
        active|=sound.IsWildlifeCallActive();
        Check(sound.IsAvailable()&&sound.Error().empty(),"Frog call disabled the soundscape");
        Check(sound.DebugState().playingSourceCount>=4&&sound.DebugState().playingSourceCount<=6,
            "Frog call exceeded the shared six-source budget");
    }
    Check(active&&sound.PlayedFrogCallCount()>0,"Nearby resting frog never entered actual playback");
    const int audible=sound.PlayedFrogCallCount();sound.SetMuted(true);
    for(int tick=0;tick<180;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},nullptr,0.0f,frogs);
    Check(sound.PlayedFrogCallCount()==audible&&!sound.IsWildlifeCallActive(),
        "Muted frog voice retained or created transient playback");
    sound.SetMuted(false);listener.x=100.0f;
    for(int tick=0;tick<900;++tick)
        sound.Update(80,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{},nullptr,0.0f,frogs);
    Check(sound.PlayedFrogCallCount()==audible,"Out-of-range frog created actual playback");
}
void ExerciseShipwreckCreak() {
    using namespace island;
    Soundscape sound;
    sound.SetShipwreckSource(0.0f,0.35f,0.0f);
    SoundscapeListenerState listener;
    listener.x=4.0f;listener.y=1.72f;listener.z=0.0f;
    Check(sound.Start(2.0f,false,0,0,0,.5f,listener.x,listener.z),
        "Shipwreck-creak dummy audio startup failed: "+sound.Error());
    for(int tick=0;tick<1800&&sound.PlayedShipwreckCreakCount()==0;++tick) {
        sound.Update(2.0f,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{});
        Check(sound.IsAvailable()&&sound.Error().empty(),
            "Shipwreck creak disabled the soundscape");
        Check(sound.DebugState().playingSourceCount>=4
                &&sound.DebugState().playingSourceCount<=6,
            "Shipwreck creak exceeded the shared six-source budget");
    }
    Check(sound.PlayedShipwreckCreakCount()>0,
        "Nearby wave-exposed shipwreck never entered actual creak playback");
    if (const char* driver=std::getenv("SDL_AUDIODRIVER");
        driver&&std::string_view(driver)=="disk") {
        std::this_thread::sleep_for(std::chrono::milliseconds(650));
    }
    const int audible=sound.PlayedShipwreckCreakCount();
    sound.SetMuted(true);
    for(int tick=0;tick<300;++tick)
        sound.Update(2.0f,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{});
    Check(sound.PlayedShipwreckCreakCount()==audible,
        "Muted shipwreck created transient playback");
    sound.SetMuted(false);listener.x=80.0f;
    for(int tick=0;tick<2400;++tick)
        sound.Update(2.0f,false,0,0,0,1.0f/60,.5f,listener,{},{},{},{},{});
    Check(sound.PlayedShipwreckCreakCount()==audible,
        "Distant shipwreck created actual creak playback");
}
}

int main(int argc, char** argv) {
    const char* driver = std::getenv("SDL_AUDIODRIVER");
    Check(driver && (std::string_view(driver) == "dummy"
                     || std::string_view(driver) == "disk"),
          "Test requires the isolated dummy or disk audio driver");
    island::InitializeRuntimeDiagnostics(argc, argv);
    try {
        Exercise(false);
        Exercise(true);
        ExerciseCampfire(false);
        ExerciseCampfire(true);
        ExerciseBearVoice();
        ExerciseGoatVoice();
        ExerciseFrogVoice();
        ExerciseShipwreckCreak();
        std::cout << "Soundscape stream startup/entry/swim/mute runtime regression passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
