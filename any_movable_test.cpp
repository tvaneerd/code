#include "any_movable.h"

#include <gtest/gtest.h>

#include <type_traits>

namespace
{

    struct moveonly
    {
        int val = 17;
        moveonly() = default;
        moveonly(moveonly && other) : val(other.val)
        {
            other.val = 23;
        }
        moveonly & operator=(moveonly &&) = default;

        bool operator==(int v) const { return val == v; }
        bool operator!=(int v) const { return val != v; }
    };

    struct BigMoveOnly : moveonly
    {
        char b[1000];
    };

}


//
// Can't believe I'm doing this.
// This replaces global new/delete for EVERY cpp file in this exe.
// What exe is this?  Hopefully just CommonTest.
// Maybe any_movable should have its own test exe?
//
static int largeNew = 0;
static int largeDelete = 0;
static std::atomic<void *> largeNewObjects[128];
static const int LargeSize = 4096;
static bool newTracking_tooManyLargeObjects = false;

void * operator new(std::size_t sz) // no inline, required by [replacement.functions]/3
{
    // use good old malloc
    if (sz == 0) ++sz; // avoid std::malloc(0) which may return nullptr on success
    void * ptr = std::malloc(sz);

    if (ptr == nullptr)
        throw std::bad_alloc{}; // required by [new.delete.single]/3


    if (sz >= LargeSize && !newTracking_tooManyLargeObjects)
    {
        // we are tracking large objects because operator delete doesn't give us a size
        // so instead we need to remember that it was a big object...

        // find a place for it
        bool found = false;
        for (auto & atom_ptr : largeNewObjects)
        {
            void * tmp = nullptr;
            if (atom_ptr.compare_exchange_strong(tmp, ptr))  // was null, now "ours"
            {
                found = true;
                break;
            }
        }
        if (found)
            largeNew++;
        else
            newTracking_tooManyLargeObjects = true;
    }

    return ptr;
}

void operator delete(void * ptr) noexcept
{
    // was this being tracked?
    for (auto & atom_ptr : largeNewObjects)
    {
        if (atom_ptr == ptr) // at least memory_order_acquire
        {
            // found a large object!
            largeDelete++;
            atom_ptr = nullptr; // at least _release
        }
    }

    std::free(ptr);
}



namespace UnitTestCommon
{


    TEST(any_movable, ctor_default)
    {
        any_movable a;

        EXPECT_FALSE(a.has_value());
    }

    TEST(any_movable, ctor_is_explicit)
    {
        // should any ctors be explicit???
        // check std::any??
        // I don't think so.
        EXPECT_FALSE(false);
    }

    TEST(any_movable, ctor_value)
    {
        any_movable a = 17;

        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(17, a.access<int>());
    }


    TEST(any_movable, ctor_move_val)
    {
        moveonly m;
        // will not compile:
        //any_movable ax = m;

        any_movable a = std::move(m);

        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(17, a.access<moveonly>().val);
    }

    TEST(any_movable, ctor_move_any)
    {
        moveonly m;

        any_movable a = std::move(m);
    //    any_movable bx = a; // does not compile
        any_movable b = std::move(a);

        // was the inner value moved from?
        // which of the following should be true:
        EXPECT_FALSE(a.has_value());
        //EXPECT_EQ(23, a.access<moveonly>().val);

        EXPECT_EQ(17, b.access<moveonly>().val);
    }

    TEST(any_movable, move_assign_val)
    {
        moveonly m;
        any_movable a;

        a = std::move(m);

        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(17, a.access<moveonly>().val);
        EXPECT_EQ(23, m.val); // moved from value
    }

    TEST(any_movable, move_assign_any)
    {
        moveonly m;
        any_movable a;
        any_movable am = std::move(m);

        a = std::move(am);

        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(17, a.access<moveonly>().val);
        // was the inner value moved from?
        // which of the following should be true:
        EXPECT_FALSE(am.has_value());
        //EXPECT_EQ(23, am.access<moveonly>().val);
    }


    template<typename T>
    constexpr bool is_move_only_v = !std::is_copy_constructible_v<T>
        && !std::is_copy_assignable_v<T>
        && std::is_move_constructible_v<T>
        && std::is_move_assignable_v<T>;

    TEST(any_movable, move_only)
    {
        static_assert(is_move_only_v<any_movable>, "any_movable should be move-only - move ctor/assign but no copy ctor/assign");
    }


    TEST(any_movable, std_swap_swaps_the_values)
    {
        any_movable a = 17;
        any_movable b = 23;
        std::swap(a, b);
        EXPECT_EQ(23, a.access<int>());
        EXPECT_EQ(17, b.access<int>());
    }

