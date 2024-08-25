#pragma once
#ifndef CATA_SRC_A_STAR_H
#define CATA_SRC_A_STAR_H

#include <algorithm>
#include <limits>
#include <optional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename Node, typename Cost = int, typename VisitedSet = std::unordered_set<Node>, typename BestPathMap = std::unordered_map<Node, std::pair<Cost, Node>>>
class AStarState
{
public:
    void reset(const Node& start, Cost cost);

    std::optional<Node> get_next(Cost max);

    template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
    void generate_neighbors(const Node& current, StateCostFn&& s_cost_fn, TransitionCostFn&& t_cost_fn,
        HeuristicFn&& heuristic_fn, NeighborsFn&& neighbors_fn);

    bool has_path_to(const Node& end) const;

    Cost path_cost(const Node& end) const;

    std::vector<Node> path_to(const Node& end) const;

private:
    struct FirstElementGreaterThan {
        template <typename T, typename... Ts>
        bool operator()(const std::tuple<T, Ts...>& lhs, const std::tuple<T, Ts...>& rhs) const {
            return std::get<0>(lhs) > std::get<0>(rhs);
        }
    };

    using FrontierNode = std::tuple<Cost, Node>;
    using FrontierQueue =
        std::priority_queue<FrontierNode, std::vector<FrontierNode>, FirstElementGreaterThan>;

    VisitedSet visited_;
    BestPathMap best_paths_;
    FrontierQueue frontier_;
};

template <typename Node, typename Cost = int, typename VisitedSet = std::unordered_set<Node>, typename BestStateMap = std::unordered_map<Node, std::pair<Cost, Node>>>
class AStarPathfinder
{
public:
    template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
    std::vector<Node> find_path(Cost max, const Node& from, const Node& to,
        StateCostFn&& s_cost_fn, TransitionCostFn&& t_cost_fn, HeuristicFn&& heuristic_fn,
        NeighborsFn&& neighbors_fn);

private:
    AStarState<Node, Cost, VisitedSet, BestStateMap> state_;
};

template <typename Node, typename Cost = int, typename VisitedSet = std::unordered_set<Node>, typename BestStateMap = std::unordered_map<Node, std::pair<Cost, Node>>>
class BidirectionalAStarPathfinder
{
public:
    template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
    std::vector<Node> find_path(Cost max, const Node& from, const Node& to,
        StateCostFn&& s_cost_fn, TransitionCostFn&& t_cost_fn, HeuristicFn&& heuristic_fn,
        NeighborsFn&& neighbors_fn);

private:
    AStarState<Node, Cost, VisitedSet, BestStateMap> forward_;
    AStarState<Node, Cost, VisitedSet, BestStateMap> backward_;
};

// Implementation Details

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
void AStarState<Node, Cost, VisitedSet, BestPathMap>::reset(const Node& start, Cost cost)
{
    visited_.clear();
    best_paths_.clear();

    // priority_queue doesn't have a clear method, so we cannot reuse it, and it may
    // get quite large, so we explicitly create the underlying container and reserve
    // space beforehand.
    std::vector<FrontierNode> queue_data;
    queue_data.reserve(400);
    frontier_ = FrontierQueue(FirstElementGreaterThan(), std::move(queue_data));

    best_paths_.try_emplace(start, 0, start);
    frontier_.emplace(cost, start);
}

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
bool AStarState<Node, Cost, VisitedSet, BestPathMap>::has_path_to(const Node& end) const
{
    return best_paths_.count(end);
}

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
Cost AStarState<Node, Cost, VisitedSet, BestPathMap>::path_cost(const Node& end) const
{
    return best_paths_.at(end).first;
}

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
std::vector<Node> AStarState<Node, Cost, VisitedSet, BestPathMap>::path_to(const Node& end) const
{
    std::vector<Node> result;
    if (has_path_to(end)) {
        Node current = end;
        while (best_paths_.at(current).first != 0) {
            result.push_back(current);
            current = best_paths_.at(current).second;
        }
        std::reverse(result.begin(), result.end());
    }
    return result;
}

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
std::optional<Node> AStarState<Node, Cost, VisitedSet, BestPathMap>::get_next(Cost max)
{
    while (!frontier_.empty()) {
        auto [cost, current] = frontier_.top();
        frontier_.pop();

        if (cost >= max) {
            return std::nullopt;
        }

        if (const auto& [_, inserted] = visited_.emplace(current); !inserted) {
            continue;
        }
        return current;
    }
    return std::nullopt;
}

