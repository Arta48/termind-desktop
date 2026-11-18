#pragma once

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QJsonObject>

class AgentBridge : public QObject {
    Q_OBJECT

public:
    explicit AgentBridge(QObject* parent = nullptr);
    ~AgentBridge() override;

    void start(const QString& workspaceDir, const QString& termindCommand = "termind");
    void stop();
    bool isRunning() const;

    // Outgoing commands -> Python stdin
    void sendPrompt(const QString& text, const QString& attachmentPath = {});
    void sendPermissionReply(const QString& requestId, bool allow);
    void cancelTurn();
    void setProvider(const QString& provider);
    void setModel(const QString& model);
    void setThinking(const QString& level);
    void setYolo(bool enabled);
    void setBaseUrl(const QString& url);
    void setApiKey(const QString& apiKey);
    void sendMcpCommand(const QString& subCmd, const QString& subArg = {});
    void resetContext();
    void compactContext();

signals:
    void ready(const QJsonObject& stateInfo);
    void stateUpdated(const QJsonObject& stateInfo);
    void thoughtDelta(const QString& text);
    void textDelta(const QString& text);
    void stepDone();
    void turnDone();
    void permissionRequested(const QString& id, const QString& action, const QString& details);
    void toolStarted(const QString& name, const QJsonObject& args);
    void toolEnded(const QString& name, bool success, const QString& error);
    void shellStarted(const QString& command);
    void shellOutput(const QString& line);
    void shellEnded(const QString& command);
    void tokenUsage(int promptTokens, int outputTokens, int totalTokens);
    void errorReceived(const QString& message);
    void noticeReceived(const QString& text);
    void mcpResponse(const QString& output);
    void backendLog(const QString& stderrText);
    void processExited(int exitCode);
    void startFailed(const QString& message);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();

private:
    void sendJsonRpcNotification(const QString& method, const QJsonObject& params = {});
    void sendJsonRpcResponse(const QString& id, const QJsonObject& result);
    void handleEventLine(const QByteArray& line);

    QProcess* m_process = nullptr;
    QByteArray m_stdoutBuffer;
    bool m_intentionalStop = false;
};
