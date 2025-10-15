// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
#pragma once
#include <primitive>
#include <tuple>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <queue>
#include <format>
#include <exception>
#include <ranges>
#include <algorithm>
#include <type_traits>
#include <thread>

#include <core/macros/match.hpp>
#include <core/macros/range.hpp>

namespace minecraft {
    using engine::Color;

    /// The key for chunk storage.
    export struct ChunkPosition final {
        Int64 x, z;

        constexpr auto operator == (this const ChunkPosition self, const ChunkPosition other) -> Bool {
            return self.x == other.x and self.z == other.z;
        }
    };
}

export template <> struct std::hash<minecraft::ChunkPosition> final {
    constexpr auto operator () (minecraft::ChunkPosition const& value) const noexcept -> UInt {
        return std::hash<Int64>()(value.x) xor std::hash<Int64>()(value.z) << 1;
    }
};

namespace minecraft {
    constexpr f32 minute = 60000.f;
    /// The lenght of the day-night cycle.
    constexpr f32 dayLength = minute * 20.f;

    /// An asynchronous task scheduled on a world instance.
    class Task final {

    };

    /// A star in a generated sky.
    export struct Star final {
        core::Angle<Float> pitch, yaw;
        Float distance, intensity;
    };

    export class World final {
        struct ChunkAllocator final {
            struct AllocationGroup final {
                static constexpr auto slotsPerGroup = 32;

                struct Slot final {
                    Bool used { false };
                    UInt8 chunk[sizeof(Chunk)];
                };

                UInt8 freeSlots { slotsPerGroup };
                std::array<Slot, slotsPerGroup> slots {};
            };

            std::vector<AllocationGroup*> groups { new AllocationGroup() };

            ChunkAllocator() noexcept {}

            ChunkAllocator(ChunkAllocator const&) = delete;
            ChunkAllocator(ChunkAllocator&&) = default;
            auto operator = (ChunkAllocator const&) -> ChunkAllocator& = delete;
            auto operator = (ChunkAllocator&&) -> ChunkAllocator& = default;
            ~ChunkAllocator() = default;

            [[nodiscard]] auto consumeFirstAvailableSlot() -> Chunk* {
                for (const auto group : this->groups) {
                    if (group->freeSlots > 0) {
                        for (auto& slot : group->slots) {
                            if (not slot.used) {
                                group->freeSlots -= 1;
                                slot.used = true;
                                return (Chunk*) &slot.chunk;
                            }
                        }
                        std::unreachable(); // We know there is a slot.
                    }
                }

                this->groups.push_back(new AllocationGroup());
                this->groups.back()->freeSlots -= 1;
                this->groups.back()->slots[0].used = true;
                return (Chunk*) &this->groups.back()->slots[0].chunk;
            }

            auto freeSlot(Chunk * chunk) -> Void {
                if (chunk == nullptr) return;

                for (const auto group : this->groups) {
                    for (auto& slot : group->slots) {
                        if (chunk == (Chunk*) &slot.chunk) {
                            group->freeSlots += 1;
                            slot.used = false;
                        }
                    }
                }

                throw std::runtime_error("Attempting to free a non existent chunk");
            }

            auto reclaimGroups() -> Void {
                if (this->groups.size() < 1) return;

                for (auto group : this->groups | std::views::reverse) {
                    if (group->freeSlots == AllocationGroup::slotsPerGroup) {
                        delete group;
                        this->groups.pop_back(); // TODO: Verify if this is sound while iterating
                    }
                }
            }
        };

        ChunkAllocator chunkAllocator {};

        [[nodiscard]] auto createChunk(Int x, Int z) -> Chunk* {
            const auto chunk = this->chunkAllocator.consumeFirstAvailableSlot();
            new (chunk) Chunk(this, x, z);
            return chunk;
        }

        auto destroyChunk(Chunk * chunk) noexcept -> Void {
            chunk->~Chunk();
            this->chunkAllocator.freeSlot(chunk);
        }

    public:
        struct Time final {
        private:
            Float time;

            constexpr static Float msPerDay = dayLength;
            constexpr static Float msPerHour = msPerDay / 24.0f;
            constexpr static Float msPerMinute = msPerHour / 60.0f;

        public:
            explicit Time(Float time) : time { time } {}

            [[nodiscard]]
            constexpr auto day() const -> Float { return time / msPerDay; }

