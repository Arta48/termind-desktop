#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QPushButton;
class QTextBrowser;
class QTimer;
class QToolButton;

class MessageCard : public QFrame {
    Q_OBJECT

public:
    enum class Role {
        User,
        Assistant,
        Notice,
        Error
    };

    explicit MessageCard(Role role, const QString& initialText = {}, QWidget* parent = nullptr);

    void setText(const QString& text);
    void appendTextDelta(const QString& delta);
    void appendThoughtDelta(const QString& delta);
    void finishStreaming();

    QString rawText() const { return m_rawText; }

private:
    void renderBody();
    void renderThoughts();
    static QString markdownToHtml(const QString& markdown);
    static QString roleToProperty(Role role);
    static QString roleTitle(Role role);

    Role m_role;
    QString m_rawText;
    QString m_rawThoughts;

    QLabel* m_roleLabel = nullptr;
    QLabel* m_noticeLabel = nullptr;
    QToolButton* m_reasoningToggle = nullptr;
    QPushButton* m_copyButton = nullptr;
    QTextBrowser* m_reasoningBox = nullptr;
    QTextBrowser* m_bodyBrowser = nullptr;
    QTimer* m_renderTimer = nullptr;
};
