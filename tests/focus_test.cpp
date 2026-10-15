#include "test.hpp"
#include "core/focus.hpp"
#include <limits>
using namespace HyprCore;
TEST(directional_focus_selects_forward_centers_on_each_axis) {
    const std::vector<FocusTarget> targets{
        {1, {100, 100, 80, 80}}, {2, {0, 100, 80, 80}},
        {3, {200, 100, 80, 80}}, {4, {100, 0, 80, 80}},
        {5, {100, 200, 80, 80}}, {6, {400, 100, 80, 80}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Left) == 2);
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    CHECK(directionalFocus(targets, 1, FocusDirection::Up) == 4);
    CHECK(directionalFocus(targets, 1, FocusDirection::Down) == 5);
    CHECK(!directionalFocus(targets, 99, FocusDirection::Right));
    CHECK(!directionalFocus({}, 1, FocusDirection::Right));
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}}, 1, FocusDirection::Left));
}
TEST(directional_focus_prefers_overlapping_lanes_before_nearer_diagonals) {
    const std::vector<FocusTarget> right{
        {1, {0, 0, 100, 100}}, {2, {110, 100, 100, 100}}, {3, {400, 99, 100, 100}}};
    CHECK(directionalFocus(right, 1, FocusDirection::Right) == 3);
    const std::vector<FocusTarget> down{
        {1, {0, 0, 100, 100}}, {2, {100, 110, 100, 100}}, {3, {99, 400, 100, 100}}};
    CHECK(directionalFocus(down, 1, FocusDirection::Down) == 3);
    CHECK(directionalFocus({{1, {0, 0, 100, 100}}, {2, {150, 200, 100, 100}},
                            {3, {300, 150, 100, 100}}}, 1, FocusDirection::Right) == 2);
}
TEST(directional_focus_ties_are_stable_under_input_permutations) {
    std::vector<FocusTarget> targets{
        {1, {0, 0, 100, 100}}, {7, {100, -10, 100, 100}}, {3, {100, 10, 100, 100}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    std::reverse(targets.begin(), targets.end());
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 3);
    targets.push_back({9, {100, 0, 100, 100}});
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 9);
}
TEST(focus_eligibility_excludes_hidden_unmapped_unfocusable_and_degenerate_targets) {
    FocusTarget target{2, {100, 0, 50, 50}};
    CHECK(focusEligible(target));
    target.focusable = false; CHECK(!focusEligible(target)); target.focusable = true;
    target.mapped = false; CHECK(!focusEligible(target)); target.mapped = true;
    target.hidden = true; CHECK(!focusEligible(target)); target.hidden = false;
    target.id = 0; CHECK(!focusEligible(target)); target.id = 2;
    target.bounds.width = 0; CHECK(!focusEligible(target));
    target.bounds.width = -1; CHECK(!focusEligible(target)); target.bounds.width = 50;
    target.bounds.height = 0; CHECK(!focusEligible(target));
    target.bounds.height = -1; CHECK(!focusEligible(target));
    const std::vector<FocusTarget> targets{
        {1, {0, 0, 50, 50}}, {2, {60, 0, 50, 50}, false},
        {3, {70, 0, 50, 50}, true, false}, {4, {80, 0, 50, 50}, true, true, true},
        {0, {90, 0, 50, 50}}, {5, {100, 0, 0, 50}}, {6, {110, 0, 50, 50}}};
    CHECK(directionalFocus(targets, 1, FocusDirection::Right) == 6);
    CHECK(!directionalFocus(targets, 2, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 3, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 4, FocusDirection::Right));
    CHECK(!directionalFocus(targets, 5, FocusDirection::Right));
}
TEST(directional_wrap_is_opt_in_and_chooses_the_opposite_edge_only_when_needed) {
    const std::vector<FocusTarget> targets{
        {1, {100, 100, 50, 50}}, {2, {0, 100, 50, 50}}, {3, {-100, 100, 50, 50}},
        {4, {100, 0, 50, 50}}, {5, {100, -100, 50, 50}}};
    CHECK(!directionalFocus(targets, 1, FocusDirection::Right));
    CHECK(directionalFocus(targets, 1, FocusDirection::Right, true) == 3);
    CHECK(directionalFocus(targets, 1, FocusDirection::Down, true) == 5);
    CHECK(directionalFocus(targets, 3, FocusDirection::Left, true) == 1);
    CHECK(directionalFocus(targets, 5, FocusDirection::Up, true) == 1);
    CHECK(directionalFocus(targets, 1, FocusDirection::Left, true) == 2);
    CHECK(directionalFocus(targets, 1, FocusDirection::Up, true) == 4);
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}}, 1, FocusDirection::Right, true));
    CHECK(!directionalFocus({{1, {0, 0, 10, 10}}, {2, {0, 100, 10, 10}}},
                             1, FocusDirection::Right, true));
    CHECK(directionalFocus({{1, {100, 100, 50, 50}}, {2, {-500, 0, 50, 50}},
                            {3, {0, 100, 50, 50}}}, 1, FocusDirection::Right, true) == 3);
}
TEST(focus_cycles_preserve_input_order_filter_eligibility_and_support_reverse) {
    const std::vector<FocusTarget> targets{
        {30, {0, 0, 10, 10}}, {10, {10, 0, 10, 10}, false},
        {20, {20, 0, 10, 10}}, {40, {30, 0, 10, 10}, true, false},
        {50, {40, 0, 10, 10}, true, true, true}, {60, {50, 0, 10, 10}}};
    CHECK(cycleFocus(targets) == 30);
    CHECK(cycleFocus(targets, std::nullopt, true) == 60);
    CHECK(cycleFocus(targets, 30) == 20);
    CHECK(cycleFocus(targets, 20) == 60);
    CHECK(cycleFocus(targets, 60) == 30);
    CHECK(cycleFocus(targets, 30, true) == 60);
    CHECK(cycleFocus(targets, 60, true) == 20);
    CHECK(!cycleFocus(targets, 60, false, false));
    CHECK(!cycleFocus(targets, 30, true, false));
    CHECK(cycleFocus(targets, 10) == 30);
    CHECK(cycleFocus(targets, 999, true, false) == 60);
    CHECK(!cycleFocus({}));
    CHECK(!cycleFocus({{1, {0, 0, 0, 10}}}));
    CHECK(cycleFocus({{1, {0, 0, 10, 10}}}, 1) == 1);
    CHECK(!cycleFocus({{1, {0, 0, 10, 10}}}, 1, false, false));
}
TEST(focus_history_deduplicates_recency_and_falls_back_only_to_eligible_survivors) {
    FocusHistory history;
    CHECK(history.entries().empty());
    CHECK(!history.remember(0));
    CHECK(history.remember(10)); CHECK(history.remember(20)); CHECK(history.remember(30));
    CHECK(history.remember(10));
    CHECK(history.entries() == std::vector<FocusWindowId>({10, 30, 20}));
    const std::vector<FocusTarget> targets{
        {40, {0, 0, 10, 10}}, {10, {10, 0, 10, 10}, false},
        {20, {20, 0, 10, 10}}, {30, {30, 0, 10, 10}, true, true, true}};
    CHECK(history.fallback(targets) == 20);
    CHECK(history.fallback(targets, 20) == 40);
    CHECK(history.entries() == std::vector<FocusWindowId>({10, 30, 20}));
    CHECK(history.forget(20)); CHECK(!history.forget(20)); CHECK(!history.forget(0));
    CHECK(history.fallback(targets) == 40);
    CHECK(history.fallback(targets, 40) == 20);
    CHECK(!history.fallback({}));
    CHECK(!history.fallback({{10, {0, 0, 10, 10}, false}}));
    CHECK(!history.fallback({{40, {0, 0, 10, 10}}}, 40));
    CHECK(history.remember(20));
    CHECK(history.entries() == std::vector<FocusWindowId>({20, 10, 30}));
}
TEST(pointer_focus_uses_topmost_eligible_half_open_bounds) {
    const std::vector<FocusTarget> targets{
        {1, {-10, -10, 30, 30}}, {2, {0, 0, 10, 10}},
        {3, {0, 0, 10, 10}, false}, {4, {0, 0, 10, 10}, true, false},
        {5, {0, 0, 10, 10}, true, true, true}, {0, {0, 0, 10, 10}}};
    CHECK(focusAtPoint(targets, 0, 0) == 2);
    CHECK(focusAtPoint(targets, 9, 9) == 2);
    CHECK(focusAtPoint(targets, 10, 10) == 1);
    CHECK(focusAtPoint(targets, -10, -10) == 1);
    CHECK(!focusAtPoint(targets, 20, 20));
    CHECK(!focusAtPoint(targets, -11, 0));
    CHECK(!focusAtPoint({}, 0, 0));
    CHECK(!focusAtPoint({{1, {0, 0, 0, 10}}}, 0, 0));
}
TEST(duplicate_focus_ids_use_the_first_snapshot_without_geometry_or_flag_bypasses) {
    const std::vector<FocusTarget> geometry{
        {1, {0, 0, 10, 10}}, {2, {500, 0, 10, 10}},
        {2, {10, 0, 10, 10}}, {3, {100, 0, 10, 10}}};
    CHECK(directionalFocus(geometry, 1, FocusDirection::Right) == 3);
    const std::vector<FocusTarget> flags{
        {1, {0, 0, 100, 100}}, {2, {10, 0, 100, 100}, false}, {2, {10, 0, 100, 100}}};
    CHECK(!directionalFocus(flags, 1, FocusDirection::Right));
    CHECK(!directionalFocus(flags, 2, FocusDirection::Right, true));
    CHECK(focusAtPoint(flags, 20, 20) == 1);
    CHECK(cycleFocus(flags, 1) == 1);
    FocusHistory history;
    history.remember(2);
    CHECK(history.fallback(flags) == 1);
    CHECK(!history.fallback(flags, 1));
}
TEST(focus_cycles_do_not_get_stuck_on_duplicate_occurrences) {
    const std::vector<FocusTarget> targets{
        {1, {0, 0, 10, 10}}, {1, {10, 0, 10, 10}}, {2, {20, 0, 10, 10}}};
    CHECK(cycleFocus(targets, 1) == 2);
    CHECK(cycleFocus(targets, 2, true) == 1);
    CHECK(!focusAtPoint(targets, 15, 5));
}
TEST(focus_geometry_handles_extreme_coordinates_half_centers_and_invalid_directions) {
    const auto low = std::numeric_limits<int>::min();
    const auto high = std::numeric_limits<int>::max();
    const std::vector<FocusTarget> extremes{
        {1, {low, low, high, high}}, {2, {high, low, high, high}},
        {3, {low, high, high, high}}, {4, {high, high, high, high}}};
    CHECK(directionalFocus(extremes, 1, FocusDirection::Right) == 2);
    CHECK(directionalFocus(extremes, 2, FocusDirection::Left) == 1);
    CHECK(directionalFocus(extremes, 1, FocusDirection::Down) == 3);
    CHECK(directionalFocus(extremes, 4, FocusDirection::Up) == 2);
    CHECK(directionalFocus(extremes, 1, FocusDirection::Left, true) == 2);
    CHECK(focusAtPoint(extremes, high, high) == 4);
    CHECK(focusAtPoint(extremes, -2, -2) == 1);
    CHECK(!focusAtPoint(extremes, -1, -1));
    CHECK(!directionalFocus(extremes, 1, static_cast<FocusDirection>(99), true));
    CHECK(directionalFocus({{1, {0, 0, 1, 1}}, {2, {0, 0, 2, 1}}},
                            1, FocusDirection::Right) == 2);
    CHECK(!directionalFocus({{1, {0, 0, 2, 2}}, {2, {-1, -1, 4, 4}}},
                             1, FocusDirection::Right, true));
}
TEST(directional_policies_are_invariant_under_translation_reflection_and_permutation) {
    std::uint32_t seed = 0x71c3u;
    const auto random = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };
    for (int layout = 0; layout < 100; ++layout) {
        std::vector<FocusTarget> targets;
        for (FocusWindowId id = 1; id <= 12; ++id) {
            const auto x = static_cast<int>(random() % 801u) - 400;
            const auto y = static_cast<int>(random() % 801u) - 400;
            const auto width = static_cast<int>(random() % 100u) + 1;
            const auto height = static_cast<int>(random() % 100u) + 1;
            targets.push_back({id, {x, y, width, height}});
        }
        for (const auto direction : {FocusDirection::Left, FocusDirection::Right,
                                     FocusDirection::Up, FocusDirection::Down}) {
            for (const bool wrap : {false, true}) {
                const auto expected = directionalFocus(targets, 1, direction, wrap);
                auto reversed = targets;
                std::reverse(reversed.begin(), reversed.end());
                CHECK(directionalFocus(reversed, 1, direction, wrap) == expected);
                auto translated = targets;
                for (auto& target : translated) { target.bounds.x += 3000; target.bounds.y -= 2000; }
                CHECK(directionalFocus(translated, 1, direction, wrap) == expected);
                auto reflected = targets;
                for (auto& target : reflected) {
                    target.bounds.x = -target.bounds.x - target.bounds.width;
                    target.bounds.y = -target.bounds.y - target.bounds.height;
                }
                const auto opposite = direction == FocusDirection::Left ? FocusDirection::Right :
                    direction == FocusDirection::Right ? FocusDirection::Left :
                    direction == FocusDirection::Up ? FocusDirection::Down : FocusDirection::Up;
                CHECK(directionalFocus(reflected, 1, opposite, wrap) == expected);
            }
        }
    }
}
TEST_MAIN()
