// Copyright 2019 DeepMind Technologies Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OPEN_SPIEL_GAMES_TIC_TAC_TOE_H_
#define OPEN_SPIEL_GAMES_TIC_TAC_TOE_H_

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "open_spiel/spiel.h"

#define RLC_GET_TYPE_DECLS
#include "space_hulk/wrapper.h"

extern "C" {
#define RLC_GET_FUNCTION_DECLS
#include "space_hulk/wrapper.h"
}

#define RLC_GET_TYPE_DEFS
#include "space_hulk/wrapper.h"

// Simple game of Noughts and Crosses:
// https://en.wikipedia.org/wiki/Tic-tac-toe
//
// Parameters: none

namespace open_spiel {
namespace space_hulk {

// Constants.
inline constexpr int kNumPlayers = 2;
class SpaceHulkGame;

// State of an in-play game.
class SpaceHulkState : public State {
public:
  friend SpaceHulkGame;
  SpaceHulkState(std::shared_ptr<const Game> game);
  ~SpaceHulkState();

  SpaceHulkState(const SpaceHulkState &);
  SpaceHulkState &operator=(const SpaceHulkState &);

  bool IsChanceNode() const override { return false; }

  std::string Serialize() const override;

  Player CurrentPlayer() const override {
    if (IsTerminal()) {
      return kTerminalPlayerId;
    }
    if (IsChanceNode()) {
      return kChancePlayerId;
    }
    return get_current_player(const_cast<::Game &>(game));
  }
  std::string ActionToString(Player player, Action action_id) const override;
  std::string ToString() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::vector<double> Rewards() const override;
  std::string InformationStateString(Player player) const override;
  std::string ObservationString(Player player) const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  std::unique_ptr<State> Clone() const override;
  std::vector<Action> LegalActions() const override;

  const SpaceHulkGame *getGame() const {
    return reinterpret_cast<const SpaceHulkGame *>(game_.get());
  }
  bool canApplyAction(const AnyGameAction &action) const;

protected:
  ::Game game;
  void DoApplyAction(Action move) override;
  double previous_states_rewards = 0;
  double current_states_rewards = 0;

private:
  Player outcome_ = kInvalidPlayer;
};

// Game object.
class SpaceHulkGame : public Game {
public:
  explicit SpaceHulkGame(const GameParameters &params);
  int NumDistinctActions() const override { return actionsTable.size(); }
  std::unique_ptr<State> NewInitialState() const override {
    return std::unique_ptr<State>(new SpaceHulkState(shared_from_this()));
  }

  std::unique_ptr<State>
  DeserializeState(const std::string &str) const override;
  int NumPlayers() const override { return kNumPlayers; }
  double MinUtility() const override { return -1; }
  absl::optional<double> UtilitySum() const override { return 0; }
  double MaxUtility() const override { return 1; }
  std::vector<int> ObservationTensorShape() const override {
    return {game_size};
  }
  int MaxGameLength() const override { return 200000; }
  std::string ActionToString(Player player, Action action_id) const override;
  void asd();

  size_t game_size;
  std::vector<AnyGameAction> actionsTable;
};

} // namespace space_hulk
} // namespace open_spiel

#endif // OPEN_SPIEL_GAMES_TIC_TAC_TOE_H_
