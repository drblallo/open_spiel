# Copyright 2022 DeepMind Technologies Limited
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

"""An example of use of PPO.

Note: code adapted (with permission) from
https://github.com/vwxyzjn/cleanrl/blob/master/cleanrl/ppo.py and
https://github.com/vwxyzjn/ppo-implementation-details/blob/main/ppo_atari.py
"""

# pylint: disable=g-importing-member
import collections
from datetime import datetime
import logging
import os
import random
import sys
import time
from absl import app
from absl import flags
import numpy as np
import pandas as pd
import torch
from torch.utils.tensorboard import SummaryWriter

import pyspiel
from open_spiel.python.pytorch.ppo import PPO
from open_spiel.python.pytorch.ppo import PPOAgent
from open_spiel.python.pytorch.ppo import PPOAtariAgent
from open_spiel.python.rl_environment import ChanceEventSampler
from open_spiel.python.rl_environment import Environment
from open_spiel.python.rl_environment import ObservationType
from open_spiel.python.vector_env import SyncVectorEnv
from open_spiel.python.algorithms.random_agent import RandomAgent


FLAGS = flags.FLAGS

flags.DEFINE_string("exp_name",
                    os.path.basename(__file__).rstrip(".py"),
                    "the name of this experiment")
flags.DEFINE_string("game_name", "atari", "the id of the OpenSpiel game")
flags.DEFINE_float("learning_rate", 2.5e-4,
                   "the learning rate of the optimizer")
flags.DEFINE_integer("seed", 1, "seed of the experiment")
flags.DEFINE_integer("total_timesteps", 10_000_000,
                     "total timesteps of the experiments")
flags.DEFINE_integer("eval_every", 100, "evaluate the policy every N updates")
flags.DEFINE_bool("torch_deterministic", True,
                  "if toggled, `torch.backends.cudnn.deterministic=False`")
flags.DEFINE_bool("cuda", True, "if toggled, cuda will be enabled by default")

# Atari specific arguments
flags.DEFINE_string("gym_id", "BreakoutNoFrameskip-v4",
                    "the id of the environment")
flags.DEFINE_bool(
    "capture_video", False,
    "whether to capture videos of the agent performances (check out `videos` folder)"
)

# Algorithm specific arguments
flags.DEFINE_integer("num_envs", 8, "the number of parallel game environments")
flags.DEFINE_integer(
    "num_steps", 128,
    "the number of steps to run in each environment per policy rollout")
flags.DEFINE_bool(
    "anneal_lr", True,
    "Toggle learning rate annealing for policy and value networks")
flags.DEFINE_bool("gae", True, "Use GAE for advantage computation")
flags.DEFINE_float("gamma", 0.99, "the discount factor gamma")
flags.DEFINE_float("gae_lambda", 0.95,
                   "the lambda for the general advantage estimation")
flags.DEFINE_integer("num_minibatches", 4, "the number of mini-batches")
flags.DEFINE_integer("update_epochs", 4, "the K epochs to update the policy")
flags.DEFINE_bool("norm_adv", True, "Toggles advantages normalization")
flags.DEFINE_float("clip_coef", 0.1, "the surrogate clipping coefficient")
flags.DEFINE_bool(
    "clip_vloss", True,
    "Toggles whether or not to use a clipped loss for the value function, as per the paper"
)
flags.DEFINE_float("ent_coef", 0.01, "coefficient of the entropy")
flags.DEFINE_float("vf_coef", 0.5, "coefficient of the value function")
flags.DEFINE_float("max_grad_norm", 0.5,
                   "the maximum norm for the gradient clipping")
flags.DEFINE_float("target_kl", None, "the target KL divergence threshold")


def setup_logging():
  root = logging.getLogger()
  root.setLevel(logging.DEBUG)

  handler = logging.StreamHandler(sys.stdout)
  handler.setLevel(logging.DEBUG)
  formatter = logging.Formatter(
      "%(asctime)s - %(name)s - %(levelname)s - %(message)s")
  handler.setFormatter(formatter)
  root.addHandler(handler)



def make_single_env(game_name, seed):

  def gen_env():
    game = pyspiel.load_game(game_name)
    return Environment(game, chance_event_sampler=ChanceEventSampler(seed=seed))

  return gen_env

