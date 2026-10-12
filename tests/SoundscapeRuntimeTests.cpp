#include "IslandSimulator/Soundscape.hpp"
#include "IslandSimulator/RuntimeDiagnostics.hpp"
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
                    1.0f, 0, 1.0f/60, .5f, listener, {}, {}, {}, {});
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
}

int main(int argc, char** argv) {
    const char* driver = std::getenv("SDL_AUDIODRIVER");
    Check(driver && std::string_view(driver) == "dummy", "Test requires the dummy audio driver");
    island::InitializeRuntimeDiagnostics(argc, argv);
    try {
        Exercise(false);
        Exercise(true);
        std::cout << "Soundscape stream startup/entry/swim/mute runtime regression passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
