#include "world.hpp"

using namespace raytracer;

auto LambertMaterial::shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color {
    using Vector = math::Vector<f32, 3>;

    Vector out_color;

    for (const auto light : world.lights()) {
        const auto light_direction = (light.position - hit.origin).normalized();
        const auto distance_to_light = (light.position - hit.origin).magnitude();
        const auto view_direction = (world.get_camera_position() - hit.origin).normalized();
        const auto half = (view_direction + light_direction).normalized();

        if (world.get_shadows()) [[likely]] {
            const auto shadow_origin = hit.origin + hit.normal.normalized() * 1e-3f;
            if (auto shadow_hit = world.cast_ray(shadow_origin, light_direction))
                if (shadow_hit->distance < distance_to_light) continue;
        }

        const auto lambert_diffuse
            = Vector(light.color).hadamard(color)
            * std::max(0.f, hit.normal.dot(light_direction));

        out_color += lambert_diffuse * diffuse_reflectance;
    }

    return out_color;
}

auto BsdfMaterial::shade(Hit hit, World const& world, u32 depth) const -> raytracer::Color {
    using Vector = math::Vector<f32, 3>;
    const Vector base_color = color;
    const auto roughness = this->roughness * this->roughness;

    Vector out_color;

    const auto base_reflectivity = math::mix(Vector(.04f), base_color, metallic);

    for (const auto light : world.lights()) {
        const auto light_direction = (light.position - hit.origin).normalized();
        const auto distance_to_light = (light.position - hit.origin).magnitude();
        const auto view_direction = (world.get_camera_position() - hit.origin).normalized();
        const auto half = (view_direction + light_direction).normalized();

        const auto normal_distribution
            = math::sq(roughness)
            / (f32(math::pi) * math::sq(
                math::sq(hit.normal.dot(half)) * (math::sq(roughness) - 1.f) + 1.f
            ));

        const auto fresnel
            = base_reflectivity + (Vector(1.f) - base_reflectivity) * std::pow(
                1.f - std::clamp(half.dot(view_direction), 0.f, 1.f),
                5.f
            );

        const auto direct_k = math::sq(roughness + 1.f) / 8.f;
        const auto ndotv = std::clamp(hit.normal.dot(view_direction), 0.f, 1.f);
        const auto ndotl = std::clamp(hit.normal.dot(light_direction), 0.f, 1.f);
        const auto microfacets
            = (ndotv / std::max(.001f, ndotv * (1.f - direct_k) + direct_k))
            * (ndotl / std::max(.001f, ndotl * (1.f - direct_k) + direct_k));

        const auto cook_torrance
            = (fresnel * normal_distribution * microfacets)
            / (4.f * view_direction.dot(hit.normal) * light_direction.dot(hit.normal));

        const auto lambert_diffuse
            = Vector(light.color).hadamard(base_color)
            * std::max(0.f, hit.normal.dot(light_direction));
        const auto diffuse_reflectance = (Vector(1.f) - fresnel) * (1.f - metallic);

        switch (world.get_bsdf_mode()) {
            [[likely]]
            case BsdfMaterial::Mode::Default:
                out_color += diffuse_reflectance.hadamard(lambert_diffuse)
                          +  cook_torrance.hadamard(Vector(light.color)) * ndotl;
                break;
            case BsdfMaterial::Mode::Diffuse:
                out_color += lambert_diffuse;
                break;
            case BsdfMaterial::Mode::CookTorrance:
                out_color += cook_torrance;
                break;
            case BsdfMaterial::Mode::Fresnel:
                out_color += fresnel;
                break;
            case BsdfMaterial::Mode::NormalDistribution:
                out_color += normal_distribution;
                break;
            case BsdfMaterial::Mode::Microfacets:
                out_color += microfacets;
                break;
        }
    }

    return out_color;
}
