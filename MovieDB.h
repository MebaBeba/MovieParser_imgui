#ifndef MOVIEDB_H
#define MOVIEDB_H

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Movie{
    std::string imdbID;
    std::string title;
    std::string year;
    std::string released;
    std::string runtime;
    std::string genre;
    std::string director;
    std::string writer;
    std::string actors;
    std::string language;
    std::string country;
    std::string awards;
    std::string imdbRating;
    bool watched;
    std::string personalRating;

    Movie() : watched(false) {}

    static Movie fromJson(const json& j);

    json toJson() const;
};

class MovieDatabase{
    std::vector<Movie> movies;
    std::string apiKey;
    std::string dataFile;

    void loadFromFile();
    
    void saveToFile() const;
    
    std::string httpRequest(const std::string& url) const;
public:
    MovieDatabase(const std::string &key, const std::string &file = "movies.json");

    Movie searchMovie(const std::string& title, const std::string& year = "");

    bool isAPIKeyValid() const;
    
    void addMovie(const Movie& movie);
    
    bool removeMovie(const std::string& imdbID);
    
    void markAsWatched(const std::string& imdbID, const std::string& personalRating = "");
    
    std::vector<Movie> searchLocal(const std::string& query) const;
    
    std::vector<Movie> filterMovies(const std::string& genre = "", 
                                    const std::string& year = "",
                                    double minRating = 0.0,
                                    bool watchedOnly = false) const;
    

    void sortMovies(std::vector<Movie>& moviesToSort, const std::string& sortBy) const;
    
    void showStatistics() const;
    
    void displayMovies(const std::vector<Movie>& moviesToShow) const;
    
    const std::vector<Movie>& getAllMovies() const { return movies; }
    
    std::map<std::string, int> getTopActors() const;
    
    std::map<std::string, int> getTopDirectors() const;
    
    std::map<std::string, int> getGenreDistribution() const;
};


#endif 