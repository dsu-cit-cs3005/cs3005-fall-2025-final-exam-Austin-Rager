#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <iomanip>
#include "Arena.h"
#include "RobotBase.h"

int main(){
    srand(static_cast<unsigned>(time(nullptr)));

    Arena arena;
    arena.load_config("config.txt");
    arena.place_obstacles();
    arena.display();

    return 0;
}

