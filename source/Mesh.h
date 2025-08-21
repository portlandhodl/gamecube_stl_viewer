#ifndef MESH_H
#define MESH_H

#include <gccore.h>
#include <cstdio>

/**
 * 3D Vector structure
 */
struct Vector3 {
    f32 x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(f32 x_, f32 y_, f32 z_) : x(x_), y(y_), z(z_) {}
};

/**
 * Triangle structure representing a face in the mesh
 */
struct Triangle {
    Vector3 normal;
    Vector3 vertices[3];

    Triangle() {}
};

/**
 * Mesh class for handling 3D geometry data
 */
class Mesh {
public:
    Mesh();
    ~Mesh();

    bool LoadFromSTL(const char* filename);
    void Clear();

    // Getters
    const Triangle* GetTriangles() const { return triangles; }
    int GetTriangleCount() const { return triangleCount; }

    // Bounding box information
    Vector3 GetMinBounds() const { return minBounds; }
    Vector3 GetMaxBounds() const { return maxBounds; }
    Vector3 GetCenter() const;
    f32 GetMaxSize() const;

    bool IsValid() const { return triangles != nullptr && triangleCount > 0; }

private:
    Triangle* triangles;
    int triangleCount;

    // Bounding box
    Vector3 minBounds;
    Vector3 maxBounds;

    void CalculateBounds();
    bool LoadBinarySTL(FILE* file);
    bool LoadASCIISTL(FILE* file);
    bool IsBinarySTL(FILE* file);
    bool ReadFloat(FILE* file, f32& value);
};

#endif // MESH_H
