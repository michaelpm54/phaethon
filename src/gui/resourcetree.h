#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QStandardItemModel>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QModelIndex>
#include <QVariant>

#include "verdigris/wobjectdefs.h"

#include "src/aurora/keyfile.h"
#include "src/common/ptrmap.h"
#include "src/gui/statusbar.h"
#include "src/gui/resourcetreeitem.h"

namespace GUI
{

class ResourceTree : public QAbstractItemModel {
    W_OBJECT(ResourceTree)

private:
    QString _rootPath;
    ResourceTreeItem *_root;
    QFileIconProvider *_iconProvider;
    std::shared_ptr<StatusBar> _status;

    typedef Common::PtrMap<QString, Aurora::Archive> ArchiveMap;
    typedef Common::PtrMap<QString, Aurora::KEYDataFile> KEYDataFileMap;

    ArchiveMap _archives;
    KEYDataFileMap _keyDataFiles;

public:
    explicit ResourceTree(std::shared_ptr<StatusBar> statusBar, QString path, QObject *parent = 0);
    ~ResourceTree();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    ResourceTreeItem *getNode(const QModelIndex &index) const;
    bool canFetchMore(const QModelIndex &index) const override;
    void fetchMore(const QModelIndex &index);
    void insertItemsFromArchive(ResourceTreeItem::ArchiveInfo &data, const QModelIndex &parent);
    void insertNodes(int position, QList<ResourceTreeItem*> &nodes, const QModelIndex &parent);
    bool hasChildren(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setRootPath(QString path);
    void populate(const QString& path, ResourceTreeItem *parentNode);
    Aurora::Archive *getArchive(const QString &path);
    Aurora::KEYDataFile *getKEYDataFile(const QString &file);
    void loadKEYDataFiles(Aurora::KEYFile &key);
};

} // namespace GUI

#endif // TREEMODEL_H
