// PriorityDispatchQueue.h - Unit IV: generic template class + functor.
#ifndef ERLHUB_PRIORITY_DISPATCH_QUEUE_H
#define ERLHUB_PRIORITY_DISPATCH_QUEUE_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ReliefOps {
namespace Logistics {

// A request raised by a field team at a relief site.
struct TriageRequest {
    std::string site;
    std::string cargoId;
    int units;
    int severity;        // 1 (stable) .. 5 (life-threatening)
    int casualties;
    double hoursWaiting;
};

// Inline scoring rule: severity dominates, then head-count, then waiting time.
inline double urgencyScore(const TriageRequest& r) {
    return r.severity * 100.0 + r.casualties * 2.0 + r.hoursWaiting * 5.0;
}

// Functor: orders requests so the most urgent medical need is served first.
struct TriageUrgency {
    bool operator()(const TriageRequest& a, const TriageRequest& b) const {
        return urgencyScore(a) < urgencyScore(b);
    }
};

// Generic max-priority queue. 'Compare(a, b)' returns true when a ranks BELOW b.
// Works for any copyable T: TriageRequest, int, ReliefCargo, ...
template <typename T, typename Compare = std::less<T> >
class PriorityDispatchQueue {
public:
    explicit PriorityDispatchQueue(const Compare& compare = Compare()) : compare_(compare) {}

    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }

    void push(const T& item) {
        items_.push_back(item);
        std::push_heap(items_.begin(), items_.end(), compare_);
    }

    const T& top() const {
        if (items_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        return items_.front();
    }

    T pop() {
        if (items_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        std::pop_heap(items_.begin(), items_.end(), compare_);
        T highest = items_.back();
        items_.pop_back();
        return highest;
    }

    // Snapshot in service order (highest priority first); the queue is unchanged.
    std::vector<T> ordered() const {
        std::vector<T> copy(items_);
        std::sort_heap(copy.begin(), copy.end(), compare_);
        std::reverse(copy.begin(), copy.end());
        return copy;
    }

private:
    std::vector<T> items_;
    Compare compare_;
};

}  // namespace Logistics
}  // namespace ReliefOps

#endif
