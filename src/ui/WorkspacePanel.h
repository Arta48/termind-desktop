#pragma once

#include <QFrame>
#include <QString>

class QFileSystemModel;
class QLabel;
class QModelIndex;
class QPoint;
class QTreeView;

class WorkspacePanel : public QFrame {
    Q_OBJECT

public:
    explicit WorkspacePanel(QWidget* parent = nullptr);

    void setRootDirectory(const QString& dir);
    QString rootDirectory() const { return m_rootDir; }

signals:
    void fileAttachRequested(const QString& filePath);

private:
    void onDoubleClicked(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);
    void previewFile(const QString& absPath);
    QString toRelativePath(const QString& absPath) const;

    QString m_rootDir;
    QLabel* m_headerLabel = nullptr;
    QFileSystemModel* m_model = nullptr;
    QTreeView* m_treeView = nullptr;
};
