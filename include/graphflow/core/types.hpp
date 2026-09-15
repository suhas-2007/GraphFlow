#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <concepts>
#include <compare>

namespace graphflow::core {

// Fast 32-bit Node ID: saves 50% memory over 64-bit on cache lines
// and supports graphs up to 4.29 billion nodes.
using NodeId = uint32_t;
using EdgeWeight = double;
using FlowType = int64_t;
using CostType = int64_t;

inline constexpr NodeId kInvalidNode = std::numeric_limits<NodeId>::max();
inline constexpr EdgeWeight kInfinityWeight = std::numeric_limits<EdgeWeight>::infinity();
inline constexpr FlowType kInfinityFlow = std::numeric_limits<FlowType>::max() / 4;
inline constexpr CostType kInfinityCost = std::numeric_limits<CostType>::max() / 4;

struct Edge {
    NodeId target{kInvalidNode};
    EdgeWeight weight{1.0};

    auto operator<=>(const Edge&) const = default;
};

struct FlowEdge {
    NodeId to{kInvalidNode};
    FlowType capacity{0};
    FlowType flow{0};
    CostType cost{0};
    uint32_t rev{0}; // Index of reverse edge in the target's adjacency list

    [[nodiscard]] FlowType residual_capacity() const noexcept {
        return capacity - flow;
    }
};

struct PathResult {
    EdgeWeight distance{kInfinityWeight};
    std::vector<NodeId> path;
    uint64_t nodes_visited{0};
    double execution_time_ms{0.0};
};

struct FlowResult {
    FlowType max_flow{0};
    CostType min_cost{0};
    std::vector<NodeId> min_cut_source_side;
    double execution_time_ms{0.0};
};

} // namespace graphflow::core
