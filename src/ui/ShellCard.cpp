#include "ShellCard.h"
#include "Icons.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

ShellCard::ShellCard(const QString& command, QWidget* parent) : QFrame(parent) {
    setObjectName("ShellCard");
    setFrameShape(QFrame::StyledPanel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_toggleButton = new QToolButton(this);
    m_toggleButton->setObjectName("ShellToggle");
    m_toggleButton->setIcon(Icons::chevronDown(QColor("#8b949e")));
    m_toggleButton->setIconSize(QSize(12, 12));
    m_toggleButton->setCheckable(true);
    m_toggleButton->setChecked(true);
    m_toggleButton->setCursor(Qt::PointingHandCursor);
    m_toggleButton->setVisible(false);
    headerLayout->addWidget(m_toggleButton);

    auto* termIcon = new QLabel(this);
    termIcon->setPixmap(Icons::terminal(QColor("#79c0ff")).pixmap(14, 14));
    headerLayout->addWidget(termIcon);

    m_commandLabel = new QLabel(command, this);
    m_commandLabel->setObjectName("ShellCommand");
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_commandLabel->setWordWrap(true);
    headerLayout->addWidget(m_commandLabel, 1);

    m_copyButton = new QPushButton("Copy", this);
    m_copyButton->setObjectName("CopyButton");
    m_copyButton->setIcon(Icons::copy(QColor("#8b949e")));
    m_copyButton->setIconSize(QSize(12, 12));
    m_copyButton->setCursor(Qt::PointingHandCursor);
    m_copyButton->setVisible(false);
    headerLayout->addWidget(m_copyButton);

    m_statusLabel = new QLabel("Running", this);
    m_statusLabel->setObjectName("ShellStatus");
    m_statusLabel->setStyleSheet("background-color: #2c1e0c; color: #f0883e;");
    headerLayout->addWidget(m_statusLabel);

    layout->addLayout(headerLayout);

    m_outputEdit = new QPlainTextEdit(this);
    m_outputEdit->setObjectName("ShellOutput");
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setMaximumBlockCount(3000);
    m_outputEdit->setVisible(false);
    layout->addWidget(m_outputEdit);

    connect(m_toggleButton, &QToolButton::toggled, this, [this](bool expanded) {
        m_toggleButton->setIcon(expanded ? Icons::chevronDown(QColor("#8b949e"))
        : Icons::chevronRight(QColor("#8b949e")));
        m_outputEdit->setVisible(m_hasOutput && expanded);
    });

    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        if (auto* cb = QGuiApplication::clipboard()) {
            cb->setText(m_outputEdit->toPlainText());
            m_copyButton->setIcon(Icons::check(QColor("#3fb950")));
            m_copyButton->setText("Copied");
            QTimer::singleShot(1300, this, [this]() {
                if (m_copyButton) {
                    m_copyButton->setIcon(Icons::copy(QColor("#8b949e")));
                    m_copyButton->setText("Copy");
                }
            });
        }
    });
}

void ShellCard::appendOutput(const QString& line) {
    if (line.isEmpty()) return;

    if (!m_hasOutput) {
        m_hasOutput = true;
        m_toggleButton->setVisible(true);
        m_copyButton->setVisible(true);
        m_outputEdit->setVisible(m_toggleButton->isChecked());
    }

    QTextCursor cursor = m_outputEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line);
    m_outputEdit->setTextCursor(cursor);

    if (auto* sb = m_outputEdit->verticalScrollBar()) {
        sb->setValue(sb->maximum());
    }
    adjustOutputHeight();
}

void ShellCard::markFinished(bool cancelled) {
    if (cancelled) {
        m_statusLabel->setText("■ Stopped");
        m_statusLabel->setStyleSheet("background-color: #21262d; color: #f0883e;");
    } else {
        m_statusLabel->setText("✓ Done");
        m_statusLabel->setStyleSheet("background-color: #13281b; color: #3fb950;");
    }
}

void ShellCard::adjustOutputHeight() {
    int blocks = m_outputEdit->document()->blockCount();
    if (blocks > 1 && m_outputEdit->document()->lastBlock().text().isEmpty()) {
        --blocks;
    }
    blocks = qMax(1, blocks);
    const int lineHeight = m_outputEdit->fontMetrics().lineSpacing();
    const int target = qBound(34, blocks * lineHeight + 16, 260);
    if (m_outputEdit->height() != target) {
        m_outputEdit->setFixedHeight(target);
    }
}
