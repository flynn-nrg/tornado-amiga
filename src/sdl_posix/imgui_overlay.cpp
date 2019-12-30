/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "SDL.h"

#include "imgui.h"
#include "imgui_sdl.h"

#include "dev_window.h"

#include "imgui_overlay.h"

#define SMPTE_BANNER_HEIGHT 60
#define MAX_SLIDERS 64

static bool show_text = true;
static bool sliders_attach[MAX_SLIDERS];
static float sliders_value[MAX_SLIDERS];

static imguiOverlayData *od = 0;
static ImVec2 screenSize;

static int rocket_enabled = 0;

extern "C" void imgui_overlay_init(SDL_Renderer *renderer, int sizex, int sizey,
                                   int rocket_enable) {

  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGuiSDL::Initialize(renderer, sizex, sizey);
  screenSize = ImVec2(sizex, sizey);
  rocket_enabled = rocket_enable;

  for (int i = 0; i < MAX_SLIDERS; i++) {
    sliders_attach[i] = true;
  }
}

extern "C" void imgui_overlay_render() {

  if (rocket_enabled) {
    ImGuiIO &io = ImGui::GetIO();
    int wheel = 0;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        exit(EXIT_SUCCESS);
      else if (e.type == SDL_MOUSEWHEEL) {
        wheel = e.wheel.y;
      }
    }

    int mouseX, mouseY;
    const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

    io.DeltaTime = 1.0f / 60.0f;
    io.MousePos =
        ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
    io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
    io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
    io.MouseWheel = static_cast<float>(wheel);
  }

  ImGui::NewFrame();
  if (od) {
    ImGui::SetNextWindowPos(ImVec2(0, screenSize.y - SMPTE_BANNER_HEIGHT), 0,
                            ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x, SMPTE_BANNER_HEIGHT), 0);
    ImGui::Begin(
        "SMPTE", &show_text,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMouseInputs);
    ImGui::SetWindowFontScale(4.0);

    char buf[255];
    snprintf(buf, 255, "%2d:%02d:%02d", od->smpte_time / (60 * 50) % 60,
             (od->smpte_time / 50) % 60, od->smpte_time % 50);
    ImVec2 textSize = ImGui::CalcTextSize(buf, NULL, false, -1.0f);
    ImGui::SetCursorPosX((screenSize.x / 2.0) - (textSize.x / 2.0));
    ImGui::Text("%s", buf);
    ImGui::End();

    if (od->sliderNum > 0) {
      ImGui::SetNextWindowPos(ImVec2(screenSize.x / 2, 0), 0, ImVec2(0, 0));
      ImGui::SetNextWindowSize(
          ImVec2(screenSize.x / 2, screenSize.y - SMPTE_BANNER_HEIGHT), 0);
      ImGui::Begin(
          "Sliders", &show_text,
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |
              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
      for (int i = 0; i < od->sliderNum; i++) {
        if (sliders_attach[i] == true) {
          od->sliders[i].attached = 1;
          sliders_value[i] = od->sliders[i].current;
        } else {
          od->sliders[i].attached = 0;
          od->sliders[i].current = sliders_value[i];
        }

        char cbname[16];
        snprintf(cbname, 16, "%x", i);
        ImGui::Checkbox(cbname, &sliders_attach[i]);

        ImGui::SameLine(0.0f, -1.0f);
        ImGui::SliderFloat(od->sliders[i].varName, &sliders_value[i],
                           od->sliders[i].min, od->sliders[i].max, "%.2f",
                           1.0f);
      }
      ImGui::End();
    }
  }

  ImGui::Render();
  ImGuiSDL::Render(ImGui::GetDrawData());
}

extern "C" void imgui_overlay_set(imguiOverlayData *overlayData) {
  od = overlayData;
}

extern "C" void imgui_overlay_close() { ImGui::DestroyContext(); }
