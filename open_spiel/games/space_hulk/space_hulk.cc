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

#include "open_spiel/games/space_hulk/space_hulk.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "open_spiel/spiel_utils.h"
#include "open_spiel/utils/tensor_view.h"

namespace open_spiel {
namespace space_hulk {
namespace {

// Facts about the game.
const GameType kGameType{
    /*short_name=*/"space_hulk",
    /*long_name=*/"Tic Tac Toe",
    GameType::Dynamics::kSequential,
    GameType::ChanceMode::kSampledStochastic,
    GameType::Information::kImperfectInformation,
    GameType::Utility::kZeroSum,
    GameType::RewardModel::kRewards,
    /*max_num_players=*/2,
    /*min_num_players=*/2,
    /*provides_information_state_string=*/true,
    /*provides_information_state_tensor=*/false,
    /*provides_observation_string=*/true,
    /*provides_observation_tensor=*/true,
    /*parameter_specification=*/{}  // no parameters
};

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new SpaceHulkGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);

RegisterSingleTensorObserver single_tensor(kGameType.short_name);

}  // namespace

bool SpaceHulkState::canApplyAction(const AnyGameAction &action) const {
    uint8_t result;
    rl_can_apply_impl__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit_Game_r_bool(&result, const_cast<AnyGameAction*>(&action), const_cast<::Game*>(&game));

    return result != 0;
}

void SpaceHulkState::DoApplyAction(Action move) {

  SPIEL_CHECK_GE(move , 0);
  SPIEL_CHECK_GT(getGame()->actionsTable.size(),  move);
  SPIEL_CHECK_TRUE(canApplyAction(getGame()->actionsTable[move]));
  auto* action = &getGame()->actionsTable[move];
  rl_apply__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit_Game(const_cast<AnyGameAction*>(action), const_cast<::Game*>(&game));
}

std::vector<Action> SpaceHulkState::LegalActions() const {
  if (IsTerminal()) return {};
  // Can move in any empty cell.
  std::vector<Action> moves;
  for (int action_index = 0; action_index < getGame()->actionsTable.size(); ++action_index) {
    if (canApplyAction(getGame()->actionsTable[action_index])) {
      moves.push_back(action_index);
    }
  }
  return moves;
}

std::string SpaceHulkState::ActionToString(Player player,
                                           Action action_id) const {
  return game_->ActionToString(player, action_id);
}


SpaceHulkState::SpaceHulkState(std::shared_ptr<const Game> game) : State(game) {
    rl_play__r_Game(&this->game);
}
SpaceHulkState::~SpaceHulkState(){ 
    rl_m_drop__Game(&this->game);
}

std::string SpaceHulkState::ToString() const {
  ::String rl_string;
  rl_to_string__Game_r_String(&rl_string, const_cast<::Game*>(&game));
  std::string str((char*)rl_string._data._data);
  rl_m_drop__String(&rl_string);
  return str;
}

SpaceHulkState::SpaceHulkState(const SpaceHulkState & other) : State(other){
    rl_play__r_Game(&this->game);
    rl_m_assign__Game_Game(&game, const_cast<::Game*>(&other.game));
}
  
SpaceHulkState& SpaceHulkState::operator=(const SpaceHulkState & Other)
{
    if (this == &Other)
        return *this;
    rl_m_assign__Game_Game(&game, const_cast<::Game*>(&Other.game));
    return *this;
}

std::string SpaceHulkState::Serialize() const {
    return ToString();
}

std::unique_ptr<State> SpaceHulkGame::DeserializeState(const std::string& str) const {
    auto state = NewInitialState();
    auto* casted = reinterpret_cast<SpaceHulkState*>(state.get());
    String rl_string;
    rl_string._data._data = reinterpret_cast<int8_t*>(const_cast<char*>(&str.front()));
    rl_string._data._capacity = str.capacity();
    rl_string._data._size = str.size();
    uint8_t result;
    rl_from_string__Game_String_r_bool(&result, &casted->game, &rl_string);
    return state;
}

bool SpaceHulkState::IsTerminal() const {
    std::uint8_t result;
    rl_m_is_done__Game_r_bool(&result, const_cast<::Game*>(&game));
    return result != 0;
}

std::vector<double> SpaceHulkState::Returns() const {
    double value;
    rl_m_score__Board_r_double(&value, const_cast<::Board*>(&game.board)); 
    return {value, -value};
}

std::string SpaceHulkState::InformationStateString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return HistoryString();
}

std::string SpaceHulkState::ObservationString(Player player) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);
  return ToString();
}

void SpaceHulkState::ObservationTensor(Player player,
                                       absl::Span<float> values) const {
  SPIEL_CHECK_GE(player, 0);
  SPIEL_CHECK_LT(player, num_players_);

  SPIEL_CHECK_EQ(values.size(), getGame()->game_size);
  VectorTint8_tT t;

  rl_as_byte_vector__Game_r_VectorTint8_tT(&t, const_cast<::Game*>(&game));

  for (int cell = 0; cell < t._size; ++cell) {
      values[cell] = float(t._data[cell]) / 30;
  }

  rl_m_drop__VectorTint8_tT(&t);
}


std::unique_ptr<State> SpaceHulkState::Clone() const {
  return std::unique_ptr<State>(new SpaceHulkState(*this));
}

std::string SpaceHulkGame::ActionToString(Player player,
                                          Action action_id) const {

  ::String rl_string;
  const auto* action = &actionsTable[action_id];
  rl_to_string__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit_r_String(&rl_string, const_cast<AnyGameAction*>(action));
  std::string str((char*)rl_string._data._data);
  rl_m_drop__String(&rl_string);
  return str;
}

SpaceHulkGame::SpaceHulkGame(const GameParameters& params)
    : Game(kGameType, params) {
        ::Game fake_game;
        rl_play__r_Game(&fake_game);
        VectorTint8_tT serialized;
        rl_as_byte_vector__Game_r_VectorTint8_tT(&serialized, &fake_game);

        game_size = static_cast<size_t>(serialized._size);

        rl_m_drop__VectorTint8_tT(&serialized);
        rl_m_drop__Game(&fake_game);

        AnyGameAction action;

        VectorTalt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuitT result;
        rl_enumerate__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit_r_VectorTalt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuitT(&result, &action);
        actionsTable.resize(result._size);
        for (size_t i = 0; i < result._size; i++) {
            rl_m_init__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit(&actionsTable[i]);
            rl_m_assign__alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit_alt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuit(&actionsTable[i], &result._data[i]);
        }

        rl_m_drop__VectorTalt_GameBeginMove_or_GameTurn_or_GameMove_or_GameEndMove_or_GameShoot_or_GameDoNothing_or_GameOverwatch_or_GameGuard_or_GameAssault_or_GameGuardReroll_or_GameFaceAttacker_or_GamePassTurn_or_GameQuitT(&result);

    }

}  // namespace space_hulk
}  // namespace open_spiel
