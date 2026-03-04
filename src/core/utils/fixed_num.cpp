#include "fixed_num.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

/* static */ Fixed Fixed::str_to_fixed(const std::string &str) {
  size_t scale = 0;
  if (size_t dotPos = str.find('.'); dotPos != std::string::npos)
    scale = str.length() - dotPos - 1;

  return Fixed(str, scale);
}

Fixed::Fixed() : m_value(0), m_scale(0) {}

Fixed::Fixed(int64_t val, int scale) : m_value(val) {
  m_scale = _constraint_scale(scale);
}

Fixed::Fixed(const std::string &str) {
  Fixed fixed = str_to_fixed(str);
  m_scale = fixed.scale();
  m_value = fixed.value();
}

Fixed::Fixed(const std::string &str, int scale) {
  m_scale = _constraint_scale(scale);
  m_value = _value_from_string(str, m_scale);
}

Fixed::Fixed(double val, int scale) {
  m_scale = _constraint_scale(scale);
  m_value = _value_from_double(val, m_scale);
}

Fixed::Fixed(int val, int scale) : m_value(val) {
  m_scale = _constraint_scale(scale);
}

int Fixed::_constraint_scale(int scale) {
  if (scale >= 0 && scale <= MAX_SCALE)
    return scale;

  if (scale >= MAX_SCALE)
    return MAX_SCALE;

  return 0;
}

int64_t Fixed::_value_from_string(const std::string &str, int scale) {
  // Parse string manually to avoid floating-point precision loss
  bool isNegative = false;
  size_t startPos = 0;
  
  if (!str.empty() && str[0] == '-') {
    isNegative = true;
    startPos = 1;
  }
  
  size_t dotPos = str.find('.');
  
  int64_t integerPart = 0;
  int64_t fractionalPart = 0;
  int fractionalDigits = 0;
  
  if (dotPos == std::string::npos) {
    // No decimal point
    for (size_t i = startPos; i < str.length(); ++i) {
      if (str[i] >= '0' && str[i] <= '9') {
        integerPart = integerPart * 10 + (str[i] - '0');
      }
    }
  } else {
    // Parse integer part
    for (size_t i = startPos; i < dotPos; ++i) {
      if (str[i] >= '0' && str[i] <= '9') {
        integerPart = integerPart * 10 + (str[i] - '0');
      }
    }
    
    // Parse fractional part
    for (size_t i = dotPos + 1; i < str.length(); ++i) {
      if (str[i] >= '0' && str[i] <= '9') {
        fractionalPart = fractionalPart * 10 + (str[i] - '0');
        fractionalDigits++;
      }
    }
  }
  
  // Calculate multiplier for integer part
  int64_t multiplier = 1;
  for (int i = 0; i < scale; ++i) {
    multiplier *= 10;
  }
  
  int64_t result = integerPart * multiplier;
  
  // Add fractional part adjusted to the target scale
  if (fractionalDigits > 0) {
    if (fractionalDigits < scale) {
      // Need to multiply fractional part
      for (int i = fractionalDigits; i < scale; ++i) {
        fractionalPart *= 10;
      }
      result += fractionalPart;
    } else if (fractionalDigits > scale) {
      // Need to divide fractional part
      for (int i = scale; i < fractionalDigits; ++i) {
        fractionalPart /= 10;
      }
      result += fractionalPart;
    } else {
      result += fractionalPart;
    }
  }
  
  return isNegative ? -result : result;
}

int64_t Fixed::_value_from_double(double val, int scale) {
  double multiplier = 1.0;
  for (int i = 0; i < scale; ++i) {
    multiplier *= 10.0;
  }
  return static_cast<int64_t>(val * multiplier);
}

std::string Fixed::to_string() const {
  // Calculate base using integer multiplication to avoid floating-point errors
  int64_t base = 1;
  for (int i = 0; i < m_scale; ++i) {
    base *= 10;
  }
  
  const bool isNegative = (m_value < 0);
  const int64_t absValue = std::llabs(m_value);

  const int64_t main = absValue / base;
  const int64_t frac = absValue % base;

  const int zeroing = (m_scale - std::to_string(frac).length());
  const std::string sign = isNegative ? "-" : "";

  return sign + std::to_string(main) + "." +
         std::string(zeroing > 0 ? zeroing : 0, '0') + std::to_string(frac);
}

void Fixed::operator+=(const Fixed &other) {
  Fixed tmp = _oper_add(other);
  m_value = tmp.m_value;
  m_scale = tmp.m_scale;
}

