// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <vector>
#include <coroutine>
#include <exception>
#include <queue>
#include <random>
#include <cstring>
#include <optional>
#include <thread>
#include <atomic>
export module Minecraft:Chunk;

import Core;
import Engine;

import :Block;
import :Utility;

namespace minecraft {
    export constexpr auto chunkSide = 16;
    export constexpr auto chunkHeight = 256;

    /// Updated by the generation coroutine, this informs the outside world on how much progress was made generating a chunk.
    /// This is because chunk generation is in fact partially dependent on neighbor generation.
    ///
    /// Generating using coroutines is no longer a performance optimization, it is a hard requirement to prevent chunk generation
    /// depending on generating an infinite amount of neighbors, avoiding infinite recursion through chunks.
    ///
    /// The process looks like this:
    /// 1. A chunk generates until the end, continuing from the last stage if it was already partially generated.
    /// 2. It now needs to fill in blocks from nearby chunk structures which happen to spill over.
    ///    To do so it generates its neighbors enough to determine structure positions, but not so far that the
    ///    neigbor also requires generating its neigbors — essentially leaving the neigbor almost complete but awaiting
    ///    its own completion where its nearby structure blocks are filled in.
    ///
    /// This added complexity removes the hard recursive dependency between chunks.
    /// The dependency is manifested in code by the completion stage resuming the next missing neighbor instead of
    /// continuing any further.
    enum class GenerationStage: UInt8 {
        terrain,
        completion,
        done
    };

    /// A block that spilled into another chunk during generation.
    struct SpilledBlock final {
        struct ChunkPosition final { Int x, z; } position;
        UInt16 x, y, z;
        Block const* block;
    };

    /// Note that "affecting" only refers to the mesh, lighting is yet to be optimized.
    struct ModificationStatus final {
        Bool modified           : 1 { false };
        Bool affectingMeshNorth : 1 { false };
        Bool affectingMeshSouth : 1 { false };
        Bool affectingMeshEast  : 1 { false };
        Bool affectingMeshWest  : 1 { false };
    };

    /// Warning: Never stack allocate a chunk as they contain massive arrays.
    class Chunk final {
    public:
        struct Task {
            struct promise_type {
                auto get_return_object() -> Task {
                    return { std::coroutine_handle<promise_type>::from_promise(*this) };
                }
                auto initial_suspend() -> std::suspend_always { return {}; }
                auto final_suspend() noexcept -> std::suspend_always { return {}; }
                auto return_void() -> Void {}
                auto unhandled_exception() -> Void { std::terminate(); }
            };

            using Handle = std::coroutine_handle<promise_type>;
            Handle handle;

            Task(Task&& other) noexcept : handle { other.handle }, isDestroyed { other.isDestroyed } {
                other.isDestroyed = true;
            }

            auto operator = (Task&& other) noexcept -> Task& {
                if (this == &other) return *this;
                this->handle = other.handle;
                this->isDestroyed = other.isDestroyed;
                return *this;
            }

            Task(Task const&) = delete;
            auto operator = (Task const&) noexcept -> Task& = delete;

            explicit(false) Task(Handle handle) : handle { handle } {}

            Bool isDestroyed { false };

            ~Task() {
                if (not isDestroyed) {
                    handle.destroy();
                    this->isDestroyed = true;
                }
            }

            auto resume() -> Bool {
                if (not handle.done()) {
                    handle.resume();
                    return true;
                }
                return false;
            }

            auto done() -> Bool {
                return this->handle.done();
            }

            // constexpr auto await_ready() const noexcept -> Bool { return false; }
            // constexpr auto await_suspend(std::coroutine_handle<>) const noexcept -> Void {}
            // constexpr auto await_resume() const noexcept -> Void {}
        };

        using LightLevel = UInt8;

        /// The world we are contained in, our parent in the object hierarchy.
        World * world;
        /// X and Z describe the position of this chunk. While I like to avoid storing information twice,
        /// I would have to pass in a position context to most methods, which is even more fragile and bug prone.
        Int64 x, z;
        /// The current generation stage of this chunk.
        std::atomic<GenerationStage> generationStage { GenerationStage::terrain };
        /// A flag notifying the world that this chunk was modified and it (along with all its neighbors) requires
        /// a relight and remesh.
        ///
        /// It contains additional bits which determine which neighbors are affected.
        ModificationStatus modificationStatus {};
        /// The chunk generation coroutine.
        Task generator;
        /// Blocks that spill onto other chunks.
        std::vector<SpilledBlock> spill;
        /// The mesh created from opaque blocks.
        std::vector<BlockVertex> opaqueMesh {};
        /// The mesh created from transparent blocks, which requires sorting.
        std::vector<BlockVertex> transparentMesh {};
        /// A 3D array of all the blocks in this chunk.
        /// Note that these may or may not point to a shared instance.
        Block const* blocks[chunkSide][chunkHeight][chunkSide] { nullptr };
        /// A 3D array of light levels in this chunk.
        LightLevel light[chunkSide][chunkHeight][chunkSide] { 0 };

