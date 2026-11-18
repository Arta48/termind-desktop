#include "MainWindow.h"
#include "ChatView.h"
#include "Icons.h"
#include "InputBar.h"
#include "SettingsDialog.h"
#include "WorkspacePanel.h"
#include "core/AgentBridge.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(const QString& initialWorkspace, QWidget* parent)
: QMainWindow(parent), m_workspaceDir(initialWorkspace) {
    setWindowTitle("Termind Desktop");
    resize(1280, 720);
    setWindowIcon(QIcon(":/icon.png"));

    m_bridge = new AgentBridge(this);

    setupUi();
    connectSignals();

    QSettings settings;
    restoreGeometry(settings.value("window/geometry").toByteArray());
    m_splitter->restoreState(settings.value("window/splitter").toByteArray());

    startWorkspace(m_workspaceDir);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/splitter", m_splitter->saveState());

    if (m_bridge) {
        m_bridge->stop();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Top Toolbar ---
    auto* toolbar = new QFrame(central);
    toolbar->setObjectName("TopToolbar");
    auto* tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(12, 8, 12, 8);
    tbLayout->setSpacing(8);

    m_openFolderBtn = new QPushButton("Open Folder", toolbar);
    m_openFolderBtn->setIcon(Icons::folder(QColor("#c9d1d9")));
    m_openFolderBtn->setIconSize(QSize(15, 15));
    m_openFolderBtn->setCursor(Qt::PointingHandCursor);
    tbLayout->addWidget(m_openFolderBtn);

    tbLayout->addSpacing(4);

    auto* provLbl = new QLabel("Provider", toolbar);
    provLbl->setObjectName("ToolbarLabel");
    tbLayout->addWidget(provLbl);

    m_providerCombo = new QComboBox(toolbar);
    for (const auto& pair : knownProviders()) {
        m_providerCombo->addItem(pair.first);
    }
    tbLayout->addWidget(m_providerCombo);

    auto* modelLbl = new QLabel("Model", toolbar);
    modelLbl->setObjectName("ToolbarLabel");
    tbLayout->addWidget(modelLbl);

    m_modelCombo = new QComboBox(toolbar);
    m_modelCombo->setEditable(true);
    m_modelCombo->setInsertPolicy(QComboBox::NoInsert);
    m_modelCombo->setMinimumWidth(215);
    tbLayout->addWidget(m_modelCombo);

    auto* thinkLbl = new QLabel("Thinking", toolbar);
    thinkLbl->setObjectName("ToolbarLabel");
    tbLayout->addWidget(thinkLbl);

    m_thinkingCombo = new QComboBox(toolbar);
    m_thinkingCombo->addItems(thinkingLevels());
    tbLayout->addWidget(m_thinkingCombo);

    m_yoloButton = new QPushButton("YOLO", toolbar);
    m_yoloButton->setObjectName("YoloButton");
    m_yoloButton->setIcon(Icons::bolt(QColor("#f0883e")));
    m_yoloButton->setIconSize(QSize(14, 14));
    m_yoloButton->setCheckable(true);
    m_yoloButton->setCursor(Qt::PointingHandCursor);
    m_yoloButton->setToolTip("Auto-approve file edits and shell commands without prompting");
    tbLayout->addWidget(m_yoloButton);

    tbLayout->addStretch();

    m_compactButton = new QPushButton("Compact", toolbar);
    m_compactButton->setObjectName("GhostButton");
    m_compactButton->setIcon(Icons::layers(QColor("#9aa4b2")));
    m_compactButton->setIconSize(QSize(14, 14));
    m_compactButton->setCursor(Qt::PointingHandCursor);
    m_compactButton->setToolTip("Summarize and compact conversation memory");
    tbLayout->addWidget(m_compactButton);

    m_resetButton = new QPushButton("Reset", toolbar);
    m_resetButton->setObjectName("GhostButton");
    m_resetButton->setIcon(Icons::refresh(QColor("#9aa4b2")));
    m_resetButton->setIconSize(QSize(14, 14));
    m_resetButton->setCursor(Qt::PointingHandCursor);
    m_resetButton->setToolTip("Clear conversation memory");
    tbLayout->addWidget(m_resetButton);

    m_settingsButton = new QPushButton("Settings", toolbar);
    m_settingsButton->setObjectName("GhostButton");
    m_settingsButton->setIcon(Icons::settings(QColor("#9aa4b2")));
    m_settingsButton->setIconSize(QSize(14, 14));
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    tbLayout->addWidget(m_settingsButton);

    mainLayout->addWidget(toolbar);

    // --- Center Splitter ---
    m_splitter = new QSplitter(Qt::Horizontal, central);
    m_workspacePanel = new WorkspacePanel(m_splitter);

    auto* rightPane = new QWidget(m_splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_chatView = new ChatView(rightPane);
    m_inputBar = new InputBar(rightPane);

    rightLayout->addWidget(m_chatView, 1);
    rightLayout->addWidget(m_inputBar);

    m_splitter->addWidget(m_workspacePanel);
    m_splitter->addWidget(rightPane);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({250, 1030});

    mainLayout->addWidget(m_splitter, 1);
    setCentralWidget(central);

    // --- Status Bar ---
    m_statusDot = new QLabel("●", this);
    m_statusLabel = new QLabel("Starting backend...", this);
    m_tokenLabel = new QLabel("Tokens: 0 in · 0 out · 0 total", this);

    statusBar()->addWidget(m_statusDot);
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_tokenLabel);

    setStatusIndicator("#f0883e", "Starting backend...");
}

void MainWindow::connectSignals() {
    connect(m_openFolderBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(this, "Select Workspace Directory", m_workspaceDir);
        if (!dir.isEmpty()) {
            startWorkspace(dir);
        }
    });

    connect(m_providerCombo, &QComboBox::currentTextChanged, this, [this](const QString& provider) {
        if (!m_updatingUi && !provider.isEmpty() && provider != m_state.provider) {
            m_bridge->setProvider(provider);
        }
    });

    connect(m_modelCombo, &QComboBox::textActivated, this, [this](const QString& model) {
        const QString trimmed = model.trimmed();
        if (!m_updatingUi && !trimmed.isEmpty() && trimmed != m_state.model) {
            m_bridge->setModel(trimmed);
        }
    });
    if (auto* le = m_modelCombo->lineEdit()) {
        connect(le, &QLineEdit::editingFinished, this, [this, le]() {
            const QString trimmed = le->text().trimmed();
            if (!m_updatingUi && !trimmed.isEmpty() && trimmed != m_state.model) {
                m_bridge->setModel(trimmed);
            }
        });
    }

    connect(m_thinkingCombo, &QComboBox::currentTextChanged, this, [this](const QString& lvl) {
        if (!m_updatingUi && !lvl.isEmpty() && lvl != m_state.thinking) {
            m_bridge->setThinking(lvl);
        }
    });

    connect(m_yoloButton, &QPushButton::toggled, this, [this](bool checked) {
        if (!m_updatingUi && checked != m_state.yolo) {
            m_bridge->setYolo(checked);
        }
    });

    connect(m_compactButton, &QPushButton::clicked, this, [this]() {
        if (!m_backendReady || m_turnBusy) return;
        m_turnCancelled = false;
        setTurnBusy(true);
        m_bridge->compactContext();
    });

    connect(m_resetButton, &QPushButton::clicked, this, [this]() {
        if (!m_backendReady || m_turnBusy) return;
        m_bridge->resetContext();
        m_chatView->clearChat();
        m_tokenLabel->setText("Tokens: 0 in · 0 out · 0 total");
        m_chatView->addNotice("Conversation context reset.");
    });

    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettingsDialog);

    connect(m_workspacePanel, &WorkspacePanel::fileAttachRequested, this, [this](const QString& path) {
        m_inputBar->setAttachment(path);
        m_inputBar->focusInput();
    });

    connect(m_inputBar, &InputBar::sendRequested, this, [this](const QString& text, const QString& attachment) {
        if (!m_backendReady || m_turnBusy) return;
        m_turnCancelled = false;
        setTurnBusy(true);
        m_chatView->addUserMessage(text, attachment);
        m_bridge->sendPrompt(text, attachment);
    });

    connect(m_inputBar, &InputBar::cancelRequested, this, [this]() {
        if (!m_turnBusy) return;
        m_turnCancelled = true;
        setStatusIndicator("#f0883e", "Cancelling...");
        m_bridge->cancelTurn();
    });

    connect(m_chatView, &ChatView::permissionResolved, m_bridge, &AgentBridge::sendPermissionReply);

    // AgentBridge -> UI
    connect(m_bridge, &AgentBridge::ready, this, [this](const QJsonObject& obj) {
        m_backendReady = true;
        applyAgentState(AgentStateInfo::fromJson(obj));
        setTurnBusy(false);
        setStatusIndicator("#3fb950", QString("Ready · %1 · %2").arg(m_state.osName, m_state.workspace));
        m_inputBar->focusInput();
    });

    connect(m_bridge, &AgentBridge::stateUpdated, this, [this](const QJsonObject& obj) {
        applyAgentState(AgentStateInfo::fromJson(obj));
    });

    connect(m_bridge, &AgentBridge::thoughtDelta, m_chatView, &ChatView::appendThoughtDelta);
    connect(m_bridge, &AgentBridge::textDelta, m_chatView, &ChatView::appendTextDelta);
    connect(m_bridge, &AgentBridge::stepDone, m_chatView, &ChatView::finishAssistantStep);

    connect(m_bridge, &AgentBridge::turnDone, this, [this]() {
        m_chatView->finishTurn(m_turnCancelled);
        m_turnCancelled = false;
        setTurnBusy(false);
        setStatusIndicator("#3fb950", QString("Ready · %1 · %2").arg(m_state.osName, m_state.workspace));
    });

    connect(m_bridge, &AgentBridge::permissionRequested, m_chatView, &ChatView::addApprovalRequest);
    connect(m_bridge, &AgentBridge::toolStarted, m_chatView, &ChatView::startTool);
    connect(m_bridge, &AgentBridge::toolEnded, m_chatView, &ChatView::endTool);

    connect(m_bridge, &AgentBridge::shellStarted, m_chatView, &ChatView::startShell);
    connect(m_bridge, &AgentBridge::shellOutput, m_chatView, &ChatView::appendShellOutput);
    connect(m_bridge, &AgentBridge::shellEnded, this, [this](const QString& cmd) {
        m_chatView->endShell(cmd, m_turnCancelled);
    });

    connect(m_bridge, &AgentBridge::tokenUsage, this, [this](int prompt, int output, int total) {
        m_tokenLabel->setText(QString("Tokens: %1 in · %2 out · %3 total").arg(prompt).arg(output).arg(total));
    });

    connect(m_bridge, &AgentBridge::errorReceived, m_chatView, &ChatView::addError);
    connect(m_bridge, &AgentBridge::noticeReceived, m_chatView, &ChatView::addNotice);

    connect(m_bridge, &AgentBridge::mcpResponse, this, [this](const QString& output) {
        if (m_settingsDialog && m_settingsDialog->isVisible()) {
            m_settingsDialog->appendMcpOutput(output);
        } else {
            m_chatView->addNotice(output);
        }
    });

    connect(m_bridge, &AgentBridge::backendLog, this, [](const QString& stderrText) {
        qDebug().noquote() << "[termind stderr]" << stderrText.trimmed();
    });

    connect(m_bridge, &AgentBridge::processExited, this, [this](int exitCode) {
        m_backendReady = false;
        m_turnCancelled = false;
        m_chatView->finishTurn(true);
        setTurnBusy(false);
        setStatusIndicator("#ff7b72", QString("Backend exited (code %1)").arg(exitCode));
    });

    connect(m_bridge, &AgentBridge::startFailed, this, [this](const QString&) {
        m_backendReady = false;
        m_turnCancelled = false;
        setTurnBusy(false);
        setStatusIndicator("#ff7b72", "Failed to start backend (check Settings)");
    });
}