void Fixed::operator-=(const Fixed &other) {
  Fixed tmp = _oper_sub(other);
  m_value = tmp.m_value;
  m_scale = tmp.m_scale;
}

void Fixed::operator*=(const Fixed &other) {
  Fixed tmp = _oper_mul(other);
  m_value = tmp.m_value;
  m_scale = tmp.m_scale;
}

void Fixed::operator/=(const Fixed &other) {
  Fixed tmp = _oper_div(other);
  m_value = tmp.m_value;
  m_scale = tmp.m_scale;
}

Fixed Fixed::operator+(const Fixed &other) const { return _oper_add(other); }

Fixed Fixed::operator-(const Fixed &other) const { return _oper_sub(other); }

Fixed Fixed::operator*(const Fixed &other) const { return _oper_mul(other); }

Fixed Fixed::operator/(const Fixed &other) const { return _oper_div(other); }

bool Fixed::operator>(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value > bigger_scale.m_value
                        : bigger_scale.m_value > normalized.m_value;
}
bool Fixed::operator<(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value < bigger_scale.m_value
                        : bigger_scale.m_value < normalized.m_value;
}
bool Fixed::operator==(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value == bigger_scale.m_value
                        : bigger_scale.m_value == normalized.m_value;
}
bool Fixed::operator!=(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value != bigger_scale.m_value
                        : bigger_scale.m_value != normalized.m_value;
}
bool Fixed::operator<=(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value <= bigger_scale.m_value
                        : bigger_scale.m_value <= normalized.m_value;
}
bool Fixed::operator>=(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  return (needToSwitch) ? normalized.m_value >= bigger_scale.m_value
                        : bigger_scale.m_value >= normalized.m_value;
}

Fixed Fixed::_oper_add(const Fixed &other) const {
  const Fixed &bigger_scale = (m_scale >= other.m_scale) ? *this : other;

  const Fixed &smaller_scale = (m_scale < other.m_scale) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  __int128 result = (__int128)bigger_scale.m_value + normalized.m_value;

  if (result > std::numeric_limits<int64_t>::max() ||
      result < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)result, bigger_scale.m_scale);
}

Fixed Fixed::_oper_sub(const Fixed &other) const {
  bool needToSwitch = (m_scale < other.m_scale);

  const Fixed &bigger_scale = (!needToSwitch) ? *this : other;

  const Fixed &smaller_scale = (needToSwitch) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  __int128 result;

  // We suppose that bigger scale is *this by default
  // for the arithmetics to be correct,
  // we should keep the order of nums right
  if (needToSwitch)
    result = (__int128)normalized.m_value - bigger_scale.m_value;
  else
    result = (__int128)bigger_scale.m_value - normalized.m_value;

  if (result > std::numeric_limits<int64_t>::max() ||
      result < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)result, bigger_scale.m_scale);
}

Fixed Fixed::_oper_mul(const Fixed &other) const {
  const int resultScale = std::max(m_scale, other.m_scale);
  const int divisorPow = (m_scale + other.m_scale) - resultScale;

  __int128 product = (__int128)m_value * other.m_value;

  // Scale correction: divide by 10^(m_scale + other_scale - resultScale)
  for (int i = 0; i < divisorPow; ++i) {
    product /= 10;
  }

  if (product > std::numeric_limits<int64_t>::max() ||
      product < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)product, resultScale);
}

Fixed Fixed::_oper_div(const Fixed &other) const {
  if (m_value == 0 || other.m_value == 0)
    return Fixed{};

  const int resultScale = std::max(m_scale, other.m_scale);

  __int128 numerator = (__int128)m_value;
  const int scaleAdjust = other.m_scale + resultScale - m_scale;

  for (int i = 0; i < scaleAdjust; ++i) {
    numerator *= 10;
  }

  __int128 result = numerator / other.m_value;

  if (result > std::numeric_limits<int64_t>::max() ||
      result < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)result, resultScale);
}

Fixed Fixed::_normalize_scale(const Fixed &other, int scale) const {
  int resultScale = scale;
  if (other.m_scale > scale) {
    resultScale = other.m_scale;
  }

  int scaleDiff = resultScale - other.m_scale;

  int64_t multiplier = 1;

  for (size_t i = 0; i < scaleDiff; ++i) {
    multiplier *= 10;
  }

  __int128 scaledNum = (__int128)other.m_value * multiplier;

  return Fixed((int64_t)scaledNum, resultScale);
}
