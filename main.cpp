#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "MovieDB.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <fstream>
#include "ImGuiFileDialog.h"
#include <iostream>

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


void SortMovies(std::vector<Movie> &movies, ImGuiTableSortSpecs* sortSpecs){
    if(!sortSpecs || sortSpecs->SpecsCount == 0) return;

    const ImGuiTableColumnSortSpecs* spec = sortSpecs->Specs;

    std::sort(movies.begin(), movies.end(), [spec](const Movie &a, const Movie &b){
        switch(spec->ColumnIndex){
            case 0:
                if(spec->SortDirection == ImGuiSortDirection_Ascending){
                    return a.title < b.title;
                }else{
                    return a.title > b.title;
                }
            case 1:
                if(spec->SortDirection == ImGuiSortDirection_Ascending){
                    return a.year < b.year;
                }else{
                    return a.year > b.year;
                }
            case 2:
                if(spec->SortDirection == ImGuiSortDirection_Ascending){
                    return a.imdbRating < b.imdbRating;
                }else{
                    return a.imdbRating > b.imdbRating;
                }
            case 3:
                if(spec->SortDirection == ImGuiSortDirection_Ascending){
                    return a.watched < b.watched;
                }else{
                    return a.watched > b.watched;
                }
            default:
                return false;
        }      
    });
}

