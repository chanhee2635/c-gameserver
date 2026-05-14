#pragma once

using Job = std::function<void()>;

class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
    void Push(Job job);
    void Execute();
    void DoAsync(Job job);
    void DoTimer(uint32 afterMs, Job job);

    template<typename T, typename Ret, typename... Args>
    void DoAsync(Ret(T::* memfn)(Args...), Args... args)
    {
        auto self = std::static_pointer_cast<T>(shared_from_this());
        auto tuple = std::make_tuple(std::move(args)...);
        Push([self, memfn, t = std::move(tuple)]() mutable {
            std::apply([&](auto&&... a) {
                ((*self).*memfn)(std::move(a)...);
                }, t);
            });
    }

    template<typename T, typename Ret, typename... Args>
    void DoTimer(uint32 afterMs, Ret(T::* memfn)(Args...), Args... args)
    {
        auto self = std::static_pointer_cast<T>(shared_from_this());
        auto tuple = std::make_tuple(std::move(args)...);

        DoTimer(afterMs, [self, memfn, t = std::move(tuple)]() mutable {
            std::apply([&](auto&&... a) {
                ((*self).*memfn)(std::move(a)...);
                }, t);
            });
    }

private:
    Mutex        _lock;
    Queue<Job>   _jobs;
    Atomic<bool> _pending = false;
};

template<typename F>
Job MakeJob(F&& fn)
{
    using FD = std::decay_t<F>;
    FD* ptr = xnew<FD>(std::forward<F>(fn));
    return [ptr]() mutable { (*ptr)(); xdelete(ptr); };
}