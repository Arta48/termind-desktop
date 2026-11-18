#include "SettingsDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Termind Settings");
    resize(620, 480);

    auto* rootLayout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    rootLayout->addWidget(tabs, 1);

    // --- Tab 1: Provider & Backend ---
    auto* providerTab = new QWidget(this);
    auto* providerTabLayout = new QVBoxLayout(providerTab);

    auto* activeGroup = new QGroupBox("Active Provider Credentials", providerTab);
    auto* form = new QFormLayout(activeGroup);

    m_providerValueLabel = new QLabel("-", activeGroup);
    m_providerValueLabel->setStyleSheet("font-weight: 700;");
    form->addRow("Provider:", m_providerValueLabel);

    m_maskedKeyLabel = new QLabel("(not set)", activeGroup);
    form->addRow("Current Key:", m_maskedKeyLabel);

    auto* keyRow = new QHBoxLayout();
    m_apiKeyEdit = new QLineEdit(activeGroup);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("Enter new API key...");
    keyRow->addWidget(m_apiKeyEdit, 1);

    m_toggleKeyVisBtn = new QPushButton("Show", activeGroup);
    m_toggleKeyVisBtn->setCheckable(true);
    keyRow->addWidget(m_toggleKeyVisBtn);

    auto* saveKeyBtn = new QPushButton("Save Key", activeGroup);
    auto* resetKeyBtn = new QPushButton("Reset", activeGroup);
    keyRow->addWidget(saveKeyBtn);
    keyRow->addWidget(resetKeyBtn);
    form->addRow("Set API Key:", keyRow);

    auto* urlRow = new QHBoxLayout();
    m_baseUrlEdit = new QLineEdit(activeGroup);
    m_baseUrlEdit->setPlaceholderText("Custom Base URL (or leave empty for default)");
    urlRow->addWidget(m_baseUrlEdit, 1);

    auto* saveUrlBtn = new QPushButton("Save URL", activeGroup);
    auto* resetUrlBtn = new QPushButton("Reset", activeGroup);
    urlRow->addWidget(saveUrlBtn);
    urlRow->addWidget(resetUrlBtn);
    form->addRow("Base URL:", urlRow);

    providerTabLayout->addWidget(activeGroup);

    auto* backendGroup = new QGroupBox("Desktop Backend", providerTab);
    auto* backendForm = new QFormLayout(backendGroup);
    auto* cmdRow = new QHBoxLayout();

    QSettings settings;
    m_backendCmdEdit = new QLineEdit(settings.value("backend/command", "termind").toString(), backendGroup);
    m_backendCmdEdit->setPlaceholderText("termind");
    cmdRow->addWidget(m_backendCmdEdit, 1);

    auto* applyCmdBtn = new QPushButton("Apply & Restart Backend", backendGroup);
    cmdRow->addWidget(applyCmdBtn);
    backendForm->addRow("Command:", cmdRow);

    providerTabLayout->addWidget(backendGroup);
    providerTabLayout->addStretch();
    tabs->addTab(providerTab, "Provider & API");

    // --- Tab 2: MCP Servers ---
    auto* mcpTab = new QWidget(this);
    auto* mcpLayout = new QVBoxLayout(mcpTab);

    m_mcpLogEdit = new QPlainTextEdit(mcpTab);
    m_mcpLogEdit->setReadOnly(true);
    mcpLayout->addWidget(m_mcpLogEdit, 1);

    auto* mcpControls = new QHBoxLayout();
    m_mcpServerEdit = new QLineEdit(mcpTab);
    m_mcpServerEdit->setPlaceholderText("Server name (leave blank for 'all')");
    mcpControls->addWidget(m_mcpServerEdit, 1);

    auto* statusBtn = new QPushButton("Status", mcpTab);
    auto* connectBtn = new QPushButton("Connect", mcpTab);
    auto* disconnectBtn = new QPushButton("Disconnect", mcpTab);
    auto* reloadBtn = new QPushButton("Reload", mcpTab);
    mcpControls->addWidget(statusBtn);
    mcpControls->addWidget(connectBtn);
    mcpControls->addWidget(disconnectBtn);
    mcpControls->addWidget(reloadBtn);
    mcpLayout->addLayout(mcpControls);

    tabs->addTab(mcpTab, "MCP Plugins");

    // Bottom close button
    auto* bottomRow = new QHBoxLayout();
    bottomRow->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    bottomRow->addWidget(closeBtn);
    rootLayout->addLayout(bottomRow);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::hide);

    connect(m_toggleKeyVisBtn, &QPushButton::toggled, this, [this](bool show) {
        m_apiKeyEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
        m_toggleKeyVisBtn->setText(show ? "Hide" : "Show");
    });

    connect(saveKeyBtn, &QPushButton::clicked, this, [this]() {
        const QString key = m_apiKeyEdit->text().trimmed();
        if (!key.isEmpty()) {
            emit saveApiKeyRequested(key);
            m_apiKeyEdit->clear();
        }
    });
    connect(resetKeyBtn, &QPushButton::clicked, this, &SettingsDialog::resetApiKeyRequested);

    connect(saveUrlBtn, &QPushButton::clicked, this, [this]() {
        const QString url = m_baseUrlEdit->text().trimmed();
        if (!url.isEmpty()) {
            emit saveBaseUrlRequested(url);
        }
    });
    connect(resetUrlBtn, &QPushButton::clicked, this, &SettingsDialog::resetBaseUrlRequested);

    connect(applyCmdBtn, &QPushButton::clicked, this, [this]() {
        const QString cmd = m_backendCmdEdit->text().trimmed();
        if (!cmd.isEmpty()) {
            QSettings settings;
            settings.setValue("backend/command", cmd);
            emit backendCommandChanged(cmd);
        }
    });

    connect(statusBtn, &QPushButton::clicked, this, [this]() {
        emit mcpCommandRequested("status", {});
    });
    connect(reloadBtn, &QPushButton::clicked, this, [this]() {
        emit mcpCommandRequested("reload", {});
    });
    connect(connectBtn, &QPushButton::clicked, this, [this]() {
        const QString target = m_mcpServerEdit->text().trimmed();
        emit mcpCommandRequested("connect", target.isEmpty() ? "all" : target);
    });
    connect(disconnectBtn, &QPushButton::clicked, this, [this]() {
        const QString target = m_mcpServerEdit->text().trimmed();
        emit mcpCommandRequested("disconnect", target.isEmpty() ? "all" : target);
    });
}

void SettingsDialog::updateFromState(const AgentStateInfo& state) {
    m_providerValueLabel->setText(state.provider);
    m_maskedKeyLabel->setText(state.apiKeyMasked.isEmpty() ? "(not set)" : state.apiKeyMasked);
    if (!m_baseUrlEdit->hasFocus()) {
        m_baseUrlEdit->setText(state.baseUrl);
    }
    if (!state.mcpStatus.isEmpty() && m_mcpLogEdit->toPlainText().isEmpty()) {
        m_mcpLogEdit->setPlainText(state.mcpStatus);
    }
}

void SettingsDialog::resetMcpLog() {
    m_mcpLogEdit->clear();
}

void SettingsDialog::appendMcpOutput(const QString& output) {
    if (output.trimmed().isEmpty()) return;
    if (output.startsWith("MCP Servers:") ||
        output.startsWith("No MCP servers") ||
        output.startsWith("MCP Python SDK")) {
        m_mcpLogEdit->setPlainText(output);
    } else {
        m_mcpLogEdit->appendPlainText(output);
    }
}
