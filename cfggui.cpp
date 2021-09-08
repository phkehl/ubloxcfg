// flipflip's cfggui
//
// Copyright (c) 2021 Philippe Kehl (flipflip at oinkzwurgl dot org),
// https://oinkzwurgl.org/hacking/ubloxcfg
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see <https://www.gnu.org/licenses/>.

// This is a UBX, NMEA and RTCM3 message parser. It only parses the frames, not the content
// of the message (it does not decode any message fields).
// The parser will pass-through all data that is input. Unknown parts (other protocols,
// spurious data, incorrect messages, etc.) are output as GARBAGE type messages. GARBAGE messages
// are not guaranteed to be combined and can be split arbitrarily (into several GARBAGE messages).

#include <cstdio>
#include <vector>
#include <string>
#include <unistd.h>
#include <memory>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#  include <SDL_opengles2.h>
#endif

// #if defined(IMGUI_IMPL_OPENGL_ES2)
// #  include <GLES2/gl2.h>
// #elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
// #  include <GL/gl3w.h>
// #else
// #  error Nope!
// #endif

#include "implot.h"

#include "ff_debug.h"
#include "config.h"
#include "platform.hpp"

#include "gui_app.hpp"
#include "gui_settings.hpp"

// Capture initial log output until GuiApp is taking over
static void sInitLog(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg)
{
    GuiAppInitialLog *initLog = static_cast<GuiAppInitialLog *>(cfg->arg);
    initLog->Add(level, std::string(str));
    if (level < cfg->level)
    {
        return;
    }
    fputs(str, stderr);
}

