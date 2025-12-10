#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <filesystem>
#include "RobotBase.h"

class Arena {
    protected:
    int arenaHeight;
    int arenaWidth;
    std::vector<std::vector<char>> grid;
    int mounds;
    int pits;
    int flamethrowers;
    int maxRound;
    int round;
    bool watch_live;
    int maxRobots;
    std::vector<RobotBase*> robots;
    std::vector<void*> robot_handles;
    std::vector<char> robot_characters;

    public:
    Arena();
    virtual ~Arena();
    void load_config(std::string fileName);
    void place_obstacles();
    void display();
    void load_all_robots();
    void cleanup();
    void run_game();

    private:
    bool cellEmpty(int& row, int& col);
    std::vector<std::string> find_robot_files();
    bool matches_robot_pattern(std::string fileName);
    std::string compileRobot(const std::string& fileName);
    RobotBase* loadRobot(const std::string& sharedLib);
    void setupRobot(RobotBase* robot, int index);
    RobotBase* findRobotAt(int row, int col);
    void process_robot_turn(RobotBase* robot);
    void get_radar_results(RobotBase* robot, int direction, std::vector<RadarObj>& results);
    void handle_shot(RobotBase* robot, int shot_row, int shot_col);
    void handle_movement(RobotBase* robot, int direction, int distance);
    int count_living_robots();
    void declare_winner();
    int calculate_damage(WeaponType weapon);
};