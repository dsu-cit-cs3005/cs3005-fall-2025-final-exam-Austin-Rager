#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <dlfcn.h>
#include <filesystem>
#include <unistd.h>
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
    std::string robotName = fileName.substr(6, fileName.size() - 10);

    std::string sharedLib = std::string("lib") + robotName + ".so";

    const std::string compileCMD = std::string("g++ -shared -fPIC -o ") + sharedLib + " " + fileName + " RobotBase.o -I. -std=c++20";

    std::cout << "Compiling " + fileName + "...\n";

    int result = system(compileCMD.c_str());

    if (result != 0){
        std::cout << "Error";
        return "";
    }

    return sharedLib;
}

RobotBase* Arena::loadRobot(const std::string& sharedLib){
    std::string fullPath = "./" + sharedLib;
    void* handle = dlopen(fullPath.c_str(), RTLD_LAZY);
    if (!handle){
        std::cout << "Error loading library: " << dlerror() << std::endl;
        return nullptr;
    }

    void* sym = dlsym(handle, "create_robot");

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
    robot->set_boundaries(arenaHeight,arenaWidth);

    std::string characters = "@#$%&!*^~+";
    robot->m_character = characters[index % characters.length()];

    int row, col;
     
    do{
        row = rand() % arenaHeight;
        col = rand() % arenaWidth;
    } while (!cellEmpty(row, col));

    robot->move_to(row,col);
    std::cout << "Loaded robot: " << robot->m_name << " at (" << row << ", " << col << ")\n";
    
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
    std::cout << "Loaded " << robots.size() << " robots\n";
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

int Arena::count_living_robots(){ 
    int counter = 0;
    for (auto robot : robots){
        if (robot->get_health() > 0){
            counter += 1;
        }
    }
    return counter;   
}

void Arena::run_game(){
    round = 0;
    while (round < maxRound){
        std::cout << "=========== starting round " << round << " ===========";

        display();
        if (count_living_robots() <= 1){
            declare_winner();
            break;
        }
        for (const auto robot: robots){
            process_robot_turn(robot);
        }
        if (watch_live == true){
            sleep(1);
        }
    }
    declare_winner();
}

void Arena::process_robot_turn(RobotBase* robot){
    int row,col;
    robot->get_current_location(row, col);

    if (robot->get_health() == 0){
        std::cout << robot->m_name << " is out.";
        return;
    }

    std::cout << robot->m_name << " begins turn. \n";
    std::cout << "Current health: " << robot->get_health() << "\n";
    std::cout << "Current armor: " << robot->get_armor() << "\n";
    std::cout << "Current move speed: " << robot->get_move_speed() << "\n";
    std::cout << "Current location: (" << row << "," << col << ")\n";

    int radarDirection;
    robot->get_radar_direction(radarDirection);

    std::vector<RadarObj> radarResults;
    get_radar_results(robot, radarDirection, radarResults);
    if (radarResults.empty()){
        std::cout << "Found nothing.\n";
    }
    else{
    std::cout << "Radar results:\n";
    for (const auto& obj : radarResults) {
        std::cout << "  - Found '" << obj.m_type 
                  << "' at (" << obj.m_row << "," << obj.m_col << ")\n";
        }
    }

    robot->process_radar_results(radarResults);
    int shotRow, shotCol;
    bool result = robot->get_shot_location(shotRow, shotCol);
    if (result == true){
        std::cout << "Shooting: " << robot->get_weapon() << "\n"; 
        handle_shot(robot, shotRow, shotCol);
    }
    else{
        int moveDirection, moveDistance;
        robot->get_move_direction(moveDirection, moveDistance);
        std::cout << "Moving: " << robot->m_name << "\n";
        handle_movement(robot, moveDirection, moveDistance);
    }
}