        auto isGenerated() const -> Bool { return this->generationStage == GenerationStage::done; }

        Chunk(World * parent, Int x, Int z) : world { parent }, x { x }, z { z }, generator { this->generate() } {
            for (Int ix = 0; ix < chunkSide; ix += 1) {
                for (Int iy = 0; iy < chunkHeight; iy += 1) {
                    for (Int iz = 0; iz < chunkSide; iz += 1) {
                        this->blocks[ix][iy][iz] = create<Air>();
                    }
                }
            }
        }

        Chunk(Chunk const&) = delete;
        Chunk(Chunk&&) = delete;
        auto operator = (Chunk const&) -> Chunk& = delete;
        auto operator = (Chunk&&) -> Chunk& = delete;
        ~Chunk() = default;

        auto generate() -> Task;
        auto remesh() -> Void;
        auto remeshAsync() -> Task;
        auto relight() -> Void;

        [[nodiscard]] auto faces(Int x, Int y, Int z) const -> FaceMask;

        [[nodiscard]] auto maybeGetBlockAt(Int x, Int y, Int z) const -> Block const* {
            if (not this->isGenerated()) return nullptr;
            if (x < 0 or y < 0 or z < 0 or x >= chunkSide or y >= chunkHeight or z >= chunkSide) return nullptr;
            return this->blocks[x][y][z];
        }

        auto safeSetBlockAt(Int x, Int y, Int z, Block const* block) -> Bool {
            if (x < 0 or y < 0 or z < 0 or x >= chunkSide or y >= chunkHeight or z >= chunkSide) return false;
            this->blocks[x][y][z] = block;
            return true;
        }

        auto safeSoftSetBlockAt(Int x, Int y, Int z, Block const* block) -> Bool {
            if (x < 0 or y < 0 or z < 0 or x >= chunkSide or y >= chunkHeight or z >= chunkSide) return false;
            if (not this->blocks[x][y][z]->traits.softGeneration) return false;
            this->blocks[x][y][z] = block;
            return true;
        }

        auto safeSpillingSoftSetBlockAt(Int x, Int y, Int z, Block const* block) -> Bool {
            if (y < 0 or y >= chunkHeight) {
                return false;
            } else if (x < 0  or z < 0 or x >= chunkSide or z >= chunkSide) {
                const auto gx = this->x * chunkSide + x;
                const auto gz = this->z * chunkSide + z;

                const auto cx = core::divfloor<Int>(gx, chunkSide);
                const auto cz = core::divfloor<Int>(gz, chunkSide);

                const auto lx = (x % chunkSide + chunkSide) % chunkSide;
                const auto ly = y;
                const auto lz = (z % chunkSide + chunkSide) % chunkSide;

                // if (lx < 0 or lz < 0 or lx >= chunkSide or lz >= chunkSide) std::terminate();

                this->spill.push_back({
                    .position = { .x = cx, .z = cz },
                    .x = static_cast<UInt16>(lx),
                    .y = static_cast<UInt16>(ly),
                    .z = static_cast<UInt16>(lz),
                    .block = block
                });

                return true;
            } else {
                if (not this->blocks[x][y][z]->traits.softGeneration) return false;
                this->blocks[x][y][z] = block;
                return true;
            }
        }

        auto safeSetLightAt(Int x, Int y, Int z, LightLevel light) -> Bool {
            if (x < 0 or y < 0 or z < 0 or x >= chunkSide or y >= chunkHeight or z >= chunkSide) return false;
            this->light[x][y][z] = light;
            return true;
        }

        [[nodiscard]] auto safeGetLightAt(Int x, Int y, Int z, LightLevel fallback = 15) const -> LightLevel {
            if (not this->isGenerated()) return fallback;
            if (x < 0 or y < 0 or z < 0 or x >= chunkSide or y >= chunkHeight or z >= chunkSide) return fallback;
            return this->light[x][y][z];
        }

        auto sort() -> Void;
    };

    class ChunkDistanceSort final {
        Position position;

    public:
        constexpr explicit ChunkDistanceSort(const Position position) : position { position } {}

        constexpr auto operator () (Chunk const* lhs, Chunk const* rhs) -> Bool {
            return position.distanceTo({
                .x = Float(lhs->x) * chunkSide + chunkSide / 2.0f - 0.5f,
                .y = 0,
                .z = Float(lhs->z) * chunkSide + chunkSide / 2.0f - 0.5f
            }) > position.distanceTo({
                .x = Float(rhs->x) * chunkSide + chunkSide / 2.0f - 0.5f,
                .y = 0,
                .z = Float(rhs->z) * chunkSide + chunkSide / 2.0f - 0.5f
            });
        }
    };
}
