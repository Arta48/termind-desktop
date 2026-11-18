#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QToolButton;

class ShellCard : public QFrame {
    Q_OBJECT

public:
    explicit ShellCard(const QString& command, QWidget* parent = nullptr);

    void appendOutput(const QString& line);
    void markFinished(bool cancelled = false);

private:
    void adjustOutputHeight();

    bool m_hasOutput = false;
    QToolButton* m_toggleButton = nullptr;
    QLabel* m_commandLabel = nullptr;
    QPushButton* m_copyButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPlainTextEdit* m_outputEdit = nullptr;
};