            [[nodiscard]]
            constexpr auto hour() const -> Float {
                return std::fmod(time, msPerDay) / msPerHour;
            }

            [[nodiscard]]
            constexpr auto minute() const -> Float {
                return std::fmod(time, msPerHour) / msPerMinute;
            }

            constexpr static auto dayStart(Float hour) -> Float { return msPerHour * hour; }
        };

    private:
        /// The actual chunk storage.
        std::unordered_map<ChunkPosition, Chunk*> chunks {};
        /// A cache of all chunks in a square within render distance.
        core::Array<Chunk*> chunkCache {};
        /// A backup cache of just the last used chunk to minimize hash map usage in loops, as it was rather slow.
        ///
        /// WARNING: Remember, clear this when deleting chunks or it will dangle.
        Chunk * lastQueriedChunk { nullptr };
        /// The timer used to calculate the delta and frame rate.
        core::Timer timer {};
        /// An immutable timer which therefore can only be used to query elapsed time since program start.
        core::Timer startTime {};
        /// The world time.
        Float time { Time::dayStart(4.5) };
        /// Actual, private storage of the delta time.
        Float _delta { 0 };
        /// A count of frames/updates since world creation, used to for example only perform an operation every few frames.
        /// It is more of a hack than proper feature, it should not be used permanently for the most part.
        UInt frame { 0 };
        /// The position relative to which transparent vertices are curently sorted. If this differs from the primary
        /// entity position too much transparency rendering will begin to fall apart.
        Position sortedAt {};
        /// This should be named `entityCache` to match `chunkCache` as it shares the use case.
        /// It's a cache of all entities within loaded chunks. When a chunk is unloaded, any non player entities from here
        /// which happen to exist within should be moved into long term keyed entity storage.
        core::Array<core::Shared<Entity>> entities {};
        /// The world seed used for world generation.
        UInt64 _seed;
        /// The entity which the camera is attached to.
        Entity * _primary {};

    public:
        Bool hasUnsubmittedMesh { false };

        [[nodiscard]] auto primary() const -> Entity* { return this->_primary; }
        [[nodiscard]] auto seed() const -> UInt64 { return this->_seed; }

        explicit World(const UInt64 seed) : _seed { seed } {
            core::precondition(seed != 0);

            auto player = core::Shared(Player(this));
            player->position.y = 100;

            this->entities.append(player);
            this->_primary = player.raw();
        }

        World(World const&) = delete;
        World(World&&) = default;
        auto operator = (World const&) -> World& = delete;
        auto operator = (World&&) -> World& = default;

        ~World() = default;

        [[nodiscard]] auto delta() const -> Float { return this->_delta; }

        [[nodiscard]] auto demandChunkAt(ChunkPosition position) -> Chunk* {
            if (not this->chunks.contains(position)) {
                this->chunks.insert({ position, this->createChunk(position.x, position.z)});
            }

            return this->chunks.at(position);
        }

        [[nodiscard]] auto maybeGetChunkContaining(Int x, Int y, Int z) const -> Chunk* {
            const auto cx = core::divfloor<Int>(x, chunkSide);
            const auto cz = core::divfloor<Int>(z, chunkSide);
            return this->maybeGetChunkAt(cx, cz);
        }

        [[nodiscard]] inline auto maybeGetBlockAt(Int x, Int y, Int z) const -> Block const* {
            const auto cx = core::divfloor<Int>(x, chunkSide);
            const auto cz = core::divfloor<Int>(z, chunkSide);

            const auto lx = (x % chunkSide + chunkSide) % chunkSide;
            const auto ly = y;
            const auto lz = (z % chunkSide + chunkSide) % chunkSide;

            // if (lx < 0 or lz < 0 or lx >= chunk::chunkSide or lz >= chunk::chunkSide) std::terminate();

            if (const auto chunk = this->maybeGetChunkAt(cx, cz)) {
                return chunk->maybeGetBlockAt(lx, ly, lz);
            } else {
                return nullptr;
            }
        }

        [[nodiscard]] auto isOpaqueAt(Int x, Int y, Int z) const -> Bool {
            const auto block = this->maybeGetBlockAt(x, y, z);
            if (not block) return true;
            return block->tag != Tag::air and not block->traits.hasTransparency;
        }

