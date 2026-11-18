#pragma once

#include <QFrame>
#include <QString>

class QDragEnterEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QResizeEvent;
class QVBoxLayout;

class InputBar : public QFrame {
    Q_OBJECT

public:
    explicit InputBar(QWidget* parent = nullptr);

    void setWorkspaceDir(const QString& dir);
    void setAttachment(const QString& filePath);
    void clearAttachment();
    void setBusy(bool busy);
    void focusInput();

signals:
    void sendRequested(const QString& text, const QString& attachmentPath);
    void cancelRequested();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void triggerSend();
    void openAttachDialog();
    void updateEditorHeight();
    void setComposerFocused(bool focused);

    QString m_workspaceDir;
    QString m_attachmentPath;
    bool m_busy = false;

    QVBoxLayout* m_outerLayout = nullptr;
    QFrame* m_composerBox = nullptr;

    QFrame* m_attachmentChip = nullptr;
    QLabel* m_attachmentLabel = nullptr;
    QPushButton* m_removeAttachmentBtn = nullptr;

    QPlainTextEdit* m_promptEdit = nullptr;
    QPushButton* m_attachBtn = nullptr;
    QPushButton* m_sendBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
};
