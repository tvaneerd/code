#include "any_tidy_ptr.h"

#include <gtest/gtest.h>

#include <type_traits>

namespace
{
    struct Tracker
    {
        static int aliveCount;

        Tracker() { aliveCount++; }
        ~Tracker() { aliveCount--; }
    };
    int Tracker::aliveCount = 0;
}

TEST(any_tidy_ptr, ctor_default)
{
    {
        any_tidy_ptr<Tracker> p;

        EXPECT_EQ(nullptr, p);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

// we can't test ctor without equality;
// can't test equality without ctor
TEST(any_tidy_ptr, nullptr_equality)
{
    {
        any_tidy_ptr<Tracker> p;

        EXPECT_TRUE(p == nullptr);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, equality)
{
    int deleter_called = 0;

    auto do_nothing_deleter = [&deleter_called](Tracker *) mutable { deleter_called++; };

    {
        Tracker tracker;
        EXPECT_EQ(1, Tracker::aliveCount);
        any_tidy_ptr<Tracker> p(&tracker, do_nothing_deleter);
        any_tidy_ptr<Tracker> q(&tracker, do_nothing_deleter);
        // normally any_tidy_ptr is unique,
        // but not if it is purpose-built
        EXPECT_TRUE(p == q);
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
    EXPECT_EQ(2, deleter_called);
}

TEST(any_tidy_ptr, ctor_ptr)
{
    {
        // gets the default deleter
        any_tidy_ptr<Tracker> p(new Tracker);

        EXPECT_EQ(1, Tracker::aliveCount);
        EXPECT_NE(nullptr, p);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, ctor_ptr_is_explicit)
{
    // imagine these two functions existed:
    int test(...); // takes anything, but is last choice for compiler when determining which test() is a best match for test(ptr)
    char test(any_tidy_ptr<int>);  // takes any_tidy_ptr

    int * ptr;
    (void)ptr; // avoid "unreferenced local variable" warnings
    // if we called test(ptr) which test() listed above gets called???
    // Since the any_tidy_ptr(T *) constructor is *explicit*, then test(any_tidy_ptr) should NOT be called
    // and the only remaining choice (the compiler's least favourite choice) is test(...), which returns int, not char
    // so check the return type of test(ptr) via decltype and is_same
    // This all happens at compile time.  If the static assert fails, it will fail to compile
    static_assert(std::is_same<decltype(test(ptr)), int>::value, "should call test(...), not test(any_tidy_ptr) as ctor from pointer is explicit");
}

TEST(any_tidy_ptr, ctor_ptr_nullptr)
{
    {
        Tracker tracker;
        EXPECT_EQ(1, Tracker::aliveCount);
        {
            any_tidy_ptr<Tracker> p(&tracker, nullptr);  // will not delete!
            EXPECT_EQ(1, Tracker::aliveCount);
        }
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, ctor_ptr_zero)
{
    {
        Tracker tracker;
        EXPECT_EQ(1, Tracker::aliveCount);
        {
            // I don't think anyone should construct with 0 or NULL (use nullptr!)
            // but this is here to check that it calls the nullptr constructor,
            // and not the std::function constructor (which would make std::function null, and throw on destruction)
            any_tidy_ptr<Tracker> p(&tracker, 0);  // will not delete!
            EXPECT_EQ(1, Tracker::aliveCount);
            bool threw = false;
            try
            {
                p.reset();
            }
            catch (...)
            {
                threw = true;
            }
            EXPECT_FALSE(threw);
            EXPECT_EQ(1, Tracker::aliveCount);
        }
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, ctor_array)
{
    {
        any_tidy_ptr<Tracker[]> p(new Tracker[10]);

        EXPECT_NE(nullptr, p);
        EXPECT_EQ(10, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, ctor_shared_ptr)
{
    {
        any_tidy_ptr<Tracker> q;
        {
            std::shared_ptr<Tracker> sp = std::make_shared<Tracker>();
            any_tidy_ptr<Tracker> p(sp);

            EXPECT_NE(nullptr, p);
            EXPECT_EQ(2, sp.use_count());
            EXPECT_EQ(1, Tracker::aliveCount);

            q = std::move(p);
            EXPECT_EQ(nullptr, p);
            EXPECT_EQ(2, sp.use_count());
            EXPECT_EQ(1, Tracker::aliveCount);
        }
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, ctor_shared_ptr_tidy_array)
{
    // I had issues with shared_ptr<T> combined with any_tidy_ptr<T[]> so here's a test
    {
        any_tidy_ptr<Tracker[]> q;
        {
            std::shared_ptr<Tracker> sp = std::make_shared<Tracker>();
            any_tidy_ptr<Tracker[]> p(sp);

            EXPECT_NE(nullptr, p);
            EXPECT_EQ(2, sp.use_count());
            EXPECT_EQ(1, Tracker::aliveCount);

            q = std::move(p);
            EXPECT_EQ(nullptr, p);
            EXPECT_EQ(2, sp.use_count());
            EXPECT_EQ(1, Tracker::aliveCount);
        }
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}

#if 0
// should this work?
// 1. based on the standard, unique_ptr<T[],D> does not convert to unique_ptr<T,E>, even if D converts to E
//    But it works with MSVC
// 2. should any_tidy_ptr make it work???
TEST(any_tidy_ptr, T_array_to_T)
{
    {
        any_tidy_ptr<Tracker> q;
        {
            any_tidy_ptr<Tracker[]> p(new Tracker[10]);

            EXPECT_NE(nullptr, p);
            EXPECT_EQ(10, Tracker::aliveCount);

            q = any_tidy_ptr<Tracker>(std::move(p));
            EXPECT_EQ(nullptr, p);
            EXPECT_EQ(10, Tracker::aliveCount);
        }
        EXPECT_EQ(10, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}
#endif

TEST(any_tidy_ptr, move_assignment)
{
    {
        any_tidy_ptr<Tracker> q;
        {
            any_tidy_ptr<Tracker> p(new Tracker);
            q = std::move(p);
            EXPECT_EQ(nullptr, p);
        }
        EXPECT_EQ(1, Tracker::aliveCount);
    }
    EXPECT_EQ(0, Tracker::aliveCount);
}


template<typename T>
constexpr bool is_move_only_v = !std::is_copy_constructible_v<T>
    && !std::is_copy_assignable_v<T>
    && std::is_move_constructible_v<T>
    && std::is_move_assignable_v<T>;

TEST(any_tidy_ptr, move_only)
{
    static_assert(is_move_only_v <any_tidy_ptr<int>>, "any_tidy_ptr should be move-only - move-ctor/assign but no copy-ctor/assign");
}


TEST(any_tidy_ptr, member_swap)
{
    any_tidy_ptr<Tracker> q;
    {
        any_tidy_ptr<Tracker> p(new Tracker);
        p.swap(q);
        EXPECT_EQ(nullptr, p);
        EXPECT_NE(nullptr, q);
    }
    EXPECT_EQ(1, Tracker::aliveCount);
}

TEST(any_tidy_ptr, std_swap)
{
    any_tidy_ptr<Tracker> q;
    {
        any_tidy_ptr<Tracker> p(new Tracker);
        std::swap(p, q);
        EXPECT_EQ(nullptr, p);
        EXPECT_NE(nullptr, q);
    }
    EXPECT_EQ(1, Tracker::aliveCount);
}


TEST(any_tidy_ptr, nullptr_assignment)
{
    any_tidy_ptr<Tracker> p(new Tracker);
    EXPECT_NE(nullptr, p);
    EXPECT_EQ(1, Tracker::aliveCount);
    p = nullptr;
    EXPECT_EQ(nullptr, p);
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, reset)
{
    any_tidy_ptr<Tracker> p(new Tracker);
    EXPECT_NE(nullptr, p);
    EXPECT_EQ(1, Tracker::aliveCount);
    p.reset();
    EXPECT_EQ(nullptr, p);
    EXPECT_EQ(0, Tracker::aliveCount);
}

TEST(any_tidy_ptr, release)
{
    Tracker * ptr = new Tracker;
    EXPECT_EQ(1, Tracker::aliveCount);
    {
        any_tidy_ptr<Tracker> p(ptr);
        EXPECT_NE(nullptr, p);
        Tracker * qtr = p.release();
        EXPECT_EQ(nullptr, p);
        EXPECT_EQ(ptr, qtr);
    }
    EXPECT_EQ(1, Tracker::aliveCount);
    delete ptr;
}

TEST(any_tidy_ptr, operator_bool_false)
{
    any_tidy_ptr<int> p;
    EXPECT_FALSE(!!p);
}

TEST(any_tidy_ptr, operator_bool_true)
{
    any_tidy_ptr<int> p(new int);
    EXPECT_TRUE(!!p);
}


template<typename T, typename Index, typename = void>
struct has_subscript_operator : std::false_type
{
};
template<typename T, typename Index>
struct has_subscript_operator<T, Index, std::void_t<
    decltype(std::declval<T>()[std::declval<Index>()])
    >> : std::true_type
{
};
template <typename T, typename Index>
using has_subscript_operator_t = typename has_subscript_operator<T, Index>::type;

TEST(any_tidy_ptr, operator_star)
{
    int x = 17;
    any_tidy_ptr<int> p(&x, nullptr);
    int v = *p;
    EXPECT_EQ(17, v);
}

TEST(any_tidy_ptr, operator_arrow)
{
    struct X { int x = 17; };
    any_tidy_ptr<X> p(new X);
    int v = p->x;
    EXPECT_EQ(17, v);
}

TEST(any_tidy_ptr, operator_array)
{
    static_assert(!has_subscript_operator_t<any_tidy_ptr<Tracker>, int>::value, "any_tidy_ptr<T> should not have operator[]");
    static_assert(has_subscript_operator_t<any_tidy_ptr<Tracker[]>, int>::value, "any_tidy_ptr<T[]> should have operator[]");
    int x[3] = { 10,11,12 };
    any_tidy_ptr<int[]> p(x, nullptr);
    int v = p[2];
    EXPECT_EQ(12, v);
}

TEST(any_tidy_ptr, member_typedefs)
{
    // same as unique_ptr<T, std::function>
    static_assert(std::is_same<any_tidy_ptr<int>::pointer, int *>::value, "any_tidy_ptr<T>::pointer should be T*");
    static_assert(std::is_same<any_tidy_ptr<int[]>::pointer, int *>::value, "any_tidy_ptr<T[]>::pointer should be T*");
    static_assert(std::is_same<any_tidy_ptr<int>::element_type, int>::value, "any_tidy_ptr<T>::element_type should be T");
    static_assert(std::is_same<any_tidy_ptr<int[]>::element_type, int>::value, "any_tidy_ptr<T[]>::element_type should be T");
    static_assert(std::is_same<any_tidy_ptr<int>::deleter_type, std::function<void(int *)> >::value, "any_tidy_ptr<T>::deleter_type should be std::function<void(T*)>");
    static_assert(std::is_same<any_tidy_ptr<int[]>::deleter_type, std::function<void(int *)> >::value, "any_tidy_ptr<T[]>::deleter_type should be std::function<void(T*)>");
}


TEST(any_tidy_ptr, operator_compare)
{
    int x[3] = { 10,11,12 };
    any_tidy_ptr<int> p(&x[0], nullptr);
    any_tidy_ptr<int> q(&x[1], nullptr);
    any_tidy_ptr<int> r(&x[1], nullptr);

    EXPECT_TRUE(p != q);
    EXPECT_FALSE(p == q);
    EXPECT_TRUE(p < q);
    EXPECT_FALSE(q < p);
    EXPECT_TRUE(p <= q);
    EXPECT_FALSE(q <= p);
    EXPECT_TRUE(q > p);
    EXPECT_FALSE(p > q);
    EXPECT_TRUE(q >= p);
    EXPECT_FALSE(p >= q);

    EXPECT_TRUE(q == r);
    EXPECT_FALSE(q != r);
    EXPECT_FALSE(q < r);
    EXPECT_FALSE(r < q);
    EXPECT_TRUE(q <= r);
    EXPECT_TRUE(r <= q);
    EXPECT_FALSE(q > r);
    EXPECT_FALSE(r > q);
    EXPECT_TRUE(q >= r);
    EXPECT_TRUE(r >= q);
}
