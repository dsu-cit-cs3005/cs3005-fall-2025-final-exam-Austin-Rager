#include "RobotBase.h"
#include <cmath>
#include <limits>

class Robot_Tank : public RobotBase
{
private:
    int target_row = -1;
    int target_col = -1;
    bool enemy_detected = false;
    int current_radar_dir = 1;
    const int hammer_range = 1; // Hammer only works in adjacent cells

    // Calculate Manhattan distance
    int manhattan_distance(int row1, int col1, int row2, int col2) const
    {
        return std::abs(row1 - row2) + std::abs(col1 - col2);
    }

    // Check if target is adjacent (hammer range)
    bool is_adjacent(int row1, int col1, int row2, int col2) const
    {
        int row_diff = std::abs(row1 - row2);
        int col_diff = std::abs(col1 - col2);
        return (row_diff <= 1 && col_diff <= 1 && (row_diff + col_diff) > 0);
    }

public:
    Robot_Tank() : RobotBase(2, 5, hammer)
    {
        m_name = "Tank";
        m_character = 'T';
    }

    // Rotate radar through all directions
    virtual void get_radar_direction(int& radar_direction) override
    {
        radar_direction = current_radar_dir;
        current_radar_dir = (current_radar_dir % 8) + 1;
    }

    // Find closest enemy and lock on
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        enemy_detected = false;
        int current_row, current_col;
        get_current_location(current_row, current_col);

        int min_distance = std::numeric_limits<int>::max();

        // Find the closest enemy
        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                int dist = manhattan_distance(current_row, current_col, obj.m_row, obj.m_col);
                if (dist < min_distance)
                {
                    min_distance = dist;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                    enemy_detected = true;
                }
            }
        }
    }

    // Hammer strike - only works on adjacent targets
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (enemy_detected)
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            // Only attack if enemy is adjacent
            if (is_adjacent(current_row, current_col, target_row, target_col))
            {
                shot_row = target_row;
                shot_col = target_col;
                return true;
            }
        }
        return false;
    }

    // Aggressive pursuit - move directly toward enemies
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        if (enemy_detected)
        {
            // Calculate direction to target
            int row_diff = target_row - current_row;
            int col_diff = target_col - current_col;

            // Move diagonally if possible for faster approach
            if (row_diff != 0 && col_diff != 0)
            {
                // Diagonal movement
                if (row_diff > 0 && col_diff > 0)
                    move_direction = 4; // Down-right
                else if (row_diff > 0 && col_diff < 0)
                    move_direction = 6; // Down-left
                else if (row_diff < 0 && col_diff > 0)
                    move_direction = 2; // Up-right
                else
                    move_direction = 8; // Up-left
            }
            else if (row_diff != 0)
            {
                // Vertical movement
                move_direction = (row_diff > 0) ? 5 : 1; // Down or Up
            }
            else if (col_diff != 0)
            {
                // Horizontal movement
                move_direction = (col_diff > 0) ? 3 : 7; // Right or Left
            }
            else
            {
                // Already at target position
                move_direction = 0;
                move_distance = 0;
                return;
            }

            // Move slowly but steadily (max speed is 2)
            move_distance = std::min(get_move_speed(),
                                    manhattan_distance(current_row, current_col, target_row, target_col));
        }
        else
        {
            // Patrol in a square pattern when no target
            if (current_col < m_board_col_max - 1)
            {
                move_direction = 3; // Right
                move_distance = 1;
            }
            else if (current_row < m_board_row_max - 1)
            {
                move_direction = 5; // Down
                move_distance = 1;
            }
            else
            {
                move_direction = 7; // Left
                move_distance = 1;
            }
        }
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Tank();
}
