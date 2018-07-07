#include "sampling.h"

#include <gmock/gmock.h>

#include <list>
#include <vector>
#include <map>
#include <random>

TEST( sampleTest, zeroPopZeroSample )
{
    std::list<int> pop; // list! hahahaha

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), 0, urng, [&calls](int) {calls++; });

    EXPECT_EQ( 0, calls);
}

TEST(sampleTest, zeroPopNonzeroSample)
{
    std::list<int> pop; // list! hahahaha

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), 99, urng, [&calls](int) {calls++; });

    EXPECT_EQ(0, calls);
}

TEST(sampleTest, nonemptyPopNegativeSamples)
{
    std::list<int> pop(5, 9); // five 9s

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), -17, urng, [&calls](int) {calls++; });

    EXPECT_EQ(0, calls);
}

TEST(sampleTest, nonemptyPopZeroSample)
{
    std::list<int> pop(5, 9); // five 9s

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), 0, urng, [&calls](int) {calls++; });

    EXPECT_EQ(0, calls);
}

TEST(sampleTest, nonemptyPopLessSamples)
{
    std::list<int> pop(5, 9); // five 9s

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), 3, urng, [&calls](int) {calls++; });

    EXPECT_EQ(3, calls);
}

TEST(sampleTest, nonemptyPopExactSamples)
{
    std::list<int> pop(5, 9); // five 9s

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), (int)pop.size(), urng, [&calls](int) {calls++; });

    EXPECT_EQ(5, calls);
}

TEST(sampleTest, nonemptyPopMoreSamples)
{
    std::list<int> pop(5, 9); // five 9s

    int calls = 0;
    std::mt19937 urng;

    stable_sample(pop.begin(), pop.end(), 99, urng, [&calls](int) {calls++; });

    EXPECT_EQ(5, calls);
}

template<typename Container>
void fillNumbersTo(Container & c, int amount)
{
    for (int i = 0; i < amount; i++)
        c.push_back(i);
}

TEST(sampleTest, noDuplicates)
{
    std::list<int> pop;
    fillNumbersTo(pop, 100);

    std::vector<int> res;
    std::random_device rd;
    std::mt19937 urng(rd());

    stable_sample(pop.begin(), pop.end(), 20, urng, [&res](int sel) { res.push_back(sel); });

    std::sort(res.begin(), res.end());
    std::unique(res.begin(), res.end());

    EXPECT_EQ(20, res.size());
}

TEST(sampleTest, actuallyRandom)
{
    std::list<int> pop;
    fillNumbersTo(pop, 100);

    // testing for randomness is... hard
    // you need to do a bunch of runs, and look at statistics
    // The following could be improved...

    std::random_device rd;
    std::mt19937 urng(rd());

    std::map<int, int> counters;

    const int SAMPLESIZE = 20;
    const int RUNS = 10000;
    for (int run = 0; run < RUNS; run++)
    {
        stable_sample(pop.begin(), pop.end(), SAMPLESIZE, urng, [&counters](int x) { counters[x]++; });
    }

    // we picked SAMPLESIZE numbers, and did that RUNS times
    // so we picked SAMPLESIZE * RUNS numbers.
    // Each number should have been picked equally often (give or take)
    // ie SAMPLESIZE * RUNS / pop.size() times
    //
    const int EXPECTED_COUNT = RUNS * SAMPLESIZE / (int)pop.size();
    const int ALLOWED_DELTA = EXPECTED_COUNT / 10;  // ie +/-10%  Is that good enough?
    for (int i = 0; i < (int)pop.size(); i++)
        EXPECT_NEAR(RUNS * SAMPLESIZE / (int)pop.size(), counters[i], ALLOWED_DELTA);
}

TEST(sampleTest, stability)
{
    std::vector<int> pop;
    fillNumbersTo(pop, 100);

    auto res = sample(pop, 30);

    EXPECT_EQ(30, res.size());
    EXPECT_TRUE(std::is_sorted(res.begin(), res.end())); 
}

TEST(sampleTest, sampleVector)
{
    std::vector<int> pop{ 1,2,3,4,5,6,7,8,9,0 };

    auto res = sample(pop, 3);

    static_assert(std::is_same<decltype(res), std::vector<int>>::value, "should return a vector<int>");

    EXPECT_EQ(3, res.size());
}

TEST(sampleTest, sampleVectorTransform)
{
    std::vector<int> pop{ 1,2,3,4,5,6,7,8,9,10 };

    auto res = sample(pop, 7, [](int x) {return x * 100; });

    static_assert(std::is_same<decltype(res), std::vector<int>>::value, "should return a vector<int>");

    EXPECT_EQ(7, res.size());
    // if transform worked, each sample returned is now >= 100
    for (auto s : res)
        EXPECT_LE(100, s);
}

TEST(sampleTest, sampleVectorTransformType)
{
    std::vector<int> pop{ 1,2,3,4,5,6,7,8,9,10 };

    // same test as above,
    // but we transform into floats,
    // so should return vector of floats
    auto res = sample(pop, 7, [](int x) {return x * 100.0F; });

    static_assert(std::is_same<decltype(res), std::vector<float>>::value, "should be a vector of whatever type the transform returns");

    EXPECT_EQ(7, res.size());
    // if transform worked, each sample returned is now >= 100
    for (auto s : res)
        EXPECT_LE(100, s);
}

TEST(sampleTest, sampleVectorTransformExplicitType)
{
    std::vector<int> pop{ 1,2,3,4,5,6,7,8,9,10 };

    // same test as above,
    // but we transform into doubles,
    // so should return vector of doubles

    // this was hoping to be a test of
    //     auto res = sample<double>(pop, 7, [](int x) {return x * 100.0F; });
    // but that doesn't work :-(
    auto res = sample(pop, 7, [](int x) -> double {return x * 100.0F; });

    static_assert(std::is_same<decltype(res), std::vector<double>>::value, "should be a vector of whatever type the transform returns");

    EXPECT_EQ(7, res.size());
    // if transform worked, each sample returned is now >= 100
    for (auto s : res)
        EXPECT_LE(100, s);
}

TEST(sampleTest, downsampleVector)
{
    std::vector<int> zeroToNine{ 0,1,2,3,4,5,6,7,8,9 };

    // how to test???
    // let's mostly rely on actuallyRandom test above
    // and just make sure we eventually see every number;
    // 'eventually' == a very generous 50.

    std::vector<bool> found; found.resize(10, false);
    auto allTrue = [](std::vector<bool> const & f) {
        for (auto b : f) if (!b) return false;
        return true;
    };

    int count = 0;
    do
    {
        auto pop = zeroToNine; // reload
        downsample(pop, 5);
        EXPECT_EQ(5, pop.size()); // should have shrank
        // mark which ones were sampled:
        for (auto n : pop)
            found[n] = true;
        count++;
    } while (!allTrue(found) && count < 50);
    EXPECT_GT(50, count); // expect it didn't take 50 tries to see all numbers
}
