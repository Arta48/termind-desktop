#pragma once

#include "core/RpcEvents.h"

#include <QMainWindow>
#include <QString>

class AgentBridge;
class ChatView;
class InputBar;
class QCloseEvent;
class QComboBox;
class QLabel;
class QPushButton;
class QSplitter;
class SettingsDialog;
class WorkspacePanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& initialWorkspace, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void connectSignals();
    void startWorkspace(const QString& workspaceDir);
    void applyAgentState(const AgentStateInfo& info);
    void refreshModelComboOptions(const QString& provider, const QString& activeModel);
    void setTurnBusy(bool busy);
    void updateControlsEnabled();
    void setStatusIndicator(const QString& colorHex, const QString& text);
    void openSettingsDialog();

    AgentBridge* m_bridge = nullptr;
    AgentStateInfo m_state;
    QString m_workspaceDir;

    bool m_updatingUi = false;
    bool m_backendReady = false;
    bool m_turnBusy = false;
    bool m_turnCancelled = false;

    // Toolbar controls
    QPushButton* m_openFolderBtn = nullptr;
    QComboBox* m_providerCombo = nullptr;
    QComboBox* m_modelCombo = nullptr;
    QComboBox* m_thinkingCombo = nullptr;
    QPushButton* m_yoloButton = nullptr;
    QPushButton* m_compactButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_settingsButton = nullptr;

    // Main layout
    QSplitter* m_splitter = nullptr;
    WorkspacePanel* m_workspacePanel = nullptr;
    ChatView* m_chatView = nullptr;
    InputBar* m_inputBar = nullptr;

    // Status bar
    QLabel* m_statusDot = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_tokenLabel = nullptr;

    SettingsDialog* m_settingsDialog = nullptr;
};
