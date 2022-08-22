#pragma once

#include "qcorotask.h"

namespace QCoro {

template<typename T>
class DetachedTask;

template<typename T> DetachedTask<T> detachTask(Task<T> &&);

template<typename T>
class DetachedTask : public Task<T> {
private:
    template<typename U>
    friend DetachedTask<U> QCoro::detachTask(Task<U> &&);

    explicit DetachedTask(Task<T> &&task): Task<T>(std::forward<Task<T>>(task)) {}
};


template<typename T>
DetachedTask<T> detachTask(Task<T> &&task) {
    return DetachedTask<T>{std::forward<Task<T>>(task)};
}

} // namespace QCoro