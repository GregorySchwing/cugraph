/*
 * Copyright (c) 2023, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cugraph_c/algorithms.h>

#include <c_api/array.hpp>

namespace cugraph {
namespace c_api {

struct cugraph_heirarchical_clustering_result_t {
  double modularity{0};
  cugraph_type_erased_device_array_t* vertices_{nullptr};
  cugraph_type_erased_device_array_t* clusters_{nullptr};
};

}  // namespace c_api
}  // namespace cugraph