void MainWindow::startWorkspace(const QString& workspaceDir) {
    m_workspaceDir = workspaceDir;
    m_workspacePanel->setRootDirectory(workspaceDir);
    m_inputBar->setWorkspaceDir(workspaceDir);
    m_chatView->clearChat();
    m_backendReady = false;
    m_turnBusy = false;
    m_turnCancelled = false;
    updateControlsEnabled();
    m_tokenLabel->setText("Tokens: 0 in · 0 out · 0 total");
    if (m_settingsDialog) {
        m_settingsDialog->resetMcpLog();
    }
    setWindowTitle(QString("Termind Desktop — %1").arg(workspaceDir));
    setStatusIndicator("#f0883e", QString("Starting backend in %1...").arg(workspaceDir));

    QSettings settings;
    const QString cmd = settings.value("backend/command", "termind").toString();
    m_bridge->start(workspaceDir, cmd);
}

void MainWindow::applyAgentState(const AgentStateInfo& info) {
    m_updatingUi = true;
    m_state = info;

    const int pIdx = m_providerCombo->findText(info.provider);
    if (pIdx >= 0) {
        m_providerCombo->setCurrentIndex(pIdx);
    }

    refreshModelComboOptions(info.provider, info.model);

    const int tIdx = m_thinkingCombo->findText(info.thinking);
    if (tIdx >= 0) {
        m_thinkingCombo->setCurrentIndex(tIdx);
    }

    m_yoloButton->setChecked(info.yolo);

    if (m_settingsDialog) {
        m_settingsDialog->updateFromState(info);
    }

    m_updatingUi = false;
}

