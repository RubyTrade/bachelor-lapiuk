#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "core/utils/fixed_num.hpp"

TEST(FixedNum, DefaultIsZero) {
  Fixed v;
  EXPECT_EQ(v.value(), 0);
  EXPECT_EQ(v.scale(), 0);
  EXPECT_EQ(v.to_string(), "0.0");
}

TEST(FixedNum, ScaleClampsToValidRange) {
  Fixed neg(1, -999);
  EXPECT_EQ(neg.scale(), 0);

  Fixed big(1, Fixed::MAX_SCALE + 100);
  EXPECT_EQ(big.scale(), Fixed::MAX_SCALE);
}

TEST(FixedNum, ToStringPadsZerosAndHandlesNegative) {
  Fixed a(static_cast<int64_t>(1), 3);
  EXPECT_EQ(a.to_string(), "0.001");

  Fixed b(static_cast<int64_t>(-123), 2);
  EXPECT_EQ(b.to_string(), "-1.23");

  // critical edge: negative values with integer-part == 0
  Fixed c(static_cast<int64_t>(-10), 3); // -0.010
  EXPECT_EQ(c.to_string(), "-0.010");
}

TEST(FixedNum, StrToFixedUsesFractionLengthAsScale) {
  Fixed v = Fixed::str_to_fixed("12.3400");
  EXPECT_EQ(v.scale(), 4);
  EXPECT_EQ(v.to_string(), "12.3400");

  Fixed w = Fixed::str_to_fixed("0");
  EXPECT_EQ(w.scale(), 0);
  EXPECT_EQ(w.to_string(), "0.0");

  Fixed n = Fixed::str_to_fixed("-0.010");
  EXPECT_EQ(n.scale(), 3);
  EXPECT_EQ(n.to_string(), "-0.010");
}

TEST(FixedNum, ConstructFromStringRespectsScaleArgument) {
  Fixed v("1.23", 2);
  EXPECT_EQ(v.scale(), 2);
  EXPECT_EQ(v.to_string(), "1.23");

  Fixed w("-1.2", 3);
  EXPECT_EQ(w.scale(), 3);
  // note: value comes from stod*10^scale
  EXPECT_EQ(w.to_string(), "-1.200");
}

TEST(FixedNum, ConstructFromDoubleTruncatesTowardZero) {
  Fixed v(1.239, 2);
  // implementation uses static_cast<int64_t>(val * 10^scale)
  // so this truncates (not rounds).
  EXPECT_EQ(v.to_string(), "1.23");

  Fixed n(-1.239, 2);
  EXPECT_EQ(n.to_string(), "-1.23");
}

TEST(FixedNum, AddSubAcrossScalesNormalizesToMaxScale) {
  Fixed a(static_cast<int64_t>(12), 1); // 1.2
  Fixed b(static_cast<int64_t>(3), 2);  // 0.03

  Fixed c = a + b;
  EXPECT_EQ(c.scale(), 2);
  EXPECT_EQ(c.to_string(), "1.23");

  Fixed d = b - a;
  EXPECT_EQ(d.scale(), 2);
  EXPECT_EQ(d.to_string(), "-1.17");
}

TEST(FixedNum, ComparisonsAcrossScales) {
  Fixed a(static_cast<int64_t>(120), 2); // 1.20
  Fixed b(static_cast<int64_t>(12), 1);  // 1.2

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a >= b);
  EXPECT_TRUE(a <= b);

  Fixed c(static_cast<int64_t>(119), 2); // 1.19
  EXPECT_TRUE(c < b);
  EXPECT_TRUE(b > c);
}

TEST(FixedNum, InPlaceOpsKeepScaleContract) {
  Fixed a(static_cast<int64_t>(12), 1); // 1.2
  Fixed b(static_cast<int64_t>(3), 2);  // 0.03

  a += b;
  EXPECT_EQ(a.to_string(), "1.23");

  a -= b;
  EXPECT_EQ(a.to_string(), "1.20");
}

TEST(FixedNum, MulDivScaleContract) {
  Fixed a(static_cast<int64_t>(150), 2); // 1.50
  Fixed b(static_cast<int64_t>(2), 0);   // 2

  Fixed c = a * b;
  EXPECT_EQ(c.scale(), 2);
  EXPECT_EQ(c.to_string(), "3.00");

  Fixed d = c / b;
  EXPECT_EQ(d.scale(), 2);
  EXPECT_EQ(d.to_string(), "1.50");
}

TEST(FixedNum, DivisionAcrossScalesAndSigns) {
  Fixed a(static_cast<int64_t>(12), 1); // 1.2
  Fixed b(static_cast<int64_t>(30), 2); // 0.30

  Fixed c = a / b;
  EXPECT_EQ(c.scale(), 2);
  EXPECT_EQ(c.to_string(), "4.00");

  Fixed n(static_cast<int64_t>(-300), 2); // -3.00
  Fixed d(static_cast<int64_t>(2), 0);    // 2
  Fixed e = n / d;
  EXPECT_EQ(e.to_string(), "-1.50");
}

