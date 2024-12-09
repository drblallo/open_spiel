
#include <utility>

#include "benchmark/benchmark.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include <random>

constexpr size_t minRange = 1 << 2;
constexpr size_t maxRange = 1 << 10;

template<typename Game, typename State>
static void genGame(std::shared_ptr<Game> game, std::vector<int64_t>& out, std::mt19937& gen) {
    State state(game);
    while (!state.IsTerminal()) {
        std::vector<int64_t> validActions = state.LegalActions();

        std::uniform_int_distribution<> distrib(0, validActions.size() - 1);
        out.push_back(validActions[distrib(gen)]);
        state.ApplyAction(out.back());
    }
}

template<typename Game, typename State>
static void runGame(std::shared_ptr<Game> game, std::vector<int64_t>& actions) {
    State state(game);
	for (auto action : actions)
        state.ApplyAction(action);
}

template<typename Game, typename State>
static void runBenchmark(benchmark::State& state)
{
    std::vector<std::vector<int64_t>> playOuts;
    open_spiel::GameParameters parameters;

    std::random_device rd;
    std::mt19937 gen(rd());

    auto game = std::make_shared<Game>(parameters);

	for (int i = 0; i < state.range(); i++)
    {
        playOuts.emplace_back();
		genGame<Game, State>(game, playOuts.back(), gen);
    }

    // actual benchmark body, apply the trace to a new game. 
	for (auto _ : state)
	{
		for (int i = 0; i < state.range(); i++)
			runGame<Game, State>(game, playOuts[i]);
	}
	state.SetComplexityN(state.range(0));
}

BENCHMARK(runBenchmark<GameType, StateType>)->Name(name)
		->RangeMultiplier(2)
		->Range(minRange, maxRange)
		->Complexity();

BENCHMARK_MAIN();
