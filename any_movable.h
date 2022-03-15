#ifndef any_movable_h_INCLUDED
#define any_movable_h_INCLUDED

#include <utility>  //std::move/forward
#include <new> // placement new
#include <typeinfo> // typeid, std::type_info
#include <any> // bad_any_cast
#include <type_traits>

#include <iostream>

class any_movable
{
    union Storage
    {
        unsigned char data[6 * sizeof(void *)]; // approx same size as std::function's buffer
        long double to_ensure_alignment_with_almost_anything;
    };

    struct Base
    {
        // The interface that any_movable will use to handle the held item
        // A Derived<T> template will derive from this and implement the interface for each T
        virtual ~Base() {}
        virtual Base * move_to(void * dst) { return new (dst) Base(); }
        virtual std::type_info const & getTypeInfo() const { return typeid(void); }
        virtual void * getItem() { return nullptr; }
        virtual void const * getItem() const { return nullptr; }
        virtual bool can_assign() const { return true; } // you can assign nothing to nothing!
        virtual void move_assign(void * data) { }
        virtual void youveGotToThrowItThrowIt() const { throw getItem(); } //nullptr!
        virtual bool isClass() const { return false; } // is the type you are holding a class or non-class
    };
    // Implement Base for each T
    template<typename T>
    struct Derived : Base
    {
        T t;
        template <typename ...Args>
        Derived(Args &&... args) : t(std::forward<Args>(args)...)
        {
        }

        virtual ~Derived() {}
        virtual Base * move_to(void * dst) override
        {
            return new (dst) Derived(std::move(t));
        }
        virtual std::type_info const & getTypeInfo() const override
        {
            return typeid(T);
        }
        virtual void * getItem() override
        {
            return &t;
        }
        virtual void const * getItem() const override
        {
            return &t;
        }
        virtual void move_assign(void * data) override
        {
            if constexpr (std::is_move_assignable_v<T>)
                t = std::move(*reinterpret_cast<T *>(data));
        }
        virtual bool can_assign() const override
        {
            return std::is_move_assignable_v<T>;
        }
        virtual void youveGotToThrowItThrowIt() const
        {
            throw const_cast<T *>(reinterpret_cast<T const *>(getItem())); // so evil!
        }
        virtual bool isClass() const
        {
            return std::is_class_v<T>;
        }
    };

    Storage storage;
    Base * ptr = nullptr;

    template<typename T>
    T * try_as_base() const
    {
        // What follows one of the more evil things I've done....
        // We would like to ask ptr, which holds some type X, whether X is derived from T
        // But we can't ask that because ptr is not a template (so we can't pass <T>).
        // ie It is not possible to have a virtual template function (for sound reasons).
        // HOWEVER, what we can, magically, terribly, do is
        // call a non-template function on ptr, asking it to throw a X * exception
        // which we will attempt to catch here as a T *.
        // If the catch is successful, then X derives from T.
        try
        {
            if (ptr)
                ptr->youveGotToThrowItThrowIt();
        }
        catch (T * p)
        {
            // yes it does derive from T!
            return p;
        }
        catch (...)
        {
        }
        return nullptr;
    }

    template<typename T, typename ...Args>
    std::decay_t<T> & takeAndMake(Args &&... args)
    {
        reset();
        using UT = std::decay_t<T>; // remove ref, etc
        if (sizeof(Derived<UT>) <= sizeof(Storage))
            ptr = new (storage.data) Derived<UT>(std::forward<Args>(args)...); // TODO: use C++20 std::construct_at for constexpr
        else
            ptr = new Derived<UT>(std::forward<Args>(args)...);
        return access<UT>();
    }

    void take(any_movable && other)
    {
        reset();
        if ((void *)other.ptr == other.storage.data) // so not null either
        {
            // we must transfer from one storage to another
            // but we don't know how (we don't know what Derived is)
            // so ask ptr to do it via a virtual function
            ptr = other.ptr->move_to(storage.data);
            other.ptr->~Base();
        }
        else
        {
            // take ownership (and might be null)
            ptr = other.ptr;
        }
        other.ptr = nullptr;
    }

public:

    void reset()
    {
        if ((void *)ptr == (void *)storage.data) // local? (also not null)
            ptr->~Base();  // destroy (possibly Derived), but don't deallocate
        else
            delete ptr; // null is fine here
        ptr = nullptr;
    }

    any_movable() {}
    ~any_movable()
    {
        reset();
    }

    any_movable(any_movable &) = delete;
    any_movable(any_movable const &) = delete;
    any_movable & operator=(any_movable &) = delete;
    any_movable & operator=(any_movable const &) = delete;

