#include "fixed_num.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

Fixed::Fixed(int64_t val, int scale) : m_value(val) {
  m_scale = _constraint_scale(scale);
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
  double d = std::stod(str);
  return static_cast<int64_t>(std::round(d * std::pow(10, scale)));
}

int64_t Fixed::_value_from_double(double val, int scale) {
  return static_cast<int64_t>(std::round(val * std::pow(10, scale)));
}

std::string Fixed::to_string() const {
  int64_t base = static_cast<int64_t>(std::pow(10, m_scale));
  int64_t main = m_value / base;
  int64_t frac = std::llabs(m_value % base);

  int zeroing = (m_scale - std::to_string(frac).length());

  return std::to_string(main) + "." +
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
  if (m_scale == other.m_scale)
    return Fixed(m_value + other.m_value, m_scale);

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
  if (m_scale == other.m_scale)
    return Fixed(m_value - other.m_value, m_scale);

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
  if (m_scale == other.m_scale)
    return Fixed(m_value * other.m_value, m_scale);

  const Fixed &bigger_scale = (m_scale >= other.m_scale) ? *this : other;

  const Fixed &smaller_scale = (m_scale < other.m_scale) ? *this : other;

  Fixed normalized = _normalize_scale(smaller_scale, bigger_scale.m_scale);

  __int128 result = (__int128)bigger_scale.m_value * normalized.m_value;

  if (result > std::numeric_limits<int64_t>::max() ||
      result < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)result, bigger_scale.m_scale);
}

Fixed Fixed::_oper_div(const Fixed &other) const {
  if (m_value == 0 || other.m_value == 0)
    return Fixed{};

  const Fixed &bigger_scale = (m_scale >= other.m_scale) ? *this : other;

  const Fixed &smaller_scale = (m_scale < other.m_scale) ? *this : other;

  int resultScale = bigger_scale.m_scale;

  __int128 numerator = (__int128)m_value;

  int scaleAdjust = resultScale + other.m_scale - m_scale;

  for (int i = 0; i < scaleAdjust; ++i)
    numerator *= 10;

  __int128 result = numerator / other.m_value;

  if (result > std::numeric_limits<int64_t>::max() ||
      result < std::numeric_limits<int64_t>::min()) {
    return Fixed{};
  }

  return Fixed((int64_t)result, bigger_scale.m_scale);
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
