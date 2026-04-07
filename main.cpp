#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "MovieDB.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "IMDb Movie Parser", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool isMainWindowOpened = true;

    std::string windowTitle = "Movie Parser";
    
    std::string APIkey;
    MovieDatabase *globalMDB = nullptr;

    bool isFirstFrame = true;
    bool isApiValid = false;
    bool showError = false;
    bool submitted = false;

    char buf_apiKey[256] = "";
    char buf_searchMovieTitle[256] = "";
    char buf_searchMovieYear[256] = "";
    char buf_LsearchMovieYear[256] = "";

    Movie resultMovie;
    


#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::SetNextWindowSize(ImVec2(600,400));
        if(ImGui::Begin(windowTitle.c_str(), &isMainWindowOpened, ImGuiWindowFlags_NoCollapse)){
            if(isFirstFrame){
                ImGui::OpenPopup("API key");
                isFirstFrame = false;
            }
            
            ImGui::SetNextWindowSize(ImVec2(400,250), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            if (ImGui::BeginPopupModal("API key", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                ImGui::Text("Enter your API key:");
                ImGui::InputText("##api_key", buf_apiKey, sizeof(buf_apiKey));
                if (showError && submitted) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid API key! Please try again.");
                } else {
                    ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight()));
                }
                
                if (ImGui::Button("Submit")) {
                    if (strlen(buf_apiKey) > 0) { 
                        globalMDB = new MovieDatabase(buf_apiKey);
                        isApiValid = globalMDB->isAPIKeyValid();
                        submitted = true;
                        showError = !isApiValid;
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Continue without API")) {
                    ImGui::CloseCurrentPopup();
                    windowTitle = "Movie Parser [w/oAPI]";
                }
                
                if (submitted && isApiValid) {
                    ImGui::CloseCurrentPopup();
                    windowTitle = "Movie Parser [w/API]";
                    APIkey = buf_apiKey;  
                }

                ImGui::EndPopup();
            }




            if(ImGui::BeginTabBar("TabBar")){
                if(ImGui::BeginTabItem("Search")){ //Search from API, Search in collection 
                    if(!isApiValid){
                        ImGui::BeginDisabled();
                    }
                    ImGui::Text("Search from IMDb");
                    ImGui::Text("Enter movie title: ");
                    ImGui::SameLine();
                    ImGui::InputText("##searchMovieTitle", buf_searchMovieTitle, sizeof(buf_searchMovieTitle));

                    ImGui::Text("Enter year(optional)");
                    ImGui::SameLine();
                    ImGui::InputText("##searchMovieYear", buf_searchMovieYear, sizeof(buf_searchMovieYear));
                    if(ImGui::Button("Search (API)")){
                        resultMovie = globalMDB->searchMovie(buf_searchMovieTitle, buf_searchMovieYear);
                    }
                    


                    if(!isApiValid){
                        ImGui::EndDisabled();
                        ImGui::TextColored(ImVec4(1, 0, 0, 1), "API key required!");
                    }

                    ImGui::NewLine();
                    ImGui::Text("Search from collection");
                    ImGui::Text("Enter movie title: ");
                    ImGui::SameLine();
                    ImGui::InputText("##LsearchMovieTitle", buf_LsearchMovieYear, sizeof(buf_LsearchMovieYear));
                    if(ImGui::Button("Search (Collection)")){
                        
                    }

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Collection")){ // View all movies

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Statistics")){ // Show Statistics
                    
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Edit")){ // Filter Movies(new window), Sort Movies(Collection), Mark as watched, Remove Movie

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("File")){ //Download csv,json files

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

        }ImGui::End();


        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}