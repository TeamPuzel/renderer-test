#pragma once
#include <math>
#include <draw>
#include <rt>
#include <limits>
#include <type_traits>
#include <variant>
#include <utility>
#include <concepts>
#include <vector>
#include <span>
#include <thread>
#include <ranges>
#include <random>

namespace raytracer {
    /// A simple floating point color type.
    /// It implements a lossy implicit conversion to and from draw::Color.
    /// I do not particularly care as I would never touch floats (only integers or fixed point) in my own code.
    /// However, since the assignment said that doing things properly would not count, here's a horrible, imprecise
    /// color type with all the misguided semantics like NaN and infinity.
    struct Color final {
        f32 r { 0.f }, g { 0.f }, b { 0.f };

        constexpr explicit(false) Color(draw::Color color = draw::color::CLEAR)
            : r(f32(color.r) / 255), g(f32(color.g) / 255), b(f32(color.b) / 255) {}

        constexpr explicit(false) operator draw::Color () const {
            return draw::Color::rgba(
                std::clamp(r * 255.f, 0.f, 255.f),
                std::clamp(g * 255.f, 0.f, 255.f),
                std::clamp(b * 255.f, 0.f, 255.f)
            );
        }

        constexpr explicit(false) Color(math::Vector<f32, 3> vector) : r(vector[0]), g(vector[1]), b(vector[2]) {}

        constexpr explicit(false) operator math::Vector<f32, 3> () const {
            return { r, g, b };
        }

        constexpr Color(f32 r, f32 g, f32 b) : r(r), g(g), b(b) {}

        [[clang::always_inline]] [[gnu::const]]
        static constexpr Color rgb(f32 r, f32 g, f32 b) noexcept {
            return Color { r, g, b };
        }

        [[clang::always_inline]] [[gnu::const]]
        constexpr auto operator==(this Color self, Color other) noexcept -> bool {
            return self.r == other.r and self.g == other.g and self.b == other.b;
        }

        [[clang::always_inline]] [[gnu::const]]
        constexpr auto operator!=(this Color self, Color other) noexcept -> bool {
            return !(self == other);
        }
    };

    struct Sphere final {
        math::Vector<f32, 3> position;
        f32 radius;
    };

    struct Plane final {
        math::Vector<f32, 3> position, normal;
    };

    struct Mesh final {
        math::Vector<f32, 3> position;
        std::vector<math::Vector<f32, 3>> tris;
        std::vector<std::array<usize, 3>> faces;
        f32 scale { 1.f };
        math::Angle<f32> pitch { 0.f };
        math::Angle<f32> yaw { 0.f };
        math::Angle<f32> roll { 0.f };

        enum class Shading {
            Flat,
            Smooth
        } shading { Shading::Flat };
    };

    struct PointLight final {
        math::Vector<f32, 3> position;
        raytracer::Color color;
    };

    struct Hit final {
		math::Vector<f32, 3> origin;
		math::Vector<f32, 3> normal;
		f32 distance { std::numeric_limits<f32>::max() };

		usize material_index { 0 };
	};

	class World;

	class Material {
	  public:
    	virtual auto shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color = 0;

        virtual constexpr auto operator==(Material const& rhs) const -> bool = 0;

        virtual ~Material() {}
	};

    class SolidColorMaterial final : public Material {
        raytracer::Color color;

      public:
		constexpr explicit SolidColorMaterial(raytracer::Color color) : color(color) {}

		auto shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color override {
		    return color;
		}

		constexpr auto operator==(Material const& rhs) const -> bool override {
		    const auto same = dynamic_cast<SolidColorMaterial const*>(&rhs);
		    return same and color == same->color;
		}
	};

	class LambertMaterial final : public Material {
	    raytracer::Color color;
		f32 diffuse_reflectance;

	  public:
		constexpr explicit LambertMaterial(raytracer::Color color, f32 diffuse_reflectance = 1.f)
		    : color(color), diffuse_reflectance(diffuse_reflectance) {}

		auto shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color override;

		constexpr auto operator==(Material const& rhs) const -> bool override {
		    const auto same = dynamic_cast<LambertMaterial const*>(&rhs);
		    return same and color == same->color;
		}
	};

	class BsdfMaterial final : public Material {
	    raytracer::Color color;
		f32 roughness;
		f32 metallic;

	  public:
		struct Config final {
		    raytracer::Color color { draw::color::pico::GRAY };
			f32 roughness { 1.f };
			f32 metallic { 0.f };
		};

		constexpr explicit(false) BsdfMaterial(Config config)
		    : color(config.color), roughness(config.roughness), metallic(config.metallic) {}

