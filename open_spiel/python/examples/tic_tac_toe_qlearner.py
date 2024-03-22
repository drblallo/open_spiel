# Copyright 2019 DeepMind Technologies Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tabular Q-Learner example on Tic Tac Toe.

Two Q-Learning agents are trained by playing against each other. Then, the game
can be played against the agents from the command line.

After about 10**5 training episodes, the agents reach a good policy: win rate
against random opponents is around 99% for player 0 and 92% for player 1.
"""

import logging
import sys
from absl import app
from absl import flags
import numpy as np
import torch

from open_spiel.python import rl_environment
from open_spiel.python.algorithms import random_agent
from open_spiel.python.algorithms import tabular_qlearner
from open_spiel.python.pytorch.dqn import DQN

FLAGS = flags.FLAGS

flags.DEFINE_integer("num_episodes", int(50e3), "Number of train episodes.")
flags.DEFINE_boolean(
    "interactive_play",
    True,
    "Whether to run an interactive play with the agent after training.",
)


def pretty_board(time_step):
  """Returns the board in `time_step` in a human readable format."""
  info_state = time_step.observations["info_state"][0]
  x_locations = np.nonzero(info_state[9:18])[0]
  o_locations = np.nonzero(info_state[18:])[0]
  board = np.full(3 * 3, ".")
  board[x_locations] = "X"
  board[o_locations] = "0"
  board = np.reshape(board, (3, 3))
  return board


def command_line_action(time_step):
  """Gets a valid action from the user on the command line."""
  current_player = time_step.observations["current_player"]
  legal_actions = time_step.observations["legal_actions"][current_player]
  action = -1
  while action not in legal_actions:
    print("Choose an action from {}:".format(legal_actions))
    sys.stdout.flush()
    action_str = input()
    try:
      action = int(action_str)
    except ValueError:
      continue
  return action


def eval_against_random_bots(env, trained_agents, random_agents, num_episodes):
  """Evaluates `trained_agents` against `random_agents` for `num_episodes`."""
  wins = np.zeros(2)
  for player_pos in range(2):
    if player_pos == 0:
      cur_agents = [trained_agents[0], random_agents[1]]
    else:
      cur_agents = [random_agents[0], trained_agents[1]]
    for _ in range(num_episodes):
      time_step = env.reset()
      while not time_step.last():
        player_id = time_step.observations["current_player"]
        agent_output = random_agents[0].step(time_step, is_evaluation=True) if player_id == -1 else cur_agents[player_id].step(time_step, is_evaluation=True)
        time_step = env.step([agent_output.action])
        wins[player_pos] += time_step.rewards[player_pos]
  return wins / num_episodes


def one_run(epsilon_decay_duration, learning_rate):
  game = "space_hulk"
  num_players = 2

  env = rl_environment.Environment(game)
  num_actions = env.action_spec()["num_actions"]
  state_size = env.observation_spec()["info_state"][0]
  logging.info("num actions: %s, state size %s", num_actions, state_size)

  agents = [
      #tabular_qlearner.QLearner(player_id=idx, num_actions=num_actions, discount_factor=0.95)
      DQN(player_id=idx, num_actions=num_actions, discount_factor=0.92, state_representation_size=state_size, epsilon_decay_duration=epsilon_decay_duration, hidden_layers_sizes=[512, 256, 256, 256], learning_rate=learning_rate, batch_size=256)
      for idx in range(num_players)
  ]

  # random agents for evaluation
  random_agents = [
      random_agent.RandomAgent(player_id=idx, num_actions=num_actions)
      for idx in range(num_players)
  ]

  # 1. Train the agents
  training_episodes = FLAGS.num_episodes
  for cur_episode in range(training_episodes):
    if cur_episode % int(1e2) == 0:
        logging.info(f"episode {cur_episode}")
    if cur_episode % int(1e3) == 0:
      win_rates = eval_against_random_bots(env, agents, random_agents, 100)
      logging.info("Starting episode %s, win_rates %s, decay %s, lr %s", cur_episode, win_rates, epsilon_decay_duration, learning_rate)
    time_step = env.reset()
    while not time_step.last():
      player_id = time_step.observations["current_player"]
      agent_output = random_agents[0].step(time_step) if player_id == -1 else agents[player_id].step(time_step)
      time_step = env.step([agent_output.action])

    # Episode is over, step all agents with final info state.
    for agent in agents:
      agent.step(time_step)
      if (cur_episode + 1) % int(5e3) == 0:
          agent.step_scheduler()

  if not FLAGS.interactive_play:
    return

  # 2. Play from the command line against the trained agent.
  human_player = 1
  while True:
    time_step = env.reset()
    while not time_step.last():
      player_id = time_step.observations["current_player"]
      agent_out = random_agents[0].step(time_step) if player_id == -1 else agents[player_id].step(time_step, is_evaluation=True)
      action = agent_out.action
      time_step = env.step([action])
      print(player_id, env.get_state.action_to_string(action), time_step.rewards[player_id])
    break

def main(_):
  torch.set_default_device('cuda')
  torch.backends.cuda.matmul.allow_tf32 = True
  torch.backends.cudnn.allow_tf32 = True
  for x in [2e3, 3e4]:
    for y in [0.1, 0.01]:
        one_run(x, y)

if __name__ == "__main__":
    app.run(lambda _ : main(_))

