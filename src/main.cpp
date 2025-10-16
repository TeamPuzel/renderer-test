#include <raytracer>
#include <primitive>
#include <draw>
#include <io>
#include <rt>

class RayTracer final {
    raytracer::World world;
    bool show_info { true };
    bool show_hud { true };
    raytracer::World::Ref<raytracer::Mesh> bunny;

  public:
    RayTracer() {}

    void init(Io& io) {
        static const auto background = raytracer::LambertMaterial(draw::color::pico::DARK_GRAY, .4f);

        static const auto background_b
            = raytracer::BsdfMaterial({ .color = { .2f, .2f, .2f }, .roughness = 1.f, .metallic = 0.f });
        static const auto emissive
            = raytracer::BsdfMaterial({ .color = { 1.f }, .emissive = { 1000.f } });

        static const auto rough_dielectric
            = raytracer::BsdfMaterial({ .color = draw::color::pico::GRAY, .roughness = 1.f, .metallic = 0.f });
        static const auto medium_dielectric
            = raytracer::BsdfMaterial({ .color = draw::color::pico::GRAY, .roughness = .6f, .metallic = 0.f });
        static const auto smooth_dielectric
            = raytracer::BsdfMaterial({ .color = draw::color::pico::GRAY, .roughness = .1f, .metallic = 0.f });
        static const auto rough_metal
            = raytracer::BsdfMaterial({ .color = draw::color::pico::WHITE, .roughness = 1.f, .metallic = 1.f });
        static const auto medium_metal
            = raytracer::BsdfMaterial({ .color = draw::color::pico::WHITE, .roughness = .6f, .metallic = 1.f });
        static const auto smooth_metal
            = raytracer::BsdfMaterial({ .color = draw::color::pico::WHITE, .roughness = .1f, .metallic = 1.f });

        static const auto rough_dielectric_red
            = raytracer::BsdfMaterial({ .color = draw::color::pico::RED, .roughness = 1.f, .metallic = 0.f });

        world.add(raytracer::Plane { .position = {  0.f,  0.f, 10.f }, .normal = {  0.f,  0.f, -1.f } }, background_b);
        world.add(raytracer::Plane { .position = {  0.f,  0.f,  0.f }, .normal = {  0.f,  1.f,  0.f } }, background_b);
        world.add(raytracer::Plane { .position = {  0.f, 10.f,  0.f }, .normal = {  0.f, -1.f,  0.f } }, emissive);
        world.add(raytracer::Plane { .position = {  5.f,  0.f,  0.f }, .normal = { -1.f,  0.f,  0.f } }, background_b);
        world.add(raytracer::Plane { .position = { -5.f,  0.f,  0.f }, .normal = {  1.f,  0.f,  0.f } }, background_b);

        world.add(raytracer::Sphere { .position = { -1.75f, 1.f, 0.f }, .radius = .75f }, rough_metal);
        world.add(raytracer::Sphere { .position = {    0.f, 1.f, 0.f }, .radius = .75f }, medium_metal);
        world.add(raytracer::Sphere { .position = {  1.75f, 1.f, 0.f }, .radius = .75f }, smooth_metal);
        world.add(raytracer::Sphere { .position = { -1.75f, 3.f, 0.f }, .radius = .75f }, rough_dielectric);
        world.add(raytracer::Sphere { .position = {    0.f, 3.f, 0.f }, .radius = .75f }, medium_dielectric);
        world.add(raytracer::Sphere { .position = {  1.75f, 3.f, 0.f }, .radius = .75f }, smooth_dielectric);

        world.add(raytracer::PointLight { .position = {   0.f,  5.f,  5.f }, .color = {  1.f,  .6f, .45f } });
        world.add(raytracer::PointLight { .position = { -2.5f,  5.f, -5.f }, .color = {  1.f,  .8f, .45f } });
        world.add(raytracer::PointLight { .position = {  2.5f, 2.5f, -5.f }, .color = { .35f, .45f, .65f } });

        world.add(raytracer::Sphere { .position = {  3.25f, 1.f, -2.f }, .radius = .75f }, emissive);
        world.add(raytracer::Sphere { .position = { -3.25f, 1.f, -2.f }, .radius = .75f }, rough_dielectric_red);

        auto bunny = raytracer::load_mesh(io, "res/higherpoly_bunny.obj");
        bunny.position = { 0.f, 0.f, -4.f };
        bunny.scale = 10.f;
        this->bunny = world.add(std::move(bunny), medium_metal);

        world.move({ 0.f, 3.f, -9.f });
    }

