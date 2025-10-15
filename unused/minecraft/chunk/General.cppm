// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <utility>
#include <cstring>
#include <tuple>
#include <cmath>
#include <algorithm>
#include <queue>
#include <coroutine>
module Minecraft:Chunk.General;

import :Chunk;
import :World;

using namespace minecraft;

auto Chunk::faces(Int x, Int y, Int z) const -> FaceMask {
    const auto gx = x + this->x * chunkSide;
    const auto gy = y;
    const auto gz = z + this->z * chunkSide;

    const auto current = this->maybeGetBlockAt(x, y, z);
    if (not current) std::unreachable();

    if (not current->traits.hasTransparency) {
        return FaceMask {
            .north = not this->world->isOpaqueAt(gx,     gy,     gz + 1),
            .south = not this->world->isOpaqueAt(gx,     gy,     gz - 1),
            .east  = not this->world->isOpaqueAt(gx + 1, gy,     gz    ),
            .west  = not this->world->isOpaqueAt(gx - 1, gy,     gz    ),
            .up    = not this->world->isOpaqueAt(gx,     gy + 1, gz    ),
            .down  = not this->world->isOpaqueAt(gx,     gy - 1, gz    )
        };
    } else if (not current->traits.bunchTogether) {
        const auto splitMesh = [&](Int cx, Int cy, Int cz, Bool yieldZFighting) -> Bool {
            const auto other = this->world->maybeGetBlockAt(cx, cy, cz);
            if (other == nullptr) return true;

            if (yieldZFighting and current == other) return false;

            return other->traits.hasTransparency;
        };

        return {
            .north = splitMesh(gx,     gy,     gz + 1, true ),
            .south = splitMesh(gx,     gy,     gz - 1, false),
            .east  = splitMesh(gx + 1, gy,     gz,     true ),
            .west  = splitMesh(gx - 1, gy,     gz,     false),
            .up    = splitMesh(gx,     gy + 1, gz,     true ),
            .down  = splitMesh(gx,     gy - 1, gz,     false)
        };
    } else {
        const auto splitMesh = [&](Int cx, Int cy, Int cz) -> Bool {
            const auto other = this->world->maybeGetBlockAt(cx, cy, cz);
            if (other == nullptr) return false;

            return other != current and other->traits.hasTransparency;
        };

        return {
            .north = splitMesh(gx,     gy,     gz + 1),
            .south = splitMesh(gx,     gy,     gz - 1),
            .east  = splitMesh(gx + 1, gy,     gz    ),
            .west  = splitMesh(gx - 1, gy,     gz    ),
            .up    = splitMesh(gx,     gy + 1, gz    ),
            .down  = splitMesh(gx,     gy - 1, gz    )
        };
    }
}

auto Chunk::remesh() -> Void {
    if (not this->isGenerated()) return;
    this->opaqueMesh.clear();
    this->transparentMesh.clear();

    const auto ofx = Float(this->x * chunkSide);
    const auto ofz = Float(this->z * chunkSide);

    for (Int ix = 0; ix < chunkSide; ix += 1) {
        for (Int iy = 0; iy < chunkHeight; iy += 1) {
            for (Int iz = 0; iz < chunkSide; iz += 1) {
                // This is an optimization, we need not dispatch to air as it is semantically the empty block.
                if (this->blocks[ix][iy][iz]->tag != Tag::air) {
                    this->blocks[ix][iy][iz]->mesh(
                        this->world,
                        ofx + ix, iy, ofz + iz,
                        this->faces(ix, iy, iz),
                        this->opaqueMesh,
                        this->transparentMesh
                    );
                }
            }
        }
    }

    this->sort();
    this->world->hasUnsubmittedMesh = true;
}

auto Chunk::remeshAsync() -> Task {
    if (not this->isGenerated()) co_return;
    this->opaqueMesh.clear();
    this->transparentMesh.clear();

    const auto ofx = Float(this->x * chunkSide);
    const auto ofz = Float(this->z * chunkSide);

    for (Int ix = 0; ix < chunkSide; ix += 1) {
        for (Int iy = 0; iy < chunkHeight; iy += 1) {
            for (Int iz = 0; iz < chunkSide; iz += 1) {
                // This is an optimization, we need not dispatch to air as it is semantically the empty block.
                if (this->blocks[ix][iy][iz]->tag != Tag::air) {
                    this->blocks[ix][iy][iz]->mesh(
                        this->world,
                        ofx + ix, iy, ofz + iz,
                        this->faces(ix, iy, iz),
                        this->opaqueMesh,
                        this->transparentMesh
                    );
                }
            }
            co_await std::suspend_always {};
        }
    }

    this->sort();
    this->world->hasUnsubmittedMesh = true;
}

