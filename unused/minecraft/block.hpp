// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <vector>
#include <bitset>
export module Minecraft:Block;

import Core;
import Engine;

namespace minecraft {
    export class World;
    using engine::Color;

    export namespace block {
        /// The length of a block side.
        constexpr auto sideLength = 1.0;
        /// Half a block side length for use in vertices.
        constexpr auto halfSideLength = sideLength / 2.0;
        /// The resolution of a single terrain atlas tile.
        constexpr auto textureBlockSize = 16.0;
        /// The resolution of the terrain atlas.
        constexpr auto textureSideLength = 256.0;
    }

    using namespace block;

    /// A coefficient for converting block coordinates to uv coordinates.
    constexpr auto textureUVCoefficient = textureBlockSize / textureSideLength;

    export using TagType = UInt32;

    /// Note that the tag values are fixed, they are used to identify blocks in shaders.
    export enum class Tag: TagType {
        air       = 0,
        stone     = 1,
        dirt      = 2,
        grass     = 3,
        water     = 4,
        sand      = 5,
        leaves    = 6,
        log       = 7,
        bedrock   = 8,
        tallgrass = 9,
        rose      = 10
    };

    /// Volatile layout - This type is passed directly to the GPU so this exact layout is expected and must not change
    /// without also adjusting terrain shader code.
    export struct BlockVertex final {
        struct Position final { Float x, y, z; } position;
        struct TextureCoordinates final { Float u, v; } textureCoordinates;
        Color color, recolor;
        Float windFactor;
        TagType tag;

        /// This is a convenience constructor.
        constexpr BlockVertex(
            Float x, Float y, Float z,
            Float u, Float v,
            Float r, Float g, Float b, Float a,
            Color recolor,
            TagType tag,
            Float windFactor = 0.0f
        ) noexcept : position { x, y, z },
                     textureCoordinates { u, v },
                     color { r, g, b, a },
                     recolor { recolor },
                     tag { tag },
                     windFactor { windFactor } {}
    };

    /// Used to compute a mesh.
    export struct FaceMask final {
        Bool north : 1 { false },
             south : 1 { false },
             east  : 1 { false },
             west  : 1 { false },
             up    : 1 { false },
             down  : 1 { false };
    };

    /// Used to compute ambient occlusion.
    export struct NeighborMask final {
        std::bitset<27> bits;

        auto at(this const NeighborMask self, Int x, Int y, Int z) -> Bool {
            constexpr auto width = 3, height = 3;
            x += 1; y += 1; z += 1; // We want 0 0 0 to be the center, so offset everything.
            const auto offset = z * height * width + y * width + x;
            return self.bits[offset];
        }

        auto ref_at(this NeighborMask& self, Int x, Int y, Int z) -> decltype(bits)::reference {
            constexpr auto width = 3, height = 3;
            x += 1; y += 1; z += 1; // We want 0 0 0 to be the center, so offset everything.
            const auto offset = z * height * width + y * width + x;
            return self.bits[offset];
        }
    };

    export struct Traits final {
        /// Contains transparency.
        const Bool hasTransparency { false };
        /// Is this block combined with same tag neighbors (if transparent)
        const Bool bunchTogether { false };
        /// Can this block be broken.
        const Bool isBreakable { true };
        /// Can this block be replaced by structures while generating chunks.
        const Bool softGeneration { false };
        /// Does the block break if the block below is broken.
        const Bool weakFoundation { false };
        /// How much light is subtracted when passing through this block.
        const UInt8 lightAbsorption { 15 };
    };
}

export namespace minecraft {
    /// The block base class.
    ///
    /// Virtual dispatch has about the same cost as any other call, but it won't be inlined. Yet meshing blocks
    /// has to be a massive chain of if statements. Branch misprediction has almost exactly the same cost
    /// as virtual dispatch, but with as many if statements as I had it is probably worse.
    ///
    /// Additionally, the only major critical path here, meshing, is not very optimizable anyway as it relies purely
    /// on information known at runtime. That being said Air should not be a block but semantically nullptr, it carries no
    /// useful data and calling its empty virtual meshing function is unnecessary.
    class Block {
    public:
        const Tag tag;
        Traits traits;

    protected:
        explicit Block(const Tag tag, const Traits traits = {}) : tag { tag }, traits { traits } {}

