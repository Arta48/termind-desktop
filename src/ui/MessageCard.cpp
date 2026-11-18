#include "MessageCard.h"
#include "Icons.h"

#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QStyle>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtMath>

#ifdef HAVE_MD4C
#include <md4c-html.h>
#endif

namespace {

    class AutoHeightBrowser final : public QTextBrowser {
    public:
        explicit AutoHeightBrowser(QWidget* parent = nullptr) : QTextBrowser(parent) {
            setReadOnly(true);
            setOpenExternalLinks(true);
            setFrameShape(QFrame::NoFrame);
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            document()->setDocumentMargin(1.0);
            document()->setDefaultStyleSheet(
                "body { color: #e6edf3; line-height: 1.45; }"
                "a { color: #58a6ff; text-decoration: none; }"
                "code { background-color: #212836; color: #79c0ff; font-family: monospace; }"
                "blockquote { color: #8b949e; margin-left: 8px; padding-left: 10px; "
                "             border-left: 3px solid #30363d; }"
                "table { border-collapse: collapse; margin: 6px 0; }"
                "th, td { border: 1px solid #2b323c; padding: 5px 9px; }"
                "th { background-color: #1c222e; font-weight: bold; }"
                "h1, h2, h3, h4 { color: #f0f6fc; margin-top: 10px; margin-bottom: 4px; }"
                "p { margin-top: 4px; margin-bottom: 6px; }"
                "ul, ol { margin-top: 4px; margin-bottom: 6px; }"
            );

            connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                    this, [this](const QSizeF&) { syncHeight(); });
        }

        void syncHeight() {
            if (m_syncingHeight) return;
            m_syncingHeight = true;
            const int vw = viewport()->width();
            if (vw > 0) {
                document()->setTextWidth(vw);
            }
            const int docHeight = qCeil(document()->documentLayout()->documentSize().height());
            const int margins = frameWidth() * 2 + contentsMargins().top() + contentsMargins().bottom();
            const int targetHeight = qMax(22, docHeight + margins + 4);
            if (height() != targetHeight) {
                setFixedHeight(targetHeight);
                updateGeometry();
            }
            m_syncingHeight = false;
        }

    protected:
        void resizeEvent(QResizeEvent* event) override {
            QTextBrowser::resizeEvent(event);
            syncHeight();
        }

        void wheelEvent(QWheelEvent* event) override {
            event->ignore();
        }

    private:
        bool m_syncingHeight = false;
    };

} // namespace

