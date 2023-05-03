#pragma once

#include "includes.h"
#include "Engine.h"
#include <array>

class BoundingBox {
public:
  float left, top, right, bot;

  BoundingBox() : left(0.f), top(0.f), right(0.f), bot(0.f) {};
  BoundingBox(const float left, const float top, const float right, const float bot);
  BoundingBox(const ImVec2& min, const ImVec2& max);

  static BoundingBox Zero();

  const BoundingBox& operator=(const BoundingBox& rhs) {
	left = rhs.left;
	top = rhs.top;
	right = rhs.right;
	bot = rhs.bot;
	return *this;
  }
  const BoundingBox& operator=(const float v) {
	left = v;
	top = v;
	right = v;
	bot = v;
	return *this;
  }
  constexpr bool operator==(const BoundingBox& rhs) const {
	return (
	  this->left == rhs.left
	  && this->top == rhs.top
	  && this->right == rhs.right
	  && this->bot == rhs.bot);
  }
  constexpr bool operator==(const float v) const {
	return (
	  this->left == v
	  && this->top == v
	  && this->right == v
	  && this->bot == v);
  }
  constexpr bool operator!=(const BoundingBox& rhs) const {
	return (
	  this->left != rhs.left
	  && this->top != rhs.top
	  && this->right != rhs.right
	  && this->bot != rhs.bot);
  }
  constexpr bool operator!=(const float v) const {
	return (
	  this->left != v
	  && this->top != v
	  && this->right != v
	  && this->bot != v);
  }

  const BoundingBox operator+(const BoundingBox& rhs) const {
	return BoundingBox(
	  this->left + rhs.left,
	  this->top + rhs.top,
	  this->right + rhs.right,
	  this->bot + rhs.bot);
  }
  const BoundingBox operator-(const BoundingBox& rhs) const {
	return BoundingBox(
	  this->left - rhs.left,
	  this->top - rhs.top,
	  this->right - rhs.right,
	  this->bot - rhs.bot);
  }
  const BoundingBox operator*(const BoundingBox& rhs) const {
	return BoundingBox(
	  this->left * rhs.left,
	  this->top * rhs.top,
	  this->right * rhs.right,
	  this->bot * rhs.bot);
  }
  const BoundingBox operator/(const BoundingBox& rhs) {
	return rhs != 0.f ?
	  BoundingBox(
		this->left / rhs.left,
		this->top / rhs.top,
		this->right / rhs.right,
		this->bot / rhs.bot) : *this = -1.f;
  }

  const BoundingBox& operator+=(const BoundingBox& rhs) {
	left += rhs.left;
	top += rhs.top;
	right += rhs.right;
	bot += rhs.bot;
	return *this;
  }
  const BoundingBox& operator-=(const BoundingBox& rhs) {
	left -= rhs.left;
	top -= rhs.top;
	right -= rhs.right;
	bot -= rhs.bot;
	return *this;
  }
  const BoundingBox& operator*=(const BoundingBox& rhs) {
	left *= rhs.left;
	top *= rhs.top;
	right *= rhs.right;
	bot *= rhs.bot;
	return *this;
  }
  const BoundingBox& operator/=(const BoundingBox& rhs) {
	if (rhs != 0.f) {
	  left /= rhs.left;
	  top /= rhs.top;
	  right /= rhs.right;
	  bot /= rhs.bot;
	}
	else *this = -1.f;
	return *this;
  }

  const BoundingBox operator+(const float v) const {
	return BoundingBox(
	  this->left + v,
	  this->top + v,
	  this->right + v,
	  this->bot + v);
  }
  const BoundingBox operator-(const float v) const {
	return BoundingBox(
	  this->left - v,
	  this->top - v,
	  this->right - v,
	  this->bot - v);
  }
  const BoundingBox operator*(const float v) const {
	return BoundingBox(
	  this->left * v,
	  this->top * v,
	  this->right * v,
	  this->bot * v);
  }
  const BoundingBox operator/(const float v) {
	return v != 0.f ?
	  BoundingBox(
		this->left / v,
		this->top / v,
		this->right / v,
		this->bot / v) : *this = -1.f;
  }

  const BoundingBox& operator+=(const float v) {
	left += v;
	top += v;
	right += v;
	bot += v;
	return *this;
  }
  const BoundingBox& operator-=(const float v) {
	left -= v;
	top -= v;
	right -= v;
	bot -= v;
	return *this;
  }
  const BoundingBox& operator*=(const float v) {
	left *= v;
	top *= v;
	right *= v;
	bot *= v;
	return *this;
  }
  const BoundingBox& operator/=(const float v) {
	if (v != 0.f) {
	  left /= v;
	  top /= v;
	  right /= v;
	  bot /= v;
	}
	else *this = -1.f;
	return *this;
  }

public:
  ImVec2		GetMin() const;
  ImVec2		GetMax() const;
  ImVec2		GetMinBot() const;
  ImVec2		GetMaxTop() const;
  ImVec2		GetSize() const;
  ImVec2		GetCenter() const;

  bool		IsValid() const;
};

class BoundingBox3D {
public:
  Vector min, max;
  std::array<Vector, 8> points;
  BoundingBox3D() { min = points[0]; max = points[4]; };
  Vector GetCenter() {
	return Vector(
	  min.x + ((max.x - min.x) * 0.5f),
	  min.y + ((max.y - min.y) * 0.5f),
	  min.z + ((max.z - min.z) * 0.5f)
	);
  }
};