        /// TODO(CRITICAL): This is not the correct name for the function anymore.
        [[nodiscard]] auto isOpaqueExcludingEmptyAt(Int x, Int y, Int z) const -> Bool {
            const auto block = this->maybeGetBlockAt(x, y, z);
            if (not block) return false;
            return block->tag != Tag::air and not block->traits.hasTransparency or block->tag == Tag::leaves;
        }

        [[nodiscard]] auto isOpaqueExcludingEmptyOrLeavesAt(Int x, Int y, Int z) const -> Bool {
            const auto block = this->maybeGetBlockAt(x, y, z);
            if (not block) return false;
            return block->tag != Tag::air
                and not block->traits.hasTransparency
                and block->tag != Tag::leaves;
        }

        [[nodiscard]] auto lightLevelAt(Int x, Int y, Int z) const -> Chunk::LightLevel {
            const auto cx = core::divfloor<Int>(x, chunkSide);
            const auto cz = core::divfloor<Int>(z, chunkSide);

            const auto lx = (x % chunkSide + chunkSide) % chunkSide;
            const auto ly = y;
            const auto lz = (z % chunkSide + chunkSide) % chunkSide;

            if (const auto chunk = this->maybeGetChunkAt(cx, cz)) {
                return chunk->safeGetLightAt(lx, ly, lz, 0);
            } else {
                return 0;
            }
        }

        [[nodiscard]] auto neighbors(Int x, Int y, Int z) const -> NeighborMask {
            NeighborMask mask;

            // level 1
            mask.ref_at(-1, -1, -1) = this->isOpaqueExcludingEmptyAt(x - 1, y - 1, z - 1);
            mask.ref_at(-1, -1,  0) = this->isOpaqueExcludingEmptyAt(x - 1, y - 1, z    );
            mask.ref_at(-1, -1,  1) = this->isOpaqueExcludingEmptyAt(x - 1, y - 1, z + 1);

            mask.ref_at( 0, -1, -1) = this->isOpaqueExcludingEmptyAt(x,     y - 1, z - 1);
            mask.ref_at( 0, -1,  0) = this->isOpaqueExcludingEmptyAt(x,     y - 1, z    );
            mask.ref_at( 0, -1,  1) = this->isOpaqueExcludingEmptyAt(x,     y - 1, z + 1);

            mask.ref_at( 1, -1, -1) = this->isOpaqueExcludingEmptyAt(x + 1, y - 1, z - 1);
            mask.ref_at( 1, -1,  0) = this->isOpaqueExcludingEmptyAt(x + 1, y - 1, z    );
            mask.ref_at( 1, -1,  1) = this->isOpaqueExcludingEmptyAt(x + 1, y - 1, z + 1);

            // level 2
            mask.ref_at(-1,  0, -1) = this->isOpaqueExcludingEmptyAt(x - 1, y,     z - 1);
            mask.ref_at(-1,  0,  0) = this->isOpaqueExcludingEmptyAt(x - 1, y,     z    );
            mask.ref_at(-1,  0,  1) = this->isOpaqueExcludingEmptyAt(x - 1, y,     z + 1);

            mask.ref_at( 0,  0, -1) = this->isOpaqueExcludingEmptyAt(x,     y,     z - 1);
            mask.ref_at( 0,  0,  0) = this->isOpaqueExcludingEmptyAt(x,     y,     z    );
            mask.ref_at( 0,  0,  1) = this->isOpaqueExcludingEmptyAt(x,     y,     z + 1);

            mask.ref_at( 1,  0, -1) = this->isOpaqueExcludingEmptyAt(x + 1, y,     z - 1);
            mask.ref_at( 1,  0,  0) = this->isOpaqueExcludingEmptyAt(x + 1, y,     z    );
            mask.ref_at( 1,  0,  1) = this->isOpaqueExcludingEmptyAt(x + 1, y,     z + 1);

            // level 3
            mask.ref_at(-1,  1, -1) = this->isOpaqueExcludingEmptyAt(x - 1, y + 1, z - 1);
            mask.ref_at(-1,  1,  0) = this->isOpaqueExcludingEmptyAt(x - 1, y + 1, z    );
            mask.ref_at(-1,  1,  1) = this->isOpaqueExcludingEmptyAt(x - 1, y + 1, z + 1);

            mask.ref_at( 0,  1, -1) = this->isOpaqueExcludingEmptyAt(x,     y + 1, z - 1);
            mask.ref_at( 0,  1,  0) = this->isOpaqueExcludingEmptyAt(x,     y + 1, z    );
            mask.ref_at( 0,  1,  1) = this->isOpaqueExcludingEmptyAt(x,     y + 1, z + 1);

            mask.ref_at( 1,  1, -1) = this->isOpaqueExcludingEmptyAt(x + 1, y + 1, z - 1);
            mask.ref_at( 1,  1,  0) = this->isOpaqueExcludingEmptyAt(x + 1, y + 1, z    );
            mask.ref_at( 1,  1,  1) = this->isOpaqueExcludingEmptyAt(x + 1, y + 1, z + 1);

            return mask;
        }