MessageCard::MessageCard(Role role, const QString& initialText, QWidget* parent)
: QFrame(parent), m_role(role) {
    setObjectName("MessageCard");
    setProperty("role", roleToProperty(role));
    setFrameShape(QFrame::StyledPanel);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(35);
    connect(m_renderTimer, &QTimer::timeout, this, &MessageCard::renderBody);

    // Compact single-row pill layout for Notices
    if (role == Role::Notice) {
        const bool isMuted = initialText.startsWith("YOLO auto-approved:");
        setProperty("muted", isMuted);

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(12, 7, 12, 7);
        row->setSpacing(8);

        auto* iconLbl = new QLabel(this);
        const QIcon ic = isMuted ? Icons::bolt(QColor("#f0883e")) : Icons::info(QColor("#d4b16a"));
        iconLbl->setPixmap(ic.pixmap(14, 14));
        row->addWidget(iconLbl, 0, Qt::AlignTop);

        m_noticeLabel = new QLabel(initialText, this);
        m_noticeLabel->setObjectName("NoticeText");
        m_noticeLabel->setWordWrap(true);
        m_noticeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(m_noticeLabel, 1);

        m_rawText = initialText;
        return;
    }

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 11, 14, 12);
    rootLayout->setSpacing(7);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(7);

    if (role == Role::Assistant) {
        auto* iconLbl = new QLabel(this);
        iconLbl->setPixmap(Icons::sparkle(QColor("#58a6ff")).pixmap(14, 14));
        headerLayout->addWidget(iconLbl);
    } else if (role == Role::Error) {
        auto* iconLbl = new QLabel(this);
        iconLbl->setPixmap(Icons::warning(QColor("#ff7b72")).pixmap(14, 14));
        headerLayout->addWidget(iconLbl);
    }

    m_roleLabel = new QLabel(roleTitle(role), this);
    m_roleLabel->setObjectName("RoleLabel");
    headerLayout->addWidget(m_roleLabel);

    headerLayout->addStretch();

    if (role == Role::Assistant) {
        m_reasoningToggle = new QToolButton(this);
        m_reasoningToggle->setObjectName("ReasoningToggle");
        m_reasoningToggle->setCheckable(true);
        m_reasoningToggle->setChecked(false);
        m_reasoningToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_reasoningToggle->setIcon(Icons::chevronRight(QColor("#8b949e")));
        m_reasoningToggle->setIconSize(QSize(12, 12));
        m_reasoningToggle->setText("Reasoning");
        m_reasoningToggle->setCursor(Qt::PointingHandCursor);
        m_reasoningToggle->setVisible(false);
        headerLayout->addWidget(m_reasoningToggle);

        m_copyButton = new QPushButton("Copy", this);
        m_copyButton->setObjectName("CopyButton");
        m_copyButton->setIcon(Icons::copy(QColor("#8b949e")));
        m_copyButton->setIconSize(QSize(13, 13));
        m_copyButton->setCursor(Qt::PointingHandCursor);
        headerLayout->addWidget(m_copyButton);

        connect(m_copyButton, &QPushButton::clicked, this, [this]() {
            if (auto* cb = QGuiApplication::clipboard()) {
                cb->setText(m_rawText);
                m_copyButton->setIcon(Icons::check(QColor("#3fb950")));
                m_copyButton->setText("Copied");
                QTimer::singleShot(1400, this, [this]() {
                    if (m_copyButton) {
                        m_copyButton->setIcon(Icons::copy(QColor("#8b949e")));
                        m_copyButton->setText("Copy");
                    }
                });
            }
        });
    }

    rootLayout->addLayout(headerLayout);

    if (role == Role::Assistant) {
        auto* reasoning = new AutoHeightBrowser(this);
        reasoning->setObjectName("ReasoningBox");
        reasoning->setVisible(false);
        m_reasoningBox = reasoning;
        rootLayout->addWidget(m_reasoningBox);

        connect(m_reasoningToggle, &QToolButton::toggled, this, [this](bool checked) {
            m_reasoningBox->setVisible(checked);
            m_reasoningToggle->setIcon(checked ? Icons::chevronDown(QColor("#e6edf3"))
            : Icons::chevronRight(QColor("#8b949e")));
            m_reasoningToggle->setText(checked ? "Hide Reasoning" : "Reasoning");
            if (checked) {
                static_cast<AutoHeightBrowser*>(m_reasoningBox)->syncHeight();
            }
        });
    }

    auto* body = new AutoHeightBrowser(this);
    body->setObjectName("MessageBody");
    m_bodyBrowser = body;
    rootLayout->addWidget(m_bodyBrowser);

    if (!initialText.isEmpty()) {
        setText(initialText);
    } else {
        m_bodyBrowser->setVisible(false);
    }
}

void MessageCard::setText(const QString& text) {
    m_rawText = text;
    m_renderTimer->stop();
    if (m_noticeLabel) {
        m_noticeLabel->setText(text);
        return;
    }
    renderBody();
}

void MessageCard::appendTextDelta(const QString& delta) {
    if (delta.isEmpty()) return;
    m_rawText += delta;
    if (m_noticeLabel) {
        m_noticeLabel->setText(m_rawText);
        return;
    }
    if (!m_bodyBrowser->isVisible()) {
        m_bodyBrowser->setVisible(true);
    }
    if (!m_renderTimer->isActive()) {
        m_renderTimer->start();
    }
}

void MessageCard::appendThoughtDelta(const QString& delta) {
    if (delta.isEmpty() || !m_reasoningBox || !m_reasoningToggle) return;
    m_rawThoughts += delta;
    m_reasoningToggle->setVisible(true);
    renderThoughts();
}