    TEST(any_movable, any_reset_does_nothing_when_already_empty)
    {
        any_movable a;
        EXPECT_FALSE(a.has_value());
        a.reset();
        EXPECT_FALSE(a.has_value());
    }

    TEST(any_movable, reset_removes_value)
    {
        any_movable a = 17;
        a.reset();
        EXPECT_FALSE(a.has_value());
        EXPECT_FALSE(a.has_type<int>());
    }

    TEST(any_movable, operator_bool)
    {
        // should any_movable work in an if statement?
        //
        // any_movable a;
        // if (a) // ie a.has_value()
        //    ...
        // 
        // or is that just confusing:
        //
        // any_movable a = false;
        // if (a) // true! because has_value() is true
        //     ...

        // std::any does not have bool operator (but optional does. Go figure)
    }

    TEST(any_movable, emplace)
    {
        struct Foo
        {
            Foo(int x, int y, int z) : x(x), y(y), z(z)
            {}
            int x, y, z;
        };

        any_movable a;
        Foo & foo = a.emplace<Foo>(3, 5, 7);

        EXPECT_EQ(&foo, &a.access<Foo>()); // emplace returns reference to internal value

        EXPECT_EQ(3, foo.x);
        EXPECT_EQ(5, foo.y);
        EXPECT_EQ(7, foo.z);
    }

    TEST(any_movable, can_cast_to_base_type)
    {
        // I'm not even sure this feature is a good idea (and the implementation magic is horrendous)

        int x = 0;
        struct Base
        {
            int & r;
            Base(int & r) : r(r) {}
            virtual void set() { r = 17;  }
        };
        struct Derived : Base
        {
            Derived(int & r) : Base(r) {}
            virtual void set() { r = 23;  }
        };

        any_movable a = Derived(x);

        Base & b = a.access_dynamic<Base>();
        b.set();
        EXPECT_EQ(23, x);
    }

    TEST(any_movable, can_NOT_cast_to_non_base_type)
    {
        // I'm not even sure this feature is a good idea (and the implementation magic is horrendous)

        int x = 0;
        struct Base
        {
            int & r;
            Base(int & r) : r(r) {}
            virtual void set() { r = 17; }
        };
        struct NotDerived
        {
            int & r;
            NotDerived(int & r) : r(r) {}
            virtual void set() { r = 23; }
        };

        any_movable a = NotDerived(x);

        EXPECT_THROW(a.access_dynamic<Base>(), std::bad_any_cast);
    }

    TEST(any_movable, reset_destroys_small_objects)
    {
        int x = 0;
        struct WriteInDestructor
        {
            int & x;
            WriteInDestructor(int & x) : x(x)
            {
            }
            ~WriteInDestructor()
            {
                x++;
            }
        };

        any_movable a = WriteInDestructor(x);
        a.reset();
        EXPECT_EQ(2, x); // there was a temporary WriteInDestructor, and one in the any

        a.emplace<WriteInDestructor>(x); // no temporary, constructed right in the any
        a.reset();
        EXPECT_EQ(3, x);
    }

    // a separate class per test, so tests can be done in parallel
    template <int N, int size=4>
    struct Counter
    {
        char buf[size];
        static int ctors;
        static int dtors;

        Counter(Counter const &) { ctors++; }
        Counter() { ctors++; }
        ~Counter() { dtors++; }
    };

    using Counter1 = Counter<1, 4>;
    int Counter1::ctors = 0;
    int Counter1::dtors = 0;

    TEST(any_movable, reset_cleans_small_objects)
    {
        any_movable a = Counter1();
        a.reset();
        EXPECT_EQ(2, Counter1::ctors);   // there was a temporary Counter, and one in the any
        EXPECT_EQ(2, Counter1::dtors);   // there was a temporary Counter, and one in the any
    }

    using Counter2 = Counter<2, LargeSize>;
    int Counter2::ctors = 0;
    int Counter2::dtors = 0;
    TEST(any_movable, reset_cleans_big_objects)
    {
        // technically, there could be a race condition here
        // if something large is being allocated in another thread somewhere...
        int prevLargeNew = largeNew;
        int prevLargeDelete = largeDelete;

        any_movable a = Counter2();
        a.reset();
        EXPECT_EQ(2, Counter2::ctors);   // there was a temporary Counter, and one in the any
        EXPECT_EQ(2, Counter2::dtors);   // there was a temporary Counter, and one in the any

        int diffNew = largeNew - prevLargeNew;
        int diffDelete = largeDelete - prevLargeDelete;

        // Only one Counter2 was allocated (inside the any)
        // The other one (above) was a temporary, not on the heap
        EXPECT_EQ(1, diffNew);
        EXPECT_EQ(1, diffDelete);
    }

