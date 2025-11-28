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

    private:
    bool cellEmpty(int& row, int& col);
    std::vector<std::string> find_robot_files();
    bool matches_robot_pattern(std::string fileName);
    std::string compileRobot(const std::string& fileName);
    RobotBase* loadRobot(const std::string& sharedLib);
    void setupRobot(RobotBase* robot, int index);
};