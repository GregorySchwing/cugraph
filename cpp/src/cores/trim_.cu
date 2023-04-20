#pragma once

<<<<<<< HEAD
<<<<<<< HEAD

=======
>>>>>>> 91c2d056 (update)
=======
>>>>>>> 91c2d056867ee23497c86de6d359139861a1a358
#include <cugraph/algorithms.hpp>
#include <cugraph/graph_functions.hpp>
#include <cugraph/graph_view.hpp>
#include <cugraph/utilities/error.hpp>

#include <thrust/copy.h>
#include <thrust/iterator/discard_iterator.h>

namespace cugraph {

template <typename vertex_t, typename edge_t, typename weight_t, bool multi_gpu>
<<<<<<< HEAD
<<<<<<< HEAD
std::tuple<rmm::device_uvector<vertex_t>,
           rmm::device_uvector<vertex_t>,
           std::optional<rmm::device_uvector<weight_t>>>
=======
rmm::device_uvector<vertex_t> 
>>>>>>> 91c2d056867ee23497c86de6d359139861a1a358
trim(raft::handle_t const& handle,
       graph_view_t<vertex_t, edge_t, false, multi_gpu> const& graph_view)
{

  CUGRAPH_EXPECTS(!graph_view.is_multigraph(),
                  "Invalid input argument: trim currently does not support multi-graphs.");
 
  auto in_degrees = graph_view.compute_in_degree();
  auto out_degrees = graph_view.compute_out_degree();

  // remove in-degree = 0 vertex
  rmm::device_uvector<vertex_t> remaining_vertices(graph_view.local_vertex_partition_range_size(),
                                                   handle.get_stream());
  remaining_vertices.resize(
    thrust::distance(
      remaining_vertices.begin(),
      thrust::copy_if(
        handle.get_thrust_policy(),
        thrust::make_counting_iterator(graph_view.local_vertex_partition_range_first()),
        thrust::make_counting_iterator(graph_view.local_vertex_partition_range_last()),
        remaining_vertices.begin(),
        [in_degrees] __device__(
          auto v) { return in_degrees[v] > edge_t{0}; })),
    handle.get_stream());
  // remove out-degree = 0 vertex
  remaining_vertices.resize(
    thrust::distance(
      remaining_vertices.begin(),
      thrust::copy_if(
        handle.get_thrust_policy(),
        thrust::make_counting_iterator(graph_view.local_vertex_partition_range_first()),
        thrust::make_counting_iterator(graph_view.local_vertex_partition_range_last()),
        remaining_vertices.begin(),
        [in_degrees] __device__(
          auto v) { return out_degrees[v] > edge_t{0}; })),
    handle.get_stream());


  return remaining_vertices;
}
}
