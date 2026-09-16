#pragma once

#include "puddin/core/types.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace puddin::view {

class CliView {
public:
    explicit CliView(int terminal_width = 70, int terminal_height = 20);

    std::string render_to_string(
        const core::Table& table,
        const std::vector<core::Ball>& balls,
        const std::vector<core::Vec2>& preview_points = {},
        core::Vec2 ghost_ball = {0.0, 0.0}
    ) const;

    void render(
        const core::Table& table,
        const std::vector<core::Ball>& balls,
        const std::vector<core::Vec2>& preview_points = {},
        core::Vec2 ghost_ball = {0.0, 0.0},
        std::ostream& out = std::cout
    ) const;

private:
    int width_;
    int height_;
};

} // namespace puddin::view