		auto shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color override;

		constexpr auto operator==(Material const& rhs) const -> bool override {
		    const auto same = dynamic_cast<BsdfMaterial const*>(&rhs);
		    return same and color == same->color and roughness == same->roughness and metallic == same->metallic;
		}

		enum class Mode {
		    Default,
			Diffuse,
			CookTorrance,
			Fresnel,
			NormalDistribution,
			Microfacets
		};
	};

	constexpr std::ostream& operator<<(std::ostream& os, BsdfMaterial::Mode const& value) {
	    switch (value) {
            case BsdfMaterial::Mode::Default:            os << "Default";            break;
            case BsdfMaterial::Mode::Diffuse:            os << "Diffuse";            break;
            case BsdfMaterial::Mode::CookTorrance:       os << "CookTorrance";       break;
            case BsdfMaterial::Mode::Fresnel:            os << "Fresnel";            break;
            case BsdfMaterial::Mode::NormalDistribution: os << "NormalDistribution"; break;
            case BsdfMaterial::Mode::Microfacets:        os << "Microfacets";        break;
        }

        return os;
    }

    using Shape = std::variant<Sphere, Plane, Mesh>;

    template <typename T, typename... Us> concept any_of = (std::same_as<T, Us> or ...);

    template <typename T, typename U> concept subtype = std::is_base_of<U, T>::value;

    template <
        std::input_iterator I,
        // ???
        // std::equality_comparable_with<decltype(*std::declval<typename std::iterator_traits<I>::value_type>)> E
        typename E
    > auto indirect_find(I begin, I end, E const& e) -> I {
        for (; begin != end; ++begin) if (**begin == e) return begin;
        return begin;
    }

