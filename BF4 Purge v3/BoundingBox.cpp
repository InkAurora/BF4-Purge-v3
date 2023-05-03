#include "BoundingBox.h"

BoundingBox::BoundingBox(const float left, const float top, const float right, const float bot) {
  this->left = left;
  this->bot = bot;
  this->right = right;
  this->top = top;
}

BoundingBox::BoundingBox(const ImVec2& min, const ImVec2& max) {
  this->left = min.x;
  this->top = min.y;
  this->right = max.x;
  this->bot = max.y;
}

BoundingBox BoundingBox::Zero() { return BoundingBox(0.f, 0.f, 0.f, 0.f); }

ImVec2 BoundingBox::GetMin() const { return ImVec2(left, top); }

ImVec2 BoundingBox::GetMax() const { return ImVec2(right, bot); }

ImVec2 BoundingBox::GetMinBot() const { return ImVec2(left, bot); }

ImVec2 BoundingBox::GetMaxTop() const { return ImVec2(right, top); }

ImVec2 BoundingBox::GetSize() const { return ImVec2(fabsf(right - left), fabsf(bot - top)); }

ImVec2 BoundingBox::GetCenter() const { return ImVec2(GetSize().x / 2.f, GetSize().y / 2.f); }

bool BoundingBox::IsValid() const { return *this != 0.f; }