def train(agents, current_trainee, envs, writer, current_epoch, total_epocs):
  batch_size = int(FLAGS.num_envs * FLAGS.num_steps)
  num_updates = FLAGS.total_timesteps // batch_size
  n_reward_window = 50
  recent_rewards = collections.deque(maxlen=n_reward_window)
  time_step = envs.reset()
  for update in range(num_updates):
    for _ in range(FLAGS.num_steps):
      for i, curr_env in enumerate(envs.envs):
          while time_step[i].observations["current_player"] != current_trainee:
            output = agents[time_step[i].observations["current_player"]].step([time_step[i]], is_evaluation=True)
            time_step[i] = curr_env.step([output[0].action])
            if curr_env._state.is_terminal():
                time_step[i] = curr_env.reset()


      agent_output = agents[current_trainee].step(time_step)
      time_step, reward, done, unreset_time_steps = envs.step(
          agent_output, reset_if_done=True)

      real_reward = max(env._state.returns()[current_trainee] for env in envs.envs if not env._state.is_terminal())
      writer.add_scalar(f"charts/player_{current_trainee}_training_returns", real_reward,
                            agents[current_trainee].total_steps_done)
      recent_rewards.append(real_reward)

      agents[current_trainee].post_step([x[current_trainee] for x in reward], done)

    if FLAGS.anneal_lr:
      agents[current_trainee].anneal_learning_rate((current_epoch * num_updates) + update, total_epocs * num_updates)

    agents[current_trainee].learn(time_step)

    if update % FLAGS.eval_every == 0:
      logging.info("-" * 80)
      logging.info("Step %s", agents[current_trainee].total_steps_done)
      logging.info("Summary of past %i rewards\n %s",
                   n_reward_window,
                   pd.Series(recent_rewards).describe())

def play_out(envs, agents):
  time_step = envs.envs[0].reset()
  while not envs.envs[0]._state.is_terminal():
    agent_output = agents[time_step.observations["current_player"]].step([time_step], is_evaluation=True)[0].action
    time_step = envs.envs[0].step([agent_output])
    print(0, envs.envs[0].get_state.action_to_string(agent_output), time_step.rewards[0])

def main(_):
  setup_logging()


  if FLAGS.game_name == "atari":
    # pylint: disable=unused-import
    # pylint: disable=g-import-not-at-top
    import open_spiel.python.games.atari

  current_day = datetime.now().strftime("%d")
  current_month_text = datetime.now().strftime("%h")
  run_name = f"{FLAGS.game_name}__{FLAGS.exp_name}__"
  if FLAGS.game_name == "atari":
    run_name += f"{FLAGS.gym_id}__"
  run_name += f"{FLAGS.seed}__{current_month_text}__{current_day}__{int(time.time())}"

  writers = [SummaryWriter(f"runs/{run_name}_p{i}") for i in range(2)]
  for writer in writers:
    writer.add_text(
      "hyperparameters",
      "|param|value|\n|-|-|\n%s" %
      ("\n".join([f"|{key}|{value}|" for key, value in vars(FLAGS).items()])),
    )

  random.seed(FLAGS.seed)
  np.random.seed(FLAGS.seed)
  torch.manual_seed(FLAGS.seed)
  torch.backends.cudnn.deterministic = FLAGS.torch_deterministic

  device = torch.device(
      "cuda" if torch.cuda.is_available() and FLAGS.cuda else "cpu")
  logging.info("Using device: %s", str(device))

  envs = SyncVectorEnv([
        make_single_env(FLAGS.game_name, FLAGS.seed + i)()
        for i in range(FLAGS.num_envs)
    ])
  agent_fn = PPOAgent

  game = envs.envs[0]._game  # pylint: disable=protected-access
  info_state_shape = game.observation_tensor_shape()

  agents = [PPO(
      input_shape=(info_state_shape[0],),
      num_actions=game.num_distinct_actions(),
      num_players=game.num_players(),
      player_id=i,
      num_envs=FLAGS.num_envs,
      steps_per_batch=FLAGS.num_steps,
      num_minibatches=FLAGS.num_minibatches,
      update_epochs=FLAGS.update_epochs,
      learning_rate=FLAGS.learning_rate,
      gae=FLAGS.gae,
      gamma=FLAGS.gamma,
      gae_lambda=FLAGS.gae_lambda,
      normalize_advantages=FLAGS.norm_adv,
      clip_coef=FLAGS.clip_coef,
      clip_vloss=FLAGS.clip_vloss,
      entropy_coef=FLAGS.ent_coef,
      value_coef=FLAGS.vf_coef,
      max_grad_norm=FLAGS.max_grad_norm,
      target_kl=FLAGS.target_kl,
      device=device,
      writer=writers[i],
      agent_fn=agent_fn,
  ) for i in range(2)]


  for i in range(10):
    train(agents, 0, envs, writers[0], i, 10)
    train(agents, 1, envs, writers[1], i, 10)
    play_out(envs, agents)

  for writer in writers:
    writer.close()
  logging.info("All done. Have a pleasant day :)")



if __name__ == "__main__":
  app.run(main)