auto Chunk::relight() -> Void {
    if (this->isGenerated()) std::memset(&this->light, 0, chunkSide * chunkHeight * chunkSide);

    // Sunlight
    for (Int ix = 0; ix < chunkSide; ix += 1) {
        for (Int iz = 0; iz < chunkSide; iz += 1) {
            LightLevel sunlight = 15;
            for (Int iy = chunkHeight - 1; iy >= 0; iy -= 1) {
                const auto block = this->blocks[ix][iy][iz];
                if (sunlight - block->traits.lightAbsorption <= 0) break;

                sunlight -= block->traits.lightAbsorption;
                this->light[ix][iy][iz] = sunlight;
            }
        }
    }

    struct Node final { Int x, y, z; LightLevel light; };
    std::queue<Node> queue;

    for (Int ix = -1; ix <= chunkSide; ix += 1) {
        for (Int iz = -1; iz <= chunkSide; iz += 1) {
            for (Int iy = 0; iy < chunkHeight; iy += 1) {
                const auto gx = ix + this->x * chunkSide;
                const auto gy = iy;
                const auto gz = iz + this->z * chunkSide;

                if (const auto light = this->world->lightLevelAt(gx, gy, gz); light != 0)
                    queue.push({ ix, iy, iz, light });
            }
        }
    }

    const std::array<std::tuple<Int, Int, Int>, 6> neighbors = {
        std::make_tuple(-1, 0, 0), std::make_tuple(1, 0, 0),  // X-axis neighbors
        std::make_tuple(0, -1, 0), std::make_tuple(0, 1, 0),  // Y-axis neighbors
        std::make_tuple(0, 0, -1), std::make_tuple(0, 0, 1)   // Z-axis neighbors
    };

    while (not queue.empty()) {
        const auto [x, y, z, light] = queue.front();
        queue.pop();

        for (const auto [dx, dy, dz] : neighbors) {
            Int nx = x + dx;
            Int ny = y + dy;
            Int nz = z + dz;

            // Bounds check
            if (nx < 0 || nx >= chunkSide || ny < 0 || ny >= chunkHeight || nz < 0 || nz >= chunkSide) {
                continue;
            }

            // Calculate light level for the neighbor
            const auto block = this->blocks[nx][ny][nz];
            LightLevel newLight = std::max(0, light - block->traits.lightAbsorption - 1);

            // If the new light level is stronger, propagate it
            if (newLight > 0 and newLight > this->safeGetLightAt(nx, ny, nz)) {
                this->safeSetLightAt(nx, ny, nz, newLight);
                queue.push({nx, ny, nz, newLight});
            }
        }
    }
}

inline auto triDistance(Position position, std::array<BlockVertex, 3> tri) -> Float {
    const auto average = Position {
        .x = (tri[0].position.x + tri[1].position.x + tri[2].position.x) / 3.0f,
        .y = (tri[0].position.y + tri[1].position.y + tri[2].position.y) / 3.0f,
        .z = (tri[0].position.z + tri[1].position.z + tri[2].position.z) / 3.0f
    };

    const auto diff = Position {
        .x = average.x - position.x, .y = average.y - position.y, .z = average.z - position.z
    };

    return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

struct TriCompare final {
    Position origin;

    auto operator () (
        this const auto self,
        std::array<BlockVertex, 3> const& lhs,
        std::array<BlockVertex, 3> const& rhs
    ) -> Bool {
        return triDistance(self.origin, lhs) > triDistance(self.origin, rhs);
    }
};

auto Chunk::sort() -> Void {
    // This is sound because we know we are drawing triangles.
    const auto front = (std::array<BlockVertex, 3> *) this->transparentMesh.data();
    const auto back = ((std::array<BlockVertex, 3> *) this->transparentMesh.data()) + this->transparentMesh.size() / 3;

    std::sort(front, back, TriCompare { .origin = this->world->primary()->position });
}
