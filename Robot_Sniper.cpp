#include "RobotBase.h"
#include <cmath>
#include <vector>
#include <algorithm>

class Robot_Sniper : public RobotBase
{
private:
    int target_row = -1;
    int target_col = -1;
    bool target_acquired = false;
    int radar_dir = 1;
    std::vector<std::pair<int, int>> safe_positions; // Positions away from enemies
    int turns_stationary = 0;

    // Calculate distance
    int distance(int row1, int col1, int row2, int col2) const
    {
        return std::abs(row1 - row2) + std::abs(col1 - col2);
    }

    // Find optimal sniping position (far from enemies)
    bool should_reposition(int current_row, int current_col) const
    {
        if (!target_acquired) return false;

        int dist = distance(current_row, current_col, target_row, target_col);

        // Reposition if too close (prefer long-range)
        return dist < 4;
    }

public:
    Robot_Sniper() : RobotBase(5, 2, railgun)
    {
        m_name = "Sniper";
        m_character = 'S';
    }

    // Systematic radar sweep
    virtual void get_radar_direction(int& radar_direction) override
    {
        if (target_acquired)
        {
            // Keep radar locked on target direction if we have one
            radar_direction = radar_dir;
        }
        else
        {
            // Sweep all directions
            radar_direction = radar_dir;
            radar_dir = (radar_dir % 8) + 1;
        }
    }

    // Identify targets and prioritize distant ones
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        target_acquired = false;
        int current_row, current_col;
        get_current_location(current_row, current_col);

        int max_distance = 0; // Prefer distant targets for railgun

        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                int dist = distance(current_row, current_col, obj.m_row, obj.m_col);

                // Prefer targets that are farther away (railgun advantage)
                if (dist > max_distance || !target_acquired)
                {
                    max_distance = dist;
                    target_row = obj.m_row;
                    target_col = obj.m_col;
                    target_acquired = true;
                }
            }
        }
    }

    // Railgun has infinite range in straight lines
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (target_acquired)
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            // Railgun shoots in straight lines (same row or column)
            if (current_row == target_row || current_col == target_col)
            {
                shot_row = target_row;
                shot_col = target_col;
                return true;
            }
        }
        return false;
    }

    // Kiting strategy: maintain distance and line up shots
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        if (target_acquired)
        {
            // Check if we can shoot from current position
            bool can_shoot = (current_row == target_row || current_col == target_col);

            if (can_shoot && !should_reposition(current_row, current_col))
            {
                // Perfect position - don't move
                move_direction = 0;
                move_distance = 0;
                turns_stationary++;
            }
            else
            {
                turns_stationary = 0;

                // Move to align for a shot
                int row_diff = target_row - current_row;
                int col_diff = target_col - current_col;

                // If too close, create distance first
                if (should_reposition(current_row, current_col))
                {
                    // Move away from target
                    if (std::abs(row_diff) > std::abs(col_diff))
                    {
                        move_direction = (row_diff > 0) ? 1 : 5; // Move away vertically
                    }
                    else
                    {
                        move_direction = (col_diff > 0) ? 7 : 3; // Move away horizontally
                    }
                    move_distance = get_move_speed(); // Use full speed to escape
                }
                else
                {
                    // Align for shot (prioritize column alignment for railgun)
                    if (current_col != target_col)
                    {
                        // Align column
                        move_direction = (col_diff > 0) ? 3 : 7; // Right or Left
                        move_distance = std::min(get_move_speed(), std::abs(col_diff));
                    }
                    else if (current_row != target_row)
                    {
                        // Align row
                        move_direction = (row_diff > 0) ? 5 : 1; // Down or Up
                        move_distance = std::min(get_move_speed(), std::abs(row_diff));
                    }
                }
            }
        }
        else
        {
            // No target - move to edges for better sniping positions
            turns_stationary = 0;

            if (current_row > m_board_row_max / 2)
            {
                move_direction = 5; // Move down to edge
                move_distance = 2;
            }
            else
            {
                move_direction = 1; // Move up to edge
                move_distance = 2;
            }
        }
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Sniper();
}
