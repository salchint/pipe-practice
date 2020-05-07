#include <gtest/gtest.h>
//#include "../src/main.cpp"
#include <vector>
#include <array>

using std::array;
using std::vector;

// Prototyping
vector<int> circularArrayRotation(vector<int> a, int k, vector<int> queries);


// Tests
TEST(RingbufferTests, test00) {
  vector<int> a {1,2,3};

  vector<int> q {0,1,2};

  constexpr int k {2};
  const static vector<int> expected {2,3,1};

  auto res = circularArrayRotation(a, k, q);

  //std::cout << "size = " << a.size() << std::endl;
  //std::cout << "size = " << expected.size() << std::endl;
  std::cout << "size = " << res.size() << std::endl;

  for (auto i{0u}; i<a.size(); ++i) {
    std::cout << "i = " << i << std::endl;
    EXPECT_EQ(expected[i], res[i]);
  }
}

TEST(RingbufferTests, test01) {
  vector<int> a {0,1,2,3,4,5,6,7,8,9};
  vector<int> q {9,9,9,9};
  constexpr int k {9};
  const static vector<int> expected {0,0,0,0};
  auto res = circularArrayRotation(a, k, q);
  for (auto i{0u}; i<q.size(); ++i) {
    EXPECT_EQ(expected[i], res[i]);
  }
}

TEST(RingbufferTests, test02) {
  vector<int> a {0,1,2,3,4,5,6,7,8,9};
  vector<int> q {0};
  constexpr int k {0};
  const static vector<int> expected {0};
  auto res = circularArrayRotation(a, k, q);
  for (auto i{0u}; i<q.size(); ++i) {
    EXPECT_EQ(expected[i], res[i]);
  }
}

TEST(RingbufferTests, test03) {
  vector<int> a {0,1,2,3,4,5,6,7,8,9};
  vector<int> q {0};
  constexpr int k {1};
  const static vector<int> expected {9};
  auto res = circularArrayRotation(a, k, q);
  for (auto i{0u}; i<q.size(); ++i) {
    EXPECT_EQ(expected[i], res[i]);
  }
}

TEST(RingbufferTests, test04) {
  vector<int> a {0,1,2,3,4,5,6,7,8,9};
  vector<int> q {1};
  constexpr int k {1};
  const static vector<int> expected {0};
  auto res = circularArrayRotation(a, k, q);
  for (auto i{0u}; i<q.size(); ++i) {
    EXPECT_EQ(expected[i], res[i]);
  }
}

TEST(RingbufferTests, test05) {
  vector<int> a {0,1,2,3,4,5,6,7,8,9};
  vector<int> q {9,8};
  constexpr int k {1};
  const static vector<int> expected {8,7};
  auto res = circularArrayRotation(a, k, q);
  for (auto i{0u}; i<q.size(); ++i) {
    EXPECT_EQ(expected[i], res[i]);
  }
}

