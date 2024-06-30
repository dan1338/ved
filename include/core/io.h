#pragma once

#include <filesystem>
#include <vector>
#include <set>
#include <variant>

#include "core/media_file.h"
#include "ffmpeg/io.h"

namespace core
{
    namespace io
    {
        namespace fs = std::filesystem;

        struct Directory;
        struct File;

        using DirectoryEntry = std::variant<Directory, File>;
        using DirectoryContents = std::set<DirectoryEntry>;

        static DirectoryContents iter_directory(const fs::path &path);

        struct Directory
        {
            fs::path path;

            Directory back() const
            {
                return {path.parent_path()};
            }

            DirectoryContents iter() const
            {
                return iter_directory(path);
            }

            bool operator<(const Directory &rhs) const
            {
                return path < rhs.path;
            }
        };

        struct File
        {
            fs::path path;

            core::MediaFile open() const
            {
                return ffmpeg::io::open_file(path);
            }

            bool operator<(const File &rhs) const
            {
                return path < rhs.path;
            }
        };

        static DirectoryContents iter_directory(const fs::path &path)
        {
            DirectoryContents entries;

            for (const auto &ent : fs::directory_iterator(path)) {
                if (ent.is_directory()) {
                    entries.insert(Directory{ent.path()});
                } else {
                    entries.insert(File{ent.path()});
                }
            }

            return entries;
        }
    }
}