TEST(FixedNum, AddAndMulCommutativityForExactCases) {
  Fixed a(static_cast<int64_t>(12), 1); // 1.2
  Fixed b(static_cast<int64_t>(3), 2);  // 0.03

  EXPECT_EQ((a + b).to_string(), (b + a).to_string());

  Fixed x(static_cast<int64_t>(12), 1); // 1.2
  Fixed y(static_cast<int64_t>(50), 2); // 0.50
  EXPECT_EQ((x * y).to_string(), (y * x).to_string());
}

TEST(FixedNum, IdentityElements) {
  Fixed a(static_cast<int64_t>(123), 2);
  Fixed zero;
  Fixed one(static_cast<int64_t>(1), 0);

  EXPECT_EQ((a + zero).to_string(), a.to_string());
  EXPECT_EQ((a - zero).to_string(), a.to_string());
  EXPECT_EQ((a * one).to_string(), a.to_string());
}

TEST(FixedNum, MultiplySameScale) {
  Fixed a(static_cast<int64_t>(120), 2); // 1.20
  Fixed b(static_cast<int64_t>(50), 2);  // 0.50

  Fixed c = a * b;
  EXPECT_EQ(c.scale(), 2);
  EXPECT_EQ(c.to_string(), "0.60");
}

TEST(FixedNum, DivideByZeroReturnsDefault) {
  Fixed a(static_cast<int64_t>(100), 2);
  Fixed zero;

  Fixed c = a / zero;
  EXPECT_EQ(c.value(), 0);
  EXPECT_EQ(c.scale(), 0);
  EXPECT_EQ(c.to_string(), "0.0");
}

TEST(FixedNum, OverflowContractsReturnDefault) {
  Fixed addA(std::numeric_limits<int64_t>::max(), 0);
  Fixed addB(1, 0);
  Fixed addC = addA + addB;
  EXPECT_EQ(addC.value(), 0);
  EXPECT_EQ(addC.scale(), 0);

  Fixed subA(std::numeric_limits<int64_t>::min(), 0);
  Fixed subB(1, 0);
  Fixed subC = subA - subB;
  EXPECT_EQ(subC.value(), 0);
  EXPECT_EQ(subC.scale(), 0);

  Fixed mulA(std::numeric_limits<int64_t>::max() / 2 + 10, 0);
  Fixed mulB(3, 0);
  Fixed mulC = mulA * mulB;
  EXPECT_EQ(mulC.value(), 0);
  EXPECT_EQ(mulC.scale(), 0);
}

TEST(FixedNum, NegativeScaleClampedToZero) {
  Fixed f(100, -5);
  EXPECT_EQ(f.scale(), 0);
}

TEST(FixedNum, VeryLargeScaleClamped) {
  Fixed f(100, 1000);
  EXPECT_LE(f.scale(), Fixed::MAX_SCALE);
}

TEST(FixedNum, StringWithNoDecimalPoint) {
  Fixed f = Fixed::str_to_fixed("42");
  EXPECT_EQ(f.to_string(), "42.0");
}

TEST(FixedNum, StringWithLeadingZeros) {
  Fixed f = Fixed::str_to_fixed("00042.500");
  EXPECT_EQ(f.to_string(), "42.500");
}

TEST(FixedNum, NegativeZero) {
  Fixed f(static_cast<int64_t>(-0), 2);
  EXPECT_EQ(f.to_string(), "0.00");
}

TEST(FixedNum, VerySmallNumber) {
  Fixed f = Fixed::str_to_fixed("0.00000001");
  EXPECT_EQ(f.to_string(), "0.00000001");
}

TEST(FixedNum, VeryLargeNumber) {
  Fixed f = Fixed::str_to_fixed("9999999999.99");
  EXPECT_EQ(f.to_string(), "9999999999.99");
}

TEST(FixedNum, ComparisonWithDifferentScales) {
  Fixed a(static_cast<int64_t>(1000), 3); // 1.000
  Fixed b(static_cast<int64_t>(10), 1);   // 1.0

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(a > b);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a >= b);
}

TEST(FixedNum, SubtractionResultingInNegative) {
  Fixed a(static_cast<int64_t>(100), 2); // 1.00
  Fixed b(static_cast<int64_t>(300), 2); // 3.00

  Fixed c = a - b;
  EXPECT_EQ(c.to_string(), "-2.00");
}

TEST(FixedNum, MultiplicationByNegative) {
  Fixed a(static_cast<int64_t>(200), 2);  // 2.00
  Fixed b(static_cast<int64_t>(-150), 2); // -1.50

  Fixed c = a * b;
  EXPECT_EQ(c.to_string(), "-3.00");
}

TEST(FixedNum, DivisionByNegative) {
  Fixed a(static_cast<int64_t>(600), 2);  // 6.00
  Fixed b(static_cast<int64_t>(-200), 2); // -2.00

  Fixed c = a / b;
  EXPECT_EQ(c.to_string(), "-3.00");
}

TEST(FixedNum, ChainedOperations) {
  Fixed a(static_cast<int64_t>(100), 1); // 10.0
  Fixed b(static_cast<int64_t>(20), 1);  // 2.0
  Fixed c(static_cast<int64_t>(30), 1);  // 3.0

  Fixed result = (a + b) * c / b;
  EXPECT_EQ(result.to_string(), "18.0");
}
