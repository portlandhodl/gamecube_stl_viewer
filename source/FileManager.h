#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>

/**
 * Structure to hold file information
 */
struct FileEntry {
    std::string name;
    std::string path;
    long size;

    FileEntry() : size(0) {}
    FileEntry(const std::string& n, const std::string& p, long s = 0)
        : name(n), path(p), size(s) {}
};

/**
 * File management class for handling STL file discovery and validation
 */
class FileManager {
public:
    FileManager();
    ~FileManager();

    bool Initialize();
    void ScanForSTLFiles();
    void RefreshFileList();

    // File access
    const std::vector<FileEntry>& GetFiles() const { return files; }
    int GetFileCount() const { return static_cast<int>(files.size()); }
    const FileEntry* GetFile(int index) const;

    // File validation
    bool IsValidSTLFile(const std::string& filepath) const;
    long GetFileSize(const std::string& filepath) const;

    // Path utilities
    std::string GetFileExtension(const std::string& filename) const;
    bool FileExists(const std::string& filepath) const;

private:
    std::vector<FileEntry> files;
    bool filesystemInitialized;

    void ScanDirectory(const std::string& path);
    bool IsSTLExtension(const std::string& filename) const;
    void AddFile(const std::string& name, const std::string& path);
};

#endif // FILE_MANAGER_H
