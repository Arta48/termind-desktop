#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QPushButton;
class QTextEdit;

class ApprovalCard : public QFrame {
    Q_OBJECT

public:
    explicit ApprovalCard(const QString& requestId,
                          const QString& action,
                          const QString& details,
                          QWidget* parent = nullptr);

    QString requestId() const { return m_requestId; }
    void markCancelled();

signals:
    void resolved(const QString& requestId, bool allow);

private:
    void finishWithStatus(const QString& statusText, const QString& colorHex);
    static QString formatDetailsAsHtml(const QString& details);

    QString m_requestId;
    bool m_resolved = false;

    QLabel* m_titleLabel = nullptr;
    QTextEdit* m_detailsEdit = nullptr;
    QPushButton* m_allowButton = nullptr;
    QPushButton* m_rejectButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};