        /// The general camera entity attached projection matrix.
        [[nodiscard]] auto primaryMatrix() const -> core::Matrix<Float, 4, 4> {
            using Matrix = decltype(primaryMatrix());
            using core::deg;
            using engine::width;
            using engine::height;

            return Matrix::translation(-this->primary()->position.x, -this->primary()->position.y, -this->primary()->position.z)
                 * Matrix::rotation(Matrix::RotationAxis::yaw, this->primary()->orientation.yaw)
                 * Matrix::rotation(Matrix::RotationAxis::pitch, this->primary()->orientation.pitch)
                 * Matrix::projection(Float(width()), Float(height()), settings::shared.fov, 0.1, 1000);
        }

        /// The general fixed position camera entity attached projection matrix.
        [[nodiscard]] auto primaryFixedMatrix() const -> core::Matrix<Float, 4, 4> {
            using Matrix = decltype(primaryFixedMatrix());
            using core::deg;
            using engine::width;
            using engine::height;

            return Matrix::rotation(Matrix::RotationAxis::yaw, this->primary()->orientation.yaw)
                 * Matrix::rotation(Matrix::RotationAxis::pitch, this->primary()->orientation.pitch)
                 * Matrix::projection(Float(width()), Float(height()), settings::shared.fov, 0.1, 1000);
        }

        /// The general fixed position camera entity attached projection matrix, rotated as the day goes on.
        [[nodiscard]] auto primaryCelestialMatrix() const -> core::Matrix<Float, 4, 4> {
            using Matrix = decltype(primaryCelestialMatrix());
            using core::deg;
            using core::normalize;
            using engine::width;
            using engine::height;

            return Matrix::rotation(Matrix::RotationAxis::pitch, -deg(normalize<Float>(this->timeOfDay().hour(), 0, 24, 0, 360)))
                 * Matrix::rotation(Matrix::RotationAxis::yaw, this->primary()->orientation.yaw + deg(90))
                 * Matrix::rotation(Matrix::RotationAxis::pitch, this->primary()->orientation.pitch)
                 * Matrix::projection(Float(width()), Float(height()), settings::shared.fov, 0.1, 8000);
        }

        /// TODO(CRITICAL): This is copied only to copy directly into OpenGL memory anyway.
        ///                 Especially stupid on devices like mine where the memory is shared.
        ///                 Instead just calculate the size of all meshes combined and write them
        ///                 Into OpenGL memory directly. Around 40% of the flame graph is in fact copying memory.
        auto unifiedMesh(
            engine::abstract::VertexBuffer<BlockVertex>& opaque,
            engine::abstract::VertexBuffer<BlockVertex>& transparent
        ) const -> Void {
            for (const auto chunk : this->chunkCache) {
                opaque.append(chunk->opaqueMesh);
                transparent.append(chunk->transparentMesh);
            }
        }

        auto remeshNeighbors(ChunkPosition position) -> Void {
            const auto hasNorth = this->chunks.contains({ .x = position.x, .z = position.z + 1 });
            if (hasNorth) this->chunks.at({ .x = position.x, .z = position.z + 1 })->remesh();

            const auto hasSouth = this->chunks.contains({ .x = position.x, .z = position.z - 1 });
            if (hasSouth) this->chunks.at({ .x = position.x, .z = position.z - 1 })->remesh();

            const auto hasEast = this->chunks.contains({ .x = position.x + 1, .z = position.z });
            if (hasEast) this->chunks.at({ .x = position.x + 1, .z = position.z })->remesh();

            const auto hasWest = this->chunks.contains({ .x = position.x - 1, .z = position.z });
            if (hasWest) this->chunks.at({ .x = position.x - 1, .z = position.z })->remesh();
        }
        auto remeshNeighbors(Int x, Int z) -> void { this->remeshNeighbors({ x, z }); }

