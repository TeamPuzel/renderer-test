// Programming 3 assignment --------------------------------------------------------------------------------------------

/// Binary Wavefront File format
///
/// The format appears to be less compact for small, trivial files but more compact for larger, complex files.
/// With good compression like brotli the files I tested ended up about slightly smaller or same size
/// which I feel is good enough as I value simplicity above everything else.
/// Overall there seems to be no advantage to either format beyond choosing between human or machine readability.
///
/// Obviously there is excess padding through explicitly storing default values but that's something
/// compression solves well enough anyway.
///
/// file(.bobj) struct BinaryObj : big_endian {
///    vertex_count: u32
///    texture_coordinate_count: u32
///    normal_count: u32
///    face_count: u32
///    smooth_shading: bool
///    vertices: [Vector<f32, 3>; vertex_count]
///    texture_coordinates: [Vector<f32, 3>; texture_coordinate_count]
///    normals: [Vector<f32, 3>; normal_count]
///    faces: [Face; face_count]
/// }
///
/// struct FaceElement {
///     vertex: u32
///     texture_coordinate: u32
///     normal: u32
/// }
///
/// type Face = [FaceElement; 3]
struct Mesh final {
    struct FaceElement final {
        u32 vertex = 0;
        u32 texture_coordinate = 0;
        u32 normal = 0;
    };

    using Face = std::array<FaceElement, 3>;

    std::vector<math::Vector<f32, 3>> vertices;
    std::vector<math::Vector<f32, 3>> texture_coordinates;
    std::vector<math::Vector<f32, 3>> normals;
    std::vector<Face> faces;

    enum class Shading {
        Flat,
        Smooth
    } shading { Shading::Flat };
};

static auto load_bobj(Io& io, std::string_view path) -> Mesh {
    Mesh mesh;

    auto file = io.read_file(path);
    io::BinaryReader reader = file | std::views::all;

    const auto vertex_count             = reader.u32();
    const auto texture_coordinate_count = reader.u32();
    const auto normal_count             = reader.u32();
    const auto face_count               = reader.u32();
    const auto smooth_shading           = reader.boolean();

    mesh.vertices.reserve(vertex_count);
    mesh.texture_coordinates.reserve(texture_coordinate_count);
    mesh.normals.reserve(normal_count);
    mesh.faces.reserve(face_count);

    for (usize i = 0; i < vertex_count; i += 1) mesh.vertices.push_back({
        reader.f32(),
        reader.f32(),
        reader.f32()
    });

    for (usize i = 0; i < texture_coordinate_count; i += 1) mesh.texture_coordinates.push_back({
        reader.f32(),
        reader.f32(),
        reader.f32()
    });

    for (usize i = 0; i < normal_count; i += 1) mesh.normals.push_back({
        reader.f32(),
        reader.f32(),
        reader.f32()
    });

    const auto read_face_element = [&] -> Mesh::FaceElement {
        return {
            .vertex             = reader.u32(),
            .texture_coordinate = reader.u32(),
            .normal             = reader.u32()
        };
    };

    for (usize i = 0; i < texture_coordinate_count; i += 1) mesh.faces.push_back({
        read_face_element(),
        read_face_element(),
        read_face_element()
    });

    mesh.shading = smooth_shading ? Mesh::Shading::Smooth : Mesh::Shading::Flat;

    return mesh;
}

static auto save_bobj(Io& io, std::string_view path, Mesh const& mesh) {
    std::vector<u8> output;
    output.reserve(
        4 * sizeof(u32) +                                   // counts
        sizeof(bool) +                                      // shading
        mesh.vertices.size() * 3 * sizeof(f32) +            // vertices
        mesh.texture_coordinates.size() * 3 * sizeof(f32) + // texture coordinates
        mesh.normals.size() * 3 * sizeof(f32) +             // normals
        mesh.faces.size() * 3 * 3 * sizeof(u32)             // faces
    );

    io::BinaryWriter writer = std::back_inserter(output);

    writer.u32(mesh.vertices.size());
    writer.u32(mesh.texture_coordinates.size());
    writer.u32(mesh.normals.size());
    writer.u32(mesh.faces.size());
    writer.boolean(mesh.shading == Mesh::Shading::Smooth);

    for (auto vertex : mesh.vertices) {
        writer.f32(vertex.x());
        writer.f32(vertex.y());
        writer.f32(vertex.z());
    }

    for (auto coord : mesh.texture_coordinates) {
        writer.f32(coord.x());
        writer.f32(coord.y());
        writer.f32(coord.z());
    }

    for (auto normal : mesh.normals) {
        writer.f32(normal.x());
        writer.f32(normal.y());
        writer.f32(normal.z());
    }

    for (auto face : mesh.faces) {
        for (auto element : face) {
            writer.u32(element.vertex);
            writer.u32(element.texture_coordinate);
            writer.u32(element.normal);
        }
    }

    io.write_file(path, output);
}

