#include "ApprovalCard.h"
#include "Icons.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

ApprovalCard::ApprovalCard(const QString& requestId,
                           const QString& action,
                           const QString& details,
                           QWidget* parent)
: QFrame(parent), m_requestId(requestId) {
    setObjectName("ApprovalCard");
    setFrameShape(QFrame::StyledPanel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 11, 14, 11);
    layout->setSpacing(9);

    auto* titleRow = new QHBoxLayout();
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(8);

    auto* warnIcon = new QLabel(this);
    warnIcon->setPixmap(Icons::warning(QColor("#f0b429")).pixmap(15, 15));
    titleRow->addWidget(warnIcon, 0, Qt::AlignVCenter);

    m_titleLabel = new QLabel(QString("Permission Required — %1").arg(action), this);
    m_titleLabel->setObjectName("ApprovalTitle");
    m_titleLabel->setWordWrap(true);
    titleRow->addWidget(m_titleLabel, 1);

    layout->addLayout(titleRow);

    if (!details.trimmed().isEmpty()) {
        m_detailsEdit = new QTextEdit(this);
        m_detailsEdit->setObjectName("ApprovalDiff");
        m_detailsEdit->setReadOnly(true);
        m_detailsEdit->setHtml(formatDetailsAsHtml(details));
        m_detailsEdit->setMinimumHeight(56);
        m_detailsEdit->setMaximumHeight(230);
        layout->addWidget(m_detailsEdit);
    }

    auto* btnRow = new QHBoxLayout();
    btnRow->setContentsMargins(0, 0, 0, 0);
    btnRow->setSpacing(8);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    btnRow->addWidget(m_statusLabel);

    btnRow->addStretch();

    m_rejectButton = new QPushButton("Reject", this);
    m_rejectButton->setObjectName("RejectButton");
    m_rejectButton->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_rejectButton);

    m_allowButton = new QPushButton("Allow", this);
    m_allowButton->setObjectName("AllowButton");
    m_allowButton->setIcon(Icons::check(QColor("#ffffff")));
    m_allowButton->setIconSize(QSize(13, 13));
    m_allowButton->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_allowButton);

    layout->addLayout(btnRow);

    connect(m_allowButton, &QPushButton::clicked, this, [this]() {
        if (m_resolved) return;
        m_resolved = true;
        finishWithStatus("✓ Allowed", "#3fb950");
        emit resolved(m_requestId, true);
    });

    connect(m_rejectButton, &QPushButton::clicked, this, [this]() {
        if (m_resolved) return;
        m_resolved = true;
        finishWithStatus("✕ Rejected", "#ff7b72");
        emit resolved(m_requestId, false);
    });
}

void ApprovalCard::markCancelled() {
    if (m_resolved) return;
    m_resolved = true;
    finishWithStatus("■ Cancelled", "#8b949e");
}

void ApprovalCard::finishWithStatus(const QString& statusText, const QString& colorHex) {
    m_allowButton->setVisible(false);
    m_rejectButton->setVisible(false);
    m_allowButton->setEnabled(false);
    m_rejectButton->setEnabled(false);

    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(QString("color: %1; font-weight: 700;").arg(colorHex));
    m_statusLabel->setVisible(true);
}

QString ApprovalCard::formatDetailsAsHtml(const QString& details) {
    enum class Section { Neutral, Remove, Add };
    Section sec = Section::Neutral;

    QString html = "<pre style='margin:0; font-family:monospace; white-space:pre-wrap;'>";
    const QStringList lines = details.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("--- Remove:")) {
            sec = Section::Remove;
            html += QString("<div style='color:#ff7b72; font-weight:bold;'>%1</div>").arg(line.toHtmlEscaped());
            continue;
        }
        if (line.startsWith("+++ Add:")) {
            sec = Section::Add;
            html += QString("<div style='color:#7ee787; font-weight:bold;'>%1</div>").arg(line.toHtmlEscaped());
            continue;
        }

        if (sec == Section::Remove || (sec == Section::Neutral && line.startsWith('-'))) {
            html += QString("<div style='background-color:#35191d; color:#ffa198;'>%1</div>")
            .arg(line.toHtmlEscaped());
        } else if (sec == Section::Add || (sec == Section::Neutral && line.startsWith('+'))) {
            html += QString("<div style='background-color:#153121; color:#7ee787;'>%1</div>")
            .arg(line.toHtmlEscaped());
        } else {
            html += QString("<div style='color:#c9d1d9;'>%1</div>").arg(line.toHtmlEscaped());
        }
    }
    html += "</pre>";
    return html;
}
