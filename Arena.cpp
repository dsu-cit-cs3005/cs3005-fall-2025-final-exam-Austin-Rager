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
        } else if (key == "max_robots") {
            inFile >> maxRobots;
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
    if (grid[row][col] != '.'){
        return false;
    }
    if (findRobotAt(row, col) != nullptr){
        return false;
    }
    return true;
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
        if (maxRobots > 0 && robots.size() >= static_cast<size_t>(maxRobots)){
            std::cout << "Reached max robot limit of " << maxRobots << "\n";
            break;
        }

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
        round++;
        std::cout << "=========== starting round " << round << " ===========" << std::endl;

        display();
        if (count_living_robots() <= 1){
            declare_winner();
            return;
        }
        for (const auto robot: robots){
            process_robot_turn(robot);
        }
        if (watch_live){
            sleep(1);
        }
    }
    declare_winner();
}

void Arena::process_robot_turn(RobotBase* robot){
    int row,col;
    robot->get_current_location(row, col);

    if (robot->get_health() <= 0){
        std::cout << robot->m_name << " is out.\n";
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

void Arena::declare_winner(){
    RobotBase* winner = nullptr;
    int highest_health = 0;
    int living_count = 0;
    
    for (auto robot : robots){
        if (robot->get_health() > 0){
            living_count++;
            if (robot->get_health() > highest_health){
                highest_health = robot->get_health();
                winner = robot;
            }
        }
    }
    std::cout << "\n========== GAME OVER ==========" << std::endl;
    
    if (living_count == 0){
        std::cout << "Draw - all robots destroyed!" << std::endl;
    }
    else if (living_count == 1){
        std::cout << winner->m_name << " wins!" << std::endl;
    }
    else{
        // Multiple robots alive (max rounds reached)
        std::cout << winner->m_name << " wins with " << highest_health << " health remaining!" << std::endl;
    }
}

void Arena::handle_movement(RobotBase* robot, int direction, int distance){
    int currentRow, currentCol;
    robot->get_current_location(currentRow, currentCol);

    if (direction == 0 || distance == 0){
        std::cout << robot->m_name << " stays in place." << std::endl;
        return;
    }
    int maxSpeed = robot->get_move_speed();
    if (distance > maxSpeed){
        distance = maxSpeed;
    }
    int delta_row = directions[direction].first;
    int delta_col = directions[direction].second;  

    for (int step = 0; step < distance; step++){
            int next_row = currentRow + delta_row;
            int next_col = currentCol + delta_col;
            
            if (next_row < 0 || next_row >= arenaHeight || 
                next_col < 0 || next_col >= arenaWidth){
                break;
            }
            
            char cell = grid[next_row][next_col];
            
            if (cell == 'M'){
                std::cout << robot->m_name << " blocked by mound." << std::endl;
                break;
            }
            else if (cell == 'P'){
                currentRow = next_row;
                currentCol = next_col;
                robot->disable_movement();
                std::cout << robot->m_name << " fell into a pit!" << std::endl;
                break;
            }
            else if (cell == 'F'){
                currentRow = next_row;
                currentCol = next_col;
                
                int damage = rand() % 21 + 30;
                
                int armor = robot->get_armor();
                damage = damage * (100 - armor * 10) / 100;
                
                robot->take_damage(damage);
                robot->reduce_armor(1);
                
                std::cout << robot->m_name << " hit by flamethrower! Takes " 
                        << damage << " damage." << std::endl;
                
            }
            else{
                RobotBase* other = findRobotAt(next_row, next_col);
                if (other != nullptr){
                    std::cout << robot->m_name << " blocked by " << other->m_name << "." << std::endl;
                    break;
                }
                
                currentRow = next_row;
                currentCol = next_col;
            }
        }
        
        robot->move_to(currentRow, currentCol);
        std::cout << robot->m_name << " moves to (" << currentRow << "," << currentCol << ")" << std::endl;
}

void Arena::get_radar_results(RobotBase* robot, int direction, std::vector<RadarObj>& results){
    results.clear();
    
    int robot_row, robot_col;
    robot->get_current_location(robot_row, robot_col);
    
    if (direction == 0){
        for (int dir = 1; dir <= 8; dir++){
            int check_row = robot_row + directions[dir].first;
            int check_col = robot_col + directions[dir].second;
            
            if (check_row < 0 || check_row >= arenaHeight ||
                check_col < 0 || check_col >= arenaWidth){
                continue;
            }
            
            char cell = grid[check_row][check_col];
            if (cell == 'M' || cell == 'P' || cell == 'F'){
                results.push_back(RadarObj(cell, check_row, check_col));
            }
            
            RobotBase* other = findRobotAt(check_row, check_col);
            if (other != nullptr && other != robot){
                if (other->get_health() > 0){
                    results.push_back(RadarObj('R', check_row, check_col));
                }
                else{
                    results.push_back(RadarObj('X', check_row, check_col));
                }
            }
        }
    }
    else{
        int delta_row = directions[direction].first;
        int delta_col = directions[direction].second;
        
        int side_row, side_col;
        if (delta_row == 0){
            side_row = 1;
            side_col = 0;
        }
        else if (delta_col == 0){
            side_row = 0;
            side_col = 1;
        }
        else{  // Diagonal direction - perpendicular is (-delta_col, delta_row)
            side_row = -delta_col;
            side_col = delta_row;
        }

        int current_row = robot_row;
        int current_col = robot_col;
        
        while (true){
            current_row += delta_row;
            current_col += delta_col;
            
            if (current_row < 0 || current_row >= arenaHeight ||
                current_col < 0 || current_col >= arenaWidth){
                break;
            }
            
            for (int offset = -1; offset <= 1; offset++){
                int check_row = current_row + (side_row * offset);
                int check_col = current_col + (side_col * offset);
                
                if (check_row < 0 || check_row >= arenaHeight ||
                    check_col < 0 || check_col >= arenaWidth){
                    continue;
                }
                
                char cell = grid[check_row][check_col];
                if (cell == 'M' || cell == 'P' || cell == 'F'){
                    results.push_back(RadarObj(cell, check_row, check_col));
                }
                
                RobotBase* other = findRobotAt(check_row, check_col);
                if (other != nullptr && other != robot){
                    if (other->get_health() > 0){
                        results.push_back(RadarObj('R', check_row, check_col));
                    }
                    else{
                        results.push_back(RadarObj('X', check_row, check_col));
                    }
                }
            }
        }
    }
}

int Arena::calculate_damage(WeaponType weapon){
    switch(weapon){
        case flamethrower:
            return rand() % 21 + 30;  // 30-50
        case railgun:
            return rand() % 11 + 10;  // 10-20
        case grenade:
            return rand() % 31 + 10;  // 10-40
        case hammer:
            return rand() % 11 + 50;  // 50-60
        default:
            return 10;
    }
}

void Arena::handle_shot(RobotBase* robot, int shot_row, int shot_col){
    int shooter_row, shooter_col;
    robot->get_current_location(shooter_row, shooter_col);
    
    WeaponType weapon = robot->get_weapon();
    
    int delta_row = 0;
    int delta_col = 0;
    
    if (shot_row > shooter_row) delta_row = 1;
    else if (shot_row < shooter_row) delta_row = -1;
    
    if (shot_col > shooter_col) delta_col = 1;
    else if (shot_col < shooter_col) delta_col = -1;
    
    std::vector<std::pair<int, int>> affected_cells;
    
    if (weapon == railgun){
        int current_row = shooter_row + delta_row;
        int current_col = shooter_col + delta_col;
        
        while (current_row >= 0 && current_row < arenaHeight &&
               current_col >= 0 && current_col < arenaWidth){
            affected_cells.push_back({current_row, current_col});
            current_row += delta_row;
            current_col += delta_col;
        }
    }
    else if (weapon == flamethrower){
        int side_row, side_col;
        if (delta_row == 0){
            side_row = 1;
            side_col = 0;
        }
        else if (delta_col == 0){
            side_row = 0;
            side_col = 1;
        }
        else{  // Diagonal direction - perpendicular is (-delta_col, delta_row)
            side_row = -delta_col;
            side_col = delta_row;
        }

        int current_row = shooter_row;
        int current_col = shooter_col;

        for (int dist = 0; dist < 4; dist++){
            current_row += delta_row;
            current_col += delta_col;
            
            for (int offset = -1; offset <= 1; offset++){
                int check_row = current_row + (side_row * offset);
                int check_col = current_col + (side_col * offset);
                
                if (check_row >= 0 && check_row < arenaHeight &&
                    check_col >= 0 && check_col < arenaWidth){
                    affected_cells.push_back({check_row, check_col});
                }
            }
        }
    }
    else if (weapon == hammer){
        if (shot_row >= 0 && shot_row < arenaHeight &&
            shot_col >= 0 && shot_col < arenaWidth){
            int row_diff = abs(shot_row - shooter_row);
            int col_diff = abs(shot_col - shooter_col);
            if (row_diff <= 1 && col_diff <= 1){
                affected_cells.push_back({shot_row, shot_col});
            }
        }
    }
    else if (weapon == grenade){
        if (robot->get_grenades() <= 0){
            std::cout << robot->m_name << " is out of grenades!" << std::endl;
            return;
        }
        robot->decrement_grenades();
        
        for (int r = shot_row - 1; r <= shot_row + 1; r++){
            for (int c = shot_col - 1; c <= shot_col + 1; c++){
                if (r >= 0 && r < arenaHeight && c >= 0 && c < arenaWidth){
                    affected_cells.push_back({r, c});
                }
            }
        }
    }
    
    bool hit_something = false;
    for (const auto& cell : affected_cells){
        RobotBase* target = findRobotAt(cell.first, cell.second);
        
        if (target != nullptr && target != robot){
            int damage = calculate_damage(weapon);
            
            int armor = target->get_armor();
            damage = damage * (100 - armor * 10) / 100;
            
            target->take_damage(damage);
            target->reduce_armor(1);
            
            std::cout << target->m_name << " takes " << damage 
                      << " damage. Health: " << target->get_health() << std::endl;
            
            hit_something = true;
        }
    }
    
    if (!hit_something){
        std::cout << "Shot missed!" << std::endl;
    }
}