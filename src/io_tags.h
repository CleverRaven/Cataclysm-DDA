#pragma once
#ifndef CATA_SRC_IO_TAGS_H
#define CATA_SRC_IO_TAGS_H

namespace io
{

class JsonArrayInputArchive;
class JsonArrayOutputArchive;
class JsonObjectInputArchive;
class JsonObjectOutputArchive;

/**
 * Tag that indicates the class is stored in a JSON array.
 */
struct array_archive_tag {
    using InputArchive = JsonArrayInputArchive;
    using OutputArchive = JsonArrayOutputArchive;
};

/**
 * Tag that indicates the class is stored in a JSON object.
 */
struct object_archive_tag {
    using InputArchive = JsonObjectInputArchive;
    using OutputArchive = JsonObjectOutputArchive;
};

} // namespace io

#endif // CATA_SRC_IO_TAGS_H