        auto remeshAffectedNeighbors(ChunkPosition position, ModificationStatus status) -> Void {
            const auto hasNorth = this->chunks.contains({ .x = position.x, .z = position.z + 1 });
            if (hasNorth and status.affectingMeshNorth) this->chunks.at({ .x = position.x, .z = position.z + 1 })->remesh();

            const auto hasSouth = this->chunks.contains({ .x = position.x, .z = position.z - 1 });
            if (hasSouth and status.affectingMeshSouth) this->chunks.at({ .x = position.x, .z = position.z - 1 })->remesh();

            const auto hasEast = this->chunks.contains({ .x = position.x + 1, .z = position.z });
            if (hasEast and status.affectingMeshEast) this->chunks.at({ .x = position.x + 1, .z = position.z })->remesh();

            const auto hasWest = this->chunks.contains({ .x = position.x - 1, .z = position.z });
            if (hasWest and status.affectingMeshWest) this->chunks.at({ .x = position.x - 1, .z = position.z })->remesh();
        }
        auto remeshAffectedNeighbors(Int x, Int z, ModificationStatus status) -> Void {
            this->remeshAffectedNeighbors({ x, z }, status);
        }

        auto relightNeighbors(ChunkPosition position) -> Void {
            const auto hasNorth = this->chunks.contains({ .x = position.x, .z = position.z + 1 });
            if (hasNorth) this->chunks.at({ .x = position.x, .z = position.z + 1 })->relight();

            const auto hasSouth = this->chunks.contains({ .x = position.x, .z = position.z - 1 });
            if (hasSouth) this->chunks.at({ .x = position.x, .z = position.z - 1 })->relight();

            const auto hasEast = this->chunks.contains({ .x = position.x + 1, .z = position.z });
            if (hasEast) this->chunks.at({ .x = position.x + 1, .z = position.z })->relight();

            const auto hasWest = this->chunks.contains({ .x = position.x - 1, .z = position.z });
            if (hasWest) this->chunks.at({ .x = position.x - 1, .z = position.z })->relight();
        }
        auto relightNeighbors(Int x, Int z) -> Void { this->relightNeighbors({ x, z }); }

        // NOTE: This performs a const cast, but the effects are only directly visible here.
        [[nodiscard]] auto maybeGetChunkAt(ChunkPosition position) const -> Chunk* {
            Chunk * chunk;
            if (this->lastQueriedChunk and this->lastQueriedChunk->x == position.x and this->lastQueriedChunk->z == position.z) {
                chunk = this->lastQueriedChunk;
            } else [[unlikely]] {
                if (not this->chunks.contains(position)) return nullptr;
                chunk = this->chunks.at(position);
                const_cast<World*>(this)->lastQueriedChunk = chunk;
                // __builtin_prefetch(chunk, 1, 3);
            }
            return chunk;
        }

        [[nodiscard]]
        auto maybeGetChunkAt(const Int x, const Int z) const -> Chunk* { return this->maybeGetChunkAt({ x, z }); }

        auto castRayAtMaybeBlock(
            this World const& self, Position position, Orientation orientation, Float range = 1000
        ) -> std::tuple<Block const*, Position> {
            constexpr auto step = 0.01f;

            if (range <= 0) return { nullptr, { std::round(position.x), std::round(position.y), std::round(position.z) } };

            const auto query = self.maybeGetBlockAt(
                std::round(position.x),
                std::round(position.y),
                std::round(position.z)
            );

            if (not query) {
                return { nullptr, { std::round(position.x), std::round(position.y), std::round(position.z) } };
            } else if (query->tag != Tag::air) {
                return { query, { std::round(position.x), std::round(position.y), std::round(position.z) } };
            } else {
                return self.castRayAtMaybeBlock(position.movedInDirectionAtSpeed(orientation, step), orientation, range - step);
            }
        }

        /// Like `castRayAtMaybeBlock` but will walk back to the previous block.
        // auto castRayAtMaybeBlockBefore(
        //     this World const& self, Position position, Orientation orientation, Float range = 1000
        // ) -> std::tuple<Block const*, Position> {
        //     constexpr auto step = 0.01f;
        //
        //     if (range <= 0) return { nullptr, { std::round(position.x), std::round(position.y), std::round(position.z) } };
        //
        //     const auto query = self.maybeGetBlockAt(
        //         std::round(position.x),
        //         std::round(position.y),
        //         std::round(position.z)
        //     );
        //
        //     if (not query) {
        //         return { nullptr, { std::round(position.x), std::round(position.y), std::round(position.z) } };
        //     } else if (query->tag != Tag::air) {
        //         return { query, { std::round(position.x), std::round(position.y), std::round(position.z) } };
        //     } else {
        //         return self.castRayAtMaybeBlock(position.movedInDirectionAtSpeed(orientation, step), orientation, range - step);
        //     }
        // }

