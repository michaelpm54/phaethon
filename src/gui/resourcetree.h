/* Phaethon - A FLOSS resource explorer for BioWare's Aurora engine games
 *
 * Phaethon is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * Phaethon is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * Phaethon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phaethon. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Phaethon's tree of game resource files.
 */

#ifndef RESOURCETREE_H
#define RESOURCETREE_H

#include "verdigris/wobjectdefs.h"

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QModelIndex>
#include <QVariant>

#include "src/gui/resourcetreeitem.h"

namespace GUI {

class ResourceTreeItem;

class ResourceTree : public QAbstractItemModel {
    W_OBJECT(ResourceTree)
public:
    explicit ResourceTree(const QString &path = "", QObject *parent = 0);
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
    bool canFetchMore(const QModelIndex &index);
    void fetchMore(const QModelIndex &index);
    bool insertNodes(int position, QList<ResourceTreeItem*> &nodes, QModelIndex parent);
    bool hasChildren(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setRootPath(const QString& path);
    void populate(const QString& path, ResourceTreeItem *parentNode);

private:
    ResourceTreeItem *_root;
};

} // End of namespace GUI

#endif // RESOURCETREE_H
