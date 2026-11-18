#include "WorkspacePanel.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFontDatabase>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

    bool isImageExtension(const QString& suffix) {
        static const QStringList exts = {"png", "jpg", "jpeg", "webp", "gif", "bmp"};
        return exts.contains(suffix.toLower());
    }

} // namespace

WorkspacePanel::WorkspacePanel(QWidget* parent) : QFrame(parent) {
    setObjectName("WorkspacePanel");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_headerLabel = new QLabel("EXPLORER", this);
    m_headerLabel->setObjectName("PanelHeader");
    layout->addWidget(m_headerLabel);

    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    m_treeView = new QTreeView(this);
    m_treeView->setObjectName("WorkspaceTree");
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setIndentation(16);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    m_treeView->setColumnHidden(1, true);
    m_treeView->setColumnHidden(2, true);
    m_treeView->setColumnHidden(3, true);

    layout->addWidget(m_treeView, 1);

    connect(m_treeView, &QTreeView::doubleClicked, this, &WorkspacePanel::onDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &WorkspacePanel::onContextMenu);
}

void WorkspacePanel::setRootDirectory(const QString& dir) {
    const QString canonDir = QFileInfo(dir).canonicalFilePath();
    m_rootDir = canonDir.isEmpty() ? dir : canonDir;
    const QModelIndex rootIdx = m_model->setRootPath(m_rootDir);
    m_treeView->setRootIndex(rootIdx);

    const QString folderName = QFileInfo(m_rootDir).fileName();
    m_headerLabel->setText(QString("EXPLORER · %1").arg((folderName.isEmpty() ? m_rootDir : folderName).toUpper()));
    m_headerLabel->setToolTip(m_rootDir);
}

void WorkspacePanel::onDoubleClicked(const QModelIndex& index) {
    if (!index.isValid() || m_model->isDir(index)) return;
    previewFile(m_model->filePath(index));
}

void WorkspacePanel::onContextMenu(const QPoint& pos) {
    const QModelIndex idx = m_treeView->indexAt(pos);
    if (!idx.isValid()) return;

    const QString absPath = m_model->filePath(idx);
    const QString relPath = toRelativePath(absPath);
    const bool isDir = m_model->isDir(idx);

    QMenu menu(this);
    QAction* attachAct = nullptr;
    QAction* previewAct = nullptr;
    if (!isDir) {
        attachAct = menu.addAction("Attach to Prompt");
        previewAct = menu.addAction("Preview File");
        menu.addSeparator();
    }
    QAction* copyRelAct = menu.addAction("Copy Relative Path");

    QAction* chosen = menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == attachAct) {
        emit fileAttachRequested(relPath);
    } else if (chosen == previewAct) {
        previewFile(absPath);
    } else if (chosen == copyRelAct) {
        if (auto* cb = QApplication::clipboard()) {
            cb->setText(relPath);
        }
    }
}

void WorkspacePanel::previewFile(const QString& absPath) {
    const QFileInfo fi(absPath);
    if (!fi.exists() || fi.isDir()) return;

    if (isImageExtension(fi.suffix())) {
        QPixmap pix(absPath);
        if (pix.isNull()) {
            QMessageBox::warning(this, "Preview Error", "Could not load image.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setWindowTitle(toRelativePath(absPath));
        dlg->resize(760, 560);

        auto* lay = new QVBoxLayout(dlg);
        auto* scroll = new QScrollArea(dlg);
        scroll->setWidgetResizable(true);
        auto* imgLabel = new QLabel(scroll);
        imgLabel->setAlignment(Qt::AlignCenter);
        if (pix.width() > 1000 || pix.height() > 800) {
            imgLabel->setPixmap(pix.scaled(1000, 800, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            imgLabel->setPixmap(pix);
        }
        scroll->setWidget(imgLabel);
        lay->addWidget(scroll);
        dlg->show();
        return;
    }

    QFile f(absPath);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Preview Error", QString("Cannot open file: %1").arg(f.errorString()));
        return;
    }

    constexpr qint64 kMaxPreviewBytes = 256 * 1024;
    const QByteArray bytes = f.read(kMaxPreviewBytes + 1);
    const bool truncated = bytes.size() > kMaxPreviewBytes;
    const QByteArray slice = truncated ? bytes.left(kMaxPreviewBytes) : bytes;

    if (slice.left(1024).contains('\0')) {
        QMessageBox::information(this, "Binary File", "This appears to be a binary file and cannot be previewed as text.");
        return;
    }

    auto* dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(toRelativePath(absPath) + (truncated ? " (truncated at 256 KB)" : ""));
    dlg->resize(820, 600);

    auto* lay = new QVBoxLayout(dlg);
    auto* editor = new QPlainTextEdit(dlg);
    editor->setReadOnly(true);
    editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor->setPlainText(QString::fromUtf8(slice));
    lay->addWidget(editor);
    dlg->show();
}

QString WorkspacePanel::toRelativePath(const QString& absPath) const {
    if (m_rootDir.isEmpty()) return absPath;
    return QDir(m_rootDir).relativeFilePath(absPath);
}
