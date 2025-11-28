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

void Arena::display() {
    // Print column headers
    std::cout << "   ";
    for (int column = 0; column < arenaWidth; column++) {
        std::cout << std::setw(3) << std::right << column; 
    }
    std::cout << "\n";
    std::cout << std::endl;

    // Print each row
    for (int row = 0; row < arenaHeight; row++) {
        std::cout << std::setw(2) << std::right << row;
        std::cout << "  ";

        for (int col = 0; col < arenaWidth; col++) {
            RobotBase* robot = findRobotAt(row, col);
            
            if (robot != nullptr) {
                // Robot found at this cell
                if (robot->get_health() > 0) {
                    // Living robot: print "R" + character
                    std::cout << " R" << robot->m_character;
                } else {
                    // Dead robot: print "X" + character
                    std::cout << " X" << robot->m_character;
                }
            } else {
                // No robot, print terrain
                std::cout << std::setw(3) << std::right << grid[row][col];
            }
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

std::string Arena::compileRobot(const std::string& fileName){
    std::string robotName = fileName.substr(3, fileName.size() - 6);

    std::string sharedLib = std::string("lib") + robotName + ".so";

    const std::string compileCMD = std::string("g++ -shared -fPIC -o ") + sharedLib + " " + fileName + " RobotBase.o -I. -std=c++20";

    std::cout << "Compiling" + fileName + "...\n";

    int result = system(compileCMD.c_str());

    if (result != 0){
        std::cout << "Error";
        return "";
    }

    return sharedLib;
}

RobotBase* Arena::loadRobot(const std::string& sharedLib){
    void* handle = dlopen(sharedLib.c_str(), RTLD_LAZY);
    if (!handle){
        std::cout << "Error loading library: " << dlerror() << std::endl;
        return nullptr;
    }

    void* sym = dlsym(handle, "createRobot");

    if (sym == nullptr){
        std::cout << dlerror() <<std::endl;
        dlclose(handle);
        return nullptr;
    }
    typedef RobotBase* (*CreateRobotFunc)();
    CreateRobotFunc createRobot = reinterpret_cast<CreateRobotFunc>(sym);

    RobotBase* robot = createRobot();

    if (robot == nullptr){
        std::cout << "Error creating robot." << std::endl;
        dlclose(handle);
        return nullptr;
    }

    robot_handles.push_back(handle);
    return robot;
}

void Arena::setupRobot(RobotBase* robot, int index){
    robot->set_boundaries(20,20);

    std::string characters = "@#$%&!*^~+";
    robot->m_character = characters[index % characters.length()];

    int row, col;
     
    do{
        row = rand();
        col = rand();
    }

    while (cellEmpty(row, col));{
        robot->move_to(row,col);
        std::cout << "Loaded robot: " << robot->m_name << " at (" << row << ", " << col << ")\n";
    }
}

void Arena::load_all_robots(){
    std::cout << "Loading robots...\n";
    std::vector<std::string> robot_files = find_robot_files();

    for (const auto& filename : robot_files){
        std::string shared_lib = compileRobot(filename);
        if (shared_lib.empty()){
            continue;
        }
        RobotBase* robot = loadRobot(shared_lib);
        if (robot == nullptr){
            continue;
        }

        setupRobot(robot, robots.size());
        robots.push_back(robot);
    }
    std::cout << "Loaded " << robots.size() << "robots\n";
}

RobotBase* Arena::findRobotAt(int row, int col){
    for (const auto& robot : robots){
        int robotRow, robotCol;
        robot->get_current_location(robotRow, robotCol);

        if (robotRow == row && robotCol == col){
            return robot;
        }
    }
    return nullptr;
}

void Arena::cleanup(){
    for (const auto robot : robots){
        delete robot;
    }

    for (const auto handle : robot_handles){
        dlclose(handle);
    }
}