    public:
        /// Compute the mesh for the block.
        ///
        /// Preconditions:
        /// True immutability - Constant pointers must guarantee to never be mutated externally as an effect during the call.
        /// Exclusivity - There can't be any other references to `vertices`.
        ///
        /// Invariants:
        /// In-out semantics
        virtual auto mesh(
            World const* world,
            Float x, Float y, Float z,
            FaceMask faces,
            std::vector<BlockVertex>& opaqueVertices,
            std::vector<BlockVertex>& transparentVertices
        ) const noexcept -> Void {}

        /// Determine the color adjustment for the block, black by default (has no effect).
        virtual auto color(World const* world, Int x, Int y, Int z) const noexcept -> Color {
            return engine::hexa(0x00000000);
        }

        [[nodiscard]] virtual auto windFactor() const noexcept -> Float { return 0.0f; }

        virtual ~Block() noexcept = default;
    };

    class Air: public Block {
    public:
        Air() noexcept : Block(Tag::air, {
            .hasTransparency = true,
            .isBreakable = false,
            .softGeneration = true,
            .lightAbsorption = 0
        }) {}
    };

    class BasicBlock: public Block {
    protected:
        struct AtlasOffsets final {
            struct Offset final { Float x, y; } front, back, left, right, top, bottom;
        } atlasOffsets;

    public:
        explicit BasicBlock(
            const Tag tag,
            const AtlasOffsets offsets,
            const Traits traits = {}
        ) : Block(tag, traits), atlasOffsets { offsets } {}

        auto mesh(
            World const* world,
            Float x, Float y, Float z,
            FaceMask faces,
            std::vector<BlockVertex>& opaqueVertices,
            std::vector<BlockVertex>& transparentVertices
        ) const noexcept -> Void override;
    };

    class CrossBlock: public Block {
    protected:
        struct AtlasOffsets final {
            struct Offset final { Float x, y; } left, right;
        } atlasOffsets;

    public:
        explicit CrossBlock(
            const Tag tag,
            const AtlasOffsets offsets,
            const Traits traits = {}
        ) : Block(tag, traits), atlasOffsets { offsets } {}

        auto mesh(
            World const* world,
            Float x, Float y, Float z,
            FaceMask faces,
            std::vector<BlockVertex>& opaqueVertices,
            std::vector<BlockVertex>& transparentVertices
        ) const noexcept -> Void override;
    };

    class LiquidBlock: public Block {
    protected:
        struct AtlasOffsets final {
            struct Offset final { Float x, y; } front, back, left, right, top, bottom;
        } atlasOffsets;

    public:
        /// The y offset of top vertices used by liquids.
        static constexpr auto topOffset = -2.0 / 16.0;

        explicit LiquidBlock(
            const Tag tag,
            const AtlasOffsets offsets,
            const Traits traits = {}
        ) : Block(tag, traits), atlasOffsets { offsets } {}

        auto mesh(
            World const* world,
            Float x, Float y, Float z,
            FaceMask faces,
            std::vector<BlockVertex>& opaqueVertices,
            std::vector<BlockVertex>& transparentVertices
        ) const noexcept -> Void override;
    };

    class Stone final: public BasicBlock {
    public:
        Stone() : BasicBlock(Tag::stone, {
            .front  = { .x = 1, .y = 0 },
            .back   = { .x = 1, .y = 0 },
            .left   = { .x = 1, .y = 0 },
            .right  = { .x = 1, .y = 0 },
            .top    = { .x = 1, .y = 0 },
            .bottom = { .x = 1, .y = 0 }
        }) {}
    };

    class Dirt final: public BasicBlock {
    public:
        Dirt() : BasicBlock(Tag::dirt, {
            .front  = { .x = 2, .y = 0 },
            .back   = { .x = 2, .y = 0 },
            .left   = { .x = 2, .y = 0 },
            .right  = { .x = 2, .y = 0 },
            .top    = { .x = 2, .y = 0 },
            .bottom = { .x = 2, .y = 0 }
        }) {}
    };

    class Grass final: public BasicBlock {
    public:
        Grass() : BasicBlock(Tag::grass, {
            .front  = { .x = 3, .y = 0 },
            .back   = { .x = 3, .y = 0 },
            .left   = { .x = 3, .y = 0 },
            .right  = { .x = 3, .y = 0 },
            .top    = { .x = 0, .y = 0 },
            .bottom = { .x = 2, .y = 0 }
        }) {}

        auto color(World const * world, Int x, Int y, Int z) const noexcept -> Color override {
            return engine::hex(0x79C05A); // Just forest color for now since there's no biomes.
        }
    };

