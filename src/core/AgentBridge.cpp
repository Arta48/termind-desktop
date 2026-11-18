#include "AgentBridge.h"
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QProcessEnvironment>
#include <QStandardPaths>

AgentBridge::AgentBridge(QObject* parent) : QObject(parent) {
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &AgentBridge::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &AgentBridge::onReadyReadStderr);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) {
            const QString msg = tr("Failed to start termind backend: %1").arg(m_process->errorString());
            emit errorReceived(msg);
            emit startFailed(msg);
        }
    });
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus) {
                if (!m_intentionalStop) {
                    emit processExited(exitCode);
                }
            });
}

AgentBridge::~AgentBridge() {
    stop();
}

void AgentBridge::start(const QString& workspaceDir, const QString& termindCommand) {
    stop();
    m_stdoutBuffer.clear();
    m_intentionalStop = false;

    const QString trimmedCmd = termindCommand.trimmed().isEmpty() ? QString("termind") : termindCommand.trimmed();
    m_process->setWorkingDirectory(workspaceDir);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QStringList rpcArgs = {"--rpc", workspaceDir};

    #ifdef Q_OS_WIN
    // Prefer direct venv python.exe on Windows to avoid orphaned cmd.exe wrapper processes
    if (trimmedCmd == "termind") {
        const QString localBase = qEnvironmentVariable("LOCALAPPDATA") + "\\Programs\\Termind";
        const QString localPython = localBase + "\\.venv\\Scripts\\python.exe";
        const QString localPkgInit = localBase + "\\.venv\\Lib\\site-packages\\termind\\__init__.py";
        const QString localScript = localBase + "\\termind.py";
        if (QFile::exists(localPython)) {
            if (QFile::exists(localPkgInit)) {
                env.insert("PYTHONUTF8", "1");
                m_process->setProcessEnvironment(env);
                m_process->start(localPython, QStringList{"-P", "-m", "termind"} + rpcArgs);
                return;
            }
            if (QFile::exists(localScript)) {
                env.insert("PYTHONUTF8", "1");
                m_process->setProcessEnvironment(env);
                m_process->start(localPython, QStringList{localScript} + rpcArgs);
                return;
            }
        }
    }

    const QStringList cmdParts = QProcess::splitCommand(trimmedCmd);
    if (cmdParts.isEmpty()) {
        const QString msg = tr("Backend command is empty.");
        emit errorReceived(msg);
        emit startFailed(msg);
        return;
    }

    const QString exePath = QStandardPaths::findExecutable(cmdParts.first());
    const QString target = exePath.isEmpty() ? cmdParts.first() : exePath;
    m_process->setProcessEnvironment(env);
    if (target.endsWith(".cmd", Qt::CaseInsensitive) || target.endsWith(".bat", Qt::CaseInsensitive) ||
        (exePath.isEmpty() && !cmdParts.first().endsWith(".exe", Qt::CaseInsensitive))) {
        m_process->start("cmd.exe", QStringList{"/c"} + cmdParts + rpcArgs);
    } else {
        m_process->start(target, cmdParts.mid(1) + rpcArgs);
    }
    #else
    const QStringList cmdParts = QProcess::splitCommand(trimmedCmd);
    if (cmdParts.isEmpty()) {
        const QString msg = tr("Backend command is empty.");
        emit errorReceived(msg);
        emit startFailed(msg);
        return;
    }
    m_process->setProcessEnvironment(env);
    m_process->start(cmdParts.first(), cmdParts.mid(1) + rpcArgs);
    #endif
}

void AgentBridge::stop() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_intentionalStop = true;
        if (m_process->state() == QProcess::Running) {
            sendJsonRpcNotification("quit");
            m_process->closeWriteChannel();
        }
        if (!m_process->waitForFinished(1500)) {
            m_process->terminate();
            if (!m_process->waitForFinished(1000)) {
                m_process->kill();
                m_process->waitForFinished(500);
            }
        }
    }
}

bool AgentBridge::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

void AgentBridge::sendJsonRpcNotification(const QString& method, const QJsonObject& params) {
    if (!isRunning()) return;
    QJsonObject msg{
        {"jsonrpc", "2.0"},
        {"method", method}
    };
    if (!params.isEmpty()) {
        msg["params"] = params;
    }
    const QByteArray line = QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n";
    m_process->write(line);
}

void AgentBridge::sendJsonRpcResponse(const QString& id, const QJsonObject& result) {
    if (!isRunning()) return;
    const QJsonObject msg{
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };
    const QByteArray line = QJsonDocument(msg).toJson(QJsonDocument::Compact) + "\n";
    m_process->write(line);
}

