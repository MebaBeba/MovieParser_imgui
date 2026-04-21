#include "MovieDB.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <iomanip>
#include <ctime>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string *output){
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

Movie Movie::fromJson(const json& j){
    Movie m;
    m.imdbID = j.value("imdbID", "");
    m.title = j.value("Title", "");
    m.year = j.value("Year", "");
    m.released = j.value("Released", "");
    m.runtime = j.value("Runtime", "");
    m.genre = j.value("Genre", "");
    m.director = j.value("Director", "");
    m.writer = j.value("Writer", "");
    m.actors = j.value("Actors", "");
    m.language = j.value("Language", "");
    m.country = j.value("Country", "");
    m.awards = j.value("Awards", "");
    m.imdbRating = j.value("imdbRating", "");
    m.watched = j.value("watched", false);
    m.personalRating = j.value("personalRating", "");
    return m;
}

json Movie::toJson() const {
    json j;
    j["imdbID"] = imdbID;
    j["Title"] = title;
    j["Year"] = year;
    j["Released"] = released;
    j["Runtime"] = runtime;
    j["Genre"] = genre;
    j["Director"] = director;
    j["Writer"] = writer;
    j["Actors"] = actors;
    j["Language"] = language;
    j["Country"] = country;
    j["Awards"] = awards;
    j["imdbRating"] = imdbRating;
    j["watched"] = watched;
    j["personalRating"] = personalRating;
    return j;
}

MovieDatabase::MovieDatabase(const std::string &key, const std::string &file) : apiKey(key), dataFile(file) {
    loadFromFile();
}

MovieDatabase::~MovieDatabase(){
    saveToFile();
}

void MovieDatabase::loadFromFile(){
    std::ifstream file(dataFile);
    if(file.is_open()){
        try
        {
            json j;
            file >> j;
            movies.clear();
            for(const auto &item : j){
                movies.push_back(Movie::fromJson(item));
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "Error loading file! " << e.what() << '\n';
        }
        
    }
}

void MovieDatabase::saveToFile() const {
    json j = json::array();
    for(const auto &movie : movies){
        j.push_back(movie.toJson());
    }

    std::ofstream file(dataFile);
    if(file.is_open()){
        file << std::setw(4) << j << std::endl;
    }
}

void MovieDatabase::addMovie(const Movie &movie){
    auto it = std::find_if(movies.begin(), movies.end(),
        [&movie](Movie &m) { return movie.imdbID == m.imdbID; });

    if(it == movies.end()){
        movies.push_back(movie);
        saveToFile();
        std::cout << "Movie added succesfully\n";
    }else{
        std::cout << "Movie already exists in collection!\n";
    }
}

bool MovieDatabase::removeMovie(const std::string &imdbID){
    auto it = std::find_if(movies.begin(), movies.end(), 
        [&imdbID](Movie &m) { return m.imdbID == imdbID; });
    
    if(it != movies.end()){
        movies.erase(it);
        saveToFile();
        return true;
    }
    return false;
}

std::string MovieDatabase::httpRequest(const std::string& url) const{
    CURL* curl = curl_easy_init();
    std::string response;

    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            std::cerr << "HTTP request failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    return response;
}

bool MovieDatabase::isAPIKeyValid() const {
    std::string url = "http://www.omdbapi.com/?apikey=" + apiKey + "&t=Inception&type=movie";
    
    std::string response = httpRequest(url);
    
    if (response.empty()) {
        return false;  
    }
    
    try {
        json j = json::parse(response);

        if (j.contains("Error")) {
            std::string error = j["Error"]; 
            
            if (error == "Invalid API key!") {
                return false; 
            }
            
            if (error == "Movie not found!") {
                return true;  
            }
            
            return false;
        }

        return j.contains("Title") && j.contains("imdbID");
        
    } catch (const std::exception& e) {
        return false; 
    }
}

Movie MovieDatabase::searchMovie(const std::string& title, const std::string &year){
    std::string url = "https://www.omdbapi.com/?apikey=" + apiKey + "&t=" + title;
    if(!year.empty()){
        url += "&y=" + year;
    }

    std::string searchTitle = title;
    std::replace(searchTitle.begin(), searchTitle.end(), ' ', '+');

    size_t pos = url.find(title);
    if(pos != std::string::npos){
        url.replace(pos, title.length(), searchTitle);
    }

    std::string response = httpRequest(url);
    if(response.empty()){
        throw std::runtime_error("No response from API");
    }

    json j = json::parse(response);

    if(j.contains("Error")){
        throw std::runtime_error(j["Error"]);
    }

    return Movie::fromJson(j);
}

void MovieDatabase::markAsWatched(const std::string& title){
    auto it = std::find_if(movies.begin(), movies.end(), [&title](const Movie& movie){
        return movie.title == title;
    });

    if(it != movies.end()){
        if(it->watched == false){
            it->watched = true;
        }else{
            it->watched = false;
        }
    }
}

void MovieDatabase::setPersonalRating(const std::string& title, const std::string& personalRating){
    auto it = std::find_if(movies.begin(), movies.end(), [&title](const Movie& movie){
        return movie.title == title;
    });

    if(it != movies.end()){
        it->personalRating = personalRating;
    }
}

std::vector<Movie> MovieDatabase::searchLocal(const std::string& query) const{
    std::vector<Movie> result;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for(auto movie : movies){
        std::string lowerTitle = movie.title;
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);

        if(lowerTitle.find(lowerQuery) != std::string::npos){
            result.push_back(movie);
        }
    }
    return result;
}

