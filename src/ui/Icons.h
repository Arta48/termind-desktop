#pragma once

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRectF>

namespace Icons {

    template <typename DrawFn>
    inline QIcon makeIcon(const QColor& color, DrawFn&& drawFn, qreal strokeWidth = 1.6) {
        QPixmap pix(32, 32);
        pix.setDevicePixelRatio(2.0);
        pix.fill(Qt::transparent);

        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        drawFn(p);
        return QIcon(pix);
    }

    inline QIcon folder(const QColor& c = QColor("#c9d1d9")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath path;
            path.moveTo(2.0, 4.5);
            path.lineTo(6.2, 4.5);
            path.lineTo(7.8, 6.2);
            path.lineTo(14.0, 6.2);
            path.lineTo(14.0, 12.5);
            path.lineTo(2.0, 12.5);
            path.closeSubpath();
            p.drawPath(path);
        });
    }

    inline QIcon bolt(const QColor& c = QColor("#c9d1d9")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath path;
            path.moveTo(9.0, 1.8);
            path.lineTo(3.8, 8.8);
            path.lineTo(8.0, 8.8);
            path.lineTo(7.0, 14.2);
            path.lineTo(12.2, 7.2);
            path.lineTo(8.0, 7.2);
            path.closeSubpath();
            p.drawPath(path);
        });
    }

    inline QIcon layers(const QColor& c = QColor("#c9d1d9")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath top;
            top.moveTo(8.0, 2.2);
            top.lineTo(13.8, 5.2);
            top.lineTo(8.0, 8.2);
            top.lineTo(2.2, 5.2);
            top.closeSubpath();
            p.drawPath(top);

            QPainterPath mid;
            mid.moveTo(2.2, 8.2);
            mid.lineTo(8.0, 11.2);
            mid.lineTo(13.8, 8.2);
            p.drawPath(mid);

            QPainterPath bot;
            bot.moveTo(2.2, 11.0);
            bot.lineTo(8.0, 14.0);
            bot.lineTo(13.8, 11.0);
            p.drawPath(bot);
        });
    }

    inline QIcon refresh(const QColor& c = QColor("#c9d1d9")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawArc(QRectF(2.8, 2.8, 10.4, 10.4), 30 * 16, 275 * 16);
            p.drawLine(QPointF(11.0, 3.0), QPointF(13.5, 3.5));
            p.drawLine(QPointF(13.5, 3.5), QPointF(13.0, 6.2));
        });
    }

    inline QIcon settings(const QColor& c = QColor("#c9d1d9")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawEllipse(QPointF(8.0, 8.0), 2.3, 2.3);
            p.drawEllipse(QPointF(8.0, 8.0), 5.2, 5.2);
            for (int i = 0; i < 4; ++i) {
                p.save();
                p.translate(8.0, 8.0);
                p.rotate(i * 45.0);
                p.drawLine(QPointF(0.0, -6.7), QPointF(0.0, -5.2));
                p.drawLine(QPointF(0.0, 5.2), QPointF(0.0, 6.7));
                p.restore();
            }
        });
    }

    inline QIcon paperclip(const QColor& c = QColor("#8b949e")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath path;
            path.moveTo(12.2, 6.8);
            path.lineTo(6.8, 12.2);
            path.arcTo(QRectF(3.3, 8.7, 4.4, 4.4), 225, -180);
            path.lineTo(11.0, 4.5);
            path.arcTo(QRectF(8.2, 1.8, 3.6, 3.6), 45, -180);
            path.lineTo(5.6, 10.0);
            p.drawPath(path);
        });
    }

    inline QIcon send(const QColor& c = QColor("#ffffff")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawLine(QPointF(8.0, 13.0), QPointF(8.0, 3.2));
            p.drawLine(QPointF(3.8, 7.2), QPointF(8.0, 3.0));
            p.drawLine(QPointF(12.2, 7.2), QPointF(8.0, 3.0));
        }, 1.9);
    }

    inline QIcon stop(const QColor& c = QColor("#ffffff")) {
        return makeIcon(c, [&c](QPainter& p) {
            p.setBrush(c);
            p.drawRoundedRect(QRectF(4.2, 4.2, 7.6, 7.6), 1.5, 1.5);
        });
    }

    inline QIcon copy(const QColor& c = QColor("#8b949e")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawRoundedRect(QRectF(5.5, 5.5, 8.0, 8.0), 1.5, 1.5);
            QPainterPath back;
            back.moveTo(3.8, 10.2);
            back.lineTo(2.8, 10.2);
            back.lineTo(2.8, 2.8);
            back.lineTo(10.2, 2.8);
            back.lineTo(10.2, 3.8);
            p.drawPath(back);
        }, 1.4);
    }

    inline QIcon terminal(const QColor& c = QColor("#79c0ff")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawRoundedRect(QRectF(2.0, 2.8, 12.0, 10.4), 2.0, 2.0);
            p.drawLine(QPointF(4.5, 6.2), QPointF(6.8, 8.0));
            p.drawLine(QPointF(4.5, 9.8), QPointF(6.8, 8.0));
            p.drawLine(QPointF(8.2, 9.8), QPointF(11.2, 9.8));
        }, 1.5);
    }

    inline QIcon chevronDown(const QColor& c = QColor("#8b949e")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawLine(QPointF(4.0, 6.0), QPointF(8.0, 10.0));
            p.drawLine(QPointF(12.0, 6.0), QPointF(8.0, 10.0));
        });
    }

    inline QIcon chevronRight(const QColor& c = QColor("#8b949e")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawLine(QPointF(6.0, 4.0), QPointF(10.0, 8.0));
            p.drawLine(QPointF(6.0, 12.0), QPointF(10.0, 8.0));
        });
    }

    inline QIcon warning(const QColor& c = QColor("#f0b429")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath tri;
            tri.moveTo(8.0, 2.2);
            tri.lineTo(14.0, 13.2);
            tri.lineTo(2.0, 13.2);
            tri.closeSubpath();
            p.drawPath(tri);
            p.drawLine(QPointF(8.0, 6.2), QPointF(8.0, 9.5));
            p.drawPoint(QPointF(8.0, 11.4));
        });
    }

    inline QIcon check(const QColor& c = QColor("#3fb950")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawLine(QPointF(3.2, 8.5), QPointF(6.5, 11.8));
            p.drawLine(QPointF(6.5, 11.8), QPointF(13.0, 4.6));
        }, 1.8);
    }

    inline QIcon info(const QColor& c = QColor("#8b949e")) {
        return makeIcon(c, [](QPainter& p) {
            p.drawEllipse(QPointF(8.0, 8.0), 5.8, 5.8);
            p.drawLine(QPointF(8.0, 7.2), QPointF(8.0, 11.2));
            p.drawPoint(QPointF(8.0, 5.0));
        });
    }

    inline QIcon sparkle(const QColor& c = QColor("#58a6ff")) {
        return makeIcon(c, [](QPainter& p) {
            QPainterPath star;
            star.moveTo(8.0, 1.8);
            star.quadTo(8.2, 7.8, 14.2, 8.0);
            star.quadTo(8.2, 8.2, 8.0, 14.2);
            star.quadTo(7.8, 8.2, 1.8, 8.0);
            star.quadTo(7.8, 7.8, 8.0, 1.8);
            p.drawPath(star);
        });
    }

} // namespace Icons
