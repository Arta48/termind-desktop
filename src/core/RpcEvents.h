#pragma once

#include <QJsonObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

struct AgentStateInfo {
    QString workspace;
    QString osName;
    QString provider = "gemini";
    QString model = "gemini-3.8-flash";
    QString thinking = "high";
    bool yolo = false;
    QString baseUrl;
    QString apiKeyMasked;
    QString mcpStatus;

    static AgentStateInfo fromJson(const QJsonObject& obj) {
        AgentStateInfo s;
        s.workspace = obj.value("workspace").toString();
        s.osName = obj.value("os").toString();
        s.provider = obj.value("provider").toString("gemini");
        s.model = obj.value("model").toString("gemini-3.8-flash");
        s.thinking = obj.value("thinking").toString("high");
        s.yolo = obj.value("yolo").toBool(false);
        s.baseUrl = obj.value("base_url").toString();
        s.apiKeyMasked = obj.value("api_key_masked").toString();
        s.mcpStatus = obj.value("mcp").toString();
        return s;
    }
};

inline const QVector<QPair<QString, QString>>& knownProviders() {
    static const QVector<QPair<QString, QString>> providers = {
        {"gemini", "gemini-3.8-flash"},
        {"openai", "gpt-4o"},
        {"deepseek", "deepseek-flash"},
        {"claude", "claude-sonnet-4-6"},
        {"groq", "llama-3.3-70b-versatile"},
        {"ollama", "qwen2.5-coder:latest"},
        {"openrouter", "anthropic/claude-sonnet-4.6"},
    };
    return providers;
}

inline const QStringList& thinkingLevels() {
    static const QStringList levels = {"off", "low", "medium", "high"};
    return levels;
}
