#include <concepts>
#include <functional>
#include <iostream>

template <typename T, typename R, typename... Args>
concept my_invocable_r = std::invocable<T, Args...> && requires (const T t, const Args&... args)
{
    { t(args...) } -> std::convertible_to<R>;
    // { std::invoke(t, args...) } -> std::convertible_to<R>; //Also an option
};

template <typename T> concept is_kernel = my_invocable_r<T, int, int>;

__global__ void kern(const is_kernel auto& lam)
{
    int result = lam(int(threadIdx.x));
}

inline void invoke_kernel(const is_kernel auto& lam)
{
    kern<<<1,1>>>(lam);
}

inline auto generic_function()
{
    // invoke_kernel([] __device__ (int){return 0;}); //Will not compile
    return 0;
}

inline void variadic_function(const std::integral auto&... is)
{
    // auto l = [=] __host__ __device__ (int){return int(is + ...);}; //Will not compile
}

inline __host__ __device__ void capture(const auto&){}

int main(int argc, char** argv)
{
    // Issue 1: __device__ lambdas do not satisfy invocable concepts without additional __host__ attribute: result of <type>::operator() appears to be incorrect?
    auto lam0 = []                     (int){return 0;};
    auto lam1 = [] __host__ __device__ (int){return 0;};
    auto lam2 = [] __device__          (int){return 0;};
    
    std::cout << is_kernel<decltype(lam0)> << " " << std::invocable<decltype(lam0), int> << std::endl;
    std::cout << is_kernel<decltype(lam1)> << " " << std::invocable<decltype(lam1), int> << std::endl;
    std::cout << is_kernel<decltype(lam2)> << " " << std::invocable<decltype(lam2), int> << std::endl;
    
    // Note that this behaviour is potentially intended, since the line
    // { t(args...) } -> std::convertible_to<R>;
    // above may fail since lam2 is not callable on the host, but it is still useful to be able
    // to query the return type in such a way.
    
    // End issue 1.
    
    // Issue 2: defining lambdas in generic functions or other lambdas
    generic_function(); // (see definition of this function)
    [](const auto&){/*invoke_kernel([] __host__ __device__ (int){return 0;});*/}(1); //Will not compile
    // End issue 2.
    
    // Issue 3: cannot capture parameter packs!
    variadic_function(1, 2, 3, 4, 5);
    // End issue 3.
    
    // Issue 3: cannot capture variables in an if-constexpr context (must always capture all variables
    // for all constexpr logical branches)
    int v = 0;
    auto lam3 = [=] __host__ __device__ (int)
    {
        capture(v);
        if constexpr (1 == 1) return v; // will not compile if previous line not present, inflates the size of the lambda object
        else return 12;
    };
    // End issue 3.
}