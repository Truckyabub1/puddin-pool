#include "puddin/core/GameRules.h"
#include <algorithm>

namespace puddin::core {

GameRulesEngine::GameRulesEngine(GameVariant variant) : variant_(variant) {
    reset();
}

void GameRulesEngine::reset() {
    current_player_ = 1;
    player_scores_[0] = 0;
    player_scores_[1] = 0;
    player_scores_[2] = 0;

    table_open_ = true;
    player_groups_[1] = PlayerGroup::OpenTable;
    player_groups_[2] = PlayerGroup::OpenTable;

    push_out_active_ = false;
    pocketed_in_current_frame_ = 0;
    needs_14_1_rerack_ = false;
}

void GameRulesEngine::set_variant(GameVariant variant) {
    variant_ = variant;
    reset();
}

PlayerGroup GameRulesEngine::get_player_group(int player_id) const {
    if (player_id >= 1 && player_id <= 2) {
        return player_groups_[player_id];
    }
    return PlayerGroup::OpenTable;
}

int GameRulesEngine::get_player_score(int player_id) const {
    if (player_id >= 1 && player_id <= 2) {
        return player_scores_[player_id];
    }
    return 0;
}

int GameRulesEngine::get_lowest_ball_on_table(const std::vector<bool>& active_balls) const {
    for (size_t i = 1; i < active_balls.size(); ++i) {
        if (active_balls[i]) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

TurnResult GameRulesEngine::process_shot(const ShotOutcome& outcome, const std::vector<bool>& active_balls) {
    switch (variant_) {
        case GameVariant::EightBall:
            return process_eight_ball(outcome);
        case GameVariant::NineBall:
            return process_nine_ball(outcome, active_balls);
        case GameVariant::StraightPool14_1:
            return process_straight_pool(outcome);
        case GameVariant::OnePocket:
            return process_one_pocket(outcome);
        case GameVariant::ThreeCushionCarom:
            return process_carom(outcome);
    }
    return TurnResult::SwitchTurn;
}

TurnResult GameRulesEngine::process_eight_ball(const ShotOutcome& outcome) {
    bool has_8 = std::find(outcome.pocketed_balls.begin(), outcome.pocketed_balls.end(), 8) != outcome.pocketed_balls.end();

    // Scratch on 8-ball is loss of game
    if (outcome.was_scratch) {
        if (has_8) return TurnResult::Loss;
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    // 8-ball pocketed
    if (has_8) {
        PlayerGroup grp = player_groups_[current_player_];
        if (grp != PlayerGroup::OpenTable && outcome.called_ball == 8) {
            return TurnResult::Win; // Legally pocketed 8-ball
        }
        return TurnResult::Loss; // Pocketed early or without call
    }

    // Assign groups on open table if a called ball is legally pocketed
    if (table_open_ && outcome.called_ball > 0) {
        bool called_was_pocketed = std::find(
            outcome.pocketed_balls.begin(),
            outcome.pocketed_balls.end(),
            outcome.called_ball
        ) != outcome.pocketed_balls.end();

        if (called_was_pocketed) {
            if (outcome.called_ball >= 1 && outcome.called_ball <= 7) {
                player_groups_[current_player_] = PlayerGroup::Solids;
                player_groups_[3 - current_player_] = PlayerGroup::Stripes;
                table_open_ = false;
            } else if (outcome.called_ball >= 9 && outcome.called_ball <= 15) {
                player_groups_[current_player_] = PlayerGroup::Stripes;
                player_groups_[3 - current_player_] = PlayerGroup::Solids;
                table_open_ = false;
            }
        }
    }

    // Check legal contact
    bool legal_hit = true;
    if (!table_open_) {
        PlayerGroup grp = player_groups_[current_player_];
        if (grp == PlayerGroup::Solids && (outcome.first_ball_hit < 1 || outcome.first_ball_hit > 7)) {
            legal_hit = false;
        } else if (grp == PlayerGroup::Stripes && (outcome.first_ball_hit < 9 || outcome.first_ball_hit > 15)) {
            legal_hit = false;
        }
    }

    if (!legal_hit) {
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    // Continue turn if any assigned ball was pocketed
    bool pocketed_own = false;
    PlayerGroup grp = player_groups_[current_player_];
    for (int b : outcome.pocketed_balls) {
        if (grp == PlayerGroup::Solids && b >= 1 && b <= 7) pocketed_own = true;
        else if (grp == PlayerGroup::Stripes && b >= 9 && b <= 15) pocketed_own = true;
        else if (table_open_ && b != 0 && b != 8) pocketed_own = true;
    }

    if (pocketed_own) {
        return TurnResult::ContinueTurn;
    }

    current_player_ = (current_player_ == 1) ? 2 : 1;
    return TurnResult::SwitchTurn;
}

TurnResult GameRulesEngine::process_nine_ball(const ShotOutcome& outcome, const std::vector<bool>& active_balls) {
    if (outcome.was_scratch) {
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    int lowest = get_lowest_ball_on_table(active_balls);
    bool legal_first_contact = (outcome.first_ball_hit == lowest) || outcome.is_push_out;

    if (!legal_first_contact) {
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    // Check 9-ball win condition
    bool nine_pocketed = std::find(
        outcome.pocketed_balls.begin(),
        outcome.pocketed_balls.end(),
        9
    ) != outcome.pocketed_balls.end();

    if (nine_pocketed) {
        return TurnResult::Win; // 9-ball legally pocketed (on break or combo)
    }

    if (!outcome.pocketed_balls.empty()) {
        return TurnResult::ContinueTurn;
    }

    current_player_ = (current_player_ == 1) ? 2 : 1;
    return TurnResult::SwitchTurn;
}

TurnResult GameRulesEngine::process_straight_pool(const ShotOutcome& outcome) {
    if (outcome.was_scratch) {
        player_scores_[current_player_] -= 1; // 1 point penalty
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    int legal_points = 0;
    for (int b : outcome.pocketed_balls) {
        if (b > 0 && outcome.called_ball == b) {
            legal_points++;
        }
    }

    player_scores_[current_player_] += legal_points;
    pocketed_in_current_frame_ += legal_points;

    // Trigger re-rack when 14 balls are pocketed in frame
    if (pocketed_in_current_frame_ >= 14) {
        needs_14_1_rerack_ = true;
        pocketed_in_current_frame_ = 0;
    }

    if (player_scores_[current_player_] >= 50) {
        return TurnResult::Win;
    }

    if (legal_points > 0) {
        return TurnResult::ContinueTurn;
    }

    current_player_ = (current_player_ == 1) ? 2 : 1;
    return TurnResult::SwitchTurn;
}

TurnResult GameRulesEngine::process_one_pocket(const ShotOutcome& outcome) {
    if (outcome.was_scratch) {
        player_scores_[current_player_] -= 1;
        current_player_ = (current_player_ == 1) ? 2 : 1;
        return TurnResult::SwitchTurn;
    }

    int target_pocket = player_assigned_pocket_[current_player_];
    int opponent_pocket = player_assigned_pocket_[3 - current_player_];

    bool scored_own = false;
    for (int b : outcome.pocketed_balls) {
        if (outcome.called_pocket == target_pocket) {
            player_scores_[current_player_] += 1;
            scored_own = true;
        } else if (outcome.called_pocket == opponent_pocket) {
            player_scores_[3 - current_player_] += 1; // Opponent awarded point
        }
    }

    if (player_scores_[current_player_] >= 8) {
        return TurnResult::Win;
    }

    if (scored_own) {
        return TurnResult::ContinueTurn;
    }

    current_player_ = (current_player_ == 1) ? 2 : 1;
    return TurnResult::SwitchTurn;
}

TurnResult GameRulesEngine::process_carom(const ShotOutcome& outcome) {
    // 3-Cushion Carom requires at least 3 rail contacts before second carom
    if (outcome.hit_second_carom && outcome.cushions_before_second_carom >= 3) {
        player_scores_[current_player_] += 1;
        if (player_scores_[current_player_] >= 15) {
            return TurnResult::Win;
        }
        return TurnResult::ContinueTurn;
    }

    current_player_ = (current_player_ == 1) ? 2 : 1;
    return TurnResult::SwitchTurn;
}

} // namespace puddin::core