    void update(Io& io, rt::Input const& input) {
        const f32 speed = input.key_held(rt::Key::Shift) ? 1.f : .2f;
        const math::Angle<f32> rotation_speed = math::deg(2.f);

        bunny->yaw += math::deg(1);

        if (input.key_repeating(rt::Key::O, 30, 2)) world.set_fov(world.get_fov() + math::deg(1));
        if (input.key_repeating(rt::Key::P, 30, 2)) world.set_fov(world.get_fov() - math::deg(1));
        if (input.key_pressed(rt::Key::I)) world.set_checkerboard(not world.get_checkerboard());
        if (input.key_pressed(rt::Key::U)) world.set_shadows(not world.get_shadows());
        if (input.key_pressed(rt::Key::Y)) world.cycle_bsdf_mode();
        if (input.key_pressed(rt::Key::T)) world.cycle_gi_mode();

        if (input.key_pressed(rt::Key::Num6)) show_hud = not show_hud;
        if (input.key_pressed(rt::Key::Num7)) show_info = not show_info;

        if (input.key_held(rt::Key::W) and not input.key_held(rt::Key::S))
            world.move({ 0.f, 0.f, +speed });
        if (input.key_held(rt::Key::S) and not input.key_held(rt::Key::W))
            world.move({ 0.f, 0.f, -speed });
        if (input.key_held(rt::Key::A) and not input.key_held(rt::Key::D))
            world.move({ -speed, 0.f, 0.f });
        if (input.key_held(rt::Key::D) and not input.key_held(rt::Key::A))
            world.move({ +speed, 0.f, 0.f });

        if (input.key_held(rt::Key::Space) and not input.key_held(rt::Key::Control))
            world.move({ 0.f, +speed, 0.f });
        if (input.key_held(rt::Key::Control) and not input.key_held(rt::Key::Space))
            world.move({ 0.f, -speed, 0.f });

        if (input.key_held(rt::Key::Up) and not input.key_held(rt::Key::Down))
            world.rotate_pitch(+rotation_speed);
        if (input.key_held(rt::Key::Down) and not input.key_held(rt::Key::Up))
            world.rotate_pitch(-rotation_speed);
        if (input.key_held(rt::Key::Left) and not input.key_held(rt::Key::Right))
            world.rotate_yaw(+rotation_speed);
        if (input.key_held(rt::Key::Right) and not input.key_held(rt::Key::Left))
            world.rotate_yaw(-rotation_speed);
    }

    void draw(Io& io, rt::Input const& input, draw::Ref<draw::Image> target) const {
        world.draw(io, input, target);

        if (show_hud) {
            std::stringstream out;
            out << "6: toggle hud" << std::endl
                << "7: toggle info" << std::endl
                << "8: toggle rate lock" << std::endl
                << "9: toggle performance overlay" << std::endl
                << "0: toggle vsync" << std::endl
                << "+/-: adjust target scale" << std::endl
                << "O/P: adjust fov" << std::endl
                << "I: toggle checkerboard interlacing" << std::endl
                << "U: toggle shadows" << std::endl
                << "Y: cycle BSDF debug modes" << std::endl
                << "T: cycle BSDF GI modes" << std::endl
                << "W/S/A/D: move camera" << std::endl
                << "Up/Down/Left/Right: rotate camera" << std::endl;

            if (show_info) {
                out << std::endl
                    << "Fov: " << i32(world.get_fov().degrees()) << " degrees" << std::endl
                    << "Checkerboard: " << (world.get_checkerboard() ? "Enabled" : "Disabled") << std::endl
                    << "Shadows: " << (world.get_shadows() ? "Enabled" : "Disabled") << std::endl
                    << "BSDF mode: " << world.get_bsdf_mode() << std::endl
                    << "GI mode: " << world.get_gi_mode() << std::endl;
            }

            std::string line;
            for (i32 y = 8; std::getline(out, line); y += font::mine(io).height + font::mine(io).leading) {
                target | draw::draw(draw::Text(line, font::mine(io)), 8, y);
            }
        }
    }
};

auto main() -> i32 {
    RayTracer instance;
    rt::run(instance, "RayTracer", 4);
}