        auto tryBreakBlockAt(this World& self, const Int x, const Int y, const Int z) -> Bool {
            const auto block = self.maybeGetBlockAt(x, y, z);
            if (not block or not block->traits.isBreakable) return false;

            const auto cx = core::divfloor<Int>(x, chunkSide);
            const auto cz = core::divfloor<Int>(z, chunkSide);

            const auto lx = (x % chunkSide + chunkSide) % chunkSide;
            const auto ly = y;
            const auto lz = (z % chunkSide + chunkSide) % chunkSide;

            const auto chunk = self.maybeGetChunkAt(cx, cz);
            if (not chunk) return false;
            chunk->blocks[lx][ly][lz] = create<Air>();

            chunk->modificationStatus = {
                .modified = true,
                .affectingMeshNorth = lz == chunkSide - 1,
                .affectingMeshSouth = lz == 0,
                .affectingMeshEast  = lx == chunkSide - 1,
                .affectingMeshWest  = lx == 0,
            };

            if (ly + 1 < chunkHeight and chunk->blocks[lx][ly + 1][lz]->traits.weakFoundation) {
                return self.tryBreakBlockAt(x, y + 1, z);
            } else {
                return true;
            }
        }

        [[nodiscard]] auto elapsed() const -> Double { return this->startTime.elapsed(); }

        [[nodiscard]]
        auto timeOfDay() const -> Time { return Time(this->time); }

        [[nodiscard]]
        auto isCameraUnderwater() const -> Bool {
            constexpr auto wo = 2.0f / 16.0f; // The y offset of top vertices used by water.

            const auto headBlock = this->maybeGetBlockAt(
                Int(std::round(this->primary()->position.x)),
                Int(std::round(this->primary()->position.y + wo)),
                Int(std::round(this->primary()->position.z))
            );

            return headBlock and headBlock->tag == Tag::water;
        }

        /// Generate lots of stars randomly in a sphere around the player. This is similar to how
        /// minecraft handles it but the seed is not constant across worlds, instead using the world seed.
        [[nodiscard]] auto generateStars() const -> std::vector<Star> {
            using core::rng::Xoshiro256StarStar;
            using core::random;
            using core::deg;
            using core::pi;

            constexpr auto starCount = 3000;

            std::vector<Star> stars;
            stars.reserve(starCount);

            auto rng = core::rng::Xoshiro256StarStar(this->seed());

            for (Int i = 0; i < starCount; i += 1) {
                const auto pitch = std::acos(random(from -1.0f to 1.0f, rng)) * (180.0f / Float(pi)) - 90.0f; // -90° to 90°
                const auto yaw = random(from 0.0f to 360.0f, rng);

                stars.push_back({
                    .pitch     = deg(pitch),
                    .yaw       = deg(yaw),
                    .distance  = random(from 0.6f to 1.0f, rng),
                    .intensity = random(from 0.6f to 1.0f, rng)
                });
            }

            return stars;
        }

        [[nodiscard]]
        auto colorTimeMix(
            Color night, Color sunrise, Color day, Color sunset,
            std::optional<Color> underwater = std::nullopt
        ) const -> Color {
            using engine::hex;
            using engine::smix;
            using core::normalize;

            if (this->isCameraUnderwater() and underwater.has_value()) {
                return *underwater;
            } else {
                const auto hour = this->timeOfDay().hour();

                return match (hour) {
                    from 0.0  until 4.5  then night,
                    from 4.5  until 6.5  then smix(night, sunrise, normalize<Float>(hour, 4.5, 6.5, 0, 1)),
                    from 6.5  until 7.5  then smix(sunrise, day, normalize<Float>(hour, 6.5, 7.5, 0, 1)),
                    from 7.0  until 16.5 then day,
                    from 16.5 until 18.5 then smix(day, sunset, normalize<Float>(hour, 16.5, 18.5, 0, 1)),
                    from 18.5 until 19.5 then smix(sunset, night, normalize<Float>(hour, 18.5, 19.5, 0, 1))
                } otherwise (night);
            }
        }

