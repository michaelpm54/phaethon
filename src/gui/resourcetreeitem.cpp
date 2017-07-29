#include "src/common/filepath.h"
#include "src/common/readfile.h"
#include "src/gui/resourcetreeitem.h"

namespace GUI {

// Data for items within an archive
ItemData::ItemData(const QString &parentPath, const QString &fileName, Aurora::Archive *archiveData, Aurora::Archive::Resource &resource) {
    _fullPath     = parentPath + "/" + fileName;
    _isDir        = false;
    _source       = kSourceArchiveFile;
    _fileType     = TypeMan.getFileType(fileName);
    _resourceType = TypeMan.getResourceType(fileName);
    _size         = archiveData->getResourceSize(resource.index);

    _archive.data         = archiveData;
    _archive.addedMembers = false;
    _archive.index        = resource.index;

    _triedDuration = _resourceType != Aurora::kResourceSound;
    _duration      = Sound::RewindableAudioStream::kInvalidLength;
}

// Data for items which are actually on the filesystem
ItemData::ItemData(const QString &fullPath, const QFileInfo &info) {
    _fullPath = fullPath;
    _source   = info.isDir() ? kSourceDirectory : kSourceFile;
    _isDir    = info.isDir();
    _size     = _isDir ? Common::kFileInvalid : info.size();
    if (_isDir) {
        _fileType = Aurora::kFileTypeNone;
        _resourceType = Aurora::kResourceNone;
    }
    else {
        _fileType = TypeMan.getFileType(info.fileName());
        _resourceType = TypeMan.getResourceType(info.fileName());
    }

    _archive.data         = 0;
    _archive.addedMembers = false;
    _archive.index        = 0xFFFFFFFF;

    _triedDuration = _resourceType != Aurora::kResourceSound;
    _duration = Sound::RewindableAudioStream::kInvalidLength;
}

// Items within an archive
ResourceTreeItem::ResourceTreeItem(Aurora::Archive *archiveData, Aurora::Archive::Resource &resource, ResourceTreeItem *parent)
    : _parent(parent)
{
    QString fileName = TypeMan.setFileType(resource.name, resource.type).toQString();
    _name = fileName;

    _data = new ItemData(parent->getPath(), fileName, archiveData, resource);
}

// Items which are actually on the filesystem
ResourceTreeItem::ResourceTreeItem(QString fullPath, ResourceTreeItem *parent)
    : _parent(parent)
{
    QFileInfo info(fullPath);
    _name = info.fileName();

    _data = new ItemData(fullPath, info);
}

ResourceTreeItem::~ResourceTreeItem() {
    qDeleteAll(_children);
    _children.clear();
    delete _data;
}

void ResourceTreeItem::appendChild(ResourceTreeItem *child) {
    _children << child;
}

bool ResourceTreeItem::insertChild(int position, ResourceTreeItem *child) {
    if (position < 0 or position >= _children.count())
        return false;

    _children.insert(position, child);

    return true;
}

ResourceTreeItem *ResourceTreeItem::childAt(int row) const {
    return _children.at(row);
}

int ResourceTreeItem::childCount() const {
    return _children.count();
}

int ResourceTreeItem::row() const {
    if (_parent)
        return _parent->_children.indexOf(const_cast<ResourceTreeItem*>(this));
    return 0;
}

ResourceTreeItem *ResourceTreeItem::getParent() const {
    return _parent;
}

void ResourceTreeItem::setParent(ResourceTreeItem *parent) {
    _parent = parent;
}

bool ResourceTreeItem::hasChildren() const {
    return _children.count();
}

const QString &ResourceTreeItem::getName() const {
    return _name;
}

/* Data. */

bool ResourceTreeItem::isDir() const {
    return _data->_isDir;
}

const QString &ResourceTreeItem::getPath() const {
    return _data->_fullPath;
}

qint64 ResourceTreeItem::getSize() const {
    return _data->_size;
}

Source ResourceTreeItem::getSource() const {
    return _data->_source;
}

Aurora::FileType ResourceTreeItem::getFileType() const {
    return _data->_fileType;
}

Aurora::ResourceType ResourceTreeItem::getResourceType() const {
    return _data->_resourceType;
}

Common::SeekableReadStream *ResourceTreeItem::getResourceData() const {
    try {
        switch (_data->_source) {
            case kSourceDirectory:
                throw Common::Exception("Can't get file data of a directory");

            case kSourceFile:
                return new Common::ReadFile(_data->_fullPath.toStdString().c_str());

            case kSourceArchiveFile:
                if (!_data->_archive.data)
                    throw Common::Exception("No archive opened");

                return _data->_archive.data->getResource(_data->_archive.index);
        }
    } catch (Common::Exception &e) {
        e.add("Failed to get resource data for resource \"%s\"", _name.toStdString().c_str());
        throw;
    }

    assert(false);
    return 0;
}

Images::Decoder *ResourceTreeItem::getImage() const {
    if (getResourceType() != Aurora::kResourceImage)
        throw Common::Exception("\"%s\" is not an image resource", getName().toStdString().c_str());

    Common::ScopedPtr<Common::SeekableReadStream> res(getResourceData());

    Images::Decoder *img = 0;
    try {
        img = getImage(*res, _data->_fileType);
    } catch (Common::Exception &e) {
        e.add("Failed to get image from \"%s\"", getName().toStdString().c_str());
        throw;
    }

    return img;
}

Images::Decoder *ResourceTreeItem::getImage(Common::SeekableReadStream &res, Aurora::FileType type) const {
    Images::Decoder *img = 0;
    switch (type) {
        case Aurora::kFileTypeDDS:
            img = new Images::DDS(res);
            break;

        case Aurora::kFileTypeTPC:
            img = new Images::TPC(res);
            break;

        // TXB may be actually TPC
        case Aurora::kFileTypeTXB:
        case Aurora::kFileTypeTXB2:
            try {
                img = new Images::TXB(res);
            } catch (Common::Exception &e1) {

                try {
                    res.seek(0);
                    img = new Images::TPC(res);

                } catch (Common::Exception &e2) {
                    e1.add(e2);

                    throw e1;
                }
            }
            break;

        case Aurora::kFileTypeTGA:
            img = new Images::TGA(res);
            break;

        case Aurora::kFileTypeSBM:
            img = new Images::SBM(res);
            break;

        case Aurora::kFileTypeCUR:
        case Aurora::kFileTypeCURS:
            img = new Images::WinIconImage(res);
            break;

        default:
            throw Common::Exception("Unsupported image type %d", type);
    }

    return img;
}

Archive &ResourceTreeItem::getArchive() {
    return _data->_archive;
}

uint64 ResourceTreeItem::getSoundDuration() const {
    if (_data->_triedDuration)
        return _data->_duration;

    _data->_triedDuration = true;

    try {
        Common::ScopedPtr<Sound::AudioStream> sound(getAudioStream());

        Sound::RewindableAudioStream &rewSound = dynamic_cast<Sound::RewindableAudioStream &>(*sound);
        _data->_duration = rewSound.getDuration();

    } catch (...) {
    }

    return _data->_duration;
}

Sound::AudioStream *ResourceTreeItem::getAudioStream() const {
    if (_data->_resourceType != Aurora::kResourceSound)
        throw Common::Exception("\"%s\" is not a sound resource", _name.toStdString().c_str());

    Common::ScopedPtr<Common::SeekableReadStream> res(getResourceData());

    Sound::AudioStream *sound = 0;
    try {
        sound = SoundMan.makeAudioStream(res.get());
    } catch (Common::Exception &e) {
        e.add("Failed to get audio stream from \"%s\"", _name.toStdString().c_str());
        throw;
    }

    res.release();
    return sound;
}

} // End of namespace GUI
