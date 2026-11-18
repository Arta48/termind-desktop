#pragma once

#include "core/RpcEvents.h"

#include <QDialog>
#include <QString>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    void updateFromState(const AgentStateInfo& state);
    void appendMcpOutput(const QString& output);
    void resetMcpLog();

signals:
    void saveApiKeyRequested(const QString& apiKey);
    void resetApiKeyRequested();
    void saveBaseUrlRequested(const QString& url);
    void resetBaseUrlRequested();
    void mcpCommandRequested(const QString& subCmd, const QString& subArg);
    void backendCommandChanged(const QString& command);

private:
    QLabel* m_providerValueLabel = nullptr;
    QLabel* m_maskedKeyLabel = nullptr;
    QLineEdit* m_apiKeyEdit = nullptr;
    QPushButton* m_toggleKeyVisBtn = nullptr;

    QLineEdit* m_baseUrlEdit = nullptr;
    QLineEdit* m_backendCmdEdit = nullptr;

    QPlainTextEdit* m_mcpLogEdit = nullptr;
    QLineEdit* m_mcpServerEdit = nullptr;
};
