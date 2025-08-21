#include "FileManager.h"
#include <fat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

FileManager::FileManager() : filesystemInitialized(false) {
}

FileManager::~FileManager() {
}

bool FileManager::Initialize() {
    if (fatInitDefault()) {
        filesystemInitialized = true;
        printf("Filesystem initialized successfully\n");
        ScanForSTLFiles();
        return true;
    } else {
        printf("Filesystem initialization failed\n");
        return false;
    }
}

void FileManager::ScanForSTLFiles() {
    files.clear();

    if (!filesystemInitialized) {
        printf("Filesystem not initialized, cannot scan for files\n");
        return;
    }

    // Scan SD card root
    ScanDirectory("sd:/");

    // Scan USB root
    ScanDirectory("usb:/");

    // Check current directory for bitcoin.stl (fallback)
    if (FileExists("bitcoin.stl")) {
        AddFile("bitcoin.stl", "bitcoin.stl");
    }

    printf("Found %d STL file(s)\n", static_cast<int>(files.size()));

    // Sort files alphabetically
    std::sort(files.begin(), files.end(),
              [](const FileEntry& a, const FileEntry& b) {
                  return a.name < b.name;
              });
}

void FileManager::RefreshFileList() {
    ScanForSTLFiles();
}

const FileEntry* FileManager::GetFile(int index) const {
    if (index >= 0 && index < static_cast<int>(files.size())) {
        return &files[index];
    }
    return nullptr;
}

bool FileManager::IsValidSTLFile(const std::string& filepath) const {
    if (!IsSTLExtension(filepath)) {
        return false;
    }

    FILE* file = fopen(filepath.c_str(), "rb");
    if (!file) {
        return false;
    }

    // Basic validation - check if file has reasonable size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);

    // STL files should be at least 84 bytes (80 header + 4 count)
    // and not unreasonably large
    return (fileSize >= 84 && fileSize <= 100 * 1024 * 1024); // Max 100MB
}

long FileManager::GetFileSize(const std::string& filepath) const {
    struct stat statbuf;
    if (stat(filepath.c_str(), &statbuf) == 0) {
        return statbuf.st_size;
    }
    return 0;
}

std::string FileManager::GetFileExtension(const std::string& filename) const {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos && dotPos < filename.length() - 1) {
        std::string ext = filename.substr(dotPos + 1);
        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }
    return "";
}

bool FileManager::FileExists(const std::string& filepath) const {
    return (access(filepath.c_str(), F_OK) == 0);
}

void FileManager::ScanDirectory(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return; // Directory doesn't exist or can't be opened
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;

        // Skip hidden files and directories
        if (filename[0] == '.') {
            continue;
        }

        if (IsSTLExtension(filename)) {
            std::string fullPath = path + filename;
            if (IsValidSTLFile(fullPath)) {
                AddFile(filename, fullPath);
            }
        }
    }

    closedir(dir);
}

bool FileManager::IsSTLExtension(const std::string& filename) const {
    std::string ext = GetFileExtension(filename);
    return (ext == "stl");
}

void FileManager::AddFile(const std::string& name, const std::string& path) {
    // Check if file already exists in list
    for (const auto& file : files) {
        if (file.path == path) {
            return; // Already added
        }
    }

    long size = GetFileSize(path);
    files.emplace_back(name, path, size);
    printf("  Found: %s (%ld bytes)\n", name.c_str(), size);
}