    using Counter3 = Counter<3, LargeSize>;
    int Counter3::ctors = 0;
    int Counter3::dtors = 0;
    TEST(any_movable, move_cleans_big_objects)
    {
        // technically, there could be a race condition here
        // if something large is being allocated in another thread somewhere...
        int prevLargeNew = largeNew;
        int prevLargeDelete = largeDelete;
        int diffNew;
        int diffDelete;

        {
            any_movable a = Counter3();

            diffNew = largeNew - prevLargeNew;
            diffDelete = largeDelete - prevLargeDelete;
            // Only one Counter2 was allocated (inside the any)
            // The other one (above) was a temporary, not on the heap
            EXPECT_EQ(1, diffNew);
            EXPECT_EQ(0, diffDelete);  // not yet deleted
            // update:
            prevLargeNew = largeNew;
            prevLargeDelete = largeDelete;

            {
                any_movable b = std::move(a);

                diffNew = largeNew - prevLargeNew;
                diffDelete = largeDelete - prevLargeDelete;

                // nothing allocated or deleted; only moved!
                EXPECT_EQ(0, diffNew);
                EXPECT_EQ(0, diffDelete);
                // update:
                prevLargeNew = largeNew;
                prevLargeDelete = largeDelete;
            } // now b calls delete here

            diffNew = largeNew - prevLargeNew;
            diffDelete = largeDelete - prevLargeDelete;
            EXPECT_EQ(0, diffNew);
            EXPECT_EQ(1, diffDelete);
            // update:
            prevLargeNew = largeNew;
            prevLargeDelete = largeDelete;
        }
        // now a calls ... nothing, because it was moved-from and is empty
        diffNew = largeNew - prevLargeNew;
        diffDelete = largeDelete - prevLargeDelete;
        EXPECT_EQ(0, diffNew);
        EXPECT_EQ(0, diffDelete);
    }

    TEST(any_movable, any_cast_const_ptr_returns_const_ptr)
    {
        const any_movable a = 17;

        auto p = std::any_cast<int>(&a);

        static_assert(std::is_same_v<decltype(p), int const *>, "any_cast<T>(&any) should return a T const * if the any is const");
        EXPECT_EQ(17, *p);
    }
    TEST(any_movable, any_cast_ptr_returns_ptr)
    {
        any_movable a = 17;

        auto p = std::any_cast<int>(&a);

        static_assert(std::is_same_v<decltype(p), int *>, "any_cast<T>(&any) should return a T *");
        EXPECT_EQ(17, *p);
    }
    TEST(any_movable, any_cast_ptr_returns_null_when_empty)
    {
        any_movable a;

        auto p = std::any_cast<int>(&a);

        static_assert(std::is_same_v<decltype(p), int *>, "any_cast<T>(&any) should return a T *");
        EXPECT_EQ(nullptr, p);
    }
    TEST(any_movable, any_cast_const_ref_returns_const_ref)
    {
        const any_movable a = 17;

        // decltype(auto) means give me the exact type returned (ie int &)
        // whereas just auto would make r an int
        decltype(auto) r = std::any_cast<int>(a);

        static_assert(std::is_same_v<decltype(r), int const &>, "any_cast<T>(&any) should return a T const & when the any is const");
        EXPECT_EQ(17, r);
    }
    TEST(any_movable, any_cast_ref_returns_ref)
    {
        any_movable a = 17;

        // decltype(auto) means give me the exact type returned (ie int &)
        // whereas just auto would make r an int
        decltype(auto) r = std::any_cast<int>(a);

        static_assert(std::is_same_v<decltype(r), int &>, "any_cast<T>(any) should return a T &");
        EXPECT_EQ(17, r);
    }
    TEST(any_movable, any_cast_ref_ref_returns_ref_ref)
    {
        any_movable a = 17;

        // decltype(auto) means give me the exact type returned (ie int &&)
        // whereas just auto would make r an int
        decltype(auto) r = std::any_cast<int>(std::move(a));

        static_assert(std::is_same_v<decltype(r), int &&>, "any_cast<T>(std::move(any)) should return a T &&");
        EXPECT_EQ(17, r);
    }
    TEST(any_movable, any_cast_ref_throws_when_empty)
    {
        any_movable a;

        int p;

        EXPECT_THROW(p = std::any_cast<int>(a), std::bad_any_cast);
    }


    TEST(any_movable, any_dynamic_cast_holding_derived_returns_base)
    {
        int x = 0;
        struct Base
        {
            int & r;
            Base(int & r) : r(r) {}
            virtual void set() { r = 17; }
        };
        struct Derived : Base
        {
            Derived(int & r) : Base(r) {}
            virtual void set() { r = 23; }
        };

        any_movable a = Derived(x);

        Base & b = any_dynamic_cast<Base>(std::move(a));
        b.set();
        EXPECT_EQ(23, x);
    }




}