bool exportToCSV(const std::vector<Movie>& movies, const std::string& filename = "movies_export.csv"){
    std::ofstream file(filename);
    if(file.is_open()){
        file << "Title,Year,IMDB Rating,Genre,Director,Actors,Watched,Personal Rating\n";
        for(const Movie& movie : movies){
            file << "\"" << movie.title << "\","
                 << movie.year << ","
                 << movie.imdbRating << ","
                 << "\"" << movie.genre << "\","
                 << "\"" << movie.director << "\","
                 << "\"" << movie.actors << "\","
                 << (movie.watched ? "Yes" : "No") << ","
                 << "\"" << movie.personalRating << "\"\n";
        }
        return true;
    } else {
        return false;
    }
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
    std::unique_ptr<MovieDatabase> globalMDB;

    bool isFirstFrame = true;
    bool isApiValid = false;
    bool showError = false;
    bool submitted = false;
    int collectionVersion = 0;

    char buf_apiKey[256] = "";
    char buf_searchMovieTitle[256] = "";
    char buf_searchMovieYear[256] = "";
    char buf_LsearchMovieYear[256] = "";
    char buf_filterGenre[256] = "";
    char buf_filterYear[256] = "";
    float buf_filterRating = 0.0;
    bool buf_filterWatched = false;
    char buf_CSVFileName[256] = "";
    char buf_JsonFileName[256] = "";

    Movie resultMovie;
    std::vector<Movie> resultLMovies;

    static std::string exportFilePath = "";

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
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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
                        globalMDB.reset(new MovieDatabase(buf_apiKey, "../movies.json"));
                        isApiValid = globalMDB->isAPIKeyValid();
                        submitted = true;
                        showError = !isApiValid;
                    }
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Continue without API")) {
                    globalMDB.reset(new MovieDatabase("", "../movies.json"));
                    isApiValid = false;
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
                        try{
                            resultMovie = globalMDB->searchMovie(buf_searchMovieTitle, buf_searchMovieYear);
                            ImGui::OpenPopup("Result");
                        }catch(std::exception& e){
                            std::cerr << "Error: " << e.what() << std::endl;
                            ImGui::OpenPopup("Movie not found");
                            resultMovie = {};
                        }
                    }

                    ImGui::SetNextWindowSize(ImVec2(150,75), ImGuiCond_Always);
                    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                    if(ImGui::BeginPopupModal("Movie not found", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)){
                        ImGui::Text("Movie not found!");
                        if(ImGui::Button("Close")){
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::SetNextWindowSize(ImVec2(400,250), ImGuiCond_Always);
                    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                        ImGui::Text("Title: %s", resultMovie.title.c_str());
                        ImGui::Text("Year: %s", resultMovie.year.c_str());
                        
                        ImGui::NewLine();
                        ImGui::Text("Add to collection?");
                        if(ImGui::Button("Yes")){
                            globalMDB->addMovie(resultMovie);
                            collectionVersion++;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("No")){
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
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
                        resultLMovies = globalMDB->searchLocal(buf_LsearchMovieYear);
                        ImGui::OpenPopup("Collection Search Results");
                    }

                    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Always);
                    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("Collection Search Results", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
                        
                        if (resultLMovies.empty()) {
                            ImGui::TextColored(ImVec4(1, 1, 0, 1), "No movies found in collection.");
                        } else {
                            ImGui::Text("Found %zu movies:", resultLMovies.size());
                            ImGui::Separator();
                            
                            ImGui::BeginChild("MoviesList", ImVec2(0, 300), true);
                            
                            for (const auto& movie : resultLMovies) {
                                ImGui::Text("%s (%s) - Rating: %s", 
                                    movie.title.c_str(), 
                                    movie.year.c_str(), 
                                    movie.imdbRating.c_str());
                            }
                            
                            ImGui::EndChild();
                        }
                        
                        ImGui::Separator();
                        if (ImGui::Button("Close", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                        }
                        
                        ImGui::EndPopup();
                    }

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Collection")){ // View all movies
                    static std::vector<Movie> displayMovies;
                    static int lastVersion = -1;
                    static Movie selectedMovie;
                    static Movie selectedMovieInfo;


                    if(globalMDB != nullptr && collectionVersion != lastVersion){
                        displayMovies = globalMDB->getAllMovies();
                        lastVersion = collectionVersion;
                    }

                    ImGui::SetNextItemWidth(100);
                    ImGui::InputText("Genre", buf_filterGenre, sizeof(buf_filterGenre));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputText("Year", buf_filterYear, sizeof(buf_filterYear));
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(100);
                    ImGui::SliderFloat("Min Rating", &buf_filterRating, 0, 10);
                    ImGui::SameLine();
                    ImGui::Checkbox("Watched only?", &buf_filterWatched);

                    if(ImGui::Button("Refresh")){
                        if(globalMDB != nullptr){
                            displayMovies = globalMDB->filterMovies(buf_filterGenre,buf_filterYear,(double)buf_filterRating,buf_filterWatched);
                        }
                    }
                    ImGui::SameLine();
                    if(ImGui::Button("Clear Filters")){
                        memset(buf_filterYear, 0, sizeof(buf_filterYear));
                        memset(buf_filterGenre, 0, sizeof(buf_filterGenre));
                        buf_filterRating = 0;
                        buf_filterWatched = false;
                        
                        if(globalMDB != nullptr){
                            displayMovies = globalMDB->getAllMovies();
                        }
                    }

                    if (ImGui::BeginTable("MoviesTable", 5, ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_DefaultSort);
                        ImGui::TableSetupColumn("Year", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Rating", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Watched", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                        ImGui::TableHeadersRow();


                        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();

                        if(sortSpecs && sortSpecs->SpecsDirty){
                            SortMovies(displayMovies, sortSpecs);
                            sortSpecs->SpecsDirty = false;
                        }

                        for(auto& movie : displayMovies){
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%s", movie.title.c_str());
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", movie.year.c_str());
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", movie.imdbRating.c_str());
                            ImGui::TableSetColumnIndex(3);
                            ImGui::Text("%s", movie.watched ? "Yes" : "No");

                            ImGui::TableSetColumnIndex(4);
                            ImGui::PushID(movie.imdbID.c_str());
                            if(ImGui::Button("...")){
                                selectedMovie = movie;
                                ImGui::OpenPopup("MovieActions");
                            }
                            ImGui::SameLine();
                            if(ImGui::Button("i")){
                                selectedMovieInfo = movie;
                                ImGui::OpenPopup("MovieInformation");
                            }

                            ImGui::SetNextWindowSize(ImVec2(250, 120), ImGuiCond_Always);
                            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                            if(ImGui::BeginPopupModal("MovieActions", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)){
                                ImGui::Text("Movie: %s", selectedMovie.title.c_str());
                                static bool checkWatched = false;
                                static float buf_personalRating = 0.0;

                                if(ImGui::IsWindowAppearing()){
                                    checkWatched = selectedMovie.watched;
                                    buf_personalRating = selectedMovie.personalRating.empty() ? 0.0f : atof(selectedMovie.personalRating.c_str());
                                }

                                if(ImGui::Button("Remove")){
                                    globalMDB->removeMovie(selectedMovie.imdbID);
                                    collectionVersion++;
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::SameLine();
                                if(ImGui::Checkbox("Watched", &checkWatched)){
                                    globalMDB->markAsWatched(selectedMovie.title);
                                    collectionVersion++;
                                }
                                
                                ImGui::SetNextItemWidth(100.0f);
                                if(ImGui::SliderFloat("Personal Rating", &buf_personalRating, 0, 10, "%.1f")){
                                    globalMDB->setPersonalRating(selectedMovie.title, std::to_string(buf_personalRating));
                                    collectionVersion++;
                                }

                                if(ImGui::Button("Close")){
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndPopup();
                            }

                            ImGui::SetNextWindowSize(ImVec2(450, 310), ImGuiCond_Always);
                            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                            if(ImGui::BeginPopupModal("MovieInformation", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)){
                                ImGui::Text("IMDbID: %s", selectedMovieInfo.imdbID.c_str());
                                ImGui::Text("Title: %s", selectedMovieInfo.title.c_str());
                                ImGui::Text("Year: %s", selectedMovieInfo.year.c_str());
                                ImGui::Text("Released: %s", selectedMovieInfo.released.c_str());
                                ImGui::Text("Runtime: %s", selectedMovieInfo.runtime.c_str());
                                ImGui::Text("Genre: %s", selectedMovieInfo.genre.c_str());
                                ImGui::Text("Director: %s", selectedMovieInfo.director.c_str());
                                ImGui::Text("Writer: %s", selectedMovieInfo.writer.c_str());
                                ImGui::Text("Actors: %s", selectedMovieInfo.actors.c_str());
                                ImGui::Text("Language: %s", selectedMovieInfo.language.c_str());
                                ImGui::Text("Country: %s", selectedMovieInfo.country.c_str());
                                ImGui::Text("Awards: %s", selectedMovieInfo.awards.c_str());
                                ImGui::Text("ImdbRating: %s", selectedMovieInfo.imdbRating.c_str());
                                ImGui::Text("Watched: %s", selectedMovieInfo.watched ? "Yes" : "No");
                                ImGui::Text("PersonalRating: %s", selectedMovieInfo.personalRating == "" ? "None" : selectedMovieInfo.personalRating.c_str());

                                if(ImGui::Button("Close")){
                                    ImGui::CloseCurrentPopup();
                                }
                                ImGui::EndPopup();
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndTable();
                    }
                
                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("Statistics")){ // Show Statistics
                    std::vector<Movie> movies = globalMDB->getAllMovies();
                    ImGui::Text("Total movies: %zu", movies.size());
                    ImGui::SameLine();
                    ImGui::Text("Watched: %d", globalMDB->getWatchedCount());
                    ImGui::SameLine();
                    ImGui::Text("Avg Rating: %.2f/10", globalMDB->getAverageRating());

                    if(ImGui::BeginTable("Statistic", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)){
                        ImGui::TableSetupColumn("Actors");
                        ImGui::TableSetupColumn("Directors");
                        ImGui::TableSetupColumn("Genres");
                        ImGui::TableHeadersRow();

                        auto actors = globalMDB->getTopActors();
                        std::vector<std::pair<std::string, int>> sortedActors(actors.begin(), actors.end());
                        std::sort(sortedActors.begin(), sortedActors.end(),
                                [](const auto& a, const auto& b){ return a.second > b.second; });

                        auto directors = globalMDB->getTopDirectors();
                        std::vector<std::pair<std::string, int>> sortedDirectors(directors.begin(), directors.end());
                        std::sort(sortedDirectors.begin(), sortedDirectors.end(),
                                [](const auto& a, const auto& b){ return a.second > b.second; });

                        auto genres = globalMDB->getGenreDistribution();
                        std::vector<std::pair<std::string, int>> sortedGenres(genres.begin(), genres.end());
                        std::sort(sortedGenres.begin(), sortedGenres.end(),
                                [](const auto& a, const auto& b){ return a.second > b.second; });
    
                        size_t maxRows = std::max({sortedActors.size(), sortedDirectors.size(), sortedGenres.size()});

                        for(size_t i = 0; i < maxRows; ++i){
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            if (i < sortedActors.size()) {
                                ImGui::Text("%s (%d)", sortedActors[i].first.c_str(), sortedActors[i].second);
                            } else {
                                ImGui::Text(""); 
                            }
                            
                            ImGui::TableSetColumnIndex(1);
                            if (i < sortedDirectors.size()) {
                                ImGui::Text("%s (%d)", sortedDirectors[i].first.c_str(), sortedDirectors[i].second);
                            } else {
                                ImGui::Text("");
                            }
                            
                            ImGui::TableSetColumnIndex(2);
                            if (i < sortedGenres.size()) {
                                ImGui::Text("%s (%d)", sortedGenres[i].first.c_str(), sortedGenres[i].second);
                            } else {
                                ImGui::Text("");
                            }
                        }
                        ImGui::EndTable();
                    }

                    ImGui::EndTabItem();
                }
                if(ImGui::BeginTabItem("File")){ //Download csv,json files
                        if(ImGui::Button("Export to CSV")){
                            ImGuiFileDialog::Instance()->OpenDialog("ExportCSV", "Export CSV File", ".csv");
                        }

                        if(ImGui::Button("Download JSON File")){
                            ImGuiFileDialog::Instance()->OpenDialog("DownloadJSON", "Download JSON File", ".json");
                        }
                        
                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGuiFileDialog::Instance()->Display("ExportCSV", ImGuiWindowFlags_None, ImVec2(600,450))){
                            if(ImGuiFileDialog::Instance()->IsOk()){
                                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                                auto movies = globalMDB->getAllMovies();
                                
                                if(exportToCSV(movies, filePathName)){
                                    ImGui::OpenPopup("ExportSuccess");
                                } else {
                                    ImGui::OpenPopup("ExportError");
                                }
                            }
                            ImGuiFileDialog::Instance()->Close();
                        }

                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGuiFileDialog::Instance()->Display("DownloadJSON", ImGuiWindowFlags_None, ImVec2(600,450))){
                            if(ImGuiFileDialog::Instance()->IsOk()){
                                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

                                std::string sourcePath = "../movies.json";

                                if(std::filesystem::exists(sourcePath)){
                                    try{
                                        std::filesystem::copy(sourcePath, filePathName, std::filesystem::copy_options::overwrite_existing);
                                        ImGui::OpenPopup("DownloadSucces");
                                    }catch(const std::exception& e){
                                        std::cerr << "Copy failed: " << e.what() << std::endl;
                                        ImGui::OpenPopup("DownloadError");
                                    }
                                }
                            }
                            ImGuiFileDialog::Instance()->Close();
                        }

                        
                        ImGui::SetNextWindowSize(ImVec2(200, 75), ImGuiCond_Always);
                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGui::BeginPopupModal("ExportSuccess", NULL, ImGuiWindowFlags_NoResize)){
                            ImGui::TextColored(ImVec4(0,1,0,1), "Export successful!");
                            if(ImGui::Button("OK")){
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        
                        ImGui::SetNextWindowSize(ImVec2(200, 75), ImGuiCond_Always);
                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGui::BeginPopupModal("ExportError", NULL, ImGuiWindowFlags_NoResize)){
                            ImGui::TextColored(ImVec4(1,0,0,1), "Export failed!");
                            ImGui::Text("Check file permissions.");
                            if(ImGui::Button("OK")){
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::SetNextWindowSize(ImVec2(200, 75), ImGuiCond_Always);
                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGui::BeginPopupModal("DownloadSucces", NULL, ImGuiWindowFlags_NoResize)){
                            ImGui::TextColored(ImVec4(0,1,0,1), "Dwonload successful!");
                            if(ImGui::Button("OK")){
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        
                        ImGui::SetNextWindowSize(ImVec2(200, 75), ImGuiCond_Always);
                        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                        if(ImGui::BeginPopupModal("DownloadError", NULL, ImGuiWindowFlags_NoResize)){
                            ImGui::TextColored(ImVec4(1,0,0,1), "Download failed!");
                            if(ImGui::Button("OK")){
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                    
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