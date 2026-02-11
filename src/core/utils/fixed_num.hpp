#ifndef FIXED_NUM_HPP
#define FIXED_NUM_HPP

#include <cstdint>
#include <string>

class Fixed {
public:
  inline static constexpr int MAX_SCALE = 10;

  Fixed(int64_t val = 0, int scale = 0);
  Fixed(const std::string &str, int scale = 0);
  Fixed(double val, int scale = 0);
  Fixed(int val, int scale = 0);

  ~Fixed() = default;
  Fixed(Fixed &&) = default;
  Fixed(const Fixed &) = default;
  Fixed &operator=(Fixed &&) = default;
  Fixed &operator=(const Fixed &) = default;

  void operator+=(const Fixed &);
  void operator-=(const Fixed &);
  void operator*=(const Fixed &);
  void operator/=(const Fixed &);

  Fixed operator+(const Fixed &) const;
  Fixed operator-(const Fixed &) const;
  Fixed operator*(const Fixed &) const;
  Fixed operator/(const Fixed &) const;

  bool operator>(const Fixed &) const;
  bool operator<(const Fixed &) const;
  bool operator==(const Fixed &) const;
  bool operator!=(const Fixed &) const;
  bool operator<=(const Fixed &) const;
  bool operator>=(const Fixed &) const;

public:
  int scale() const { return m_scale; }
  int64_t value() const { return m_value; }

  void change_scale_to(int newScale) { m_scale = newScale; }

  std::string to_string() const;

private:
  int64_t _value_from_string(const std::string &str, int scale);
  int64_t _value_from_double(double val, int scale);

private:
  int _constraint_scale(int scale);

  Fixed _oper_add(const Fixed &other) const;
  Fixed _oper_sub(const Fixed &other) const;
  Fixed _oper_mul(const Fixed &other) const;
  Fixed _oper_div(const Fixed &other) const;

  Fixed _normalize_scale(const Fixed &other, int scale) const;

private:
  int64_t m_value;
  int m_scale;
};

#endif // FIXED_NUM_HPP
