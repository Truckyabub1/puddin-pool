add_executable(sanity_test
    sanity_test.cpp
)

target_link_libraries(sanity_test
    PRIVATE
        puddinpool
        puddin_core
        puddin_view
)

target_compile_features(sanity_test PRIVATE cxx_std_20)

add_executable(stress_and_profiling_test
    stress_and_profiling_test.cpp
)

target_link_libraries(stress_and_profiling_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(stress_and_profiling_test PRIVATE cxx_std_20)

add_executable(coach_puddin_test
    coach_puddin_test.cpp
)

target_link_libraries(coach_puddin_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(coach_puddin_test PRIVATE cxx_std_20)

add_executable(game_rules_and_legends_test
    game_rules_and_legends_test.cpp
)

target_link_libraries(game_rules_and_legends_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(game_rules_and_legends_test PRIVATE cxx_std_20)

add_executable(nine_ball_rules_test
    nine_ball_rules_test.cpp
)

target_link_libraries(nine_ball_rules_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(nine_ball_rules_test PRIVATE cxx_std_20)

add_executable(official_rules_test
    official_rules_test.cpp
)

target_link_libraries(official_rules_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(official_rules_test PRIVATE cxx_std_20)

add_executable(physics_dynamics_test
    physics_dynamics_test.cpp
)

target_link_libraries(physics_dynamics_test
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(physics_dynamics_test PRIVATE cxx_std_20)

add_executable(test_impact_reactions
    "${CMAKE_SOURCE_DIR}/core/tests/TestImpactReactions.cpp"
)

target_link_libraries(test_impact_reactions
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(test_impact_reactions PRIVATE cxx_std_20)

add_executable(test_friction_decay
    "${CMAKE_SOURCE_DIR}/core/tests/TestFrictionDecay.cpp"
)

target_link_libraries(test_friction_decay
    PRIVATE
        puddinpool
        puddin_core
)

target_compile_features(test_friction_decay PRIVATE cxx_std_20)

add_test(NAME SanityTest COMMAND sanity_test)
add_test(NAME StressAndProfilingTest COMMAND stress_and_profiling_test)
add_test(NAME CoachPuddinTest COMMAND coach_puddin_test)
add_test(NAME GameRulesAndLegendsTest COMMAND game_rules_and_legends_test)
add_test(NAME NineBallRulesTest COMMAND nine_ball_rules_test)
add_test(NAME OfficialRulesTest COMMAND official_rules_test)
add_test(NAME PhysicsDynamicsTest COMMAND physics_dynamics_test)
add_test(NAME TestImpactReactions COMMAND test_impact_reactions)
add_test(NAME TestFrictionDecay COMMAND test_friction_decay)


