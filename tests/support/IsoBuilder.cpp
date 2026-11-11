// IsoBuilder.cpp - see IsoBuilder.h for an overview.
//
// Layout of the generated image:
//   sectors 0-15   empty (system area)
//   sector 16      primary volume descriptor, pointing at the root directory
//   then           one sector per directory, then the file data

#include "IsoBuilder.h"

#include <fstream>
#include <set>
#include <stdexcept>

namespace openhogs::tests {

namespace {

constexpr std::size_t kSectorSize = 2048;
constexpr std::uint32_t kFirstDirectorySector = 18;

using Sector = std::vector<std::uint8_t>;

void putU32BothEndian(Sector& s, std::size_t offset, std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i) {
        s[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));      // little-endian copy
        s[offset + 7 - i] = static_cast<std::uint8_t>(value >> (8 * i));  // big-endian copy
    }
}

void addRecord(Sector& dir, std::size_t& offset, const std::string& name, std::uint32_t extent,
               std::uint32_t size, bool isDirectory) {
    const std::size_t length = 33 + name.size() + (name.size() % 2 == 0 ? 1 : 0);
    if (offset + length > kSectorSize) {
        throw std::runtime_error("IsoBuilder: directory does not fit in one sector");
    }
    dir[offset] = static_cast<std::uint8_t>(length);
    putU32BothEndian(dir, offset + 2, extent);
    putU32BothEndian(dir, offset + 10, size);
    dir[offset + 25] = isDirectory ? 0x02 : 0x00;
    dir[offset + 32] = static_cast<std::uint8_t>(name.size());
    for (std::size_t i = 0; i < name.size(); ++i) {
        dir[offset + 33 + i] = static_cast<std::uint8_t>(name[i]);
    }
    offset += length;
}

std::string parentOf(const std::string& path) {
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? "" : path.substr(0, slash);
}

std::string nameOf(const std::string& path) {
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

} // namespace

std::vector<std::byte> toBytes(const std::string& text) {
    std::vector<std::byte> bytes;
    for (const char c : text) {
        bytes.push_back(static_cast<std::byte>(c));
    }
    return bytes;
}

void writeIsoImage(const std::filesystem::path& path, const DiscFiles& files, bool rawSectors) {
    // Every directory, including the parents of nested ones ("" = root).
    std::set<std::string> directories{""};
    for (const auto& [filePath, content] : files) {
        for (std::string dir = parentOf(filePath); !dir.empty(); dir = parentOf(dir)) {
            directories.insert(dir);
        }
    }

    // Place directories first, then files.
    std::map<std::string, std::uint32_t> directorySector;
    std::uint32_t next = kFirstDirectorySector;
    for (const auto& dir : directories) {
        directorySector[dir] = next++;
    }
    std::map<std::string, std::uint32_t> fileSector;
    for (const auto& [filePath, content] : files) {
        fileSector[filePath] = next;
        next += static_cast<std::uint32_t>((content.size() + kSectorSize - 1) / kSectorSize);
        if (content.empty()) ++next;
    }

    std::vector<Sector> sectors(next, Sector(kSectorSize, 0));

    // Primary volume descriptor with the root directory record at 156.
    Sector& pvd = sectors[16];
    pvd[0] = 1;
    pvd[1] = 'C'; pvd[2] = 'D'; pvd[3] = '0'; pvd[4] = '0'; pvd[5] = '1';
    std::size_t rootOffset = 156;
    addRecord(pvd, rootOffset, std::string(1, '\0'), directorySector[""], kSectorSize, true);

    // Directory contents: ".", "..", sub-directories, files.
    for (const auto& dir : directories) {
        Sector& sector = sectors[directorySector[dir]];
        std::size_t offset = 0;
        addRecord(sector, offset, std::string(1, '\0'), directorySector[dir], kSectorSize, true);
        addRecord(sector, offset, std::string(1, '\1'), directorySector[parentOf(dir)], kSectorSize, true);
        for (const auto& child : directories) {
            if (!child.empty() && child != dir && parentOf(child) == dir) {
                addRecord(sector, offset, nameOf(child), directorySector[child], kSectorSize, true);
            }
        }
        for (const auto& [filePath, content] : files) {
            if (parentOf(filePath) == dir) {
                addRecord(sector, offset, nameOf(filePath) + ";1", fileSector[filePath],
                          static_cast<std::uint32_t>(content.size()), false);
            }
        }
    }

    // File data.
    for (const auto& [filePath, content] : files) {
        for (std::size_t i = 0; i < content.size(); ++i) {
            sectors[fileSector[filePath] + i / kSectorSize][i % kSectorSize] =
                std::to_integer<std::uint8_t>(content[i]);
        }
    }

    std::ofstream file(path, std::ios::binary);
    for (const Sector& data : sectors) {
        if (rawSectors) {
            // Sync pattern, address, mode 2, 8-byte subheader, data, error correction.
            Sector sector(2352, 0);
            for (std::size_t i = 1; i <= 10; ++i) sector[i] = 0xFF;
            sector[15] = 2;
            std::copy(data.begin(), data.end(), sector.begin() + 24);
            file.write(reinterpret_cast<const char*>(sector.data()), static_cast<std::streamsize>(sector.size()));
        } else {
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        }
    }
}

} // namespace openhogs::tests
