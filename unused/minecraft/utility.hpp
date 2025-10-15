// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <cmath>
export module Minecraft:Utility;

import Core;

namespace minecraft {
    struct Orientation final {
        core::Angle<Float> pitch {}, yaw {}, roll {};

        constexpr auto rotatePitch(core::Angle<Float> angle) { this->pitch += angle; }
        constexpr auto rotateYaw(core::Angle<Float> angle) { this->yaw += angle; }
        constexpr auto rotateRoll(core::Angle<Float> angle) { this->roll += angle; }

        constexpr auto rotatedPitch(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.rotatePitch(angle);
            return self;
        }
        constexpr auto rotatedYaw(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.rotateYaw(angle);
            return self;
        }
        constexpr auto rotatedRoll(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.rotateRoll(angle);
            return self;
        }

        constexpr auto settingPitch(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.pitch = angle;
            return self;
        }

        constexpr auto settingYaw(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.yaw = angle;
            return self;
        }

        constexpr auto settingRoll(this Orientation self, core::Angle<Float> angle) -> Orientation {
            self.roll = angle;
            return self;
        }
    };

    struct Position final {
        Float x {}, y {}, z {};

        [[gnu::pure]]
        auto movedInDirectionAtSpeed(this Position self, Orientation orientation, Float speed) -> Position {
            return {
                .x = self.x + orientation.pitch.cos() * orientation.yaw.sin() * speed,
                .y = self.y - orientation.pitch.sin()                         * speed,
                .z = self.z + orientation.pitch.cos() * orientation.yaw.cos() * speed
            };
        }

        auto moveInDirectionAtSpeed(Orientation orientation, Float speed) -> Void {
            *this = movedInDirectionAtSpeed(orientation, speed);
        }

        [[gnu::pure]]
        auto distanceTo(this const auto self, Position other) -> Float {
            return std::sqrt(
                std::pow(self.x - other.x, 2) +
                std::pow(self.y - other.y, 2) +
                std::pow(self.z - other.z, 2)
            );
        }

        constexpr auto addingX(this Position self, Float value) -> Position {
            self.x += value;
            return self;
        }

        constexpr auto addingY(this Position self, Float value) -> Position {
            self.y += value;
            return self;
        }

        constexpr auto addingZ(this Position self, Float value) -> Position {
            self.z += value;
            return self;
        }

        constexpr auto settingX(this Position self, Float value) -> Position {
            self.x = value;
            return self;
        }

        constexpr auto settingY(this Position self, Float value) -> Position {
            self.y = value;
            return self;
        }

        constexpr auto settingZ(this Position self, Float value) -> Position {
            self.z = value;
            return self;
        }
    };
}