void AgentBridge::sendPrompt(const QString& text, const QString& attachmentPath) {
    QJsonObject params{{"text", text}};
    if (!attachmentPath.isEmpty()) params["attachment"] = attachmentPath;
    sendJsonRpcNotification("prompt", params);
}

void AgentBridge::sendPermissionReply(const QString& requestId, bool allow) {
    sendJsonRpcResponse(requestId, {{"allow", allow}});
}

void AgentBridge::cancelTurn() { sendJsonRpcNotification("cancel"); }
void AgentBridge::setProvider(const QString& p) { sendJsonRpcNotification("set_provider", {{"provider", p}}); }
void AgentBridge::setModel(const QString& m) { sendJsonRpcNotification("set_model", {{"model", m}}); }
void AgentBridge::setThinking(const QString& lvl) { sendJsonRpcNotification("set_thinking", {{"level", lvl}}); }
void AgentBridge::setYolo(bool enabled) { sendJsonRpcNotification("set_yolo", {{"enabled", enabled}}); }
void AgentBridge::setBaseUrl(const QString& url) { sendJsonRpcNotification("set_base_url", {{"url", url}}); }
void AgentBridge::setApiKey(const QString& apiKey) { sendJsonRpcNotification("set_key", {{"api_key", apiKey}}); }

void AgentBridge::sendMcpCommand(const QString& subCmd, const QString& subArg) {
    QJsonObject params{{"sub_cmd", subCmd}};
    if (!subArg.isEmpty()) params["sub_arg"] = subArg;
    sendJsonRpcNotification("mcp_command", params);
}

void AgentBridge::resetContext() { sendJsonRpcNotification("reset"); }
void AgentBridge::compactContext() { sendJsonRpcNotification("compact"); }

void AgentBridge::onReadyReadStdout() {
    m_stdoutBuffer.append(m_process->readAllStandardOutput());
    int idx = -1;
    // Split strictly on newlines so partial TCP/pipe chunks never break JSON parsing
    while ((idx = m_stdoutBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_stdoutBuffer.left(idx).trimmed();
        m_stdoutBuffer.remove(0, idx + 1);
        if (!line.isEmpty()) {
            handleEventLine(line);
        }
    }
}

void AgentBridge::onReadyReadStderr() {
    QString err = QString::fromUtf8(m_process->readAllStandardError());
    if (!err.isEmpty()) emit backendLog(err);
}

void AgentBridge::handleEventLine(const QByteArray& line) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    // Extract method and params (compatible with JSON-RPC 2.0 and legacy format)
    QString method = obj.value("method").toString();
    if (method.isEmpty()) {
        method = obj.value("type").toString();
    }

    QJsonObject params = obj.value("params").toObject();
    if (params.isEmpty() && !obj.contains("params")) {
        params = obj;
    }

    // Server -> Client Request (permission_request has "id" and "method")
    if (obj.contains("id") && method == "permission_request") {
        emit permissionRequested(
            obj.value("id").toVariant().toString(),
            params.value("action").toString(),
            params.value("details").toString()
        );
        return;
    }

    if (method == "ready") emit ready(params);
    else if (method == "state") emit stateUpdated(params);
    else if (method == "thought_delta") emit thoughtDelta(params.value("text").toString());
    else if (method == "text_delta") emit textDelta(params.value("text").toString());
    else if (method == "step_done") emit stepDone();
    else if (method == "turn_done") emit turnDone();
    else if (method == "permission_request") {
        emit permissionRequested(
            params.value("id").toString(),
            params.value("action").toString(),
            params.value("details").toString()
        );
    }
    else if (method == "tool_start") {
        emit toolStarted(params.value("name").toString(), params.value("args").toObject());
    }
    else if (method == "tool_end") {
        emit toolEnded(
            params.value("name").toString(),
            params.value("success").toBool(true),
            params.value("error").toString()
        );
    }
    else if (method == "shell_start") emit shellStarted(params.value("command").toString());
    else if (method == "shell_output") emit shellOutput(params.value("line").toString());
    else if (method == "shell_end") emit shellEnded(params.value("command").toString());
    else if (method == "usage") {
        emit tokenUsage(params.value("prompt").toInt(), params.value("output").toInt(), params.value("total").toInt());
    }
    else if (method == "error") {
        if (obj.contains("error") && obj.value("error").isObject()) {
            emit errorReceived(obj.value("error").toObject().value("message").toString());
        } else {
            emit errorReceived(params.value("message").toString());
        }
    }
    else if (method == "notice") emit noticeReceived(params.value("text").toString());
    else if (method == "mcp_response") emit mcpResponse(params.value("output").toString());
}