        [[nodiscard]]
        auto skyColor() const -> Color {
            using engine::hex;

            constexpr auto underwater = hex(0x000000);
            constexpr auto night      = hex(0x0D131F);
            constexpr auto day        = hex(0x79A6FF);

            return this->colorTimeMix(night, day, day, day, underwater);
        }

        [[nodiscard]]
        auto fogColor() const -> Color {
            using engine::hex;

            constexpr auto underwater = hex(0x000000);
            constexpr auto night      = hex(0x182236);
            constexpr auto day        = hex(0xB8C7E6);
            constexpr auto sunrise    = hex(0xFC9862);

            return this->colorTimeMix(night, sunrise, day, sunrise, underwater);
        }

        [[nodiscard]]
        auto terrainTintColor() const -> Color {
            using engine::hex;

            constexpr auto night   = hex(0x101050);
            constexpr auto day     = hex(0xFFFFFF);
            constexpr auto sunrise = hex(0xFC9862);

            return this->colorTimeMix(night, sunrise, day, sunrise);
        }

        [[nodiscard]]
        auto waterTintColor() const -> Color {
            using engine::hex;

            constexpr auto underwater = hex(0x000000);
            constexpr auto night      = hex(0x101020);
            constexpr auto day        = hex(0xFFFFFF);
            constexpr auto sunrise    = hex(0xFC9862);

            return this->colorTimeMix(night, sunrise, day, day, underwater);
        }

        [[nodiscard]]
        auto fogDistance() const -> Float {
            using core::smooth;
            using core::normalize;
            using core::Range;

            if (this->isCameraUnderwater()) {
                return 60.0f;
            } else {
                const auto hour = this->timeOfDay().hour();

                return match (hour) {
                    from 2.5 until 5.0 then std::lerp(10.0f, 30.0f, normalize<Float>(hour, 2.5, 5, 0, 1)),
                    from 5.0 until 7.0 then std::lerp(30.0f, 10.0f, normalize<Float>(hour, 5, 7, 0, 1))
                } otherwise (10.0f);
            }
        }

        [[nodiscard]]
        auto moonOpacity() const -> Float {
            using core::normalize;
            using core::Range;

            const auto hour = this->timeOfDay().hour();

            return match (hour) {
                from 0.0 until 3.0   then 1.0f,
                from 3.0 until 6.0   then std::lerp(1.0f, 0.05f, normalize<Float>(hour, 3, 6, 0, 1)),
                from 6.0 until 18.0  then 0.05f,
                from 18.0 until 21.0 then std::lerp(0.05f, 1.0f, normalize<Float>(hour, 18, 21, 0, 1))
            } otherwise (1.0f);
        }

        [[nodiscard]]
        auto starOpacity() const -> Float {
            using core::normalize;
            using core::Range;

            const auto hour = this->timeOfDay().hour();

            return match (hour) {
                from 0.0 until 4.0   then 1.0f,
                from 4.0 until 6.0   then std::lerp(1.0f, 0.0f, normalize<Float>(hour, 4, 6, 0, 1)),
                from 6.0 until 18.0  then 0.1f,
                from 18.0 until 20.0 then std::lerp(0.0f, 1.0f, normalize<Float>(hour, 18, 20, 0, 1))
            } otherwise (1.0f);
        }

