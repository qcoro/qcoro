# QCoro::Task

You may have noticed in the various examples in the documentation that our coroutines don't return
`void` like they would if they were normal functions. Instead they return `QCoro::Task<>` and you
may be asking what it is and why it's there.

We have already established that whenever we use `co_await` in a function, we turn that function
into a coroutine. And coroutines must return something called a promise type - it's an object that is
returned to the caller of the function. Not to be confused with `std::promise`, it has nothing to do
with this. The coroutine promise type allows the caller to control the coroutine.

Now all you need to know is, that in order for the magic contained in this repository to work your coroutine
must return the type `QCoro::Task<T>` - where `T` is the true return type of your function.

If you are calling a coroutine, to obtain the actual result value you must once again `co_await` it.

