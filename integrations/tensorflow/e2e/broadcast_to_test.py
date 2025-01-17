# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import numpy as np
from pyiree.tf.support import tf_test_utils
import tensorflow.compat.v2 as tf


class BroadcastToModule(tf.Module):

  def __init__(self):
    pass

  @tf.function(input_signature=[
      tf.TensorSpec([], tf.float32),
      tf.TensorSpec([2], tf.int32)
  ])
  def scalar_broadcast_to(self, x, shape):
    return tf.broadcast_to(x, shape)


@tf_test_utils.compile_module(BroadcastToModule)
class BroadcastToTest(tf_test_utils.TracedModuleTestCase):

  def test_scalar_broadcast_to(self):

    def scalar_broadcast_to(module):
      x = np.array(1, dtype=np.float32)
      shape = np.array([3, 3], dtype=np.int32)
      result = module.scalar_broadcast_to(x, shape)

    self.compare_backends(scalar_broadcast_to)


if __name__ == "__main__":
  if hasattr(tf, "enable_v2_behavior"):
    tf.enable_v2_behavior()
  tf.test.main()
