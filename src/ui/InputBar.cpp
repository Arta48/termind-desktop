#include "InputBar.h"
#include "Icons.h"

#include <QAbstractTextDocumentLayout>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QtMath>

InputBar::InputBar(QWidget* parent) : QFrame(parent) {
    setObjectName("InputBar");
    setAcceptDrops(true);

    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(24, 10, 24, 14);
    m_outerLayout->setSpacing(0);

    m_composerBox = new QFrame(this);
    m_composerBox->setObjectName("ComposerBox");
    m_composerBox->setProperty("focused", false);

    auto* boxLayout = new QVBoxLayout(m_composerBox);
    boxLayout->setContentsMargins(12, 10, 12, 10);
    boxLayout->setSpacing(8);

    // Attachment chip row
    m_attachmentChip = new QFrame(m_composerBox);
    m_attachmentChip->setObjectName("AttachmentChip");
    m_attachmentChip->setVisible(false);

    auto* chipLayout = new QHBoxLayout(m_attachmentChip);
    chipLayout->setContentsMargins(8, 3, 6, 3);
    chipLayout->setSpacing(6);

    auto* chipIcon = new QLabel(m_attachmentChip);
    chipIcon->setPixmap(Icons::paperclip(QColor("#79c0ff")).pixmap(13, 13));
    chipLayout->addWidget(chipIcon);

    m_attachmentLabel = new QLabel(m_attachmentChip);
    chipLayout->addWidget(m_attachmentLabel);

    m_removeAttachmentBtn = new QPushButton("✕", m_attachmentChip);
    m_removeAttachmentBtn->setObjectName("RemoveAttachmentButton");
    m_removeAttachmentBtn->setFixedSize(20, 20);
    m_removeAttachmentBtn->setCursor(Qt::PointingHandCursor);
    chipLayout->addWidget(m_removeAttachmentBtn);

    auto* chipRow = new QHBoxLayout();
    chipRow->setContentsMargins(0, 0, 0, 0);
    chipRow->addWidget(m_attachmentChip);
    chipRow->addStretch();
    boxLayout->addLayout(chipRow);

    // Prompt editor
    m_promptEdit = new QPlainTextEdit(m_composerBox);
    m_promptEdit->setObjectName("PromptEdit");
    m_promptEdit->setPlaceholderText("Ask Termind to inspect, edit, or run commands...");
    m_promptEdit->setAcceptDrops(false);
    m_promptEdit->installEventFilter(this);
    m_promptEdit->setFixedHeight(36);
    boxLayout->addWidget(m_promptEdit);

    // Bottom composer toolbar
    auto* bottomRow = new QHBoxLayout();
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(8);

    m_attachBtn = new QPushButton("Attach", m_composerBox);
    m_attachBtn->setObjectName("GhostButton");
    m_attachBtn->setIcon(Icons::paperclip(QColor("#9aa4b2")));
    m_attachBtn->setIconSize(QSize(14, 14));
    m_attachBtn->setCursor(Qt::PointingHandCursor);
    m_attachBtn->setToolTip("Attach image or text file from workspace (or drag & drop)");
    bottomRow->addWidget(m_attachBtn);

    bottomRow->addStretch();

    auto* hintLbl = new QLabel("Enter to send · Shift+Enter for new line", m_composerBox);
    hintLbl->setObjectName("InputHint");
    bottomRow->addWidget(hintLbl);

    m_sendBtn = new QPushButton("Send", m_composerBox);
    m_sendBtn->setObjectName("SendButton");
    m_sendBtn->setIcon(Icons::send(QColor("#ffffff")));
    m_sendBtn->setIconSize(QSize(14, 14));
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    bottomRow->addWidget(m_sendBtn);

    m_stopBtn = new QPushButton("Stop", m_composerBox);
    m_stopBtn->setObjectName("StopButton");
    m_stopBtn->setIcon(Icons::stop(QColor("#ffffff")));
    m_stopBtn->setIconSize(QSize(13, 13));
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setVisible(false);
    bottomRow->addWidget(m_stopBtn);

    boxLayout->addLayout(bottomRow);
    m_outerLayout->addWidget(m_composerBox);

    connect(m_attachBtn, &QPushButton::clicked, this, &InputBar::openAttachDialog);
    connect(m_removeAttachmentBtn, &QPushButton::clicked, this, &InputBar::clearAttachment);
    connect(m_sendBtn, &QPushButton::clicked, this, &InputBar::triggerSend);
    connect(m_stopBtn, &QPushButton::clicked, this, &InputBar::cancelRequested);
    connect(m_promptEdit, &QPlainTextEdit::textChanged, this, &InputBar::updateEditorHeight);
}

