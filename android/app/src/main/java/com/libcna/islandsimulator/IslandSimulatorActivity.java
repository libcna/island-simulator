package com.libcna.islandsimulator;

import org.libsdl.app.SDLActivity;

/**
 * Minimal Android entry point for island-simulator.
 *
 * SDLActivity owns the Android lifecycle, surface, input, and audio bridge. The game's CMake
 * target supplies libmain.so and exports SDL_main through CNA's public entry-point header.
 */
public final class IslandSimulatorActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL3",
                "main"
        };
    }
}
