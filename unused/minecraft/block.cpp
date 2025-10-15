// Created by Lua (TeamPuzel) on 09.12.2024.
// Copyright (c) 2024 All rights reserved.
module;
#include <vector>
module Minecraft:Block.Meshing;

import :World;
import :Block;

using namespace minecraft;

constexpr auto vertexOcclusion(const Bool left, const Bool right, const Bool corner) -> Float {
    constexpr Float baseBrightness  = 1.0f;
    constexpr Float occlusionWeight = 0.35f;

    const auto occlusionFactor = Float(left) * 0.5f + Float(right) * 0.5f + Float(corner) * 0.5f;
    return core::clamp(baseBrightness - occlusionFactor * occlusionWeight, 0.0f, 1.0f);
}

auto BasicBlock::mesh(
    World const* world,
    Float x, Float y, Float z,
    FaceMask faces,
    std::vector<BlockVertex>& opaqueVertices,
    std::vector<BlockVertex>& transparentVertices
) const noexcept -> Void {
    auto& vertices = this->traits.hasTransparency ? transparentVertices : opaqueVertices;
    const auto t = TagType(this->tag);
    const auto off = this->atlasOffsets;
    const auto wf = Int(y) % 2 == 0 ? this->windFactor() : -this->windFactor();
    const auto neighbors = world->neighbors(Int(x), Int(y), Int(z));
    const auto recolor = this->color(world, Int(x), Int(y), Int(z));

    constexpr auto a = 1.0f;
    constexpr auto s = Float(halfSideLength);
    constexpr auto c = Float(textureUVCoefficient);

    if (faces.north) { // Back
        const auto light = Float(world->lightLevelAt(x, y, z + 1)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at(0,  1,  1), neighbors.at(-1,  0,  1), neighbors.at(-1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at(0,  1,  1), neighbors.at( 1,  0,  1), neighbors.at( 1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at(0, -1,  1), neighbors.at( 1,  0,  1), neighbors.at( 1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at(0, -1,  1), neighbors.at(-1,  0,  1), neighbors.at(-1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex(-s + x,  s + y,  s + z, c * off.back.x + c, c * off.back.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex( s + x,  s + y,  s + z, c * off.back.x,     c * off.back.y,       r * aotl, g * aotl, b * aotl, a, recolor, t,  wf), // TL
            BlockVertex( s + x, -s + y,  s + z, c * off.back.x,     c * off.back.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex(-s + x,  s + y,  s + z, c * off.back.x + c, c * off.back.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex( s + x, -s + y,  s + z, c * off.back.x,     c * off.back.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex(-s + x, -s + y,  s + z, c * off.back.x + c, c * off.back.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t, -wf)  // BR
        });
    }
    if (faces.south) { // Front
        const auto light = Float(world->lightLevelAt(x, y, z - 1)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at( 1,  0, -1), neighbors.at( 1,  1, -1));
        const auto aotl = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at(-1,  0, -1), neighbors.at(-1,  1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at(-1,  0, -1), neighbors.at(-1, -1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at( 1,  0, -1), neighbors.at( 1, -1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y, -s + z,   c * off.front.x + c, c * off.front.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x,  s + y, -s + z,   c * off.front.x,     c * off.front.y,       r * aotl, g * aotl, b * aotl, a, recolor, t,  wf), // TL
            BlockVertex(-s + x, -s + y, -s + z,   c * off.front.x,     c * off.front.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x,  s + y, -s + z,   c * off.front.x + c, c * off.front.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x, -s + y, -s + z,   c * off.front.x,     c * off.front.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x, -s + y, -s + z,   c * off.front.x + c, c * off.front.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t, -wf)  // BR
        });
    }
    if (faces.east) { // Right
        const auto light = Float(world->lightLevelAt(x + 1, y, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 1,  0,  1), neighbors.at( 1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 1,  0, -1), neighbors.at( 1,  1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 1, -1,  0), neighbors.at( 1,  0, -1), neighbors.at( 1, -1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 1, -1,  0), neighbors.at( 1,  0,  1), neighbors.at( 1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y,  s + z,   c * off.right.x + c, c * off.right.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex( s + x,  s + y, -s + z,   c * off.right.x,     c * off.right.y,       r * aotl, g * aotl, b * aotl, a, recolor, t,  wf), // TL
            BlockVertex( s + x, -s + y, -s + z,   c * off.right.x,     c * off.right.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x,  s + y,  s + z,   c * off.right.x + c, c * off.right.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex( s + x, -s + y, -s + z,   c * off.right.x,     c * off.right.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x, -s + y,  s + z,   c * off.right.x + c, c * off.right.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t, -wf)  // BR
        });
    }
    if (faces.west) { // Left
        const auto light = Float(world->lightLevelAt(x - 1, y, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at(-1,  0, -1), neighbors.at(-1,  1, -1));
        const auto aotl = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at(-1,  0,  1), neighbors.at(-1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at(-1, -1,  0), neighbors.at(-1,  0,  1), neighbors.at(-1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at(-1, -1,  0), neighbors.at(-1,  0, -1), neighbors.at(-1, -1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex(-s + x,  s + y, -s + z,   c * off.left.x + c, c * off.left.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x,  s + y,  s + z,   c * off.left.x,     c * off.left.y,       r * aotl, g * aotl, b * aotl, a, recolor, t,  wf), // TL
            BlockVertex(-s + x, -s + y,  s + z,   c * off.left.x,     c * off.left.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex(-s + x,  s + y, -s + z,   c * off.left.x + c, c * off.left.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x, -s + y,  s + z,   c * off.left.x,     c * off.left.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex(-s + x, -s + y, -s + z,   c * off.left.x + c, c * off.left.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t, -wf)  // BR
        });
    }
    if (faces.up) { // Top
        const auto light = Float(world->lightLevelAt(x, y + 1, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0,  1,  1), neighbors.at( 1,  1,  0), neighbors.at( 1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at( 0,  1,  1), neighbors.at(-1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at(-1,  1,  0), neighbors.at(-1,  1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 0,  1, -1), neighbors.at( 1,  1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y,  s + z,   c * off.top.x + c, c * off.top.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x,  s + y,  s + z,   c * off.top.x,     c * off.top.y,       r * aotl, g * aotl, b * aotl, a, recolor, t,  wf), // TL
            BlockVertex(-s + x,  s + y, -s + z,   c * off.top.x,     c * off.top.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t,  wf), // BL
            BlockVertex( s + x,  s + y,  s + z,   c * off.top.x + c, c * off.top.y,       r * aotr, g * aotr, b * aotr, a, recolor, t,  wf), // TR
            BlockVertex(-s + x,  s + y, -s + z,   c * off.top.x,     c * off.top.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t,  wf), // BL
            BlockVertex( s + x,  s + y, -s + z,   c * off.top.x + c, c * off.top.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t,  wf)  // BR
        });
    }
    if (faces.down) { // Bottom
        const auto light = Float(world->lightLevelAt(x, y - 1, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at( 1, -1,  0), neighbors.at( 1, -1, -1));
        const auto aotl = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at(-1, -1,  0), neighbors.at(-1, -1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 0, -1,  1), neighbors.at(-1, -1,  0), neighbors.at(-1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at( 0, -1,  1), neighbors.at( 1, -1,  0), neighbors.at( 1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x, -s + y, -s + z,   c * off.bottom.x + c, c * off.bottom.y,       r * aotr, g * aotr, b * aotr, a, recolor, t, -wf), // TR
            BlockVertex(-s + x, -s + y, -s + z,   c * off.bottom.x,     c * off.bottom.y,       r * aotl, g * aotl, b * aotl, a, recolor, t, -wf), // TL
            BlockVertex(-s + x, -s + y,  s + z,   c * off.bottom.x,     c * off.bottom.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x, -s + y, -s + z,   c * off.bottom.x + c, c * off.bottom.y,       r * aotr, g * aotr, b * aotr, a, recolor, t, -wf), // TR
            BlockVertex(-s + x, -s + y,  s + z,   c * off.bottom.x,     c * off.bottom.y + c,   r * aobl, g * aobl, b * aobl, a, recolor, t, -wf), // BL
            BlockVertex( s + x, -s + y,  s + z,   c * off.bottom.x + c, c * off.bottom.y + c,   r * aobr, g * aobr, b * aobr, a, recolor, t, -wf)  // BR
        });
    }
}

auto CrossBlock::mesh(
    World const* world,
    Float x, Float y, Float z,
    FaceMask faces,
    std::vector<BlockVertex>& opaqueVertices,
    std::vector<BlockVertex>& transparentVertices
) const noexcept -> Void {
    auto& vertices = this->traits.hasTransparency ? transparentVertices : opaqueVertices;
    const auto t = TagType(this->tag);
    const auto off = this->atlasOffsets;
    const auto wf = this->windFactor();

    constexpr auto a = 1.0f;
    constexpr auto s = Float(halfSideLength);
    constexpr auto c = Float(textureUVCoefficient);

    const auto rec = this->color(world, Int(x), Int(y), Int(z));

    const float light = Float(world->lightLevelAt(x, y, z)) / 15.0f;
    const float r = light;
    const float g = light;
    const float b = light;

    vertices.insert(vertices.end(), { // Left
        BlockVertex(-s + x,  s + y,  s + z, c * off.left.x + c, c * off.left.y,       r, g, b, a, rec, t, wf), // TR
        BlockVertex( s + x,  s + y, -s + z, c * off.left.x,     c * off.left.y,       r, g, b, a, rec, t, wf), // TL
        BlockVertex( s + x, -s + y, -s + z, c * off.left.x,     c * off.left.y + c,   r, g, b, a, rec, t    ), // BL
        BlockVertex(-s + x,  s + y,  s + z, c * off.left.x + c, c * off.left.y,       r, g, b, a, rec, t, wf), // TR
        BlockVertex( s + x, -s + y, -s + z, c * off.left.x,     c * off.left.y + c,   r, g, b, a, rec, t    ), // BL
        BlockVertex(-s + x, -s + y,  s + z, c * off.left.x + c, c * off.left.y + c,   r, g, b, a, rec, t    )  // BR
    });
    vertices.insert(vertices.end(), { // Right
        BlockVertex(-s + x,  s + y, -s + z, c * off.right.x + c, c * off.right.y,       r, g, b, a, rec, t, wf), // TR
        BlockVertex( s + x,  s + y,  s + z, c * off.right.x,     c * off.right.y,       r, g, b, a, rec, t, wf), // TL
        BlockVertex( s + x, -s + y,  s + z, c * off.right.x,     c * off.right.y + c,   r, g, b, a, rec, t    ), // BL
        BlockVertex(-s + x,  s + y, -s + z, c * off.right.x + c, c * off.right.y,       r, g, b, a, rec, t, wf), // TR
        BlockVertex( s + x, -s + y,  s + z, c * off.right.x,     c * off.right.y + c,   r, g, b, a, rec, t    ), // BL
        BlockVertex(-s + x, -s + y, -s + z, c * off.right.x + c, c * off.right.y + c,   r, g, b, a, rec, t    )  // BR
    });
}

using namespace minecraft;

/// TODO(CRITICAL): Unhardcode water animated texture and implment properly as custom texture functionality.
///                 Alternatively just have water implement itself.
auto LiquidBlock::mesh(
    World const* world,
    Float x, Float y, Float z,
    FaceMask faces,
    std::vector<BlockVertex>& opaqueVertices,
    std::vector<BlockVertex>& transparentVertices
) const noexcept -> Void {
    auto& vertices = this->traits.hasTransparency ? transparentVertices : opaqueVertices;
    const auto t = TagType(this->tag);
    const auto off = this->atlasOffsets;
    const auto neighbors = world->neighbors(Int(x), Int(y), Int(z));
    const auto recolor = this->color(world, Int(x), Int(y), Int(z));

    constexpr auto a = 1.0f;
    constexpr auto s = Float(halfSideLength);
    constexpr auto o = Float(LiquidBlock::topOffset);

    if (faces.north) { // Back
        const auto light = Float(world->lightLevelAt(x, y, z + 1)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at(0,  1,  1), neighbors.at(-1,  0,  1), neighbors.at(-1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at(0,  1,  1), neighbors.at( 1,  0,  1), neighbors.at( 1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at(0, -1,  1), neighbors.at( 1,  0,  1), neighbors.at( 1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at(0, -1,  1), neighbors.at(-1,  0,  1), neighbors.at(-1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex(-s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex( s + x,  s + y + o,  s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex( s + x, -s + y,      s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex(-s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex( s + x, -s + y,      s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex(-s + x, -s + y,      s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
    if (faces.south) { // Front
        const auto light = Float(world->lightLevelAt(x, y, z - 1)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at( 1,  0, -1), neighbors.at( 1,  1, -1));
        const auto aotl = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at(-1,  0, -1), neighbors.at(-1,  1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at(-1,  0, -1), neighbors.at(-1, -1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at( 1,  0, -1), neighbors.at( 1, -1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y + o, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x,  s + y + o, -s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex(-s + x, -s + y,     -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x,  s + y + o, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x, -s + y,     -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x, -s + y,     -s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
    if (faces.east) { // Right
        const auto light = Float(world->lightLevelAt(x + 1, y, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 1,  0,  1), neighbors.at( 1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 1,  0, -1), neighbors.at( 1,  1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 1, -1,  0), neighbors.at( 1,  0, -1), neighbors.at( 1, -1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 1, -1,  0), neighbors.at( 1,  0,  1), neighbors.at( 1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex( s + x,  s + y + o, -s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex( s + x, -s + y,     -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex( s + x, -s + y,     -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x, -s + y,      s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
    if (faces.west) { // Left
        const auto light = Float(world->lightLevelAt(x - 1, y, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at(-1,  0, -1), neighbors.at(-1,  1, -1));
        const auto aotl = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at(-1,  0,  1), neighbors.at(-1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at(-1, -1,  0), neighbors.at(-1,  0,  1), neighbors.at(-1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at(-1, -1,  0), neighbors.at(-1,  0, -1), neighbors.at(-1, -1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex(-s + x,  s + y + o, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x,  s + y + o,  s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex(-s + x, -s + y,      s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex(-s + x,  s + y + o, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x, -s + y,      s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex(-s + x, -s + y,     -s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
    if (faces.up) { // Top
        const auto light = Float(world->lightLevelAt(x, y + 1, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0,  1,  1), neighbors.at( 1,  1,  0), neighbors.at( 1,  1,  1));
        const auto aotl = vertexOcclusion(neighbors.at(-1,  1,  0), neighbors.at( 0,  1,  1), neighbors.at(-1,  1,  1));
        const auto aobl = vertexOcclusion(neighbors.at( 0,  1, -1), neighbors.at(-1,  1,  0), neighbors.at(-1,  1, -1));
        const auto aobr = vertexOcclusion(neighbors.at( 1,  1,  0), neighbors.at( 0,  1, -1), neighbors.at( 1,  1, -1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x,  s + y + o,  s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex(-s + x,  s + y + o, -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x,  s + y + o,  s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x,  s + y + o, -s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x,  s + y + o, -s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
    if (faces.down) { // Bottom
        const auto light = Float(world->lightLevelAt(x, y - 1, z)) / 15.0f;
        const auto r = light;
        const auto g = light;
        const auto b = light;

        const auto aotr = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at( 1, -1,  0), neighbors.at( 1, -1, -1));
        const auto aotl = vertexOcclusion(neighbors.at( 0, -1, -1), neighbors.at(-1, -1,  0), neighbors.at(-1, -1, -1));
        const auto aobl = vertexOcclusion(neighbors.at( 0, -1,  1), neighbors.at(-1, -1,  0), neighbors.at(-1, -1,  1));
        const auto aobr = vertexOcclusion(neighbors.at( 0, -1,  1), neighbors.at( 1, -1,  0), neighbors.at( 1, -1,  1));

        vertices.insert(vertices.end(), {
            BlockVertex( s + x, -s + y, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x, -s + y, -s + z,  0, 0,  r * aotl, g * aotl, b * aotl, a, recolor, t), // TL
            BlockVertex(-s + x, -s + y,  s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x, -s + y, -s + z,  1, 0,  r * aotr, g * aotr, b * aotr, a, recolor, t), // TR
            BlockVertex(-s + x, -s + y,  s + z,  0, 1,  r * aobl, g * aobl, b * aobl, a, recolor, t), // BL
            BlockVertex( s + x, -s + y,  s + z,  1, 1,  r * aobr, g * aobr, b * aobr, a, recolor, t)  // BR
        });
    }
}
