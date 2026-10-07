#include <QAbstractButton>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStylePlugin>
#include <QWidget>

// Use Qt's geometry and accessibility conventions; all glass painting is ours.
class SynterraGlass final : public QProxyStyle {
    void glass(const QStyleOption *o, QPainter *p, bool edit = false) const {
        const bool dark = o->palette.color(QPalette::Window).lightness() < 128;
        const bool enabled = o->state.testFlag(State_Enabled);
        const bool down = o->state.testFlag(State_Sunken) || o->state.testFlag(State_On);
        const bool highlighted = enabled && (o->state.testFlag(State_MouseOver) || o->state.testFlag(State_HasFocus));
        const QRectF r = QRectF(o->rect).adjusted(.5, .5, -.5, -.5);
        QColor top = dark ? QColor("#52677c") : QColor("#fafdff");
        QColor bottom = dark ? QColor("#24394f") : QColor("#bfd9ef");
        if (down) { top = dark ? QColor("#234965") : QColor("#86b7d9"); bottom = dark ? QColor("#18364e") : QColor("#c3dfee"); }
        if (edit) { top = bottom = o->palette.color(QPalette::Base); }
        QLinearGradient gradient(r.topLeft(), r.bottomLeft());
        gradient.setColorAt(0, top); gradient.setColorAt(.48, top.darker(105));
        gradient.setColorAt(.52, bottom.lighter(108)); gradient.setColorAt(1, bottom);
        p->save(); p->setRenderHint(QPainter::Antialiasing);
        if (!enabled) p->setOpacity(.52);
        p->setBrush(gradient); p->setPen(highlighted ? QColor("#78c4f4") : (dark ? QColor("#607e98") : QColor("#91b2cd")));
        p->drawRoundedRect(r, 5, 5);
        p->setBrush(Qt::NoBrush); p->setPen(QColor(255, 255, 255, dark ? 35 : 150));
        p->drawRoundedRect(r.adjusted(1, 1, -1, -1), 4, 4); p->restore();
    }
public:
    SynterraGlass() : QProxyStyle(QStyleFactory::create("Fusion")) { setObjectName("SynterraGlass"); }
    void polish(QWidget *w) override {
        QProxyStyle::polish(w);
        if (qobject_cast<QAbstractButton *>(w)) w->setAttribute(Qt::WA_Hover);
    }
    void drawPrimitive(PrimitiveElement e, const QStyleOption *o, QPainter *p, const QWidget *w = nullptr) const override {
        switch (e) {
        case PE_PanelButtonCommand: case PE_PanelButtonTool: glass(o, p); return;
        case PE_PanelLineEdit: glass(o, p, true); return;
        case PE_FrameLineEdit: return; // The line-edit panel already draws its border.
        case PE_PanelMenu: case PE_PanelTipLabel: {
            const bool dark = o->palette.color(QPalette::Window).lightness() < 128;
            p->save(); p->fillRect(o->rect, o->palette.color(e == PE_PanelTipLabel ? QPalette::ToolTipBase : QPalette::Window));
            p->setPen(dark ? QColor("#607e98") : QColor("#91b2cd")); p->drawRect(o->rect.adjusted(0, 0, -1, -1)); p->restore(); return;
        }
        case PE_IndicatorCheckBox: {
            glass(o, p, !o->state.testFlag(State_On));
            p->save(); p->setRenderHint(QPainter::Antialiasing); p->setPen(QPen(o->palette.color(QPalette::Text), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            const QRectF r = o->rect;
            if (o->state.testFlag(State_NoChange)) p->drawLine(QPointF(r.left()+3, r.center().y()), QPointF(r.right()-3, r.center().y()));
            else if (o->state.testFlag(State_On)) { QPainterPath tick; tick.moveTo(r.left()+3, r.center().y()); tick.lineTo(r.left()+r.width()*.43, r.bottom()-3); tick.lineTo(r.right()-3, r.top()+3); p->drawPath(tick); }
            p->restore(); return;
        }
        case PE_IndicatorRadioButton: {
            p->save(); p->setRenderHint(QPainter::Antialiasing); p->setBrush(o->palette.color(QPalette::Base)); p->setPen(o->palette.color(QPalette::Highlight));
            p->drawEllipse(o->rect.adjusted(1, 1, -1, -1));
            if (o->state.testFlag(State_On)) { p->setBrush(o->palette.color(QPalette::Highlight)); p->drawEllipse(o->rect.adjusted(4, 4, -4, -4)); }
            p->restore(); return;
        }
        default: QProxyStyle::drawPrimitive(e, o, p, w);
        }
    }
    void drawControl(ControlElement e, const QStyleOption *o, QPainter *p, const QWidget *w = nullptr) const override {
        if (e == CE_PushButtonBevel || e == CE_TabBarTabShape || e == CE_HeaderSection) { glass(o, p); return; }
        if (e == CE_ToolBar) {
            const QColor base = o->palette.color(QPalette::Window);
            QLinearGradient gradient(o->rect.topLeft(), o->rect.bottomLeft()); gradient.setColorAt(0, base.lighter(110)); gradient.setColorAt(1, base);
            p->fillRect(o->rect, gradient); return;
        }
        QProxyStyle::drawControl(e, o, p, w);
    }
    int pixelMetric(PixelMetric m, const QStyleOption *o = nullptr, const QWidget *w = nullptr) const override {
        if (m == PM_ButtonMargin) return 10;
        if (m == PM_DefaultFrameWidth) return 2;
        return QProxyStyle::pixelMetric(m, o, w);
    }
};

class SynterraGlassPlugin final : public QStylePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "synterraglass.json")
public:
    QStyle *create(const QString &key) override { return key.compare("SynterraGlass", Qt::CaseInsensitive) == 0 ? new SynterraGlass : nullptr; }
};
#include "synterraglass.moc"
