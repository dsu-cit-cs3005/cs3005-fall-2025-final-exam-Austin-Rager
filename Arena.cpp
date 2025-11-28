#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <dlfcn.h>
#include <filesystem>
#include "RobotBase.h"
#include "Arena.h"

namespace fs = std::filesystem;

Arena::Arena(){};

Arena::~Arena(){};

void Arena::load_config(std::string fileName){
    std::ifstream inFile;

    inFile.open(fileName);
    if (!inFile){
        std::cout << "File could not be read.";
    }

    std::string key;

    while (inFile >> key){
        if (key == "arena_rows") {
            inFile >> arenaHeight;
        } else if (key == "arena_cols") {
            inFile >> arenaWidth;
        } else if (key == "num_mounds") {
            inFile >> mounds;
        } else if (key == "num_pits") {
            inFile >> pits;
        } else if (key == "num_flamethrowers") {
            inFile >> flamethrowers;
        } else if (key == "max_rounds") {
            inFile >> maxRound;
        } else if (key == "watch_live") {
            std::string value;
            inFile >> value;
            watch_live = (value == "true");
        }
    }

    inFile.close();

    // Initialize the grid with the loaded dimensions
    grid.resize(arenaHeight);
    for (int i = 0; i < arenaHeight; i++) {
        grid[i].resize(arenaWidth, '.');
    }
}

bool Arena::cellEmpty(int& row, int& col){
    if (row < 0 || row >= arenaHeight || col < 0 || col >= arenaWidth){
        return false;
    }
    if (grid[row][col] == '.'){
        return true;
    }
    else{
        return false;
    }
}

void Arena::place_obstacles(){
    for (int i = 0; i < mounds; i++){
        int row = rand() % arenaHeight;
        int col = rand() % arenaWidth;

        while (!cellEmpty(row, col)){
            row = rand() % arenaHeight;
            col = rand() % arenaWidth;
        }
        grid[row][col] = 'M';
    }

    for (int i = 0; i < pits; i++){
        int row = rand() % arenaHeight;
        int col = rand() % arenaWidth;

        while (!cellEmpty(row, col)){
            row = rand() % arenaHeight;
            col = rand() % arenaWidth;
        }
        grid[row][col] = 'P';
    }

    for (int i = 0; i < flamethrowers; i++){
        int row = rand() % arenaHeight;
        int col = rand() % arenaWidth;

        while (!cellEmpty(row, col)){
            row = rand() % arenaHeight;
            col = rand() % arenaWidth;
        }
        grid[row][col] = 'F';
    }
}

void Arena::display(){
    std::cout << "   ";
    for (int column = 0; column < arenaWidth; column++){
        std::cout << std::setw(3) << std::right << column; 
    }
    std::cout << "\n";
    std::cout << std::endl;

    for (int row = 0; row < arenaHeight; row++){
        std::cout << std::setw(2) << std::right << row;
        std::cout << "  ";

        for (int col = 0; col < arenaWidth; col++){
            std::cout << std::setw(3) << std::right << grid[row][col];
        }

        std::cout << "\n";
    }
}

std::vector<std::string> Arena::find_robot_files(){
    std::vector<std::string> filenames;

    for (const auto& entry : fs::directory_iterator(".")){
        if (entry.is_regular_file()){
            std::string filename = entry.path().filename().string();
            if (filename.starts_with("Robot_") && filename.ends_with(".cpp")){
                filenames.push_back(filename);
            }
        }
    }
    return filenames;
}

bool Arena::matches_robot_pattern(std::string filename){
    if (filename.length() < 11){
        return false;
    }
    else if (filename.substr(0, 6) != "Robot_"){
        return false;
    }
    else if (filename.substr(filename.length() - 4) != ".cpp"){
        return false;
    }
    else{
        return true;
    }
}

std::string compileRobot(const std::string& fileName){
    std::string robotName = fileName.substr(6, fileName.size() - 4);
}