void InputBar::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    const int sideMargin = qMax(24, (width() - 980) / 2);
    m_outerLayout->setContentsMargins(sideMargin, 10, sideMargin, 14);
}

void InputBar::setWorkspaceDir(const QString& dir) {
    m_workspaceDir = dir;
}

void InputBar::setAttachment(const QString& filePath) {
    if (filePath.trimmed().isEmpty()) return;

    QString displayPath = filePath;
    if (!m_workspaceDir.isEmpty()) {
        const QString canonWs = QFileInfo(m_workspaceDir).canonicalFilePath();
        const QFileInfo fi = QFileInfo(filePath).isRelative()
            ? QFileInfo(QDir(m_workspaceDir), filePath)
            : QFileInfo(filePath);
        const QString canonFile = fi.canonicalFilePath();
#ifdef Q_OS_WIN
        constexpr Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
        constexpr Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
        const QString wsPrefix = canonWs.endsWith('/') ? canonWs : canonWs + '/';
        if (!canonWs.isEmpty() && !canonFile.isEmpty() &&
            (QString::compare(canonFile, canonWs, cs) == 0 || canonFile.startsWith(wsPrefix, cs))) {
            displayPath = QDir(canonWs).relativeFilePath(canonFile);
        }
    }

    m_attachmentPath = displayPath;
    m_attachmentLabel->setText(QFileInfo(displayPath).fileName());
    m_attachmentChip->setToolTip(displayPath);
    m_attachmentChip->setVisible(true);
}

void InputBar::clearAttachment() {
    m_attachmentPath.clear();
    m_attachmentLabel->clear();
    m_attachmentChip->setVisible(false);
}

void InputBar::setBusy(bool busy) {
    m_busy = busy;
    m_sendBtn->setVisible(!busy);
    m_sendBtn->setEnabled(!busy);
    m_stopBtn->setVisible(busy);
    m_attachBtn->setEnabled(!busy);
}

void InputBar::focusInput() {
    m_promptEdit->setFocus();
}

void InputBar::setComposerFocused(bool focused) {
    m_composerBox->setProperty("focused", focused);
    m_composerBox->style()->unpolish(m_composerBox);
    m_composerBox->style()->polish(m_composerBox);
}

bool InputBar::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_promptEdit) {
        if (event->type() == QEvent::FocusIn) {
            setComposerFocused(true);
        } else if (event->type() == QEvent::FocusOut) {
            setComposerFocused(false);
        } else if (event->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
                if (!(ke->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))) {
                    if (!m_busy) {
                        triggerSend();
                    }
                    return true;
                }
            }
        }
    }
    return QFrame::eventFilter(obj, event);
}

void InputBar::dragEnterEvent(QDragEnterEvent* event) {
    if (m_busy || !event->mimeData()->hasUrls()) return;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString local = url.toLocalFile();
        if (!local.isEmpty() && QFileInfo(local).isFile()) {
            event->acceptProposedAction();
            return;
        }
    }
}

void InputBar::dropEvent(QDropEvent* event) {
    if (m_busy || !event->mimeData()->hasUrls()) return;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString local = url.toLocalFile();
        if (!local.isEmpty() && QFileInfo(local).isFile()) {
            setAttachment(local);
            event->acceptProposedAction();
            focusInput();
            return;
        }
    }
}

void InputBar::triggerSend() {
    if (m_busy) return;
    const QString text = m_promptEdit->toPlainText().trimmed();
    if (text.isEmpty() && m_attachmentPath.isEmpty()) return;

    const QString att = m_attachmentPath;
    m_promptEdit->clear();
    clearAttachment();
    updateEditorHeight();
    emit sendRequested(text, att);
}

void InputBar::openAttachDialog() {
    const QString startDir = m_workspaceDir.isEmpty() ? QDir::homePath() : m_workspaceDir;
    const QString chosen = QFileDialog::getOpenFileName(this, "Attach File from Workspace", startDir);
    if (!chosen.isEmpty()) {
        setAttachment(chosen);
        focusInput();
    }
}

void InputBar::updateEditorHeight() {
    const int docH = qCeil(m_promptEdit->document()->documentLayout()->documentSize().height());
    const int target = qBound(36, docH + 12, 140);
    if (m_promptEdit->height() != target) {
        m_promptEdit->setFixedHeight(target);
    }
}