    template<typename T>
    any_movable(T && t)
    {
        takeAndMake<T>(std::forward<T>(t));
    }
    template<typename T>
    any_movable & operator=(T && t)
    {
        if (has_type<T>() && ptr->can_assign())
            ptr->move_assign(&t); // we already hold a T, so assign it the new value
        else
            takeAndMake<T>(std::forward<T>(t));  // rebuild from the ground up
        return *this;
    }

    any_movable(any_movable && other)
    {
        take(std::move(other));
    }
    any_movable & operator=(any_movable && other)
    {
        if (ptr && ptr->can_assign() && type() == other.type()) { // we've already got one
            ptr->move_assign(other.ptr->getItem());
            // we could just leave the moved-from T,
            // but I suspect most people expect it to be reset
            other.reset();
        }
        else {
            take(std::move(other));
        }
        return *this;
    }

    template <typename T, typename ...Args>
    std::decay_t<T> & emplace(Args &&... args)
    {
        return takeAndMake<T>(std::forward<Args>(args)...);
    }

    std::type_info const & type() const
    {
        return ptr ? ptr->getTypeInfo() : typeid(void);
    }

    bool has_value() const
    {
        return ptr != nullptr;
    }
    template <typename T>
    bool has_type() const
    {
        return ptr && ptr->getTypeInfo() == typeid(T);
    }
    template<typename T>
    bool has_dynamic_type() const
    {
        // try_as_base is slow, last resort
        return has_type<T>() || (try_as_base<T>() != nullptr);
    }

    template <typename T>
    T const * access_ptr() const
    {
        if (has_type<T>()) {
            return reinterpret_cast<T const *>(ptr->getItem());
        }
        return nullptr;
    }
    template <typename T>
    T * access_ptr()
    {
        if (has_type<T>()) {
            return reinterpret_cast<T *>(ptr->getItem());
        }
        return nullptr;
    }
    template <typename T>
    T const & access() const
    {
        if (T const * p = access_ptr<T>())
            return *p;
        throw std::bad_any_cast();
    }
    template<typename T>
    T & access()
    {
        if (T * p = access_ptr<T>())
            return *p;
        throw std::bad_any_cast();
    }

    template <typename T>
    T const * access_ptr_dynamic() const
    {
        if (T * p = access_ptr<T>())
            return p;
        return try_as_base<T>();
    }
    template <typename T>
    T * access_ptr_dynamic()
    {
        if (T * p = access_ptr<T>())
            return p;
        return try_as_base<T>();
    }
    template <typename T>
    T const & access_dynamic() const
    {
        if (T const * p = access_ptr_dynamic<T>())
            return *p;
        throw std::bad_any_cast();
    }
    template<typename T>
    T & access_dynamic()
    {
        if (T * p = access_ptr_dynamic<T>())
            return *p;
        throw std::bad_any_cast();
    }

};

template <typename T>
[[nodiscard]] T const * any_dynamic_cast(any_movable const * a)
{
    return a->access_ptr_dynamic<T>();
}
template <typename T>
[[nodiscard]] T * any_dynamic_cast(any_movable * a)
{
    return a->access_ptr_dynamic<T>();
}

template <typename T>
[[nodiscard]] T const & any_dynamic_cast(any_movable const & a)
{
    return a.access_dynamic<T>();
}
template <typename T>
[[nodiscard]] T & any_dynamic_cast(any_movable & a)
{
    return a.access_dynamic<T>();
}
template <typename T>
[[nodiscard]] T && any_dynamic_cast(any_movable && a)
{
    return std::move(a.access_dynamic<T>());
}


// it is (somewhat) OK to open std namespace when you are overloading on your own type
namespace std
{
    template <typename T>
    [[nodiscard]] T const * any_cast(any_movable const * a)
    {
        return a ? a->access_ptr<T>() : nullptr;
    }
    template <typename T>
    [[nodiscard]] T * any_cast(any_movable * a)
    {
        return a ? a->access_ptr<T>() : nullptr;
    }

    //
    // for references, std::any returns a copy of T, but I think T & makes more sense
    //
    template <typename T>
    [[nodiscard]] T const & any_cast(any_movable const & a)
    {
        return a.access<T>();
    }
    template <typename T>
    [[nodiscard]] T & any_cast(any_movable & a)
    {
        return a.access<T>();
    }
    template <typename T>
    [[nodiscard]] T && any_cast(any_movable && a)
    {
        return std::move(a.access<T>());
    }
}


#endif // _h
