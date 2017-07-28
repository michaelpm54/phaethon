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
 * MERCHANTABILIT	Y or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phaethon. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Phaethon's main window.
 */

#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

#include "verdigris/wobjectimpl.h"

#include "mainwindow.h"
#include "ui/ui_mainwindow.h"

#include "src/common/filepath.h"
#include "src/common/strutil.h"
#include "src/gui/mainwindow.h"
#include "src/aurora/util.h"
#include "src/gui/resourcetree.h"
#include "src/gui/resourcetreeitem.h"

namespace GUI {

W_OBJECT_IMPL(MainWindow)

MainWindow::MainWindow(QWidget *parent, const char *version, QSize size, QString path) :
    QMainWindow(parent),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);

    connect(this, &MainWindow::openDir, this, &MainWindow::setTreeViewModel);
    connect(_ui->actionOpenDirectory, &QAction::triggered, this, &MainWindow::on_actionOpen_directory_triggered);
    connect(_ui->actionClose, &QAction::triggered, this, &MainWindow::on_actionClose_triggered);
    connect(_ui->actionQuit, &QAction::triggered, this, &MainWindow::on_actionQuit_triggered);
    connect(_ui->bLoadKotorDir, &QPushButton::clicked, this, &MainWindow::on_pushButton_1_clicked);
    connect(_ui->bCloseDir, &QPushButton::clicked, this, &MainWindow::on_pushButton_2_clicked);
    connect(_ui->bUnused1, &QPushButton::clicked, this, &MainWindow::on_pushButton_3_clicked);
    connect(_ui->bUnused2, &QPushButton::clicked, this, &MainWindow::on_pushButton_4_clicked);

	if (size != QSize())
		resize(size);

    _panelPreviewEmpty = new PanelPreviewEmpty();
    _panelPreviewImage = new PanelPreviewImage();
    _panelPreviewSound = new PanelPreviewSound();
    _panelPreviewText  = new PanelPreviewText();

    _ui->resLayout->addWidget(_panelPreviewEmpty);

	connect(this, &MainWindow::openDir, this, &MainWindow::setTreeViewModel);

    _statusLabel = new QLabel(this);
    _statusLabel->setText("None");
    _statusLabel->setAlignment(Qt::AlignLeft);

    _ui->statusBar->addWidget(_statusLabel, 2);

    _ui->treeView->setHeaderHidden(true);

    // resource info panel
    clearLabels();

	if (!path.isEmpty())
        emit openDir(path);
}

MainWindow::~MainWindow() {
    delete _ui;
}

void MainWindow::setTreeViewModel(const QString &path) {
    QString cPath = QDir(path).canonicalPath();

    _treeModel = new ResourceTree(path, _ui->treeView);
    _ui->treeView->setModel(_treeModel);

    connect(_ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::selection);

    _statusLabel->setText(tr("Root: %1").arg(cPath));
    _ui->treeView->show();

    _ui->actionClose->setEnabled(true);
}

void MainWindow::on_actionOpen_directory_triggered() {
    QString dir = QFileDialog::getExistingDirectory(this,
                                                    tr("Open directory"),
                                                    QString(QStandardPaths::HomeLocation),
                                                    QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty())
        emit openDir(dir);
}

void MainWindow::on_actionClose_triggered() {
    showPreviewPanel(_panelPreviewEmpty);

    _ui->treeView->setModel(nullptr);

	clearLabels();

    _statusLabel->setText("None");

    clearLabels();
}

void MainWindow::on_actionQuit_triggered() {
    QCoreApplication::quit();
}

// testing
void MainWindow::on_pushButton_1_clicked() {
	QString myKotorPath("/home/mike/kotor");
	QDir dir(myKotorPath);
	if (dir.exists())
		emit openDir(myKotorPath);
}

void MainWindow::on_pushButton_2_clicked() {
    on_actionClose_triggered();
}

void MainWindow::on_pushButton_3_clicked() {
}

void MainWindow::on_pushButton_4_clicked() {
}

QString getSizeLabel(size_t size) {
    if (size == Common::kFileInvalid)
        return "-";

    if (size < 1024)
        return QString("%1").arg(size);

    QString humanRead = Common::FilePath::getHumanReadableSize(size).toQString();

    return QString("%1 (%2)").arg(humanRead).arg(size);
}

QString getFileTypeLabel(Aurora::FileType type) {
    QString label = QString("%1").arg(type);

    if (type != Aurora::kFileTypeNone) {
        QString temp = TypeMan.getExtension(type).toQString();
        label += QString(" (%1)").arg(temp);
    }

    return label;
}

QString getResTypeLabel(Aurora::ResourceType type) {
    QString label = QString("%1").arg(type);

    if (type != Aurora::kResourceNone) {
        QString temp = getResourceTypeDescription(type).toQString();
        label += QString(" (%1)").arg(temp);
    }

    return label;
}

void MainWindow::setLabels() {
    QString labelName     = "Resource name: ";
    QString labelSize     = "Size: ";
    QString labelFileType = "File type: ";
    QString labelResType  = "Resource type: ";

    labelName += _currentItem->getName();

    if (_currentItem->getSource() == Source::kSourceDirectory) {

        labelSize     += "-";
        labelFileType += "Directory";
        labelResType  += "Directory";

    } else if ((_currentItem->getSource() == Source::kSourceFile) ||
               (_currentItem->getSource() == Source::kSourceArchiveFile)) {

        Aurora::FileType     fileType = _currentItem->getFileType();
        Aurora::ResourceType resType  = _currentItem->getResourceType();

        labelSize     += getSizeLabel(_currentItem->getSize());
        labelFileType += getFileTypeLabel(fileType);
        labelResType  += getResTypeLabel(resType);
    }

    _ui->resLabelName->setText(labelName);
    _ui->resLabelSize->setText(labelSize);
    _ui->resLabelFileType->setText(labelFileType);
    _ui->resLabelResType->setText(labelResType);
}

void MainWindow::clearLabels() {
    _ui->resLabelName->setText("Resource name:");
    _ui->resLabelSize->setText("Size:");
    _ui->resLabelFileType->setText("File type:");
    _ui->resLabelResType->setText("Resource type:");
}

void MainWindow::showPreviewPanel(QFrame *panel) {
    if (_ui->resLayout->count()) {
        QFrame *old = static_cast<QFrame*>(_ui->resLayout->itemAt(0)->widget());
        if (old != panel) {
            _ui->resLayout->removeWidget(old);
            old->setParent(0);
            _ui->resLayout->addWidget(panel);
        }
    }
}

void MainWindow::showPreviewPanel() {
    switch (_currentItem->getResourceType()) {
        case Aurora::kResourceImage:
            showPreviewPanel(_panelPreviewImage);
            break;

        case Aurora::kResourceSound:
            showPreviewPanel(_panelPreviewSound);
            break;

        default:
        {
            switch (_currentItem->getFileType()) {
                case Aurora::FileType::kFileTypeICO:
                    showPreviewPanel(_panelPreviewImage);
                    break;

                case Aurora::FileType::kFileTypeINI:
                case Aurora::FileType::kFileTypeTXT:
                    showPreviewPanel(_panelPreviewText);
                    break;

                default:
                    showPreviewPanel(_panelPreviewEmpty);
            }
            break;
        }
    }
}

void MainWindow::selection(const QItemSelection &selected) {
    const QModelIndex index = selected.indexes().at(0);
    _currentItem = _treeModel->getNode(index);
    setLabels();
    showPreviewPanel();
}

} // End of namespace GUI
