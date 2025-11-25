#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include "RobotBase.h"
#include "Arena.h"

Arena::Arena(){};

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
}

void Arena::place_obstacles(){
    
}