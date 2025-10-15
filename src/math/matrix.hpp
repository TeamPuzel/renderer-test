#pragma once
#include "angle.hpp"
#include <concepts>
#include <primitive>

namespace math {
    /// A row-major combined matrix and vector type.
    ///
    /// Note that the row-major design means you multiply from left to right and vectors are horizontal.
    /// The associated `Matrix::Vector` type provides the vector type which can be used with a given matrix type.
    ///
    /// Similarly there is a type alias, `Vector<T, N>`, which is just shorthand for a vector-like matrix.
    /// Generally you need not worry about remembering that the width is the primary vector size.
    ///
    /// The matrix provides it's dimensions as constants and the element type through `Matrix::Element`.
    template <std::floating_point T, const usize W, const usize H> requires (W > 0 and H > 0) class Matrix final {
        T data[W][H];

      public:
        static constexpr auto width = W;
        static constexpr auto height = H;
        using Element = T;

        template <std::floating_point U, const usize W2, const usize H2> requires (W2 > 0 and H2 > 0) friend class Matrix;

        /// Constructs a matrix from a 2d array of values.
        constexpr explicit Matrix(Element const(&values)[W][H]) {
            for (int i = 0; i < W; i += 1)
                for (int j = 0; j < H; j += 1)
                    this->data[i][j] = values[i][j];
        }

        /// Constructs a zero initialized matrix.
        ///
        /// For an identity matrix use `Matrix::identity` instead.
        constexpr Matrix() {
            for (int i = 0; i < W; i += 1)
                for (int j = 0; j < H; j += 1)
                    this->data[i][j] = 0;
        }

        constexpr Matrix(Element value) {
            for (int i = 0; i < W; i += 1)
                for (int j = 0; j < H; j += 1)
                    this->data[i][j] = value;
        }

        constexpr auto operator == (this Matrix const& self, Matrix const& other) -> bool {
            for (int i = 0; i < W; i += 1)
                for (int j = 0; j < H; j += 1)
                    if (self.data[i][j] != other.data[i][j]) return false;
            return true;
        }

        constexpr auto operator != (this Matrix const& self, Matrix const& other) -> bool {
            return not (self == other);
        }

        /// Generic matrix multiplication.
        template <const usize W2, const usize H2>
            requires (W == H2)
        constexpr auto operator * (this Matrix const& self, Matrix<Element, W2, H2> const& other) -> Matrix<Element, W2, H> {
            Matrix<Element, W2, H> result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W2; j += 1)
                    for (usize k = 0; k < W; k += 1)
                        result.data[i][j] += self.data[i][k] * other.data[k][j];

            return result;
        }

