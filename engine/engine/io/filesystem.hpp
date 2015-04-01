#ifndef SCC_ENGINE_IO_FILESYSTEM_H

#include <map>
#include <list>
#include <vector>
#include <memory>

#include "engine/io/common.hpp"
#include "engine/io/stream.hpp"
#include "engine/io/filestream.hpp"

namespace io {

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();

private:
    typedef std::pair<std::string, MountPtr> MountItem;
    typedef std::vector<MountItem> MountList;

private:
    std::map<MountPriority, MountList, std::greater<MountPriority>> _mounts;

protected:
    std::pair<MountPriority, MountList::iterator> find_mount(
        const Mount* mount);

    void iter_file_mounts(
        const std::string &path,
        const std::function<bool (Mount*, const std::string&)> &handler);
    void sort_mount_list(MountList &list);

public:
    void listdir(const std::string &path,
                 std::vector<std::string> &items);

    void mount(
        const std::string &mount_point,
        MountPtr &&mount,
        MountPriority priority);

    std::unique_ptr<Stream> open(
            const std::string &path,
            const OpenMode openmode,
            const WriteMode writemode = WriteMode::IGNORE
            );

    void stat(const std::string &path,
              VFSStat &stat);
};

}

#endif
