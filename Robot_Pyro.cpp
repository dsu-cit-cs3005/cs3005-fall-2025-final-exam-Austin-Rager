#include "RobotBase.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>

class Robot_Pyro : public RobotBase
{
private:
    int primary_target_row = -1;
    int primary_target_col = -1;
    bool target_locked = false;
    int radar_focus = 1;
    int aggression_level = 0; // Increases when enemies are nearby
    const int flame_range = 4;

    std::vector<std::pair<int, int>> enemy_positions; // Track multiple enemies

    // Distance calculation
    int calc_distance(int row1, int col1, int row2, int col2) const
    {
        return std::abs(row1 - row2) + std::abs(col1 - col2);
    }

    // Find the weakest or closest target
    void select_best_target(int current_row, int current_col)
    {
        if (enemy_positions.empty())
        {
            target_locked = false;
            return;
        }

        // Select closest enemy within flame range
        int min_dist = 999;
        target_locked = false;

        for (const auto& pos : enemy_positions)
        {
            int dist = calc_distance(current_row, current_col, pos.first, pos.second);
            if (dist <= flame_range && dist < min_dist)
            {
                min_dist = dist;
                primary_target_row = pos.first;
                primary_target_col = pos.second;
                target_locked = true;
            }
        }

        // If no one in range, target the closest
        if (!target_locked && !enemy_positions.empty())
        {
            min_dist = 999;
            for (const auto& pos : enemy_positions)
            {
                int dist = calc_distance(current_row, current_col, pos.first, pos.second);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    primary_target_row = pos.first;
                    primary_target_col = pos.second;
                    target_locked = true;
                }
            }
        }
    }

public:
    Robot_Pyro() : RobotBase(3, 4, flamethrower)
    {
        m_name = "Pyro";
        m_character = 'P';
        std::srand(static_cast<unsigned int>(std::time(nullptr)) + 12345); // Different seed
    }

    // Aggressive radar scanning
    virtual void get_radar_direction(int& radar_direction) override
    {
        if (target_locked && aggression_level > 2)
        {
            // Keep radar locked when highly aggressive
            radar_direction = radar_focus;
        }
        else
        {
            // Scan in all directions
            radar_direction = radar_focus;
            radar_focus = (radar_focus % 8) + 1;
        }
    }

    // Track all enemies and select target
    virtual void process_radar_results(const std::vector<RadarObj>& radar_results) override
    {
        enemy_positions.clear();
        int current_row, current_col;
        get_current_location(current_row, current_col);

        // Collect all enemy positions
        for (const auto& obj : radar_results)
        {
            if (obj.m_type == 'R')
            {
                enemy_positions.push_back({obj.m_row, obj.m_col});
            }
        }

        // Update aggression based on enemy proximity
        aggression_level = 0;
        for (const auto& pos : enemy_positions)
        {
            int dist = calc_distance(current_row, current_col, pos.first, pos.second);
            if (dist <= 3) aggression_level += 2;
            else if (dist <= 6) aggression_level += 1;
        }

        // Select the best target
        select_best_target(current_row, current_col);
    }

    // Flamethrower attack
    virtual bool get_shot_location(int& shot_row, int& shot_col) override
    {
        if (target_locked)
        {
            int current_row, current_col;
            get_current_location(current_row, current_col);

            int dist = calc_distance(current_row, current_col, primary_target_row, primary_target_col);

            // Flamethrower effective within range
            if (dist <= flame_range)
            {
                shot_row = primary_target_row;
                shot_col = primary_target_col;
                return true;
            }
        }
        return false;
    }

    // Aggressive closing behavior
    virtual void get_move_direction(int& move_direction, int& move_distance) override
    {
        int current_row, current_col;
        get_current_location(current_row, current_col);

        if (target_locked)
        {
            int dist = calc_distance(current_row, current_col, primary_target_row, primary_target_col);

            // Optimal flamethrower range is 2-3 cells
            if (dist > 3)
            {
                // Close the distance aggressively
                int row_diff = primary_target_row - current_row;
                int col_diff = primary_target_col - current_col;

                // Move diagonally when possible for speed
                if (row_diff != 0 && col_diff != 0)
                {
                    if (row_diff > 0 && col_diff > 0)
                        move_direction = 4; // Down-right
                    else if (row_diff > 0 && col_diff < 0)
                        move_direction = 6; // Down-left
                    else if (row_diff < 0 && col_diff > 0)
                        move_direction = 2; // Up-right
                    else
                        move_direction = 8; // Up-left
                    move_distance = get_move_speed();
                }
                else if (std::abs(row_diff) > std::abs(col_diff))
                {
                    move_direction = (row_diff > 0) ? 5 : 1; // Down or Up
                    move_distance = get_move_speed();
                }
                else
                {
                    move_direction = (col_diff > 0) ? 3 : 7; // Right or Left
                    move_distance = get_move_speed();
                }
            }
            else if (dist < 2)
            {
                // Too close - back off slightly for better angle
                int row_diff = current_row - primary_target_row;
                int col_diff = current_col - primary_target_col;

                if (std::abs(row_diff) > std::abs(col_diff))
                {
                    move_direction = (row_diff > 0) ? 5 : 1;
                }
                else
                {
                    move_direction = (col_diff > 0) ? 3 : 7;
                }
                move_distance = 1;
            }
            else
            {
                // Perfect range - circle strafe
                int row_diff = primary_target_row - current_row;
                int col_diff = primary_target_col - current_col;

                // Strafe perpendicular to target
                if (std::abs(row_diff) > std::abs(col_diff))
                {
                    move_direction = (std::rand() % 2 == 0) ? 3 : 7; // Left or Right
                }
                else
                {
                    move_direction = (std::rand() % 2 == 0) ? 1 : 5; // Up or Down
                }
                move_distance = 1;
            }
        }
        else
        {
            // Hunt mode - aggressive patrol
            if (current_row < m_board_row_max / 2)
            {
                move_direction = 4; // Down-right diagonal
            }
            else
            {
                move_direction = 8; // Up-left diagonal
            }
            move_distance = 2;
        }
    }
};

// Factory function
extern "C" RobotBase* create_robot()
{
    return new Robot_Pyro();
}
