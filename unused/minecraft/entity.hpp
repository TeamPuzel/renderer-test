// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <vector>
export module Minecraft:Entity;

import Core;
import Engine;

import :Utility;
import :Block;

namespace minecraft {
    export class World;
    export constexpr auto entityRange = 4.5;

    struct EntityVertex final {
        struct Position final { Float x, y, z; } position;
        struct TextureCoordinates final { Float u, v; } textureCoordinates;
    };

    export class Entity {
    public:
        World * world { nullptr };
        Position position {};
        Orientation orientation {};

        [[nodiscard]] auto isPrimary() const -> Bool;

        Entity() = delete;
        constexpr explicit Entity(World * world) : world { world } {}
        constexpr explicit Entity(World * world, Position position) : world { world }, position { position } {}

        virtual auto update() -> Void = 0;
        virtual auto mesh(std::vector<EntityVertex>& vertices) const noexcept -> Void {}
        virtual ~Entity() = default;
    };

    export class Player final: public Entity {
    public:
        using Entity::Entity;
        auto update() -> void override;
    };

    export class BlockParticle final: public Entity {
    public:
        using Entity::Entity;

        auto update() -> void override {

        }
    };
}