        constexpr auto operator * (this Matrix const& self, Element const& scalar) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] * scalar;

            return result;
        }

        constexpr auto hadamard(this Matrix const& self, Matrix const& other) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] * other.data[i][j];

            return result;
        }

        constexpr auto operator / (this Matrix const& self, Element const& scalar) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] / scalar;

            return result;
        }

        constexpr auto operator + (this Matrix const& self) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = +self.data[i][j];

            return result;
        }

        constexpr auto operator + (this Matrix const& self, Matrix const& other) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] + other.data[i][j];

            return result;
        }

        constexpr auto operator + (this Matrix const& self, Element const& scalar) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] + scalar;

            return result;
        }

        constexpr auto operator - (this Matrix const& self) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = -self.data[i][j];

            return result;
        }

        constexpr auto operator - (this Matrix const& self, Matrix const& other) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] - other.data[i][j];

            return result;
        }

        constexpr auto operator - (this Matrix const& self, Element const& scalar) -> Matrix {
            Matrix result;

            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    result.data[i][j] = self.data[i][j] - scalar;

            return result;
        }

        constexpr void operator += (this Matrix& self, Matrix const& other) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] + other.data[i][j];
        }

        constexpr void operator += (this Matrix& self, Element const& scalar) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] + scalar;
        }

        constexpr void operator -= (this Matrix& self, Matrix const& other) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] - other.data[i][j];
        }

        constexpr void operator -= (this Matrix& self, Element const& scalar) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] - scalar;
        }

        constexpr void operator *= (this Matrix& self, Element const& scalar) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] * scalar;
        }

        constexpr void operator /= (this Matrix& self, Element const& scalar) {
            for (usize i = 0; i < H; i += 1)
                for (usize j = 0; j < W; j += 1)
                    self.data[i][j] = self.data[i][j] / scalar;
        }

        /// The rotation axis for constructing rotatation matrices.
        enum class RotationAxis { Pitch, Yaw, Roll };

        /// Creates a rotation matrix.
        static constexpr auto rotation(RotationAxis axis, Angle<Element> angle) -> Matrix requires (W == 4 and H == 4) {
            const auto a = angle;
            switch (axis) {
                case RotationAxis::Pitch: return Matrix({
                    { 1, 0, 0, 0 },
                    { 0, a.cos(), -a.sin(), 0 },
                    { 0, a.sin(),  a.cos(), 0 },
                    { 0, 0, 0, 1 }
                });
                case RotationAxis::Yaw: return Matrix({
                    {  a.cos(), 0, a.sin(), 0 },
                    { 0, 1, 0, 0 },
                    { -a.sin(), 0, a.cos(), 0 },
                    { 0, 0, 0, 1 }
                });
                case RotationAxis::Roll: return Matrix({
                    { a.cos(), -a.sin(), 0, 0 },
                    { a.sin(),  a.cos(), 0, 0 },
                    { 0, 0, 1, 0 },
                    { 0, 0, 0, 1 }
                });
            };
        }

        /// Creates a rotation matrix.
        static constexpr auto rotation(RotationAxis axis, Angle<Element> angle) -> Matrix requires (W == 3 and H == 3) {
            const auto a = angle;
            switch (axis) {
                case RotationAxis::Pitch: return Matrix({
                    { 1, 0, 0 },
                    { 0, a.cos(), -a.sin() },
                    { 0, a.sin(),  a.cos() }
                });
                case RotationAxis::Yaw: return Matrix({
                    {  a.cos(), 0, a.sin() },
                    { 0, 1, 0 },
                    { -a.sin(), 0, a.cos() }
                });
                case RotationAxis::Roll: return Matrix({
                    { a.cos(), -a.sin(), 0 },
                    { a.sin(),  a.cos(), 0 },
                    { 0, 0, 1 }
                });
            };
        }

        /// Creates a translation matrix.
        static constexpr auto translation(Element x, Element y, Element z) -> Matrix requires (W == 4 and H == 4) {
            return Matrix({
                { 1, 0, 0, 0 },
                { 0, 1, 0, 0 },
                { 0, 0, 1, 0 },
                { x, y, z, 1 }
            });
        }

        /// Creates a scaling matrix.
        static constexpr auto scaling(Element x, Element y, Element z) -> Matrix requires (W == 4 and H == 4) {
            return Matrix({
                { x, 0, 0, 0 },
                { 0, y, 0, 0 },
                { 0, 0, z, 0 },
                { 0, 0, 0, 1 }
            });
        }

        /// Creates a standard projection matrix.
        static constexpr auto projection(Element w, Element h, Angle<Element> fov, Element near, Element far) -> Matrix requires (W == 4 and H == 4) {
            const auto aspect = h / w;
            const auto q = far / (far - near);
            const auto f = 1 / (fov / rad(2)).tan();
            return Matrix({
                { aspect * f, 0, 0, 0 },
                { 0, f, 0, 0 },
                { 0, 0, q, 1 },
                { 0, 0, -near * q, 0 }
            });
        }

        /// Creates and identity matrix.
        static constexpr auto identity() -> Matrix requires (W == 4 and H == 4) {
            return Matrix({
                { 1, 0, 0, 0 },
                { 0, 1, 0, 0 },
                { 0, 0, 1, 0 },
                { 0, 0, 0, 1 }
            });
        }

        constexpr auto operator [] (usize x, usize y) & -> Element& { return this->data[x][y]; }
        constexpr auto operator [] (usize x, usize y) const& -> Element const& { return this->data[x][y]; }

        // Vector implementation ---------------------------------------------------------------------------------------

        /// Associated vector type.
        using Vector = Matrix<Element, W, 1>;

        /// Vector constructor
        constexpr explicit Matrix(Element const(&values)[W]) requires (H == 1) {
            for (int i = 0; i < W; i += 1)
                this->data[i][0] = values[i];
        }

        /// Vector constructor
        template <std::convertible_to<Element>... Ts>
        constexpr explicit(false) Matrix(Ts... elements) requires (H == 1 and W == sizeof...(Ts)) {
            const Element values[] = { elements... };
            for (int i = 0; i < W; i += 1)
                this->data[i][0] = values[i];
        }

        constexpr auto operator [] (usize index) & -> Element& requires (H == 1) {
            return this->data[index][0];
        }
        constexpr auto operator [] (usize index) const& -> Element const& requires (H == 1) {
            return this->data[index][0];
        }

        [[nodiscard]] auto x() & -> Element& requires (H == 1 and W >= 1) { return this->data[0][0]; }
        [[nodiscard]] auto x() const& -> Element const& requires (H == 1 and W >= 1) { return this->data[0][0]; }

        [[nodiscard]] auto y() & -> Element& requires (H == 1 and W >= 2) { return this->data[1][0]; }
        [[nodiscard]] auto y() const& -> Element const& requires (H == 1 and W >= 2) { return this->data[1][0]; }

        [[nodiscard]] auto z() & -> Element& requires (H == 1 and W >= 3) { return this->data[2][0]; }
        [[nodiscard]] auto z() const& -> Element const& requires (H == 1 and W >= 3) { return this->data[2][0]; }

        [[nodiscard]] auto w() & -> Element& requires (H == 1 and W >= 4) { return this->data[3][0]; }
        [[nodiscard]] auto w() const& -> Element const& requires (H == 1 and W >= 4) { return this->data[3][0]; }

        constexpr auto dot(this Vector const& self, Vector const& other) -> Element {
            Element acc = 0; for (usize i = 0; i < W; i += 1) acc += self[i] * other[i]; return acc;
        }

        constexpr auto cross(this Vector const& self, Vector const& other) -> Vector requires (W == 3) {
            return Vector {
    			self.y() * other.z() - self.z() * other.y(),
    			self.z() * other.x() - self.x() * other.z(),
    			self.x() * other.y() - self.y() * other.x()
    		};
        }

        constexpr auto magnitude(this Vector const& self) -> Element {
            Element acc = 0; for (usize i = 0; i < W; i += 1) acc += self[i] * self[i]; return std::sqrt(acc);
        }

        constexpr auto normalized(this Vector const& self) -> Vector {
            Element m = self.magnitude();
            Vector result; for (usize i = 0; i < W; i += 1) result[i] = self[i] / m; return result;
        }
    };

    /// A vector type, an alias of a matrix.
    template <std::floating_point T, const usize LENGTH> using Vector = Matrix<T, LENGTH, 1>;

    template <std::floating_point... T> Matrix(T...) -> Matrix<T...[0], sizeof...(T), 1>;

    template <std::floating_point T, const usize W, const usize H>
    constexpr auto mix(Matrix<T, W, H> lhs, Matrix<T, W, H> rhs, f32 t) -> Matrix<T, W, H> {
        Matrix<T, W, H> result;

        for (usize i = 0; i < H; i += 1)
            for (usize j = 0; j < W; j += 1)
                result[i, j] = lhs[i, j] + t * (rhs[i, j] - lhs[i, j]);

        return result;
    }

    template <std::floating_point T> constexpr auto mix(T lhs, T rhs, f32 t) -> T {
        return lhs + t * (rhs - lhs);
    }
}