void MessageCard::finishStreaming() {
    if (m_renderTimer->isActive()) {
        m_renderTimer->stop();
        renderBody();
    }
}

void MessageCard::renderBody() {
    if (!m_bodyBrowser) return;
    if (m_rawText.isEmpty()) {
        m_bodyBrowser->setVisible(false);
        return;
    }
    m_bodyBrowser->setVisible(true);
    if (m_role == Role::Assistant) {
        #ifdef HAVE_MD4C
        m_bodyBrowser->setHtml(markdownToHtml(m_rawText));
        #else
        m_bodyBrowser->document()->setMarkdown(m_rawText);
        #endif
    } else {
        m_bodyBrowser->setPlainText(m_rawText);
    }
    static_cast<AutoHeightBrowser*>(m_bodyBrowser)->syncHeight();
}

void MessageCard::renderThoughts() {
    if (!m_reasoningBox) return;
    m_reasoningBox->setPlainText(m_rawThoughts);
    if (m_reasoningBox->isVisible()) {
        static_cast<AutoHeightBrowser*>(m_reasoningBox)->syncHeight();
    }
}

QString MessageCard::markdownToHtml(const QString& markdown) {
    #ifdef HAVE_MD4C
    const QByteArray utf8 = markdown.toUtf8();
    QByteArray outHtml;
    outHtml.reserve(utf8.size() * 2);

    auto callback = [](const MD_CHAR* text, MD_SIZE size, void* userdata) {
        auto* buf = static_cast<QByteArray*>(userdata);
        buf->append(text, static_cast<qsizetype>(size));
    };

    const int rc = md_html(
        utf8.constData(),
                           static_cast<MD_SIZE>(utf8.size()),
                           callback,
                           &outHtml,
                           MD_DIALECT_GITHUB | MD_FLAG_NOHTML,
                           MD_HTML_FLAG_SKIP_UTF8_BOM
    );
    if (rc == 0) {
        const QString html = QString::fromUtf8(outHtml);
        // Convert <pre><code>...</code></pre> into full-width dark code boxes without inline <code> pill highlight
        static const QRegularExpression codeBlockRe(
            R"re(<pre><code(?:\s+class="language-([^"]*)")?>([\s\S]*?)</code></pre>)re"
        );
        QString result;
        qsizetype lastPos = 0;
        auto it = codeBlockRe.globalMatch(html);
        while (it.hasNext()) {
            const auto m = it.next();
            result += html.mid(lastPos, m.capturedStart() - lastPos);
            const QString lang = m.captured(1).trimmed();
            QString code = m.captured(2);
            if (code.endsWith('\n')) {
                code.chop(1);
            }

            result += "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"0\" "
                      "style=\"background-color:#0b0f17; border:1px solid #262e3a; margin:8px 0;\">";
            if (!lang.isEmpty()) {
                result += QString("<tr><td style=\"background-color:#161b24; color:#7d8590; "
                                  "font-size:11px; font-family:monospace; padding:4px 10px; "
                                  "border:none; border-bottom:1px solid #262e3a;\">%1</td></tr>")
                              .arg(lang.toHtmlEscaped());
            }
            result += QString("<tr><td style=\"border:none; padding:10px 12px;\">"
                              "<pre style=\"margin:0; background-color:transparent; "
                              "color:#e6edf3; font-family:monospace; white-space:pre-wrap;\">%1</pre>"
                              "</td></tr></table>")
                          .arg(code);

                          lastPos = m.capturedEnd();
        }
        result += html.mid(lastPos);
        return result;
    }
    #endif
    return QString("<pre>%1</pre>").arg(markdown.toHtmlEscaped());
}

QString MessageCard::roleToProperty(Role role) {
    switch (role) {
        case Role::User: return "user";
        case Role::Assistant: return "assistant";
        case Role::Notice: return "notice";
        case Role::Error: return "error";
    }
    return "assistant";
}

QString MessageCard::roleTitle(Role role) {
    switch (role) {
        case Role::User: return "You";
        case Role::Assistant: return "Termind";
        case Role::Notice: return "Notice";
        case Role::Error: return "Error";
    }
    return "Termind";
}
