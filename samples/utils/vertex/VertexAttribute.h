#pragma once

#include <utility>

#include <cstdint>
enum class VertexAttributeBits : std::uint32_t
{
    POSITION = 1 << 0,
    COLOR = 1 << 1,
    TEXCOORD = 1 << 2,
    NORMAL = 1 << 3,
};

template <VertexAttributeBits bits, VertexAttributeBits attribute>
constexpr bool has_attribute() {
    return (std::to_underlying(bits) & std::to_underlying(attribute)) != 0;
}

template <VertexAttributeBits bits>
constexpr int count_attributes() {
    int count = 0;
    if constexpr (has_attribute<bits, VertexAttributeBits::POSITION>()) count++;
    if constexpr (has_attribute<bits, VertexAttributeBits::COLOR>())    count++;
    if constexpr (has_attribute<bits, VertexAttributeBits::TEXCOORD>()) count++;
    if constexpr (has_attribute<bits, VertexAttributeBits::NORMAL>())   count++;
    return count;
}

template <bool, typename T>
struct Attribute_Position {};

template <typename T>
struct Attribute_Position<true, T> {
     T position;
};

template <bool, typename T>
struct Attribute_Color {};

template <typename T>
struct Attribute_Color<true, T> {
    T color;
};

template <bool, typename T>
struct Attribute_TexCoord {};

template <typename T>
struct Attribute_TexCoord<true, T> {
     T texCoord;
};

template <bool, typename T>
struct Attribute_Normal {};

template <typename T>
struct Attribute_Normal<true, T> {
     T normal;
};
