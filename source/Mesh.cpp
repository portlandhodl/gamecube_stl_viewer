#include "Mesh.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

Mesh::Mesh() : triangles(nullptr), triangleCount(0) {
    minBounds = Vector3(1e9f, 1e9f, 1e9f);
    maxBounds = Vector3(-1e9f, -1e9f, -1e9f);
}

Mesh::~Mesh() {
    Clear();
}

void Mesh::Clear() {
    if (triangles) {
        free(triangles);
        triangles = nullptr;
    }
    triangleCount = 0;
    minBounds = Vector3(1e9f, 1e9f, 1e9f);
    maxBounds = Vector3(-1e9f, -1e9f, -1e9f);
}

bool Mesh::LoadFromSTL(const char* filename) {
    printf("Loading STL file: %s\n", filename);

    Clear();

    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("ERROR: Cannot open file: %s\n", filename);
        return false;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("File size: %ld bytes\n", fileSize);

    bool success = false;
    if (IsBinarySTL(file)) {
        printf("Detected format: BINARY\n");
        success = LoadBinarySTL(file);
    } else {
        printf("Detected format: ASCII\n");
        success = LoadASCIISTL(file);
    }

    fclose(file);

    if (success) {
        CalculateBounds();
        printf("STL loaded successfully! Triangles: %d\n", triangleCount);

        Vector3 center = GetCenter();
        f32 maxSize = GetMaxSize();
        printf("Model bounds: X(%.2f to %.2f) Y(%.2f to %.2f) Z(%.2f to %.2f)\n",
               minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, minBounds.z, maxBounds.z);
        printf("Model center: (%.2f, %.2f, %.2f), Max size: %.2f\n",
               center.x, center.y, center.z, maxSize);
    }

    return success;
}

bool Mesh::IsBinarySTL(FILE* file) {
    char header[81] = {0};
    fseek(file, 0, SEEK_SET);

    if (fread(header, 1, 80, file) != 80) {
        return false;
    }

    // Check if it contains "solid" keyword (ASCII indicator)
    // But also verify it's not a binary file that happens to have "solid" in header
    bool hasASCIIKeyword = (strstr(header, "solid") != NULL);

    if (!hasASCIIKeyword) {
        return true; // Definitely binary
    }

    // Read triangle count for binary validation
    unsigned char countBytes[4];
    if (fread(countBytes, 1, 4, file) != 4) {
        return false;
    }

    unsigned int triangleCount = countBytes[0] | (countBytes[1] << 8) |
                                (countBytes[2] << 16) | (countBytes[3] << 24);

    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);

    // Binary STL expected size: 80 (header) + 4 (count) + triangleCount * 50
    long expectedBinarySize = 80 + 4 + (triangleCount * 50);

    // If file size matches binary format expectation, it's binary
    return (fileSize == expectedBinarySize);
}

bool Mesh::LoadBinarySTL(FILE* file) {
    fseek(file, 80, SEEK_SET); // Skip header

    // Read triangle count
    unsigned char countBytes[4];
    if (fread(countBytes, 1, 4, file) != 4) {
        printf("ERROR: Failed to read triangle count\n");
        return false;
    }

    unsigned int count = countBytes[0] | (countBytes[1] << 8) |
                        (countBytes[2] << 16) | (countBytes[3] << 24);

    if (count == 0 || count > 1000000) {
        printf("ERROR: Invalid triangle count: %u\n", count);
        return false;
    }

    triangleCount = static_cast<int>(count);
    triangles = static_cast<Triangle*>(malloc(triangleCount * sizeof(Triangle)));

    if (!triangles) {
        printf("ERROR: Failed to allocate memory for triangles\n");
        return false;
    }

    // Read triangles
    for (int i = 0; i < triangleCount; i++) {
        Triangle* tri = &triangles[i];

        // Read normal (3 floats, little-endian)
        if (!ReadFloat(file, tri->normal.x) ||
            !ReadFloat(file, tri->normal.y) ||
            !ReadFloat(file, tri->normal.z)) {
            printf("ERROR: Failed to read normal for triangle %d\n", i);
            return false;
        }

        // Read vertices (9 floats total)
        for (int j = 0; j < 3; j++) {
            if (!ReadFloat(file, tri->vertices[j].x) ||
                !ReadFloat(file, tri->vertices[j].y) ||
                !ReadFloat(file, tri->vertices[j].z)) {
                printf("ERROR: Failed to read vertex %d for triangle %d\n", j, i);
                return false;
            }
        }

        // Skip attribute byte count (2 bytes)
        fseek(file, 2, SEEK_CUR);
    }

    return true;
}

bool Mesh::ReadFloat(FILE* file, f32& value) {
    unsigned char bytes[4];
    if (fread(bytes, 1, 4, file) != 4) {
        return false;
    }

    // Convert little-endian bytes to float
    union { f32 f; unsigned int i; } converter;
    converter.i = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    value = converter.f;

    return true;
}

bool Mesh::LoadASCIISTL(FILE* file) {
    // For now, focus on binary STL support as it's more reliable
    // ASCII STL parsing can be added later if needed
    printf("ASCII STL format not yet implemented\n");
    return false;
}

void Mesh::CalculateBounds() {
    if (!triangles || triangleCount == 0) {
        return;
    }

    minBounds = Vector3(1e9f, 1e9f, 1e9f);
    maxBounds = Vector3(-1e9f, -1e9f, -1e9f);

    for (int i = 0; i < triangleCount; i++) {
        for (int j = 0; j < 3; j++) {
            const Vector3& vertex = triangles[i].vertices[j];

            if (vertex.x < minBounds.x) minBounds.x = vertex.x;
            if (vertex.x > maxBounds.x) maxBounds.x = vertex.x;
            if (vertex.y < minBounds.y) minBounds.y = vertex.y;
            if (vertex.y > maxBounds.y) maxBounds.y = vertex.y;
            if (vertex.z < minBounds.z) minBounds.z = vertex.z;
            if (vertex.z > maxBounds.z) maxBounds.z = vertex.z;
        }
    }
}

Vector3 Mesh::GetCenter() const {
    return Vector3(
        (minBounds.x + maxBounds.x) * 0.5f,
        (minBounds.y + maxBounds.y) * 0.5f,
        (minBounds.z + maxBounds.z) * 0.5f
    );
}

f32 Mesh::GetMaxSize() const {
    f32 sizeX = maxBounds.x - minBounds.x;
    f32 sizeY = maxBounds.y - minBounds.y;
    f32 sizeZ = maxBounds.z - minBounds.z;

    f32 maxSize = sizeX;
    if (sizeY > maxSize) maxSize = sizeY;
    if (sizeZ > maxSize) maxSize = sizeZ;

    return maxSize;
}
