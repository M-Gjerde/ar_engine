//
// Created by magnus on 8/2/21.
//

#include <algorithm>
#include "GUI.h"

GUI::GUI(VkDevice device) {
    mainDevice = device;

    ImGui::CreateContext();

}

void GUI::init(int width, int height){
    // Color scheme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(width, height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void GUI::initResources(){
    ImGuiIO& io = ImGui::GetIO();

    // Create font texture
    unsigned char* fontData;
    int texWidth, texHeight;
    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);

    // --- Create Target image for copy

    // --- Image view

    // Staging buffers for font data upload
    // Copy buffer data to font image
    // Prepare for transfer
    // Copy
    // Prepare for shader read

    // --- Font texture Sampler

    // --- Descriptor pool


    // --- Descriptor Set Layout

    // --- DescriptorSet

    // Pipeline Layout
    // Push constants for UI rendering parameters

    // Setup graphics pipeline for UI rendering


    // Vertex bindings an attributes based on ImGui vertex definition
}

// Starts a new imGui frame and sets up windows and ui elements
void GUI::newFrame(bool updateFrameGraph)
{
    ImGui::NewFrame();

    // Init imGui windows and elements

    ImVec4 clear_color = ImColor(114, 144, 154);
    static float f = 0.0f;
    ImGui::TextUnformatted("Title");
    ImGui::TextUnformatted("DeviceName");

    // Update frame time display
    if (updateFrameGraph) {
        std::rotate(uiSettings.frameTimes.begin(), uiSettings.frameTimes.begin() + 1, uiSettings.frameTimes.end());
        float frameTime = 1000.0f / (2 * 1000.0f);
        uiSettings.frameTimes.back() = frameTime;
        if (frameTime < uiSettings.frameTimeMin) {
            uiSettings.frameTimeMin = frameTime;
        }
        if (frameTime > uiSettings.frameTimeMax) {
            uiSettings.frameTimeMax = frameTime;
        }
    }

    ImGui::PlotLines("Frame Times", &uiSettings.frameTimes[0], 50, 0, "", uiSettings.frameTimeMin, uiSettings.frameTimeMax, ImVec2(0, 80));

    ImGui::Text("Camera");
    float pos[] = {3, 3, 3};
    ImGui::InputFloat3("position", pos, "2");
    ImGui::InputFloat3("rotation", pos, "2");

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Example settings");
    ImGui::Checkbox("Render models", &uiSettings.displayModels);
    ImGui::Checkbox("Display logos", &uiSettings.displayLogos);
    ImGui::Checkbox("Display background", &uiSettings.displayBackground);
    ImGui::Checkbox("Animate light", &uiSettings.animateLight);
    ImGui::SliderFloat("Light speed", &uiSettings.lightSpeed, 0.1f, 1.0f);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::ShowDemoWindow();

    // Render to generate draw buffers
    ImGui::Render();
}


GUI::~GUI() {
    ImGui::DestroyContext();
}
