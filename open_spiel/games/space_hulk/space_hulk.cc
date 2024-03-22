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
    GameType::Utility::kGeneralSum,
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
    return ::can_apply_impl(const_cast<AnyGameAction&>(action), const_cast<::Game&>(game));

}

void SpaceHulkState::DoApplyAction(Action move) {

  SPIEL_CHECK_GE(move , 0);
  SPIEL_CHECK_GT(getGame()->actionsTable.size(),  move);
  SPIEL_CHECK_TRUE(canApplyAction(getGame()->actionsTable[move]));
  auto* action = &getGame()->actionsTable[move];


  apply(const_cast<AnyGameAction&>(*action), const_cast<::Game&>(game));

  previous_states_rewards = current_states_rewards;
  current_states_rewards = score(const_cast<::Game&>(game));
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
        // ToDo Fix
}

std::string SpaceHulkState::ToString() const {
  ::String rl_string;
  rl_to_string__Game_r_String(&rl_string, const_cast<::Game*>(&game));
  std::string str((char*)rl_string.content._data.content._data);
  rl_m_drop__String(&rl_string);
  return str;
}

SpaceHulkState::SpaceHulkState(const SpaceHulkState & other) : State(other){
    rl_play__r_Game(&this->game);
    rl_m_assign__Game_Game(&game, const_cast<::Game*>(&other.game));
    previous_states_rewards = other.previous_states_rewards;
    current_states_rewards = other.current_states_rewards;
}
  
SpaceHulkState& SpaceHulkState::operator=(const SpaceHulkState & Other)
{
    if (this == &Other)
        return *this;
    rl_m_assign__Game_Game(&game, const_cast<::Game*>(&Other.game));
    previous_states_rewards = Other.previous_states_rewards;
    current_states_rewards = Other.current_states_rewards;
    return *this;
}

std::string SpaceHulkState::Serialize() const {
    return ToString();
}

std::unique_ptr<State> SpaceHulkGame::DeserializeState(const std::string& str) const {
    auto state = NewInitialState();
    auto* casted = reinterpret_cast<SpaceHulkState*>(state.get());
    String rl_string;
    rl_string.content._data.content._data = reinterpret_cast<int8_t*>(const_cast<char*>(&str.front()));
    rl_string.content._data.content._capacity = str.capacity();
    rl_string.content._data.content._size = str.size();
    from_string(casted->game, rl_string);
    rl_string.content._data.content._data = nullptr;
    rl_string.content._data.content._size = 0;
    rl_string.content._data.content._capacity = 0;
    return state;

}

bool SpaceHulkState::IsTerminal() const {
    return const_cast<::Game&>(game).is_done();
}

std::vector<double> SpaceHulkState::Rewards() const {
    auto diff = current_states_rewards - previous_states_rewards;
    return {diff, -diff};
}

std::vector<double> SpaceHulkState::Returns() const {
    return {current_states_rewards, -current_states_rewards};
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
  std::vector<double> v(values.size(), 0.0);
  VectorTdoubleT to_fill;
  to_fill.content._data = v.data();
  to_fill.content._size = v.size();
  to_fill.content._capacity = v.size();
  
  to_observation_tensor(const_cast<::Game&>(game), to_fill);

  for (size_t i = 0; i < v.size(); i++)
      values[i] = v[i];

  //TensorView<2> view(values, {256, getGame()->game_size / 256}, true);
  //for (int cell = 0; cell < t.content._size; ++cell) {
        //view[{*((uint8_t*)&t.content._data[cell]), cell}] = 1.0;
  //}

  to_fill.content._data = nullptr;
  to_fill.content._size = 0;
  to_fill.content._capacity = 0;

}


std::unique_ptr<State> SpaceHulkState::Clone() const {
  return std::unique_ptr<State>(new SpaceHulkState(*this));
}

ActionsAndProbs SpaceHulkState::ChanceOutcomes() const {
    ActionsAndProbs toReturn;
    auto actions = LegalActions();
    toReturn.reserve(actions.size());
    for (auto action : actions)
        toReturn.push_back(std::pair{action, 1.0/actions.size()});
    return toReturn;
}

std::string SpaceHulkGame::ActionToString(Player player,
                                          Action action_id) const {

  const auto* action = &actionsTable[action_id];
  auto rl_string = to_string(const_cast<AnyGameAction&>(*action));
  std::string str((char*)rl_string.content._data.content._data, rl_string.size());
  return str;
}

SpaceHulkGame::SpaceHulkGame(const GameParameters& params)
    : Game(kGameType, params) {
        auto fake_game = play();

        game_size =  observation_tensor_size(fake_game);

        AnyGameAction action;

        auto result = enumerate(action);
        actionsTable.resize(result.size());
        for (int64_t i = 0; i < result.size(); i++) {
            actionsTable[i] = result.content._data[i];
            std::cout << ActionToString(0, i) << std::endl;
        }


    }

}  // namespace space_hulk
}  // namespace open_spiel
