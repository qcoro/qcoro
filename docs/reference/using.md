# Using QCoro

Using QCoro is super-easy. All you need to do is include a header that provides
coroutine support for a type that you want to use with your coroutine and that's
it.

There's one extra thing that you have to do: once you use the `co_await` keyword
in your function, the function facticaly becomes a coroutine. And coroutines
must have a special return type. The type is sort of a promise type (not to confuse
with `std::promise`) that is returned to the caller when the coroutine is suspended.

QCoro provides implementation of this "special" type in [QCoro::Task](task.md). So
in order to turn your function into a coroutine, you need to change the return type
of your function from `T` to `QCoro::Task<T>`.