    static auto load_mesh(Io& io, std::string_view path) -> Mesh {
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
                    mesh.tris.emplace_back(
                        std::stof(std::string(next().value())),
                        std::stof(std::string(next().value())),
                        std::stof(std::string(next().value()))
                    );
                } else if (id == "f") {
                    mesh.faces.push_back({
                        std::stoul(std::string(next().value())) - 1,
                        std::stoul(std::string(next().value())) - 1,
                        std::stoul(std::string(next().value())) - 1
                    });
                } else if (id == "s") [[unlikely]] {
                    mesh.shading = std::stoul(std::string(next().value())) ? Mesh::Shading::Smooth : Mesh::Shading::Flat;
                }
            }
        }

        return mesh;
    }

    class World final {
        // Collection of shapes and their bound materials.
        std::vector<std::pair<Shape, usize>> object_data;
        // Collection of materials where indices remain consistent.
        std::vector<Box<Material>> material_data;
        // Collection of point lights.
        std::vector<PointLight> light_data;

        math::Vector<f32, 3> camera_position;
        math::Angle<f32> camera_pitch { 0.f };
        math::Angle<f32> camera_yaw { 0.f };
        math::Angle<f32> camera_roll { 0.f };

        raytracer::Color background_color { draw::color::BLACK };

        math::Angle<f32> fov { math::deg(80.f).radians() };
        bool checkerboard { true };
        bool shadows { true };
        BsdfMaterial::Mode bsdf_mode { BsdfMaterial::Mode::Default };

      public:
        World() {
            material_data.push_back(Box<SolidColorMaterial>::make(SolidColorMaterial(draw::color::pico::RED)));
        }

        auto lights() const -> std::span<const PointLight> {
            return light_data;
        }

        auto material(usize index) const -> Material const& {
            return *material_data[index];
        }

        auto get_background_color() const -> raytracer::Color {
            return background_color;
        }

        /// Since objects are not guaranteed a stable address in memory, a reference type is provided
        /// to provide a handle to an object which can outlive storage resizing.
        template <typename Object> class Ref {
            World * world;
            usize index;

            Ref(World * world, usize index) : world(world), index(index) {}

          public:
            friend class World;

            Ref() : world(nullptr), index(0) {}

            auto operator*() const -> Object& { return std::get<Object>(world->object_data[index].first); }
            auto operator->() const -> Object* { return &std::get<Object>(world->object_data[index].first); }
        };

        template <any_of<Sphere, Plane, Mesh> Object, subtype<Material> Mat> auto add(Object object, Mat material) -> Ref<Object> {
            usize material_index;
            if (const auto it = indirect_find(material_data.begin(), material_data.end(), material); it != material_data.end()) {
                material_index = it - material_data.begin();
            } else {
                material_data.push_back(Box<Mat>::make(material));
                material_index = material_data.size() - 1;
            }

            object_data.push_back({ object, material_index });
            return { this, object_data.size() - 1 };
        }

        template <any_of<Sphere, Plane, Mesh> Object> auto add(Object object) -> Ref<Object> {
            return add(object, SolidColorMaterial(draw::color::pico::RED));
        }

        void add(PointLight light) {
            light_data.push_back(light);
        }

        void move(math::Vector<f32, 3> vector) {
            using Matrix = math::Matrix<f32, 3, 3>;
            const auto rotation_matrix = Matrix::rotation(Matrix::RotationAxis::Yaw, camera_yaw);
            camera_position += vector * rotation_matrix;
        }

        void set_fov(math::Angle<f32> angle) {
            fov = angle;
        }

        auto get_fov() const -> math::Angle<f32> {
            return fov;
        }

        void set_checkerboard(bool value) {
            checkerboard = value;
        }

        auto get_checkerboard() const -> bool {
            return checkerboard;
        }

        void set_shadows(bool value) {
            shadows = value;
        }

        [[gnu::const]]
        auto get_shadows() const -> bool {
            return shadows;
        }

        void set_bsdf_mode(BsdfMaterial::Mode value) {
            bsdf_mode = value;
        }

        [[gnu::const]]
        auto get_bsdf_mode() const -> BsdfMaterial::Mode {
            return bsdf_mode;
        }

        void cycle_bsdf_mode() {
            using enum BsdfMaterial::Mode;
            switch (bsdf_mode) {
                case Default:            bsdf_mode = Diffuse;            break;
                case Diffuse:            bsdf_mode = CookTorrance;       break;
                case CookTorrance:       bsdf_mode = Fresnel;            break;
                case Fresnel:            bsdf_mode = NormalDistribution; break;
                case NormalDistribution: bsdf_mode = Microfacets;        break;
                case Microfacets:        bsdf_mode = Default;            break;
            }
        }

        auto random() const -> f32 {
            static std::mt19937 rng;
            static std::uniform_real_distribution<float> dist { 0.f, 1.f };
            return dist(rng);
        }

        [[gnu::const]]
        auto get_camera_position() const -> math::Vector<f32, 3> {
            return camera_position;
        }

        void rotate_pitch(math::Angle<f32> angle) {
            camera_pitch += angle;
        }

        void rotate_yaw(math::Angle<f32> angle) {
            camera_yaw += angle;
        }

        void rotate_roll(math::Angle<f32> angle) {
            camera_roll += angle;
        }

        [[gnu::const]] // Mark const since this is a pure function and can be optimized away.
        auto rotation_matrix() const -> math::Matrix<f32, 3, 3> {
            using Matrix = math::Matrix<f32, 3, 3>;
            return Matrix::rotation(Matrix::RotationAxis::Pitch, camera_pitch)
                 * Matrix::rotation(Matrix::RotationAxis::Yaw, camera_yaw);
        }

        [[gnu::const]] // Mark const since this is a pure function and can be optimized away.
        auto view_direction() const -> math::Vector<f32, 3> {
            return math::Vector<f32, 3> { 0.f, 0.f, 1.f } * rotation_matrix();
        }

        auto cast_ray(math::Vector<f32, 3> origin, math::Vector<f32, 3> direction) const -> std::optional<Hit> {
            std::optional<Hit> best_hit;

            for (auto const& [shape, material] : object_data) {
                std::visit(
                    [&] (auto const& object) {
                        using T = std::decay_t<decltype(object)>;

                        if constexpr (std::same_as<T, Sphere>) {
                            auto l = origin - object.position;
                            f32 a = direction.dot(direction);
                            f32 b = 2.0f * direction.dot(l);
                            f32 c = l.dot(l) - object.radius * object.radius;

                            f32 disc = b * b - 4 * a * c;
                            if (disc >= 0) {
                                f32 sqrt_disc = std::sqrt(disc);
                                f32 t0 = (-b - sqrt_disc) / (2 * a);
                                f32 t1 = (-b + sqrt_disc) / (2 * a);
                                f32 distance = (t0 > 0) ? t0 : ((t1 > 0) ? t1 : -1);
                                if (distance > 0 and (not best_hit or distance < best_hit->distance)) {
                                    auto hit_point = origin + direction * distance;
                                    best_hit = {
                                        .origin = hit_point,
                                        .normal = (hit_point - object.position).normalized(),
                                        .distance = distance,
                                        .material_index = material
                                    };
                                }
                            }
                        } else if constexpr (std::same_as<T, Plane>) {
                            f32 denom = direction.dot(object.normal);
                            if (std::abs(denom) > 1e-6f) {
                                f32 distance = (object.position - origin).dot(object.normal) / denom;
                                if (distance > 0 and (not best_hit or distance < best_hit->distance)) {
                                    auto hit_point = origin + direction * distance;
                                    best_hit = {
                                        .origin = hit_point,
                                        .normal = object.normal.normalized(),
                                        .distance = distance,
                                        .material_index = material
                                    };
                                }
                            }
                        } else if constexpr (std::same_as<T, Mesh>) {
                            using Matrix = math::Matrix<f32, 3, 3>;
                            Matrix mesh_rotation =
                                Matrix::rotation(Matrix::RotationAxis::Pitch, object.pitch) *
                                Matrix::rotation(Matrix::RotationAxis::Yaw, object.yaw) *
                                Matrix::rotation(Matrix::RotationAxis::Roll, object.roll);

                            auto transform_vertex = [&](math::Vector<f32,3> v) -> math::Vector<f32,3> {
                                return (v * object.scale) * mesh_rotation + object.position;
                            };

                            auto ray_triangle_hit = [&] (
                                math::Vector<f32,3> orig,
                                math::Vector<f32,3> dir,
                                math::Vector<f32,3> v0,
                                math::Vector<f32,3> v1,
                                math::Vector<f32,3> v2
                            ) -> std::optional<Hit> {
                                constexpr f32 EPSILON = 1e-6f;
                                math::Vector<f32,3> edge1 = v1 - v0;
                                math::Vector<f32,3> edge2 = v2 - v0;
                                math::Vector<f32,3> h = dir.cross(edge2);
                                f32 a = edge1.dot(h);
                                if (std::abs(a) < EPSILON) return std::nullopt;

                                f32 f = 1.f / a;
                                math::Vector<f32,3> s = orig - v0;
                                f32 u = f * s.dot(h);
                                if (u < 0.f || u > 1.f) return std::nullopt;

                                math::Vector<f32,3> q = s.cross(edge1);
                                f32 v = f * dir.dot(q);
                                if (v < 0.f || u + v > 1.f) return std::nullopt;

                                f32 t = f * edge2.dot(q);
                                if (t > EPSILON) {
                                    auto hit_point = orig + dir * t;
                                    math::Vector<f32,3> normal = edge1.cross(edge2).normalized();
                                    return Hit{ .origin = hit_point, .normal = normal, .distance = t, .material_index = material };
                                }

                                return std::nullopt;
                            };

                            for (auto const& face : object.faces) {
                                auto v0 = transform_vertex(object.tris[face[0]]);
                                auto v1 = transform_vertex(object.tris[face[1]]);
                                auto v2 = transform_vertex(object.tris[face[2]]);

                                if (auto h = ray_triangle_hit(origin, direction, v0, v1, v2)) {
                                    if (not best_hit or h->distance < best_hit->distance) best_hit = *h;
                                }
                            }
                        }
                    },
                    shape
                );
            }

            return best_hit;
        }

        void draw(Io& io, rt::Input const& input, draw::Ref<draw::Image> target) const {
            const f32 aspect = f32(target.width()) / f32(target.height());
            const f32 half_fov_tan = std::tan(fov.radians() / 2.f);
            const i32 width = target.width();
            const i32 height = target.height();
            const auto rotation_matrix = this->rotation_matrix();

            const u32 thread_count = std::thread::hardware_concurrency();
            const i32 rows_per_thread = (height + thread_count - 1) / thread_count;

            std::vector<std::jthread> threads;
            threads.reserve(thread_count);

            for (u32 t = 0; t < thread_count; t += 1) {
                const i32 y_start = t * rows_per_thread;
                const i32 y_end = std::min(height, y_start + rows_per_thread);

                threads.emplace_back([&, y_start, y_end] {
                    for (i32 y = y_start; y < y_end; y += 1) {
                        for (i32 x = 0; x < width; x += 1) {
                            if (checkerboard and (x + y + input.counter()) % 2 == 0) continue;

                            const f32 ndc_x = (2.f * (x + .5f) / width - 1.f) * aspect;
                            const f32 ndc_y = (1.f - 2.f * (y + .5f) / height);

                            const f32 px = ndc_x * half_fov_tan;
                            const f32 py = ndc_y * half_fov_tan;

                            math::Vector<f32, 3> forward_ray_dir = { px, py, 1.f };
                            forward_ray_dir = forward_ray_dir.normalized();
                            const auto ray_dir = forward_ray_dir * rotation_matrix;

                            if (const auto hit = cast_ray(camera_position, ray_dir)) {
                                target | draw::pixel(x, y, material_data[hit->material_index]->shade(*hit, *this, 0));
                            }
                        }
                    }
                });
            }
        }
    };
}
