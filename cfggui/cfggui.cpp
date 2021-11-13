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
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#  include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#include "implot.h"

#include "ff_debug.h"
#include "config.h"
#include "platform.hpp"

#include "gui_app.hpp"
#include "gui_settings.hpp"

// Capture initial log output until GuiApp is taking over
static void sInitLog(const DEBUG_LEVEL_t level, const char *str, const DEBUG_CFG_t *cfg)
{
    GuiAppEarlyLog *earlyLog = static_cast<GuiAppEarlyLog *>(cfg->arg);
    earlyLog->Add(level, std::string(str));
    if (level < cfg->level)
    {
        return;
    }
    fputs(str, stderr);
}

static void sGlfwErrorCallback(int error, const char* description)
{
    ERROR("GLFW error %d: %s", error, description);
}

static bool sWindowActivity;

static void sCursorPositionCallback(GLFWwindow *window, double xpos, double ypos)
{
    UNUSED(window);
    UNUSED(xpos);
    UNUSED(ypos);
    sWindowActivity = true;
}

static void sMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    UNUSED(window);
    UNUSED(button);
    UNUSED(action);
    UNUSED(mods);
    sWindowActivity = true;
}

int main(int argc, char **argv)
{
    GuiAppEarlyLog earlyLog;
    DEBUG_CFG_t cfg =
    {
        .level  = DEBUG_LEVEL_DEBUG,
        .colour = isatty(fileno(stderr)) == 1,
        .mark   = NULL,
        .func   = sInitLog,
        .arg    = &earlyLog,
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
    int windowPosX   = -1;
    int windowPosY   = -1;
    const int   kWindowMinWidth  =  640;
    const int   kWindowMinHeight =  384;
    const char *kWindowTitle  = "cfggui " CONFIG_VERSION " (" CONFIG_GITHASH ")";

    DEBUG("GLFW: %s", glfwGetVersionString());
    glfwSetErrorCallback(sGlfwErrorCallback);
    if (glfwInit() != GLFW_TRUE)
    {
        exit(EXIT_FAILURE);
    }

    // https://en.wikipedia.org/wiki/OpenGL_Shading_Language
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char *glsl_version = "#version 100";
    glfwWindowHint(SDL_GL_CONTEXT_FLAGS, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    DEBUG("OpenGL ES 2.0, GLSL 1.00");
#elif defined(_WIN32)
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    DEBUG("OpenGL 3.2, GLSL 1.30");
##elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // Always required on Mac
    DEBUG("OpenGL 3.2, GLSL 1.50");
#else
    // GL 3.2 + GLSL 150
    const char *glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    DEBUG("OpenGL 3.2, GLSL 1.50");
#endif

    // Create window with graphics context
    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, kWindowTitle, NULL, NULL);
    if (window == NULL)
    {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetWindowSizeLimits(window, kWindowMinWidth, kWindowMinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);

    // User activity detection, for frame rate control
    glfwSetCursorPosCallback(window, sCursorPositionCallback);
    glfwSetMouseButtonCallback(window, sMouseButtonCallback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    /* ***** Create application ************************************************************************************* */

    bool done = false;
    std::unique_ptr<GuiApp> app = nullptr;
    std::shared_ptr<GuiSettings> settings = nullptr;
    try
    {
        app = std::make_unique<GuiApp>(appArgv, earlyLog);
        settings = app->GetSettings();
        earlyLog.Clear();
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
        std::string geometry = settings->GetValue("Geometry");
        int w, h, x, y;
        if (std::sscanf(geometry.c_str(), "%d,%d,%d,%d", &w, &h, &x, &y) == 4)
        {
            if ( (x >= 0) && (y >= 0) )
            {
                glfwSetWindowPos(window, x, y);
            }
            if (( w >= kWindowMinWidth) && (h >= kWindowMinHeight) )
            {
                glfwSetWindowSize(window, w, h);
            }
        }
    }

    /* ***** main loop ********************************************************************************************** */

    // Main loop
    uint32_t lastDraw = 0;
    uint32_t lastMark = 0;
    while (!done)
    {
        glfwPollEvents();

        uint32_t now = TIME();
        const uint32_t markInterval = 10000;
        if ((now - lastMark) >= markInterval)
        {
            DEBUG("----- MARK -----");
            lastMark = ((now + (markInterval / 2 )) / markInterval) * markInterval;
        }

        if ( sWindowActivity || ((now - lastDraw) > (1000/10)))
        {
            lastDraw = now;
            sWindowActivity = false;

            app->PerfTic(GuiApp::Perf_e::NEWFRAME);

            // Update font if necessary
            if (app->PrepFrame())
            {
                ImGui_ImplOpenGL3_CreateFontsTexture();
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            app->NewFrame();

            app->PerfToc(GuiApp::Perf_e::NEWFRAME);

            // Do things
            app->PerfTic(GuiApp::Perf_e::LOOP);
            app->Loop();
            app->PerfToc(GuiApp::Perf_e::LOOP);

            // Compose the frame
            app->PerfTic(GuiApp::Perf_e::DRAW);
            app->DrawFrame();
            app->PerfToc(GuiApp::Perf_e::DRAW);

            // Confirm close modal
            if (glfwWindowShouldClose(window))
            {
                bool wantClose = true;
                app->ConfirmClose(wantClose, done);
                glfwSetWindowShouldClose(window, wantClose);
            }

            // Render
            app->PerfTic(GuiApp::Perf_e::RENDER_IM);
            //ImGui::EndFrame();
            ImGui::Render();
            app->PerfToc(GuiApp::Perf_e::RENDER_IM);
            app->PerfTic(GuiApp::Perf_e::RENDER_GL);
            glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
            const auto c = app->BackgroundColour();
            glClearColor(c.x, c.y, c.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT); // FIXME: why does this take almost 15 ms when moving the mouse but not use any CPU (it seems..)???
            glfwSetWindowOpacity(window, c.w < 0.2f ? 0.2f : c.w);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            app->PerfToc(GuiApp::Perf_e::RENDER_GL);

            glfwSwapBuffers(window);
            glFlush();
        }
        else
        {
            SLEEP(5);
        }
    }

    if (app && settings)
    {
        // Save window geometry
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        settings->SetValue("Geometry", Ff::Sprintf("%d,%d,%d,%d", windowWidth, windowHeight, windowPosX, windowPosY));

        // Tear down
        app = nullptr;
        settings = nullptr;
    }

    DEBUG("Adios!");

    /* ***** Cleanup ************************************************************************************************ */

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