int main(int argc, char **argv)
{
    GuiAppInitialLog initLog;
    DEBUG_CFG_t cfg =
    {
        .level  = DEBUG_LEVEL_DEBUG,
        .colour = isatty(fileno(stderr)) == 1,
        .mark   = NULL,
        .func   = sInitLog,
        .arg    = &initLog,
    };
    debugSetup(&cfg);

    /* ***** Command line arguments ********************************************************************************* */

    std::vector<std::string> appArgv;
    for (int ix = 0; ix < argc; ix++)
    {
        appArgv.push_back(std::string(argv[ix]));
    }

    if (!Platform::Init())
    {
        ERROR("Failed initialising stuff!");
        exit(EXIT_FAILURE);
    }

    /* ***** Setup low-level platform and renderer ****************************************************************** */

    int windowWidth  = 1280;
    int windowHeight = 768;
    int windowPosX   = SDL_WINDOWPOS_UNDEFINED;
    int windowPosY   = SDL_WINDOWPOS_UNDEFINED;
    const int   kWindowMinWidth  =  640;
    const int   kWindowMinHeight =  384;
    const char *kWindowTitle  = "cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")";

    SDL_version sdlVerC;
    SDL_version sdlVerL;
    SDL_VERSION(&sdlVerC);
    SDL_GetVersion(&sdlVerL);
    DEBUG("SDL %d.%d.%d (compiled %d.%d.%d)", sdlVerL.major, sdlVerL.minor, sdlVerL.patch, sdlVerC.major, sdlVerC.minor, sdlVerC.patch);

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        ERROR("Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    DEBUG("OpenGL ES 2.0, GLSL 1.00");
#elif defined(_WIN32)
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    DEBUG("OpenGL 3.2, GLSL 1.30");
##elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    DEBUG("OpenGL 3.2, GLSL 1.50");
#else
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    DEBUG("OpenGL 3.2, GLSL 1.50");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags sdlWindowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window     *sdlWindow      = SDL_CreateWindow(kWindowTitle, windowPosX, windowPosY, windowWidth, windowHeight, sdlWindowFlags);
    SDL_SetWindowMinimumSize(sdlWindow, kWindowMinWidth, kWindowMinHeight);
    SDL_GLContext   sdlGlContext   = SDL_GL_CreateContext(sdlWindow);
    SDL_GL_MakeCurrent(sdlWindow, sdlGlContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
// #if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
//     if (gl3wInit() != 0)
//     {
//         ERROR("Failed to initialize OpenGL loader!");
//         return EXIT_FAILURE;
//     }
// #endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(sdlWindow, sdlGlContext);
#ifdef _WIN32
    ImGui_ImplOpenGL2_Init();
#else
    ImGui_ImplOpenGL3_Init(glsl_version);
#endif

    /* ***** Create application ************************************************************************************* */

    bool done = false;
    std::unique_ptr<GuiApp> app = nullptr;
    std::shared_ptr<GuiSettings> settings = nullptr;
    try
    {
        app = std::make_unique<GuiApp>(appArgv, initLog);
        settings = app->GetSettings();
        initLog.Clear();
    }
    catch (std::exception &e)
    {
        ERROR("gui init fail: %s", e.what());
        done = true;
    }
    ImGuiIO &io = ImGui::GetIO();

    // Restore previous window geometry
    if (app && settings)
    {
        std::string geometry;
        settings->GetValue("Geometry", geometry, "");
        int w, h, x, y;
        SDL_DisplayMode dm;
        if ( (SDL_GetDesktopDisplayMode(0, &dm) == 0) &&
            (std::sscanf(geometry.c_str(), "%d,%d,%d,%d", &w, &h, &x, &y) == 4) &&
            (w < dm.w) && (h < dm.h) && (x >= 0) && (y >= 0) && (x < (dm.w - 100)) && (y < (dm.h - 100)) )
        {
            SDL_SetWindowPosition(sdlWindow, x, y);
            SDL_SetWindowSize(sdlWindow, w, h);
        }
    }

    /* ***** main loop ********************************************************************************************** */

    // Main loop
    uint32_t lastDraw = 0;
    uint32_t lastMark = 0;
    bool wantClose = false;
    while (!done)
    {
        bool haveEvent = false;
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
                case SDL_QUIT:
                    wantClose = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.windowID == SDL_GetWindowID(sdlWindow))
                    {
                        switch (event.window.event)
                        {
                            case SDL_WINDOWEVENT_CLOSE:
                                wantClose = true;
                                break;
                            case SDL_WINDOWEVENT_MOVED:
                                windowPosX = event.window.data1;
                                windowPosY = event.window.data2;
                                break;
                            case SDL_WINDOWEVENT_RESIZED:
                                windowWidth = event.window.data1;
                                windowHeight = event.window.data2;
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
            haveEvent = true;
        }

        uint32_t now = SDL_GetTicks();
        const uint32_t markInterval = 10000;
        if ((now - lastMark) >= markInterval)
        {
            DEBUG("----- MARK -----");
            lastMark = ((now + (markInterval / 2 )) / markInterval) * markInterval;
        }
        if (haveEvent || ((now - lastDraw) > (1000/10)))
        {
            lastDraw = now;

            // Update font if necessary
            if (app->PrepFrame())
            {
#ifdef _WIN32
                ImGui_ImplOpenGL2_CreateFontsTexture();
#else
                ImGui_ImplOpenGL3_CreateFontsTexture();
#endif
            }

            // Start the Dear ImGui frame
#ifdef _WIN32
            ImGui_ImplOpenGL2_NewFrame();
#else
            ImGui_ImplOpenGL3_NewFrame();
#endif
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            app->NewFrame();

            // Do things
            app->Loop();

            // Compose the frame
            app->DrawFrame();

            // Confirm close modal
            if (wantClose)
            {
                app->ConfirmClose(wantClose, done);
            }

            // Render
            //ImGui::EndFrame();
            ImGui::Render();
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            const auto c = app->BackgroundColour();
            glClearColor(c.x, c.y, c.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            SDL_SetWindowOpacity(sdlWindow, c.w < 0.2f ? 0.2f : c.w);
#ifdef _WIN32
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#else
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
            SDL_GL_SwapWindow(sdlWindow);
        }
        else
        {
            SDL_Delay(5);
        }
    }

    if (app && settings)
    {
        // Save window geometry
        settings->SetValue("Geometry", Ff::Sprintf("%d,%d,%d,%d", windowWidth, windowHeight, windowPosX, windowPosY));

        // Tear down
        app = nullptr;
        settings = nullptr;
    }

    DEBUG("Adios!");

    /* ***** Cleanup ************************************************************************************************ */

#ifdef _WIN32
    ImGui_ImplOpenGL2_Shutdown();
#else
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplSDL2_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(sdlGlContext);
    SDL_DestroyWindow(sdlWindow);
    SDL_Quit();

    return EXIT_SUCCESS;
}
