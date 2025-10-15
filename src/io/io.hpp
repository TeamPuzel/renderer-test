// Created by Lua (TeamPuzel) on August 13th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
#include <string_view>
#include <vector>
// #include <span>

/// Encapsulates all global side effects.
///
/// Loosely inspired by Zig's idea. Has the pleasant quality in how, similarly to an IO monad, it
/// indicates function purity, since you know functions that don't take Io& can't perform any, and
/// functions which are pure and don't take Io& can't call other Io& functions.
///
/// Unlike an IO monad it's easy to integrate in C++, it's just a context.
///
/// Io itself is an abstract class and requires a concrete platform implementation to be used.
class Io {
  public:
    class Error {
      protected:
        constexpr Error() {}

      public:
        Error(Error const&) = delete;
        Error(Error&&) = delete;
        auto operator=(Error const&) -> Error& = delete;
        auto operator=(Error&&) -> Error& = delete;
        virtual ~Error() noexcept {}
    };

  protected:
    Io() noexcept {}

    virtual auto perform_read_file(char const* path) -> std::vector<u8> = 0;
    virtual auto perform_open_library(char const* path) -> void* = 0;
    virtual void perform_close_library(void* library) = 0;
    virtual auto perform_load_symbol(void* library, char const* name) -> void* = 0;

  public:
    Io(Io const&) = delete;
    Io(Io&&) = delete;
    auto operator=(Io const&) -> Io& = delete;
    auto operator=(Io&&) -> Io& = delete;

    virtual ~Io() noexcept {}

    class DynamicLibrary final {
        Io& io;
        void* obj;

        DynamicLibrary(Io& io, void* obj) : io(io), obj(obj) {}

        friend class Io;

      public:
        DynamicLibrary(DynamicLibrary const&) = delete;
        auto operator=(DynamicLibrary const&) -> DynamicLibrary& = delete;

        DynamicLibrary(DynamicLibrary&& other) noexcept : io(other.io), obj(other.obj) {
            other.obj = nullptr;
        }

        auto operator=(DynamicLibrary&& other) noexcept -> DynamicLibrary& {
            if (this != &other) {
                if (obj) io.perform_close_library(obj);
                obj = other.obj;
                other.obj = nullptr;
            }
            return *this;
        }

        ~DynamicLibrary() noexcept {
            if (obj) {
                io.perform_close_library(obj);
                obj = nullptr;
            }
        }

        auto symbol(std::string_view name) const -> void* {
            return io.perform_load_symbol(obj, name.data());
        }
    };

    auto open_library(std::string_view path) [[clang::lifetimebound]] -> DynamicLibrary {
        return DynamicLibrary(*this, perform_open_library(path.data()));
    }

    auto read_file(std::string_view path) -> std::vector<u8> {
        return perform_read_file(path.data());
    }
};

// namespace io {
//     class BinaryWriter final {};

//     /// A sound implementation of a binary data reader for serialization purposes.
//     ///
//     /// It reads data out by individual bytes so alignment is not relevant,
//     /// among other workarounds to avoid undefined behavior.
//     class BinaryReader final {
//         std::span<const u8> data;
//         usize cursor { 0 };

//         BinaryReader(std::span<const u8> data [[clang::lifetimebound]]) : data(data) {}

//       public:
//         /// Returns a new, default reader for the given data.
//         static auto of(std::span<const u8> data [[clang::lifetimebound]]) -> BinaryReader {
//             return BinaryReader(data);
//         }

//         template <typename R> auto read() noexcept(noexcept(R::read(*this))) -> R {
//             return R::read(*this);
//         }

//         auto u8() -> ::u8 {
//             const auto ret = this->data.at(this->cursor);
//             this->cursor += 1;
//             return ret;
//         }

//         auto u16() -> ::u16 {
//             const auto ret = endian::from_le_bytes<::u16>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1)
//             );
//             this->cursor += 2;
//             return ret;
//         }

//         auto u32() -> ::u32 {
//             const auto ret = endian::from_le_bytes<::u32>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1),
//                 this->data.at(this->cursor + 2),
//                 this->data.at(this->cursor + 3)
//             );
//             this->cursor += 4;
//             return ret;
//         }

//         auto u64() -> ::u64 {
//             const auto ret = endian::from_le_bytes<::u64>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1),
//                 this->data.at(this->cursor + 2),
//                 this->data.at(this->cursor + 3),
//                 this->data.at(this->cursor + 4),
//                 this->data.at(this->cursor + 5),
//                 this->data.at(this->cursor + 6),
//                 this->data.at(this->cursor + 7)
//             );
//             this->cursor += 8;
//             return ret;
//         }

//         auto i8() -> ::i8 {
//             return *reinterpret_cast<::i8 const*>(&this->data.at(this->cursor));
//         }

//         auto i16() -> ::i16 {
//             const auto ret = endian::from_le_bytes<::i16>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1)
//             );
//             this->cursor += 2;
//             return ret;
//         }

//         auto i32() -> ::i32 {
//             const auto ret = endian::from_le_bytes<::i32>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1),
//                 this->data.at(this->cursor + 2),
//                 this->data.at(this->cursor + 3)
//             );
//             this->cursor += 4;
//             return ret;
//         }

//         auto i64() -> ::i64 {
//             const auto ret = endian::from_le_bytes<::i64>(
//                 this->data.at(this->cursor),
//                 this->data.at(this->cursor + 1),
//                 this->data.at(this->cursor + 2),
//                 this->data.at(this->cursor + 3),
//                 this->data.at(this->cursor + 4),
//                 this->data.at(this->cursor + 5),
//                 this->data.at(this->cursor + 6),
//                 this->data.at(this->cursor + 7)
//             );
//             this->cursor += 8;
//             return ret;
//         }

//         auto boolean() -> bool {
//             const bool ret = this->data.at(this->cursor) ? true : false;
//             this->cursor += 1;
//             return ret;
//         }

//         /// Assume the current position to be a C string in a fixed buffer.
//         /// Returns the C string and skips over the buffer.
//         auto cstr(usize bufsize) -> char const* {
//             // This cast is sound: u8 -> char (on arm architectures char is even already unsigned by default)
//             // The alignment is a match.
//             auto ret = reinterpret_cast<char const*>(&this->data.at(this->cursor));
//             this->cursor += bufsize;
//             return ret;
//         }

//         /// Returns the amount of data contained.
//         auto size() const noexcept -> usize {
//             return this->data.size();
//         }

//         /// Returns the current cursor offset.
//         auto position() const noexcept -> usize {
//             return this->cursor;
//         }

//         /// Resets the reader state to the start.
//         void rewind() noexcept {
//             this->cursor = 0;
//         }

//         /// Seeks to the specified byte position.
//         void seek(usize position) noexcept {
//             this->cursor = position;
//         }

//         /// Skips over the given number of bytes.
//         void skip(usize count) noexcept {
//             this->cursor += count;
//         }
//     };
// }
