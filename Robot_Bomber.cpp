#include "RobotBase.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

class Robot_Bomber : public RobotBase
{
private:
    int target_row = -1;
    int target_col = -1;
    bool has_target = false;
    int radar_sweep = 1; // Current radar direction
    const int grenade_range = 3; // Grenade effective range

    // Calculate Manhattan distance
    int manhattan_distance(int row1, int col1, int row2, int col2) const
    {
        return std::abs(row1 - row2) + std::abs(col1 - col2);
    }

public:
    Robot_Bomber() : RobotBase(4, 3, grenade)
    {
        m_name = "Bomber";
        m_character = 'B';
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    // Sweep radar in all directions
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = radar_sweep;
        radar_sweep = (radar_sweep % 8) + 1; // Cycle through 1-8
    }

    // Process radar to find targets
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        has_target = false;
        int current_row, current_col;
        get_current_location(current_row, current_col);

        int closest_dist = 999;

        // Find closest enemy
        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                int dist = manhattan_distance(current_row, current_col, obj.m_row, obj.m_col);
                if (dist < closest_dist)
                {
                    closest_dist = dist;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                    has_target = true;
                }
            }
        }
    }

    // Throw grenades at targets
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (has_target && get_grenades() > 0)
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            // Only shoot if within reasonable range
            if (manhattan_distance(current_row, current_col, target_row, target_col) <= grenade_range)
            {
                shot_row = target_row;
                shot_col = target_col;
                return true;
            }
        }
        return false;
    }

    // Hit-and-run movement: approach target, shoot, retreat
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        if (has_target)
        {
            int dist = manhattan_distance(current_row, current_col, target_row, target_col);

            // If too close and low on grenades, retreat
            if (dist <= 2 && get_grenades() < 5)
            {
                // Move away from target
                int row_diff = current_row - target_row;
                int col_diff = current_col - target_col;

                if (std::abs(row_diff) > std::abs(col_diff))
                {
                    move_direction = (row_diff > 0) ? 1 : 5; // Up or Down
                }
                else
                {
                    move_direction = (col_diff > 0) ? 3 : 7; // Right or Left
                }
                move_distance = 2;
            }
            else if (dist > grenade_range)
            {
                // Move toward target
                int row_diff = target_row - current_row;
                int col_diff = target_col - current_col;

                if (std::abs(row_diff) > std::abs(col_diff))
                {
                    move_direction = (row_diff > 0) ? 5 : 1; // Down or Up
                }
                else
                {
                    move_direction = (col_diff > 0) ? 3 : 7; // Right or Left
                }
                move_distance = std::min(2, get_move_speed());
            }
            else
            {
                // In optimal range, stay put
                move_direction = 0;
                move_distance = 0;
            }
        }
        else
        {
            // Random patrol
            move_direction = (std::rand() % 8) + 1;
            move_distance = std::rand() % get_move_speed() + 1;
        }
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Bomber();
}
