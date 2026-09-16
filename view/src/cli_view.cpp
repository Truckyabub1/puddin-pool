#include "puddin/view/cli_view.hpp"
#include <sstream>
#include <vector>
#include <cmath>

namespace puddin::view {

CliView::CliView(int terminal_width, int terminal_height)
    : width_(terminal_width), height_(terminal_height) {}

std::string CliView::render_to_string(
    const core::Table& table,
    const std::vector<core::Ball>& balls,
    const std::vector<core::Vec2>& preview_points,
    core::Vec2 ghost_ball
) const {
    std::vector<std::string> grid(height_, std::string(width_, ' '));

    // Draw borders (cushions)
    for (int x = 0; x < width_; ++x) {
        grid[0][x] = '-';
        grid[height_ - 1][x] = '-';
    }
    for (int y = 0; y < height_; ++y) {
        grid[y][0] = '|';
        grid[y][width_ - 1] = '|';
    }
    // Corners and pockets
    grid[0][0] = '+';
    grid[0][width_ - 1] = '+';
    grid[height_ - 1][0] = '+';
    grid[height_ - 1][width_ - 1] = '+';
    // Middle pockets
    grid[0][width_ / 2] = 'U';
    grid[height_ - 1][width_ / 2] = 'n';

    const int inner_w = width_ - 2;
    const int inner_h = height_ - 2;

    auto to_grid = [&](double x, double y) -> std::pair<int, int> {
        int gx = 1 + static_cast<int>((x / table.width) * (inner_w - 1));
        int gy = 1 + static_cast<int>((y / table.height) * (inner_h - 1));
        return {gx, gy};
    };

    // 1. Draw preview line segments
    if (preview_points.size() >= 2) {
        for (size_t i = 0; i + 1 < preview_points.size(); ++i) {
            auto [x0, y0] = to_grid(preview_points[i].x, preview_points[i].y);
            auto [x1, y1] = to_grid(preview_points[i + 1].x, preview_points[i + 1].y);

            int dx = std::abs(x1 - x0);
            int dy = -std::abs(y1 - y0);
            int sx = x0 < x1 ? 1 : -1;
            int sy = y0 < y1 ? 1 : -1;
            int err = dx + dy;
            int cx = x0;
            int cy = y0;

            while (true) {
                if (cx >= 1 && cx < width_ - 1 && cy >= 1 && cy < height_ - 1) {
                    if (grid[cy][cx] == ' ') {
                        grid[cy][cx] = '.';
                    }
                }
                if (cx == x1 && cy == y1) break;
                int e2 = 2 * err;
                if (e2 >= dy) { err += dy; cx += sx; }
                if (e2 <= dx) { err += dx; cy += sy; }
            }
        }
    }

    // 2. Draw ghost ball if provided
    if (ghost_ball.x > 0.0 && ghost_ball.y > 0.0) {
        auto [gx, gy] = to_grid(ghost_ball.x, ghost_ball.y);
        if (gx >= 1 && gx < width_ - 1 && gy >= 1 && gy < height_ - 1) {
            grid[gy][gx] = 'G'; // Ghost ball marker
        }
    }

    // 3. Map real balls onto grid
    for (const auto& b : balls) {
        if (b.is_sunk) continue;

        auto [gx, gy] = to_grid(b.position.x, b.position.y);
        if (gx >= 1 && gx < width_ - 1 && gy >= 1 && gy < height_ - 1) {
            if (b.id == 1) {
                grid[gy][gx] = 'C'; // Cue ball
            } else {
                grid[gy][gx] = static_cast<char>('0' + (b.id % 10));
            }
        }
    }

    std::ostringstream oss;
    for (const auto& line : grid) {
        oss << line << "\n";
    }
    return oss.str();
}

void CliView::render(
    const core::Table& table,
    const std::vector<core::Ball>& balls,
    const std::vector<core::Vec2>& preview_points,
    core::Vec2 ghost_ball,
    std::ostream& out
) const {
    out << render_to_string(table, balls, preview_points, ghost_ball);
}

} // namespace puddin::view
