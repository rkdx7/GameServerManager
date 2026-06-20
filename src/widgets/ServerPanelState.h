#pragma once
#include <QObject>
#include <QSettings>
#include <QWidget>
#include <QPushButton>
#include <Qt>

// App-wide collapsed/expanded state of the "SERVEURS" instance panel that every
// game page (MinecraftPage, CS2Page, GenericGamePage, …) shows on its left to
// switch between the servers configured for that game.
//
// The user can fold this panel toward the left with a small toggle button. The
// choice is remembered for the WHOLE application: it is persisted in QSettings
// and broadcast live through a process-wide singleton, so collapsing the panel
// on one game page instantly collapses it on every other page too, and the
// state survives across restarts.
class ServerPanelState : public QObject {
    Q_OBJECT
public:
    static ServerPanelState *instance() {
        static ServerPanelState s;
        return &s;
    }

    bool collapsed() const { return m_collapsed; }

    void setCollapsed(bool collapsed) {
        if (m_collapsed == collapsed)
            return;
        m_collapsed = collapsed;
        QSettings("GameServerManager", "App")
            .setValue("ui/serverPanelCollapsed", collapsed);
        emit collapsedChanged(collapsed);
    }

    void toggle() { setCollapsed(!m_collapsed); }

signals:
    void collapsedChanged(bool collapsed);

private:
    ServerPanelState()
        : m_collapsed(QSettings("GameServerManager", "App")
              .value("ui/serverPanelCollapsed", false).toBool()) {}

    bool m_collapsed;
};

namespace ServerPanel {

// Builds the small collapse/expand toggle button for a game page's "SERVEURS"
// side panel and wires it to the shared, app-wide ServerPanelState.
//
//   panel         : the fixed-width side panel widget
//   title         : the "SERVEURS" header label (hidden while collapsed)
//   content       : widget holding the instance list + add/delete buttons
//                   (hidden while collapsed)
//   expandedWidth : the panel width when expanded (e.g. 185)
//
// Returns the toggle button so the caller can place it in its header row. The
// button reflects — and stays in sync with — the global collapsed state.
inline QPushButton *makeCollapseToggle(QWidget *panel, QWidget *title,
                                       QWidget *content, int expandedWidth)
{
    // Wide enough for the toggle button to sit inside the panel's 10px side
    // margins (10 + 22 + 10) without being clipped against the right border.
    const int collapsedWidth = 44;

    auto *btn = new QPushButton(panel);
    btn->setFixedSize(22, 22);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #94a3b8;
            border: none;
            font-size: 15px;
            font-weight: 700;
        }
        QPushButton:hover {
            color: #4f46e5;
        }
    )");

    auto apply = [panel, title, content, btn, expandedWidth, collapsedWidth](bool collapsed) {
        panel->setFixedWidth(collapsed ? collapsedWidth : expandedWidth);
        if (title)   title->setVisible(!collapsed);
        if (content) content->setVisible(!collapsed);
        btn->setText(collapsed ? "»" : "«");
        btn->setToolTip(collapsed ? "Afficher les serveurs"
                                  : "Masquer les serveurs");
    };

    QObject::connect(btn, &QPushButton::clicked, btn,
                     []() { ServerPanelState::instance()->toggle(); });
    QObject::connect(ServerPanelState::instance(), &ServerPanelState::collapsedChanged,
                     btn, [apply](bool c) { apply(c); });

    apply(ServerPanelState::instance()->collapsed());
    return btn;
}

} // namespace ServerPanel
