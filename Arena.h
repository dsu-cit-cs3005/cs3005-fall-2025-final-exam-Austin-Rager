#pragma once
#include <iostream>
#include <vector>
#include <string>
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
    std::vector<RobotBase> robots;

    public:
    Arena();
    void load_config(std::string fileName);
    void place_obstacles();
    void display();

    private:
    bool cellEmpty(int& row, int& col);
};