    class Sand final: public BasicBlock {
    public:
        Sand() : BasicBlock(Tag::sand, {
            .front  = { .x = 2, .y = 1 },
            .back   = { .x = 2, .y = 1 },
            .left   = { .x = 2, .y = 1 },
            .right  = { .x = 2, .y = 1 },
            .top    = { .x = 2, .y = 1 },
            .bottom = { .x = 2, .y = 1 }
        }) {}
    };

    class Water final: public LiquidBlock {
    public:
        Water() : LiquidBlock(Tag::water, {
            .front  = { .x = 13, .y = 12 },
            .back   = { .x = 13, .y = 12 },
            .left   = { .x = 13, .y = 12 },
            .right  = { .x = 13, .y = 12 },
            .top    = { .x = 13, .y = 12 },
            .bottom = { .x = 13, .y = 12 }
        }, {
            .hasTransparency = true,
            .bunchTogether = true,
            .isBreakable = false,
            .lightAbsorption = 2
        }) {}

        auto color(World const * world, Int x, Int y, Int z) const noexcept -> Color override {
            return engine::hex(0x0084FF); // Just river color for now since there's no biomes.
        }
    };

    class Leaves final: public BasicBlock {
    public:
        Leaves() : BasicBlock(Tag::leaves, {
            .front  = { .x = 4, .y = 3 },
            .back   = { .x = 4, .y = 3 },
            .left   = { .x = 4, .y = 3 },
            .right  = { .x = 4, .y = 3 },
            .top    = { .x = 4, .y = 3 },
            .bottom = { .x = 4, .y = 3 }
        }, {
            .hasTransparency = true,
            .lightAbsorption = 1
        }) {}

        auto color(World const * world, Int x, Int y, Int z) const noexcept -> Color override {
            return engine::hex(0x59AE30); // Just forest color for now since there's no biomes.
        }

        [[nodiscard]] auto windFactor() const noexcept -> Float override { return 0.1f; }
    };

    class Log final: public BasicBlock {
    public:
        Log() : BasicBlock(Tag::log, {
            .front  = { .x = 4, .y = 1 },
            .back   = { .x = 4, .y = 1 },
            .left   = { .x = 4, .y = 1 },
            .right  = { .x = 4, .y = 1 },
            .top    = { .x = 5, .y = 1 },
            .bottom = { .x = 5, .y = 1 }
        }) {}
    };

    class Bedrock final: public BasicBlock {
    public:
        Bedrock() : BasicBlock(Tag::bedrock, {
            .front  = { .x = 1, .y = 1 },
            .back   = { .x = 1, .y = 1 },
            .left   = { .x = 1, .y = 1 },
            .right  = { .x = 1, .y = 1 },
            .top    = { .x = 1, .y = 1 },
            .bottom = { .x = 1, .y = 1 }
        }) {}
    };

    class TallGrass final: public CrossBlock {
    public:
        TallGrass() : CrossBlock(Tag::tallgrass, {
            .left  = { .x = 7, .y = 2 },
            .right = { .x = 7, .y = 2 },
        }, {
            .hasTransparency = true,
            .softGeneration = true,
            .weakFoundation = true,
            .lightAbsorption = 0
        }) {}

        auto color(World const * world, Int x, Int y, Int z) const noexcept -> Color override {
            return engine::hex(0x79C05A); // Just forest grass color for now since there's no biomes.
        }

        [[nodiscard]] auto windFactor() const noexcept -> Float override { return 1.0f; }
    };

    class Rose final: public CrossBlock {
    public:
        Rose() : CrossBlock(Tag::rose, {
            .left  = { .x = 12, .y = 0 },
            .right = { .x = 12, .y = 0 },
        }, {
            .hasTransparency = true,
            .softGeneration = true,
            .weakFoundation = true,
            .lightAbsorption = 0
        }) {}

        [[nodiscard]] auto windFactor() const noexcept -> Float override { return 0.2f; }
    };

    /// Returns a pointer to a shared instance of a concrete block.
    template <core::Subtype<Block> B> auto create() -> B* {
        static auto shared = B();
        return &shared;
    }

    /// Returns a pointer to a shared instance of a type erased block.
    ///
    /// This is just for ternary expressions because C++ is dumb and dies instead of handling variance sensibly...
    template <core::Subtype<Block> B> auto anycreate() -> Block* {
        static auto shared = B();
        return &shared;
    }
}

namespace minecraft {
    /// A 4x4 camera facing block particle offset into the terrain atlas, by default positioned at side center.
    struct Particle final {
        Float x, y, z;
        Float vx, vy, vz;
        Block const* source;
    };
}
