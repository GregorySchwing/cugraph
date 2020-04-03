/*
 * Copyright (c) 2020, NVIDIA CORPORATION.
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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "cub/cub.cuh"
#include <algorithm>
#include <iomanip>

#include <rmm_utils.h>

#include "utilities/graph_utils.cuh"
#include "utilities/error_utils.h"
#include <cugraph.h>
#include <graph.hpp>

#include <rmm/thrust_rmm_allocator.h>
#include <rmm/device_buffer.hpp>

#include "exact_kernels.h"

namespace cugraph {
namespace detail {

template <typename vertex_t, typename edge_t, typename weight_t>
void exact_fa2(const vertex_t *row, const vertex_t *col,
               const weight_t *v, const edge_t e, const vertex_t n,
               float *x_pos, float *y_pos, const int max_iter=1000,
               float *x_start=nullptr, float *y_start=nullptr,
               bool outbound_attraction_distribution=false,
               bool lin_log_mode=false, bool prevent_overlapping=false,
               const float edge_weight_influence=1.0,
               const float jitter_tolerance=1.0,
               const float scaling_ratio=2.0, bool strong_gravity_mode=false,
               const float gravity=1.0) { 
    
    // Temprary fix
    // For weighted graph number_of_edges returns half the vertices
    // but does not adapt the datastructure accordingly.
    const edge_t tmp_e = e * 2;

    float *d_dx{nullptr};
    float *d_dy{nullptr};
    float *d_old_dx{nullptr};
    float *d_old_dy{nullptr};
    int *d_mass{nullptr};
    float *d_swinging{nullptr};
    float *d_traction{nullptr};

    rmm::device_vector<float> dx(n, 0);
    rmm::device_vector<float> dy(n, 0);
    rmm::device_vector<float> old_dx(n, 0);
    rmm::device_vector<float> old_dy(n, 0);
    rmm::device_vector<int> mass(n, 1);
    rmm::device_vector<float> swinging(1, 0);
    rmm::device_vector<float> traction(1, 0);

    d_dx = dx.data().get();
    d_dy = dy.data().get();
    d_old_dx = dx.data().get();
    d_old_dy = dy.data().get();
    d_mass = mass.data().get();
    d_swinging = swinging.data().get();
    d_traction = traction.data().get();

    thrust::host_vector<float> h_swinging(1);
    thrust::host_vector<float> h_traction(1);

    if (x_start == nullptr || y_start == nullptr) {
        // TODO: generate random numbers
        return;
    } else {
        copy(n, x_start, x_pos);
        copy(n, y_start, y_pos);
    }

    cudaStream_t stream {nullptr};
    vertex_t* srcs{nullptr};
    vertex_t* dests{nullptr};

    ALLOC_TRY((void**)&srcs, sizeof(vertex_t) * tmp_e, stream);
    ALLOC_TRY((void**)&dests, sizeof(vertex_t) * tmp_e, stream);
    CUDA_TRY(cudaMemcpy(srcs, row, sizeof(vertex_t) * tmp_e, cudaMemcpyDefault));
    CUDA_TRY(cudaMemcpy(dests, col, sizeof(vertex_t) * tmp_e, cudaMemcpyDefault));

    thrust::stable_sort_by_key(rmm::exec_policy(stream)->on(stream), dests, dests + tmp_e, srcs);
    thrust::stable_sort_by_key(rmm::exec_policy(stream)->on(stream), srcs, srcs + tmp_e, dests);

    dim3 nthreads, nblocks;
    nthreads.x = min(tmp_e, CUDA_MAX_KERNEL_THREADS);
    nthreads.y = 1;
    nthreads.z = 1;
    nblocks.x = min((tmp_e + nthreads.x - 1) / nthreads.x, CUDA_MAX_BLOCKS);
    nblocks.y = 1;
    nblocks.z = 1;

    degree_coo<vertex_t, int><<<nthreads, nblocks>>>(n, tmp_e, dests, d_mass);

    /*
    printv(tmp_e, srcs, 0);
    printv(tmp_e, dests, 0);
 
    printf("e: %i, n: %i\n", tmp_e, n);

    thrust::host_vector<int> h_mass(n);
    thrust::copy(mass.begin(), mass.end(), h_mass.begin());
    int *m = thrust::raw_pointer_cast(h_mass.data());

    for (int i = 0; i < n; ++i)
        printf("degree %i: %i\n", i, m[i]);
    */

    float speed = 1.0;
    float speed_efficiency = 1.0;
    float outbound_att_compensation = 1.0;
    if (outbound_attraction_distribution) {
        int sum = thrust::reduce(mass.begin(), mass.end());
        printf("sum: %i\n", sum);
        outbound_att_compensation = sum / (float)n;
    }
    printf("coef: %f\n", outbound_att_compensation);

    for (int iter=0; iter < max_iter; ++iter) {
        copy(n, d_dx, d_old_dx);
        copy(n, d_dy, d_old_dy);
        fill(n, d_dx, 0.f);
        fill(n, d_dy, 0.f);

        apply_repulsion<vertex_t>(x_pos,
                y_pos, d_dx, d_dy, d_mass, scaling_ratio, n);

        /*
        apply_gravity<vertex_t>(x_pos, y_pos, d_mass, d_dx, d_dy, gravity,
                strong_gravity_mode, scaling_ratio, n);

        */
        /*
        apply_attraction<vertex_t, edge_t, weight_t>(row,
                col, v, n, x_pos, y_pos, d_dx, d_dy, d_mass,
                outbound_attraction_distribution,
                edge_weight_influence, outbound_att_compensation);

        nthreads.x = min(n, CUDA_MAX_KERNEL_THREADS);
        nthreads.y = 1;
        nthreads.z = 1;
        nblocks.x = min((n + nthreads.x - 1) / nthreads.x, CUDA_MAX_BLOCKS);
        nblocks.y = 1;
        nblocks.z = 1;

        local_speed_kernel<<<nthreads, nblocks>>>(d_dx, d_dy,
                d_old_dx, d_old_dy, d_mass, d_swinging, d_traction, n);

        thrust::copy(swinging.begin(), swinging.end(), h_swinging.begin());
        thrust::copy(traction.begin(), traction.end(), h_traction.begin());
        float *s = thrust::raw_pointer_cast(h_swinging.data());
        float *t = thrust::raw_pointer_cast(h_traction.data());

        float jt = compute_jitter_tolerance<vertex_t>(jitter_tolerance,
                speed_efficiency, s, t, n);

        speed = compute_global_speed(speed, speed_efficiency, jt, s, t); 

        update_positions_kernel<vertex_t><<<nthreads, nblocks>>>(
                x_pos, y_pos, d_dx, d_dy,
                d_old_dx, d_old_dy, speed, n);
       printf("speed at iteration %i: %f\n", iter, speed);
       */
    }
}

} // namespace detail
}  // namespace cugraph
