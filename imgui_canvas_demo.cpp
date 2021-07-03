#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <iomanip>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "imgui_canvas.h"

#include "shader.h"



// ====================================================================================================================
inline int rangedRand(const int _min, const int _max) {
  int n = _max - _min + 1;
  int remainder = RAND_MAX % n;
  int x;
  do {
    x = rand();
  } while (x >= RAND_MAX - remainder);
  return _min + x % n;
}
// ====================================================================================================================










// ====================================================================================================================
static void glfw_error_callback (int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
// ====================================================================================================================








// ====================================================================================================================
int main (int argc, char** argv) {

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  // Decide GL+GLSL versions
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(1920, 1200, "Dear ImGui canvas demo", NULL, NULL);
  if (window == NULL)
    return 1;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  bool err = gl3wInit() != 0;
  if (err) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    return 1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark(); // Setup Dear ImGui style

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // ==================================================================================================================
  Shader shader(FileManager::Read(BASE_VERTEX_SHADER_FILE_STR), FileManager::Read(MASK_FRAGMENT_SHADER_FILE_STR));
  shader.Initialize();
  shader.BindFragDataLocation();
  shader.Use();
  shader.CreateTexture();

  // ------------------------------------------------------------------------------------------------------------------
  int width = 320, height = 240;
  float imageAspectRatio = (float)height/(float)width;
  uint8_t* imageData = new uint8_t[width*height]();
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      imageData[x + y*width] = (uint8_t)ImPow(2.0,16)*(float)x/(float)width;

  uint8_t* maskData = new uint8_t[width*height]();
  bool maskEnable = true;

  ImVec2 viewSize = ImVec2(640,480);
  // ------------------------------------------------------------------------------------------------------------------
  static std::vector<ImGuiShape> shapes;

  static GLuint imageTexture, maskTexture, compositeTexture, renderBuffers, frameBuffers;

  shader.SetupTexture(GL_TEXTURE0, &imageTexture, width, height, maskData);
  shader.setUniform("image", 0);

  shader.SetupTexture(GL_TEXTURE1, &maskTexture, width, height, imageData);
  shader.setUniform("mask", 1);

  shader.SetupTexture(GL_TEXTURE2, &compositeTexture, width, height, 0, GL_RGBA);
  shader.BindTexture(0);

  shader.SetupRenderBuffer(&renderBuffers, width, height, &frameBuffers, &compositeTexture);

  // ---- add 2 x hlines and 2 x vlines for roi selection -------------------------------------------------------------
  shapes.clear();
  shapes.push_back(ImGuiShape("vLine##A_V_ImGuiShape", ImVec2(10.0, height/2.0), ImGuiCanvasShapeType::VLine, {0.0f, (float)height}, ImGuiCanvasClip::Out, true));
  shapes.push_back(ImGuiShape("vLine##B_V_ImGuiShape", ImVec2(width-10.0, height/2.0), ImGuiCanvasShapeType::VLine, {0.0f, (float)height}, ImGuiCanvasClip::In, true));
  shapes.push_back(ImGuiShape("hLine##A_H_ImGuiShape", ImVec2(width/2.0, 10.0), ImGuiCanvasShapeType::HLine, {0.0f, (float)width}, ImGuiCanvasClip::Out, true));
  shapes.push_back(ImGuiShape("hLine##B_H_ImGuiShape", ImVec2(width/2.0, height-10.0), ImGuiCanvasShapeType::HLine, {0.0f, (float)width}, ImGuiCanvasClip::In, true));
  // ==================================================================================================================

  // Main loop
  while (!glfwWindowShouldClose(window)) {

    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
 
    // ================================================================================================================
    if (ImGui::Begin("imgui_canvas demo")) {
      ImGui::Separator(); // ==========================================================================================
      viewSize.x = ImGui::GetContentRegionAvailWidth();
      viewSize.y = (int)(viewSize.x*imageAspectRatio);
      // ---- mask ----------------------------------------------------------------------------------------------------
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Mask                ");
      ImGui::SameLine();
      if (ImGui::Checkbox("##mask_enable_Checkbox", &maskEnable)) {}
      if (maskEnable) {
        ImGui::SameLine();
        if (ImGui::Button("Square##square_Button"))
          shapes.push_back(ImGuiShape("square##square_ImGuiShape", {(float)rangedRand(shapes[0].m_center.position.x,shapes[1].m_center.position.x), (float)rangedRand(shapes[2].m_center.position.y,shapes[3].m_center.position.y)}, ImGuiCanvasShapeType::Square, {(float)(width/8.0)}, ImGuiCanvasClip::Out, true));

        ImGui::SameLine();
        if (ImGui::Button("Rectangle##rectangle_Button"))
          shapes.push_back(ImGuiShape("rectangle##rectangle_ImGuiShape", {(float)rangedRand(shapes[0].m_center.position.x,shapes[1].m_center.position.x), (float)rangedRand(shapes[2].m_center.position.y,shapes[3].m_center.position.y)}, ImGuiCanvasShapeType::Rectangle, {(float)(width/8.0), (float)(height/8.0)}, ImGuiCanvasClip::Out, true));

        ImGui::SameLine();
        if (ImGui::Button("Circle##circle_Button"))
          shapes.push_back(ImGuiShape("circle##circle_ImGuiShape", {(float)rangedRand(shapes[0].m_center.position.x,shapes[1].m_center.position.x), (float)rangedRand(shapes[2].m_center.position.y,shapes[3].m_center.position.y)}, ImGuiCanvasShapeType::Circle, {(float)(width/8.0)}, ImGuiCanvasClip::Out, true));

        ImGui::SameLine();
        if (ImGui::Button("Ellipse##ellipse_Button"))
          shapes.push_back(ImGuiShape("ellipse##ellipse_ImGuiShape", {(float)rangedRand(shapes[0].m_center.position.x,shapes[1].m_center.position.x), (float)rangedRand(shapes[2].m_center.position.y,shapes[3].m_center.position.y)}, ImGuiCanvasShapeType::Ellipse, {(float)(width/8.0), (float)(height/8.0)}, ImGuiCanvasClip::Out, true));
      } else {
        ImGui::SameLine();
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Square##square_Button");
        ImGui::SameLine();
        ImGui::Button("Rectangle##rectangle_Button");
        ImGui::SameLine();
        ImGui::Button("Circle##circle_Button");
        ImGui::SameLine();
        ImGui::Button("Ellipse##ellipse_Button");
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }
      // ---- mask ----------------------------------------------------------------------------------------------------

      ImGui::Separator(); // ==========================================================================================

      shader.UpdateTexture(GL_TEXTURE0, &imageTexture, width, height, imageData);
      shader.setUniform("image", 0);

      shader.UpdateTexture(GL_TEXTURE1, &maskTexture, width, height, maskData);
      shader.setUniform("mask", 1);

      shader.UpdateTexture(GL_TEXTURE2, &compositeTexture, width, height, 0, GL_RGBA);
      shader.BindTexture(0);

      shader.UpdateRenderBuffer(&renderBuffers, width, height, &frameBuffers, &compositeTexture);

      ImGui::DrawCanvas("canvas", viewSize, ImVec2(width,height), shapes, (ImTextureID)(intptr_t)compositeTexture, maskData);

      ImGui::Separator(); // ==========================================================================================

    }
    ImGui::End();
    // ================================================================================================================

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
// ====================================================================================================================rangedRand