static auto load_obj(Io& io, std::string_view path) -> Mesh {
    const auto data = io.read_file(path);
    const auto obj = std::string_view((char const*) data.data(), data.size());

    Mesh mesh;

    for (const auto line : obj | std::views::split('\n')) {
        auto components = line | std::views::split(' ');

        auto it = components.begin();
        const auto next = [&] -> std::optional<std::string_view> {
            if (it == components.end()) [[unlikely]] return std::nullopt;
            else {
                auto const& ret = *it++;
                return std::string_view(&*ret.begin(), std::ranges::distance(ret));
            }
        };

        if (const auto id = next()) {
            if (id == "v") {
                mesh.vertices.emplace_back(
                    std::stof(std::string(next().value())),
                    std::stof(std::string(next().value())),
                    std::stof(std::string(next().value()))
                );
            } else if (id == "vt") {
                mesh.texture_coordinates.emplace_back(
                    next().transform([] (auto e) { return std::stof(std::string(e)); }).value(),
                    next().transform([] (auto e) { return std::stof(std::string(e)); }).value_or(0.f),
                    next().transform([] (auto e) { return std::stof(std::string(e)); }).value_or(0.f)
                );
            } else if (id == "vn") {
                mesh.normals.emplace_back(
                    std::stof(std::string(next().value())),
                    std::stof(std::string(next().value())),
                    std::stof(std::string(next().value()))
                );
            } else if (id == "f") {
                const static auto parse_face_element = [&] () -> Mesh::FaceElement {
                    auto element_components = next().value() | std::views::split('/');

                    auto it = element_components.begin();
                    const auto next_element_component = [&] -> std::optional<std::string_view> {
                        if (it == element_components.end()) [[unlikely]] return std::nullopt;
                        else {
                            auto const& ret = *it++;
                            return std::string_view(&*ret.begin(), std::ranges::distance(ret));
                        }
                    };

                    // C++ is a very sad language.
                    constexpr auto sv_to_u32 = [] (auto&& v) -> u32 {
                        try {
                            return std::stoul(std::string(v));
                        } catch (std::exception e) {
                            return 0;
                        }
                    };

                    return {
                        .vertex             = next_element_component().transform(sv_to_u32).value() - 1,
                        .texture_coordinate = next_element_component().transform(sv_to_u32).value_or(0) - 1,
                        .normal             = next_element_component().transform(sv_to_u32).value_or(0) - 1
                    };
                };

                mesh.faces.push_back({
                    parse_face_element(),
                    parse_face_element(),
                    parse_face_element()
                });
            } else if (id == "s") [[unlikely]] {
                mesh.shading = std::stoul(std::string(next().value())) ? Mesh::Shading::Smooth : Mesh::Shading::Flat;
            }
        }
    }

    return mesh;
}

static auto save_obj(Io& io, std::string_view path, Mesh const& mesh) {
    std::stringstream out;

    if (mesh.shading == Mesh::Shading::Smooth) out << "s 1" << '\n';

    for (auto v : mesh.vertices) out << "v " << v.x() << ' ' << v.y() << ' ' << v.z() << '\n';

    for (auto vt : mesh.texture_coordinates) {
        out << "vt " << vt.x() << ' ' << vt.y();
        if (vt.z() != 0.f) out << ' ' << vt.z();
        out << '\n';
    }

    for (auto vn : mesh.normals) out << "vn " << vn.x() << " " << vn.y() << " " << vn.z() << '\n';

    for (auto f : mesh.faces) {
        auto write_face_element = [&] (Mesh::FaceElement e) {
            out << (e.vertex + 1);
            if (not mesh.texture_coordinates.empty() or not mesh.normals.empty()) out << '/';
            if (not mesh.texture_coordinates.empty()) out << (e.texture_coordinate + 1);
            if (not mesh.normals.empty()) out << '/' << (e.normal + 1);
        };

        out << "f ";
        write_face_element(f[0]);
        out << ' ';
        write_face_element(f[1]);
        out << ' ';
        write_face_element(f[2]);
        out << '\n';
    }

    auto str = out.str();

    io.write_file(path, { (u8*) str.data(), str.size() });
}

auto main() -> i32 {
    rt::run_io([] (Io& io) {
        auto mesh = load_obj(io, "res/crash.obj");
        save_bobj(io, "crash.bobj", mesh);

        auto re_mesh = load_bobj(io, "crash.bobj");
        save_obj(io, "crash.obj", mesh);
    });
}
