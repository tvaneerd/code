#ifndef any_tidy_ptr_h_INCLUDED
#define any_tidy_ptr_h_INCLUDED

#include <memory> // std::unique_ptr
#include <functional>  // std::function
#include <type_traits> // std::remove_extent

//
// a unique_ptr with *any* deleter (ie deleter can change at runtime, not just compile time)
// ie deleter is "type-erased"
//
// so this can hold any pointer that knows how to clean up after itself
// It is heavier than unique_ptr<T> (because unique_ptr<T> has a hard-coded deleter, so it doesn't take up space)
// but it is useful for things like an Image class
// that wants to hold pixels that came from anywhere (ie OpenCV or calls to new or whatever)
//
// It can also hold shared_ptrs (to share pixels)
// or raw pointers that you don't want to delete (pass in a deleter that does nothing)
//
template <class T>
struct any_tidy_ptr : public std::unique_ptr<T, std::function<void(std::remove_extent_t<T> *)>>
{
    using pointer = std::remove_extent_t<T> *;  // if T is an array, say int[], pointer is still a pointer, ie int * 
    using base_unique = std::unique_ptr<T, std::function<void(pointer)>>;

    using base_unique::element_type;
    using base_unique::deleter_type;

    // we want all of our base unique_ptr's constructors
    using std::unique_ptr<T, std::function<void(std::remove_extent_t<T> *)>>::unique_ptr;
    // we want all of unique_ptr's constructors
    //using base_unique::base_unique;

    any_tidy_ptr() = default;

    explicit any_tidy_ptr(pointer ptr) : base_unique(ptr, std::default_delete<T>())
    {
    }

    // a pointer you don't want any_tidy_ptr to delete:
    any_tidy_ptr(pointer ptr, std::nullptr_t) : base_unique(ptr, [](pointer){})
    {
    }

    // and more!
    // this could be made to work with most any smart pointer:
    any_tidy_ptr(std::shared_ptr<element_type> const & sptr)
        : base_unique(sptr.get(), [sptr = sptr](pointer) mutable {sptr.reset(); })
    {
    }

    // We expose (via inheritance) all of unique_ptr's interface *except* get_deleter(),
    // because get_deleter() may be tied to the pointer being held - it can't delete *any* pointer (see shared_ptr example above)
    // so it is kind of bad to expose
    pointer * get_deleter() = delete;
};

#endif // _h
