// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <tuple>
module Minecraft:Entity.Update;

import Engine;

import :Entity;
import :World;

using namespace minecraft;

auto Entity::isPrimary() const -> Bool { return this == this->world->primary(); }

auto Player::update() -> void {
    using engine::Key;
    using engine::key;
    using engine::mouseButtonP;
    using core::clamp;
    using core::deg;

    const auto delta = this->world->delta();

    const auto mouse = engine::relativeMouse();
    this->orientation.yaw += deg(mouse.x / 10.0);
    this->orientation.pitch += deg(mouse.y / 10.0);
    this->orientation.pitch = clamp(this->orientation.pitch, deg(-90), deg(90));

    if (key(Key::w)) this->position.moveInDirectionAtSpeed(
        this->orientation
            .settingPitch(deg(0)),
        0.01 * delta
    );
    if (key(Key::a)) this->position.moveInDirectionAtSpeed(
        this->orientation
            .rotatedYaw(deg(270))
            .settingPitch(deg(0)),
        0.01 * delta
    );
    if (key(Key::s)) this->position.moveInDirectionAtSpeed(
        this->orientation
            .rotatedYaw(deg(180))
            .settingPitch(deg(0)),
        0.01 * delta
    );
    if (key(Key::d)) this->position.moveInDirectionAtSpeed(
        this->orientation
            .rotatedYaw(deg(90))
            .settingPitch(deg(0)),
        0.01 * delta
    );
    if (key(Key::space))     this->position.y += 0.01 * delta;
    if (key(Key::leftShift)) this->position.y -= 0.01 * delta;

    if (mouseButtonP().left) {
        auto [block, position] = this->world->castRayAtMaybeBlock(
            this->position, this->orientation, entityRange
        );
        if (block) (Void) this->world->tryBreakBlockAt(position.x, position.y, position.z);
    }
}
