#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

class ApprovalCard;
class QHBoxLayout;
class MessageCard;
class QLabel;
class QResizeEvent;
class QScrollArea;
class QVBoxLayout;
class ShellCard;

class ChatView : public QWidget {
    Q_OBJECT

public:
    explicit ChatView(QWidget* parent = nullptr);

    void clearChat();
    void addUserMessage(const QString& text, const QString& attachmentPath = {});
    void appendThoughtDelta(const QString& text);
    void appendTextDelta(const QString& text);
    void finishAssistantStep();
    void finishTurn(bool cancelled = false);

    void addNotice(const QString& text);
    void addError(const QString& message);
    void addApprovalRequest(const QString& requestId, const QString& action, const QString& details);

    void startTool(const QString& name, const QJsonObject& args);
    void endTool(const QString& name, bool success, const QString& error);

    void startShell(const QString& command);
    void appendShellOutput(const QString& line);
    void endShell(const QString& command, bool cancelled = false);

signals:
    void permissionResolved(const QString& requestId, bool allow);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void insertCard(QWidget* card);
    MessageCard* ensureAssistantCard();
    void updateColumnMargins();
    void scrollToBottom();

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_container = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QHBoxLayout* m_toolBadgeRow = nullptr;
    QLabel* m_toolBadge = nullptr;

    MessageCard* m_activeAssistantCard = nullptr;
    ShellCard* m_activeShellCard = nullptr;
    QList<ApprovalCard*> m_pendingApprovals;

    bool m_autoScroll = true;
    bool m_programmaticScroll = false;
};
