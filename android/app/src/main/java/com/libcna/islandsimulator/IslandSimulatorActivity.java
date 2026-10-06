package com.libcna.islandsimulator;

import android.content.pm.ApplicationInfo;

import org.libsdl.app.SDLActivity;

/**
 * Minimal Android entry point for island-simulator.
 *
 * SDLActivity owns the Android lifecycle, surface, input, and audio bridge. The game's CMake
 * target supplies libmain.so and exports SDL_main through CNA's public entry-point header.
 */
public final class IslandSimulatorActivity extends SDLActivity {
    private static final String DEBUG_ARGUMENTS_EXTRA =
            "com.libcna.islandsimulator.DEBUG_ARGUMENTS";

    @Override
    protected String[] getLibraries() {
        return new String[] {
                "SDL3",
                "main"
        };
    }

    /**
     * Allows repeatable fixed-view QA launches through an intent only in debuggable packages.
     * Release packages deliberately ignore externally supplied native command-line arguments.
     */
    @Override
    protected String[] getArguments() {
        if ((getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE) == 0) {
            return new String[0];
        }
        String[] arguments = getIntent().getStringArrayExtra(DEBUG_ARGUMENTS_EXTRA);
        return arguments == null ? new String[0] : arguments.clone();
    }
}
