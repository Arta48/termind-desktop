#include "ChatView.h"
#include "ApprovalCard.h"
#include "MessageCard.h"
#include "ShellCard.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLayoutItem>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

ChatView::ChatView(QWidget* parent) : QWidget(parent) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("ChatScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget(m_scrollArea);
    m_container->setObjectName("ChatContainer");

    m_cardsLayout = new QVBoxLayout(m_container);
    m_cardsLayout->setContentsMargins(24, 18, 24, 18);
    m_cardsLayout->setSpacing(10);
    m_cardsLayout->addStretch(1);

    m_scrollArea->setWidget(m_container);
    rootLayout->addWidget(m_scrollArea, 1);

    m_toolBadgeRow = new QHBoxLayout();
    m_toolBadgeRow->setContentsMargins(24, 4, 24, 6);

    m_toolBadge = new QLabel(this);
    m_toolBadge->setObjectName("ToolBadge");
    m_toolBadge->setWordWrap(true);
    m_toolBadge->setVisible(false);
    m_toolBadgeRow->addWidget(m_toolBadge, 1);
    rootLayout->addLayout(m_toolBadgeRow);

    if (auto* sb = m_scrollArea->verticalScrollBar()) {
        connect(sb, &QScrollBar::valueChanged, this, [this, sb](int value) {
            if (!m_programmaticScroll) {
                m_autoScroll = (value >= sb->maximum() - 40);
            }
        });
        connect(sb, &QScrollBar::rangeChanged, this, [this, sb](int, int maxVal) {
            if (m_autoScroll) {
                m_programmaticScroll = true;
                sb->setValue(maxVal);
                m_programmaticScroll = false;
            }
        });
    }
}

void ChatView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateColumnMargins();
}

void ChatView::updateColumnMargins() {
    const int vw = m_scrollArea->viewport()->width();
    const int sideMargin = qMax(24, (vw - 980) / 2);
    m_cardsLayout->setContentsMargins(sideMargin, 18, sideMargin, 18);
    m_toolBadgeRow->setContentsMargins(sideMargin, 4, sideMargin, 6);
}

void ChatView::clearChat() {
    m_activeAssistantCard = nullptr;
    m_activeShellCard = nullptr;
    m_pendingApprovals.clear();
    m_toolBadge->setVisible(false);

    while (m_cardsLayout->count() > 1) {
        QLayoutItem* item = m_cardsLayout->takeAt(0);
        if (auto* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_autoScroll = true;
}

void ChatView::addUserMessage(const QString& text, const QString& attachmentPath) {
    QString body = text;
    if (!attachmentPath.isEmpty()) {
        const QString fileName = QFileInfo(attachmentPath).fileName();
        const QString tag = QString("[Attached: %1]").arg(fileName.isEmpty() ? attachmentPath : fileName);
        body = body.isEmpty() ? tag : QString("%1\n\n%2").arg(tag, body);
    }
    auto* card = new MessageCard(MessageCard::Role::User, body, m_container);
    insertCard(card);
    m_autoScroll = true;
    scrollToBottom();
}

void ChatView::appendThoughtDelta(const QString& text) {
    if (text.isEmpty()) return;
    ensureAssistantCard()->appendThoughtDelta(text);
}

void ChatView::appendTextDelta(const QString& text) {
    if (text.isEmpty()) return;
    ensureAssistantCard()->appendTextDelta(text);
}

void ChatView::finishAssistantStep() {
    if (m_activeAssistantCard) {
        m_activeAssistantCard->finishStreaming();
        m_activeAssistantCard = nullptr;
    }
}

void ChatView::finishTurn(bool cancelled) {
    finishAssistantStep();
    if (m_activeShellCard) {
        m_activeShellCard->markFinished(cancelled);
        m_activeShellCard = nullptr;
    }
    if (cancelled) {
        for (ApprovalCard* card : m_pendingApprovals) {
            if (card) card->markCancelled();
        }
    }
    m_pendingApprovals.clear();
    m_toolBadge->setVisible(false);
}

void ChatView::addNotice(const QString& text) {
    if (text.trimmed().isEmpty()) return;
    finishAssistantStep();
    auto* card = new MessageCard(MessageCard::Role::Notice, text, m_container);
    insertCard(card);
}

void ChatView::addError(const QString& message) {
    if (message.trimmed().isEmpty()) return;
    finishAssistantStep();
    auto* card = new MessageCard(MessageCard::Role::Error, message, m_container);
    insertCard(card);
}

void ChatView::addApprovalRequest(const QString& requestId, const QString& action, const QString& details) {
    finishAssistantStep();
    auto* card = new ApprovalCard(requestId, action, details, m_container);
    m_pendingApprovals.append(card);
    connect(card, &ApprovalCard::resolved, this, [this, card](const QString& id, bool allow) {
        m_pendingApprovals.removeAll(card);
        emit permissionResolved(id, allow);
    });
    insertCard(card);
    m_autoScroll = true;
    scrollToBottom();
}

void ChatView::startTool(const QString& name, const QJsonObject& args) {
    QString argPreview;
    if (!args.isEmpty()) {
        argPreview = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
        if (argPreview.size() > 100) {
            argPreview = argPreview.left(97) + "...";
        }
    }
    m_toolBadge->setText(argPreview.isEmpty()
    ? QString("Running tool: %1").arg(name)
    : QString("Running tool: %1  %2").arg(name, argPreview));
    m_toolBadge->setVisible(true);
}

void ChatView::endTool(const QString& name, bool success, const QString& error) {
    Q_UNUSED(name);
    Q_UNUSED(success);
    Q_UNUSED(error);
    m_toolBadge->setVisible(false);
}

void ChatView::startShell(const QString& command) {
    finishAssistantStep();
    m_activeShellCard = new ShellCard(command, m_container);
    insertCard(m_activeShellCard);
    m_autoScroll = true;
    scrollToBottom();
}

void ChatView::appendShellOutput(const QString& line) {
    if (!m_activeShellCard) {
        startShell("(shell)");
    }
    m_activeShellCard->appendOutput(line);
}

void ChatView::endShell(const QString& command, bool cancelled) {
    Q_UNUSED(command);
    if (m_activeShellCard) {
        m_activeShellCard->markFinished(cancelled);
        m_activeShellCard = nullptr;
    }
}

void ChatView::insertCard(QWidget* card) {
    const int insertIdx = qMax(0, m_cardsLayout->count() - 1);
    m_cardsLayout->insertWidget(insertIdx, card);
}

MessageCard* ChatView::ensureAssistantCard() {
    if (!m_activeAssistantCard) {
        m_activeAssistantCard = new MessageCard(MessageCard::Role::Assistant, {}, m_container);
        insertCard(m_activeAssistantCard);
    }
    return m_activeAssistantCard;
}

void ChatView::scrollToBottom() {
    QTimer::singleShot(0, this, [this]() {
        if (auto* sb = m_scrollArea->verticalScrollBar()) {
            m_programmaticScroll = true;
            sb->setValue(sb->maximum());
            m_programmaticScroll = false;
        }
    });
}