void MainWindow::refreshModelComboOptions(const QString& provider, const QString& activeModel) {
    m_modelCombo->clear();

    QStringList options;
    for (const auto& pair : knownProviders()) {
        if (pair.first == provider) {
            options << pair.second;
        }
    }
    if (provider == "gemini") {
        options << "gemini-3.5-flash-lite" << "gemini-2.5-flash" << "gemini-2.5-pro";
    } else if (provider == "openai") {
        options << "gpt-4.1" << "o4-mini";
    } else if (provider == "deepseek") {
        options << "deepseek-chat" << "deepseek-reasoner";
    } else if (provider == "claude") {
        options << "claude-opus-4-6" << "claude-haiku-4-5";
    }

    if (!activeModel.isEmpty() && !options.contains(activeModel)) {
        options.prepend(activeModel);
    }
    options.removeDuplicates();
    m_modelCombo->addItems(options);
    m_modelCombo->setCurrentText(activeModel);
}

void MainWindow::setTurnBusy(bool busy) {
    m_turnBusy = busy;
    updateControlsEnabled();

    if (busy) {
        setStatusIndicator("#f0883e", "Termind is working...");
    }
}

void MainWindow::updateControlsEnabled() {
    const bool interactive = m_backendReady && !m_turnBusy;
    m_inputBar->setEnabled(m_backendReady);
    m_inputBar->setBusy(m_turnBusy);
    m_providerCombo->setEnabled(interactive);
    m_modelCombo->setEnabled(interactive);
    m_thinkingCombo->setEnabled(interactive);
    m_yoloButton->setEnabled(interactive);
    m_compactButton->setEnabled(interactive);
    m_resetButton->setEnabled(interactive);
    m_openFolderBtn->setEnabled(!m_turnBusy);
}

void MainWindow::setStatusIndicator(const QString& colorHex, const QString& text) {
    m_statusDot->setStyleSheet(QString("color: %1; font-size: 11px;").arg(colorHex));
    m_statusLabel->setText(text);
}

void MainWindow::openSettingsDialog() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        connect(m_settingsDialog, &SettingsDialog::saveApiKeyRequested, m_bridge, &AgentBridge::setApiKey);
        connect(m_settingsDialog, &SettingsDialog::resetApiKeyRequested, this, [this]() {
            m_bridge->setApiKey("reset");
        });
        connect(m_settingsDialog, &SettingsDialog::saveBaseUrlRequested, m_bridge, &AgentBridge::setBaseUrl);
        connect(m_settingsDialog, &SettingsDialog::resetBaseUrlRequested, this, [this]() {
            m_bridge->setBaseUrl("reset");
        });
        connect(m_settingsDialog, &SettingsDialog::mcpCommandRequested, m_bridge, &AgentBridge::sendMcpCommand);
        connect(m_settingsDialog, &SettingsDialog::backendCommandChanged, this, [this](const QString&) {
            startWorkspace(m_workspaceDir);
        });
    }
    m_settingsDialog->updateFromState(m_state);
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}
