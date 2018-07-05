#ifndef sampling_h_INCLUDED
#define sampling_h_INCLUDED

#include <iterator> // std::distance
#include <random> // uniform_int_distribution
#include <algorithm> // for_each (is it worth an include just for that?)

// like "Reservoir sampling", without the reservoir
// because we want an out() function, not a fixed place to put things.
// similar to std::sample() coming in C++17
// Note that this maintains the order of the selected samples, thus "stable"_sample
template <typename Iterator, typename Sentinel, typename UniformRandomNumberGenerator, typename Output>
void stable_sample(Iterator begin, Sentinel end, int sampleSize, UniformRandomNumberGenerator& urng, Output & out)
{
    auto left = std::distance(begin, end); // how many left to choose from
    using DistType = decltype(left);
    DistType need = sampleSize; // how many do we still need

    // Each item, in order, gets to pick for a "winning ticket"
    // the odds of each pick is based on how many we still need, and how many items are left.
    // So imagine 100 items, and we want 10 of them at random...
    // the first item chooses randomly from 1 to 100 and if they chose a number between 1 to 10, they "win"
    // the next item chooses from 1 to 99.  If the first won, the second needs a number 1 to 9, else still 1 to 10.
    //
    // It seems "unfair" that some items get to go first - if they win, the item at the end might not get a chance at all!
    // However, consider if they fail - that increases the odds of the next item winning - eventually the last item might win by default! (100% odds)
    // (For example, choose 10 out of 11 - the first item has a 10/11 chance of winning, and a 1/11 chance of failing, and that decides it for everyone else!
    //  But 10/11 and 1/11 was exactly the odds they should have had. OK, so far so good.
    //  What about the second item? If they get to try, they have 1/10 chance of fail, instead of 1/11. More chance of failure!?
    //  Oh, but that's *if* they need to try at all, ie only if the first item won (which was 10/11 odds).
    //  ie second item's total odds of failing are 10/11 * 1/10 = 1/11.  Oh, would you look at that.)
    // Do the math, or google it, it works out.

    for (Iterator curr = begin; need > 0; curr++) // curr is just a better name than begin
    {
        if (left <= need) {
            // everyone left "wins", because we still need at least that many
            // (note: we only fail to get the full sampleSize if we started with less than that many items)
            std::for_each(curr, end, out);
            return;
        }
        left--; // 0 to 99, not 1 to 100, because C++; and next time through it will be 0 to 98, etc
                // select # from 0 to 99:
        DistType r = std::uniform_int_distribution<DistType>(0, left)(urng);
        // if # is less than 10 (or whatever still needed), you are one of the 10 for the sample!
        if (r < need) {
            // winner winner chicken dinner
            out(*curr);
            --need; // and next one up needs to be 1 of the 9...
        }
    }
}

template<typename T, typename Transform>
std::vector<T> sample(std::vector<T> const & vin, int count, Transform const & transform)
{
    std::vector<T> out;
    std::random_device rd;
    std::mt19937 gen(rd());
    out.reserve(std::min(count, (int)vin.size()));
    stable_sample(vin.begin(), vin.end(), count, gen, [&transform, &out](T const & elem) { out.push_back(transform(elem)); });
    return out;
}

template<typename T>
std::vector<T> sample(std::vector<T> const & vin, int count)
{
    return sample(vin, count, [](T const & x) {return x; });
}

#endif // _h