std::vector<Movie> MovieDatabase::filterMovies(const std::string& genre, const std::string& year, double minRating, bool watchedOnly) const {
    std::vector<Movie> result;

    for(const auto& movie : movies){
        bool matches = true;
        if(!genre.empty() && movie.genre.find(genre) == std::string::npos){
            matches = false;
        }

        if(!year.empty() && movie.year.find(year) == std::string::npos){
            matches = false;
        }

        if(minRating > 0.0 && !movie.imdbRating.empty()){
            try{
                double rating = std::stod(movie.imdbRating);
                if(rating < minRating) matches = false;
            }catch(...){
                matches = false;
            }
        }

        if(watchedOnly && !movie.watched){
            matches = false;
        }

        if(matches){
            result.push_back(movie);
        }
    }
    return result;
}

void MovieDatabase::sortMovies(std::vector<Movie>& moviesToSort, const std::string& sortBy) const{
    if(sortBy == "title"){
        std::sort(moviesToSort.begin(), moviesToSort.end(), 
            [](const Movie& a, const Movie& b){ return a.title < b.title; });
    }

    else if(sortBy == "year"){
        std::sort(moviesToSort.begin(), moviesToSort.end(),
            [](const Movie& a, const Movie& b) { return std::stoi(a.year) < std::stoi(b.year); });
    }

    else if(sortBy == "rating"){
        std::sort(moviesToSort.begin(), moviesToSort.end(),
            [](const Movie& a, const Movie& b){
                double ratingA = a.imdbRating.empty() ? 0 : std::stod(a.imdbRating);
                double ratingB = b.imdbRating.empty() ? 0 : std::stod(b.imdbRating);
                return ratingA > ratingB;
            });
    }
}


std::map<std::string, int> MovieDatabase::getTopActors() const{
    std::map<std::string, int> actors;

    for(const Movie& movie : movies){
        std::stringstream ss(movie.actors);
        std::string actor;
        while(std::getline(ss, actor, ',')){
            actor.erase(0, actor.find_first_not_of(' '));
            actor.erase(actor.find_last_not_of(' '));
            if(!actor.empty() && actor != "N/A"){
                actors[actor]++;
            }
        }
    }
    return actors;
}
    
std::map<std::string, int> MovieDatabase::getTopDirectors() const{
    std::map<std::string, int> directors;

    for(const Movie& movie : movies){
        if(!movie.director.empty() && movie.director != "N/A"){
            directors[movie.director]++;
        }
    }
    return directors;
}
    
std::map<std::string, int> MovieDatabase::getGenreDistribution() const{
    std::map<std::string, int> genres;

    for(const Movie& movie : movies){
        std::stringstream ss(movie.genre);
        std::string genre;
        while(std::getline(ss,genre,' ')){
            genre.erase(0, genre.find_first_not_of(' '));
            genre.erase(genre.find_last_not_of(' ') + 1);
            if(!genre.empty() && genre != "N/A"){
                genres[genre]++;
            }
        }
    }
    return genres;
}

