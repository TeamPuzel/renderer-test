// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <vector>
#include <array>
#include <optional>
#include <coroutine>

#include "core/macros/range.hpp"
module Minecraft:Chunk.Generation;

import Core;

import :Chunk;
import :World;

using namespace minecraft;

auto Chunk::generate() -> Task { // NOLINT(*-no-recursion) False positive, it's an allocated coroutine.
    using core::rng::Xoshiro256StarStar;
    using core::random;

    auto hash = std::hash<UInt64>()(this->x ^ this->z << 1 ^ this->world->seed() << 2);
    for (Int i = 0; hash == 0; i += 1) { // Xoshiro will die if the seed is 0, take care not to accidentally hash one.
        hash = std::hash<UInt64>()(this->x * i ^ this->z * i << 1 ^ this->world->seed() << 2 ^ i << 3);
    }

    auto rng = Xoshiro256StarStar(hash);

    for (Int ix = 0; ix < chunkSide; ix += 1) {
        for (Int iz = 0; iz < chunkSide; iz += 1) {
            const auto fix = static_cast<Float>(ix);
            const auto fiz = static_cast<Float>(iz);
            const auto fx = static_cast<Float>(this->x);
            const auto fz = static_cast<Float>(this->z);

            const auto posX = fix + chunkSide * fx;
            const auto posZ = fiz + chunkSide * fz;

            const auto octavefn = [this, posX, posZ](Float frequency, Float amplitude) -> Float {
                return core::perlin(posX * frequency, posZ * frequency, 0, Int32(this->world->seed())) * amplitude;
            };

            const auto octaved = [octavefn](Float baseFrequency) -> Float {
                return octavefn(baseFrequency, 0) + octavefn(baseFrequency * 2, 0.5) + octavefn(baseFrequency * 4, 0.25);
            };

            const auto continentalness = octaved(0.005);
            const auto erosion = octaved(0.01);
            const auto peaks = octaved(0.05);

            // Terrain
            Float base; if (continentalness <= 0.3) {
                base = core::normalize<Float>(continentalness, -1, 0.3, 50, 100);
            } else if (continentalness <= 0.4) {
                base = core::normalize<Float>(continentalness, 0.3, 0.4, 100, 150);
            } else {
                base = 150;
            }

            // MARK: - Height pass -------------------------------------------------------------------------------------
            Float height = base;

            for (Int iy = 0; iy < chunkHeight; iy += 1) {
                const auto fiy = static_cast<Float>(iy);
                if (fiy <= height) this->blocks[ix][iy][iz] = create<Stone>();
                if (fiy > height and fiy < 90.0) this->blocks[ix][iy][iz] = create<Water>();

                co_await std::suspend_always {};
            }

            // MARK: - Layer pass --------------------------------------------------------------------------------------
            const auto maxThickness = 3; // TODO(!): This should be variable slightly but still determined by seed.

            auto currentThickness = 0;
            for (Int iy = chunkHeight - 1; iy >= 0; iy -= 1) {
                if (this->blocks[ix][iy][iz]->tag == Tag::stone) {
                    this->blocks[ix][iy][iz] =
                        base < 92
                            ? create<Sand>()
                            : currentThickness == 0 ? anycreate<Grass>() : anycreate<Dirt>();
                    currentThickness += 1;
                }
                if (currentThickness == maxThickness) { break; }

                co_await std::suspend_always {};
            }

            for (Int iy = chunkHeight - 1; iy >= 0; iy -= 1) {
                if (
                    iy + 1 < chunkHeight
                    and this->blocks[ix][iy][iz]->tag == Tag::grass
                    and this->blocks[ix][iy + 1][iz]->tag == Tag::air
                    and random(from 0.0 to 100.0, rng) < 25.0
                ) {
                    this->blocks[ix][iy + 1][iz] = create<TallGrass>();
                    break;
                }

                co_await std::suspend_always {};
            }

            for (Int iy = chunkHeight - 1; iy >= 0; iy -= 1) {
                if (
                    iy + 1 < chunkHeight
                    and this->blocks[ix][iy][iz]->tag == Tag::grass
                    and this->blocks[ix][iy + 1][iz]->tag == Tag::air
                    and random(from 0.0 to 100.0, rng) < 25.0
                ) {
                    this->blocks[ix][iy + 1][iz] = create<TallGrass>();
                    break;
                } else if (
                    iy + 1 < chunkHeight
                    and this->blocks[ix][iy][iz]->tag == Tag::grass
                    and this->blocks[ix][iy + 1][iz]->tag == Tag::air
                    and random(from 0.0 to 100.0, rng) < 1.0
                ) {
                    this->blocks[ix][iy + 1][iz] = create<Rose>();
                    break;
                }

                co_await std::suspend_always {};
            }
        }
    }

    // Tree pass
    {
        for (Int ix = 0; ix < chunkSide; ix += 1) {
            for (Int iz = 0; iz < chunkSide; iz += 1) {
                if (const auto treeChance = random(from 0.0 to 100.0, rng); treeChance > 1) continue;
                const auto height = random(from 4 to 7, rng);

                std::optional<Int> start = std::nullopt;
                for (Int iy = chunkHeight - 1; iy >= 0; iy -= 1) {
                    if (this->blocks[ix][iy][iz]->tag == Tag::grass) { start = iy + 1; break; }
                    if (not this->blocks[ix][iy][iz]->traits.softGeneration) goto invalid_position;
                }

                if (start) {
                    for (Int iy = *start; iy < *start + height + 1 and iy < chunkHeight; iy += 1) {
                        if (iy < *start + height) this->safeSoftSetBlockAt(ix, iy, iz, create<Log>());

                        if (iy > *start + height - 4) {
                            const auto radius = iy > *start + height - 2 ? 2 : 3;

                            for (Int tix = -radius + 1; tix < radius; tix += 1) {
                                for (Int tiz = -radius + 1; tiz < radius; tiz += 1) {
                                    this->safeSpillingSoftSetBlockAt(ix + tix, iy, iz + tiz, create<Leaves>());
                                }
                            }
                        }
                    }
                }

                co_await std::suspend_always {};
                invalid_position:
            }
        }
    }

    // COMPLETION STAGE - This is the critical point where generating further becomes a hard recursive dependency.
    // We can now inform chunks awaiting on us that we are ready to be referenced for structure generation.
    this->generationStage = GenerationStage::completion;
    co_await std::suspend_always {};

    const std::array positions = {
        ChunkPosition { .x = this->x - 1, .z = this->z - 1 },
        ChunkPosition { .x = this->x - 1, .z = this->z     },
        ChunkPosition { .x = this->x - 1, .z = this->z + 1 },

        ChunkPosition { .x = this->x,     .z = this->z - 1 },
        ChunkPosition { .x = this->x,     .z = this->z + 1 },

        ChunkPosition { .x = this->x + 1, .z = this->z - 1 },
        ChunkPosition { .x = this->x + 1, .z = this->z     },
        ChunkPosition { .x = this->x + 1, .z = this->z + 1 }
    };

    auto neighbors = std::vector<Chunk*>();

    for (const auto position : positions) neighbors.push_back(this->world->demandChunkAt(position));

    for (const auto neighbor : neighbors) {
        while (neighbor->generationStage == GenerationStage::terrain) {
            neighbor->generator.resume();
            co_await std::suspend_always {};
        }

        // for block in neighbor.spill where block.position == self.position
        for (const auto [position, x, y, z, block] : neighbor->spill) {
            if (position.x == this->x and position.z == this->z) {
                this->safeSoftSetBlockAt(x, y, z, block);
            }
        }
    }

    // TODO(CRITICAL): Get a massive performance boost by implementing these as a third, async stage.
    //                 It's a massive performance hit from like 4000fps to 90-140 whenever the world keeps generating.
    //                 Fix this and performance will genuinely be insane.
    this->relight();
    auto remeshTask = this->remeshAsync();
    while (not remeshTask.handle.done()) {
        remeshTask.resume();
        co_await std::suspend_always {};
    }

    // GENERATION END - The chunk is now fully generated and ready for use.
    this->generationStage = GenerationStage::done;

    // Remesh neighbors
    this->world->remeshNeighbors(x, z);
}