template <typename Node, typename Cost, typename VisitedSet, typename BestPathMap>
template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
void AStarState<Node, Cost, VisitedSet, BestPathMap>::generate_neighbors(
    const Node& current, StateCostFn&& s_cost_fn, TransitionCostFn&& t_cost_fn,
    HeuristicFn&& heuristic_fn,
    NeighborsFn&& neighbors_fn)
{
    // Can't use structured bindings here due to a defect in Clang 10.
    const std::pair<Cost, Node>& best_path = best_paths_[current];
    const Cost current_cost = best_path.first;
    const Node& current_parent = best_path.second;
    neighbors_fn(current_parent, current, [this, &s_cost_fn, &t_cost_fn, &heuristic_fn, &current,
        current_cost](const Node& neighbour) {
            if (visited_.count(neighbour)) {
                return;
            }
            if (const std::optional<Cost> s_cost = s_cost_fn(neighbour)) {
                if (const std::optional<Cost> t_cost = t_cost_fn(current, neighbour)) {
                    const auto& [iter, _] = best_paths_.try_emplace(neighbour, std::numeric_limits<Cost>::max(),
                        Node());
                    auto& [best_cost, parent] = *iter;
                    const Cost new_cost = current_cost + *s_cost + *t_cost;
                    if (new_cost < best_cost) {
                        best_cost = new_cost;
                        parent = current;
                        const Cost estimated_cost = new_cost + heuristic_fn(neighbour);
                        frontier_.emplace(estimated_cost, neighbour);
                    }
                }
            }
            else {
                visited_.emplace(neighbour);
            }
        });
}

template <typename Node, typename Cost, typename VisitedSet, typename BestStateMap>
template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
std::vector<Node> AStarPathfinder<Node, Cost, VisitedSet, BestStateMap>::find_path(
    Cost max_cost, const Node& from, const Node& to, StateCostFn&& s_cost_fn,
    TransitionCostFn&& t_cost_fn,
    HeuristicFn&& heuristic_fn, NeighborsFn&& neighbors_fn)
{
    if (!s_cost_fn(from) || !s_cost_fn(to)) {
        return {};
    }

    state_.reset(from, heuristic_fn(from, to));
    while (const std::optional<Node> current = state_.get_next(max_cost)) {
        if (*current == to) {
            return state_.path_to(to);
        }
        state_.generate_neighbors(*current, s_cost_fn, t_cost_fn, [&heuristic_fn,
            to](const Node& from) {
                return heuristic_fn(from, to);
            }, neighbors_fn);
    }
    return {};
}


template <typename Node, typename Cost, typename VisitedSet, typename BestStateMap>
template <typename StateCostFn, typename TransitionCostFn, typename HeuristicFn, typename NeighborsFn>
std::vector<Node> BidirectionalAStarPathfinder<Node, Cost, VisitedSet, BestStateMap>::find_path(
    Cost max_cost, const Node& from, const Node& to, StateCostFn&& s_cost_fn,
    TransitionCostFn&& t_cost_fn,
    HeuristicFn&& heuristic_fn, NeighborsFn&& neighbors_fn)
{
    if (!s_cost_fn(from) || !s_cost_fn(to)) {
        return {};
    }

    // The full cost is not used since that would result in paths that are up to 2x longer than
    // intended. Half the cost cannot be used, since there is no guarantee that both searches
    // proceed at the same pace. 2/3rds the cost is a fine balance between the two, and has the
    // effect of the worst case still visiting less states than normal A*.
    const Cost partial_max_cost = 2 * max_cost / 3;
    forward_.reset(from, heuristic_fn(from, to));
    backward_.reset(to, heuristic_fn(to, from));
    for (;; ) {
        const std::optional<Node> f_current_state = forward_.get_next(partial_max_cost);
        if (!f_current_state) {
            break;
        }
        const std::optional<Node> b_current_state = backward_.get_next(partial_max_cost);
        if (!b_current_state) {
            break;
        }

        bool f_links = backward_.has_path_to(*f_current_state);
        bool b_links = forward_.has_path_to(*b_current_state);

        if (f_links && b_links) {
            const Cost f_cost = forward_.path_cost(*f_current_state) + backward_.path_cost(
                *f_current_state);
            const Cost b_cost = forward_.path_cost(*b_current_state) + backward_.path_cost(
                *b_current_state);
            if (b_cost < f_cost) {
                f_links = false;
            }
            else {
                b_links = false;
            }
        }

        if (f_links || b_links) {
            const Node& midpoint = f_links ? *f_current_state : *b_current_state;
            std::vector<Node> forward_path = forward_.path_to(midpoint);
            std::vector<Node> backward_path = backward_.path_to(midpoint);
            if (backward_path.empty()) {
                return forward_path;
            }
            backward_path.pop_back();
            std::for_each(backward_path.rbegin(), backward_path.rend(), [&forward_path](const Node& node) {
                forward_path.push_back(node);
                });
            forward_path.push_back(to);
            return forward_path;
        }

        forward_.generate_neighbors(*f_current_state, s_cost_fn, t_cost_fn, [&heuristic_fn,
            &to](const Node& from) {
                return heuristic_fn(from, to);
            }, neighbors_fn);
        backward_.generate_neighbors(*b_current_state, s_cost_fn, [&t_cost_fn](const Node& from,
            const Node& to) {
                return t_cost_fn(to, from);
            }, [&heuristic_fn, &from](const Node& to) {
                return heuristic_fn(to, from);
                }, neighbors_fn);
    }
    return {};
}

#endif // CATA_SRC_A_STAR_H
