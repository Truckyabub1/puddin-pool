#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace puddin::core {

enum class GameVariant : uint8_t {
    EightBall = 0,
    NineBall = 1,
    StraightPool14_1 = 2,
    OnePocket = 3,
    ThreeCushionCarom = 4
};

enum class PlayerGroup : uint8_t {
    OpenTable = 0,
    Solids = 1,   // Balls 1-7
    Stripes = 2   // Balls 9-15
};

enum class FoulReason : uint8_t {
    None = 0,
    Scratch = 1,
    WrongBallFirst = 2,
    NoRailAfterContact = 3,
    BadBreak = 4,
    IllegalCarom = 5
};

enum class TurnResult : uint8_t {
    ContinueTurn = 0,
    SwitchTurn = 1,
    Win = 2,
    Loss = 3
};

struct ShotOutcome {
    int first_ball_hit{0};
    std::vector<int> pocketed_balls;
    int cushions_after_contact{0};
    bool was_scratch{false};
    int called_ball{0};
    int called_pocket{0};
    bool is_push_out{false};
    int cushions_before_second_carom{0}; // For 3-cushion carom
    bool hit_second_carom{false};
};

class GameRulesEngine {
public:
    explicit GameRulesEngine(GameVariant variant = GameVariant::EightBall);

    void reset();
    void set_variant(GameVariant variant);

    [[nodiscard]] GameVariant get_variant() const { return variant_; }
    [[nodiscard]] PlayerGroup get_player_group(int player_id) const;
    [[nodiscard]] int get_current_player() const { return current_player_; }
    [[nodiscard]] int get_player_score(int player_id) const;
    [[nodiscard]] int get_lowest_ball_on_table(const std::vector<bool>& active_balls) const;
    [[nodiscard]] bool is_table_open() const { return table_open_; }
    [[nodiscard]] bool needs_rerack_14_1() const { return needs_14_1_rerack_; }

    TurnResult process_shot(const ShotOutcome& outcome, const std::vector<bool>& active_balls);

private:
    TurnResult process_eight_ball(const ShotOutcome& outcome);
    TurnResult process_nine_ball(const ShotOutcome& outcome, const std::vector<bool>& active_balls);
    TurnResult process_straight_pool(const ShotOutcome& outcome);
    TurnResult process_one_pocket(const ShotOutcome& outcome);
    TurnResult process_carom(const ShotOutcome& outcome);

    GameVariant variant_{GameVariant::EightBall};
    int current_player_{1}; // 1 or 2
    int player_scores_[3]{0, 0, 0};

    // 8-Ball state
    bool table_open_{true};
    PlayerGroup player_groups_[3]{PlayerGroup::OpenTable, PlayerGroup::OpenTable, PlayerGroup::OpenTable};

    // 9-Ball state
    bool push_out_active_{false};

    // 14.1 state
    int pocketed_in_current_frame_{0};
    bool needs_14_1_rerack_{false};

    // One-Pocket state
    int player_assigned_pocket_[3]{0, 1, 3}; // P1 -> Pocket 1, P2 -> Pocket 3
};

} // namespace puddin::core