int MovieDatabase::getWatchedCount() const{
    int watched = std::count_if(movies.begin(), movies.end(), [](const Movie& movie){
        return movie.watched;
    });

    return watched;
}

double MovieDatabase::getAverageRating() const{
    double totalRating = 0;
    int ratedCount = 0;
    for(const Movie& movie : movies){
        if(!movie.imdbRating.empty() && movie.imdbRating != "N/A"){
            try{
                totalRating += std::stod(movie.imdbRating);
                ratedCount++;
            }catch(...) {}
        }
    }
    if(ratedCount > 0){
        return totalRating / ratedCount;
    }
}




void MovieDatabase::showStatistics() const{
    std::cout << "\n========== STATISTICS ==========\n";
    std::cout << "Total movies: " << movies.size() << std::endl;
    int watched = std::count_if(movies.begin(), movies.end(),
            [](const Movie& m){ return m.watched; });
    std::cout << "Watched: " << watched << "(" << (watched * 100) / movies.size() << "%)" << std::endl;

    double totalRating = 0;
    int ratedCount = 0;
    for(const Movie& movie : movies){
        if(!movie.imdbRating.empty() && movie.imdbRating != "N/A"){
            try{
                totalRating += std::stod(movie.imdbRating);
                ratedCount++;
            }catch(...) {}
        }
    }
    if(ratedCount > 0){
        std::cout << "Average IMDb Rating: " << std::fixed << std::setprecision(1) << (totalRating / ratedCount) << "/10" << std::endl;
    }

    std::cout << "Actors: \n";
    auto actors = getTopActors();
    std::vector<std::pair<std::string, int>> sortedActors(actors.begin(), actors.end());
    std::sort(sortedActors.begin(), sortedActors.end(),
            [](const auto& a, const auto& b){ return a.second > b.second; });

    for(const auto&[actor, count] : sortedActors){
        std::cout << "  " << actor << ": " << count << " movies\n"; 
    }

    std::cout << "Directors: \n";
    auto directors = getTopDirectors();
    std::vector<std::pair<std::string, int>> sortedDirectors(directors.begin(), directors.end());
    std::sort(sortedDirectors.begin(), sortedDirectors.end(),
            [](const auto& a, const auto& b){ return a.second > b.second; });

    for(const auto&[director, count] : sortedDirectors){
        std::cout << "  " << director << ": " << count << " movies\n"; 
    }

    std::cout << "Genres: \n";
    auto genres = getGenreDistribution();
    std::vector<std::pair<std::string, int>> sortedGenres(genres.begin(), genres.end());
    std::sort(sortedGenres.begin(), sortedGenres.end(),
            [](const auto& a, const auto& b){ return a.second > b.second; });

    for(const auto&[genre, count] : sortedGenres){
        std::cout << "  " << genre << ": " << count << " movies\n"; 
    }

    std::cout << "================================\n";
}

void MovieDatabase::displayMovies(const std::vector<Movie>& moviesToShow) const{
    if(moviesToShow.empty()){
        std::cout << "No movies to display!\n";
    }

    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << std::left << std::setw(5) << "ID" 
              << std::setw(40) << "Title" 
              << std::setw(10) << "Year" 
              << std::setw(10) << "Rating" 
              << std::setw(10) << "Watched" << "\n";
    std::cout << std::string(80, '-') << "\n";

    int index = 1;
    for(const Movie& movie : moviesToShow){
        std::cout << std::left << std::setw(5) << index++ 
                << std::setw(40) << (movie.title.length() > 37 ? movie.title.substr(0,34) + "..." : movie.title)
                << std::setw(10) << (movie.year.length() > 8 ? movie.year + ' ' : movie.year.length() == 7 ? movie.year + "     " : movie.year)
                << std::setw(10) << movie.imdbRating
                << std::setw(10) << (movie.watched ? "Yes" : "No") << "\n";
    }
    std::cout << std::string(80, '=') << "\n";
}