        auto update() -> Void {
            using engine::Key;
            using engine::key;

            this->frame += 1;

            if (key(Key::period)) {
                this->time += this->delta() * 64.0f;
            } else if (key(Key::comma)) {
                this->time += this->delta() * 32.0f;
            } else {
                this->time += this->delta();
            }

            if (key(Key::i)) {
                engine::message("Controls",
                    "W, A, S, D - Movement\n"
                    "SPACE, SHIFT - Fly up and down\n"
                    "LEFT MOUSE - Break block\n"
                    "COMMA - Speed up time 32 times\n"
                    "PERIOD - Speed up time 64 times"
                );
            }

            const auto start = core::Timer();

            for (auto& entity : this->entities) entity->update();

            // Remove far away chunks from the cache.
            std::vector<ChunkPosition> removeList;

            for (const auto chunk : this->chunkCache)
                if (this->primary()->position.distanceTo({
                    .x = Float(chunk->x * chunkSide),
                    .y = this->primary()->position.y,
                    .z = Float(chunk->z * chunkSide)
                }) > chunkSide * settings::shared.renderDistance * 1.6) // Multiply to account for diagonal and extend past fog.
                    removeList.push_back({ .x = chunk->x, .z = chunk->z });

            for (const auto position : removeList) {
                const auto index = [this, position] {
                    int i = 0; for (const auto chunk : this->chunkCache) {
                        if (chunk->x == position.x and chunk->z == position.z) return i; i += 1;
                    }
                    std::terminate(); // SAFETY: It should be impossible for the cache to contain a nonexistent chunk.
                }();
                const auto x = this->chunkCache[index]->x;
                const auto z = this->chunkCache[index]->z;

                this->chunkCache[index]->transparentMesh.clear();
                this->chunkCache[index]->transparentMesh.shrink_to_fit();
                this->chunkCache[index]->opaqueMesh.clear();
                this->chunkCache[index]->opaqueMesh.shrink_to_fit();
                // if (not this->chunkCache[index]->wasModified) this->chunks.erase({ .x = x, .z = z });
                // this->chunks.erase({ .x = x, .z = z });

                // Swap remove (why is this not a method in C++)
                if (index < chunkCache.count()) {
                    std::swap(chunkCache[index], chunkCache.last());
                    if (this->lastQueriedChunk == chunkCache.last()) this->lastQueriedChunk = nullptr; // Clear the automatic cache
                    // if (not chunkCache.back()->wasModified) delete chunkCache.back();
                    // delete chunkCache.back();
                    chunkCache.dropLast();
                }
            }

            for (int ix = -settings::shared.renderDistance; ix <= settings::shared.renderDistance; ix += 1) {
                for (int iz = -settings::shared.renderDistance; iz <= settings::shared.renderDistance; iz += 1) {
                    const auto ofx = ix + core::divfloor<Int>(this->primary()->position.x, chunkSide);
                    const auto ofz = iz + core::divfloor<Int>(this->primary()->position.z, chunkSide);

                    bool shouldContinue = false;
                    for (const auto chunk : this->chunkCache) if (chunk->x == ofx and chunk->z == ofz) { shouldContinue = true; }
                    if (shouldContinue) continue;

                    // Get or create
                    if (not this->chunks.contains({ .x = ofx, .z = ofz })) {
                        this->chunks.insert({{.x = ofx, .z = ofz}, this->createChunk(ofx, ofz)});
                    }

                    const auto current = this->chunks.at({ .x = ofx, .z = ofz });

                    if (current->opaqueMesh.empty() and current->transparentMesh.empty() and current->isGenerated()) {
                        current->remesh();
                    }

                    if (not std::ranges::contains(this->chunkCache, current)) this->chunkCache.append(current);
                }
            }

            // Sort visible chunks
            std::ranges::sort(this->chunkCache, ChunkDistanceSort(this->primary()->position));

            // Sort inside nearby chunks
            if (this->primary()->position.distanceTo(this->sortedAt) > 1) {
                for (auto chunk : this->chunkCache) chunk->sort();
                this->sortedAt = this->primary()->position;
            }

            // Update sorting on gpu with more precision to avoid flashes of bad sorting.
            if (this->primary()->position.distanceTo(this->sortedAt) > 1) {
                this->hasUnsubmittedMesh = true;
            }

            // TODO(CRITICAL): This constant OS timer querying is insanely expensive. Implement as an OS callback style timer.
            // In fact on macOS this was about 15% or so according to the flame graph.
            // SDL has such functionality but only with 10ms precision.
            // This affects chunk generation speed significantly.
            for (const auto chunk : this->chunkCache | std::views::reverse) {
                if (not chunk->isGenerated()) {
                    while (start.elapsed() < 4) {
                        if (chunk->generator.done()) break;
                        chunk->generator.resume();
                    }
                }
                if (start.elapsed() >= 4) break;
            }

            for (const auto chunk : this->chunkCache) { if (chunk->modificationStatus.modified) {
                chunk->relight();
                chunk->remesh();

                this->relightNeighbors(chunk->x, chunk->z);
                this->remeshAffectedNeighbors(chunk->x, chunk->z, chunk->modificationStatus);

                chunk->modificationStatus = {}; // Reset the status now.
            } }

            if (frame % 60 == 0) {
                engine::setTitle(std::format("Minecraft - {} fps - {} frame ms", Int32(1000.0f / delta()), start.elapsed()).data());
            }

            this->_delta = (Float) this->timer.lap();
        }
    };
}
