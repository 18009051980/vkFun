#pragma once

#include "VertexAttribute.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <cstddef>
#include <concepts>

#include <utility>


namespace std {
    template<> struct hash<glm::vec3> {
        size_t operator()(glm::vec3 const& vec) const {
            return ((hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1)) >> 1) ^ (hash<float>()(vec.z) << 1);
        }
    };

    template<> struct hash<glm::vec2> {
        size_t operator()(glm::vec2 const& vec) const {
            return (hash<float>()(vec.x) ^ (hash<float>()(vec.y) << 1));
        }
    };
};

template<VertexAttributeBits bits>
struct Vertex : public
    Attribute_Position<has_attribute<bits, VertexAttributeBits::POSITION>(), glm::vec3>,
    Attribute_Color<has_attribute<bits, VertexAttributeBits::COLOR>(), glm::vec3>,
    Attribute_TexCoord<has_attribute<bits, VertexAttributeBits::TEXCOORD>(), glm::vec2>,
    Attribute_Normal<has_attribute<bits, VertexAttributeBits::NORMAL>(), glm::vec3>
{
    Vertex() = default;
    Vertex(const Vertex& other)
    {
        using enum VertexAttributeBits;
        if constexpr (has_attribute<bits, POSITION>()) {
            this->position = other.position;
        }
        if constexpr (has_attribute<bits, COLOR>()) {
            this->color = other.color;
        }
        if constexpr (has_attribute<bits, TEXCOORD>()) {
            this->texCoord = other.texCoord;
        }
        if constexpr (has_attribute<bits, NORMAL>()) {
            this->normal = other.normal;
        }
    }

    Vertex& operator=(const Vertex& other)
    {
        if (this != &other) {
            using enum VertexAttributeBits;
            if constexpr (has_attribute<bits, POSITION>()) {
                this->position = other.position;
            }
            if constexpr (has_attribute<bits, COLOR>()) {
                this->color = other.color;
            }
            if constexpr (has_attribute<bits, TEXCOORD>()) {
                this->texCoord = other.texCoord;
            }
            if constexpr (has_attribute<bits, NORMAL>()) {
                this->normal = other.normal;
            }
        }
        return *this;
    }

    Vertex(Vertex&& other) noexcept
    {
        using enum VertexAttributeBits;
        if constexpr (has_attribute<bits, POSITION>()) {
            this->position = std::move(other.position);
        }
        if constexpr (has_attribute<bits, COLOR>()) {
            this->color = std::move(other.color);
        }
        if constexpr (has_attribute<bits, TEXCOORD>()) {
            this->texCoord = std::move(other.texCoord);
        }
        if constexpr (has_attribute<bits, NORMAL>()) {
            this->normal = std::move(other.normal);
        }
    }

    Vertex& operator=(Vertex&& other) noexcept
    {
        if (this != &other) {
            using enum VertexAttributeBits;
            if constexpr (has_attribute<bits, POSITION>()) {
                this->position = std::move(other.position);
            }
            if constexpr (has_attribute<bits, COLOR>()) {
                this->color = std::move(other.color);
            }
            if constexpr (has_attribute<bits, TEXCOORD>()) {
                this->texCoord = std::move(other.texCoord);
            }
            if constexpr (has_attribute<bits, NORMAL>()) {
                this->normal = std::move(other.normal);
            }
        }
        return *this;
    }

    template<typename... Args>
    Vertex(Args&&... args)
    {
        initialize<0>(std::forward<Args>(args)...);
    }
    template<int N>
    void initialize()
    {

    }
    template<int N, typename T, typename... Args>
    void initialize(T&& t, Args&&... args)
    {
        using enum VertexAttributeBits;
        if constexpr (N == 0) {
            if constexpr (has_attribute<bits, POSITION>()) {
                this->position = std::forward<T>(t);
                initialize<N + 1>(std::forward<Args>(args)...);
            }
            else if constexpr ((std::to_underlying(bits) & ~std::to_underlying(POSITION)))
            {
                initialize<N + 1>(std::forward<T>(t), std::forward<Args>(args)...);
            }
        }
        else if constexpr (N == 1) {
            if constexpr (has_attribute<bits, COLOR>()) {
                this->color = std::forward<T>(t);
                initialize<N + 1>(std::forward<Args>(args)...);
            }
            else if constexpr (std::to_underlying(bits) & ~std::to_underlying(COLOR))
            {
                initialize<N + 1>(std::forward<T>(t), std::forward<Args>(args)...);
            }
        }
        else if constexpr (N == 2) {
            if constexpr (has_attribute<bits, TEXCOORD>()) {
                this->texCoord = std::forward<T>(t);
                initialize<N + 1>(std::forward<Args>(args)...);
            }
            else if constexpr (std::to_underlying(bits) & ~std::to_underlying(TEXCOORD))
            {
                initialize<N + 1>(std::forward<T>(t), std::forward<Args>(args)...);
            }
        }
        else if constexpr (N == 3) {
            if constexpr (has_attribute<bits, NORMAL>()) {
                this->normal = std::forward<T>(t);
            }
        }
        
    }
    ~Vertex() = default;
    static VkVertexInputBindingDescription* getVertexInputBinding()
    {

        static VkVertexInputBindingDescription bindingDesc{
            // .
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        return &bindingDesc;
    }
    static VkVertexInputAttributeDescription* getVertexInputAttributeDescription()
    {
         
        static auto attrDesc = [](){
            std::array<VkVertexInputAttributeDescription, count_attributes<bits>()> descriptions{};
            uint32_t index = 0;
            if constexpr (has_attribute<bits, VertexAttributeBits::POSITION>()) {
                descriptions[index] = VkVertexInputAttributeDescription{
                    .location = index,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, position)
                };
                ++index;
            }
            if constexpr (has_attribute<bits, VertexAttributeBits::COLOR>()) {
                descriptions[index] = VkVertexInputAttributeDescription{
                    .location = index,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, color)
                };
                ++index;
            }
            if constexpr (has_attribute<bits, VertexAttributeBits::TEXCOORD>()) {
                descriptions[index] = VkVertexInputAttributeDescription{
                    .location = index,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, texCoord),
                };
                ++index;
            }
            if constexpr (has_attribute<bits, VertexAttributeBits::NORMAL>()) {
                descriptions[index] = VkVertexInputAttributeDescription{
                    .location = index,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, normal),
                };
                ++index;
            }
            return descriptions;
        }();
        return attrDesc.data();
    }
    static constexpr int getVertexInputAttributeDescriptionCount()
    {
        return count_attributes<bits>();
    }

    bool operator==(const Vertex<bits>& other) const {
        // Compare the attributes based on the enabled bits
        using enum VertexAttributeBits;
        if constexpr (has_attribute<bits, POSITION>()) {
            if (this->position != other.position) return false;
        }
        if constexpr (has_attribute<bits, COLOR>()) {
            if (this->color != other.color) return false;
        }
        if constexpr (has_attribute<bits, TEXCOORD>()) {
            if (this->texCoord != other.texCoord) return false;
        }
        if constexpr (has_attribute<bits, NORMAL>()) {
            if (this->normal != other.normal) return false;
        }

        return true;
    }

    size_t hash() const {
        size_t seed = 0;
        using enum VertexAttributeBits;
        if constexpr (has_attribute<bits, POSITION>()) {
            seed ^= std::hash<glm::vec3>()(this->position) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        if constexpr (has_attribute<bits, COLOR>()) {
            seed ^= std::hash<glm::vec3>()(this->color) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        if constexpr (has_attribute<bits, TEXCOORD>()) {
            seed ^= std::hash<glm::vec2>()(this->texCoord) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        if constexpr (has_attribute<bits, NORMAL>()) {
            seed ^= std::hash<glm::vec3>()(this->normal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

namespace std {
    template<VertexAttributeBits bits>
    struct hash<Vertex<bits>> {
        size_t operator()(const Vertex<bits>& vertex) const {
            return vertex.hash();
        }
    };
};

