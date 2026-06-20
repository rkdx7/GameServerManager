#pragma once
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QString>
#include <QPixmap>
#include <Qt>

// Builds a pretty gradient banner shown at the top of every game server page.
// Every game page (MinecraftPage, CS2Page, GenericGamePage, and any future game
// server) must display one of these so the pages stay visually consistent.
//
//   icon       : emoji shown on the left (e.g. "🎮"), used as a fallback when
//                imagePath is empty or the image fails to load
//   title      : large white headline (e.g. "Minecraft Server")
//   subtitle   : short description shown under the title
//   colorStart : top-left gradient color   (hex, e.g. "#16a34a")
//   colorEnd   : bottom-right gradient color (hex, e.g. "#15803d")
//   imagePath  : optional game artwork (e.g. ":/games/minecraft.png"); when it
//                loads it replaces the emoji icon, otherwise the emoji is shown
inline QFrame *buildGameBanner(const QString &icon,
                               const QString &title,
                               const QString &subtitle,
                               const QString &colorStart,
                               const QString &colorEnd,
                               const QString &imagePath = QString(),
                               QWidget *parent = nullptr)
{
    auto *banner = new QFrame(parent);
    banner->setAttribute(Qt::WA_StyledBackground, true);
    banner->setStyleSheet(QString(R"(
        QFrame {
            border-radius: 16px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 %1, stop:1 %2);
        }
    )").arg(colorStart, colorEnd));

    auto *row = new QHBoxLayout(banner);
    row->setContentsMargins(28, 22, 28, 22);
    row->setSpacing(20);

    QPixmap art;
    if (!imagePath.isEmpty())
        art.load(imagePath);

    if (!art.isNull()) {
        // Show the game artwork as a rounded square thumbnail.
        const int side = 64;
        QPixmap scaled = art.scaled(side, side, Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation);
        auto *artLbl = new QLabel(banner);
        artLbl->setFixedSize(side, side);
        artLbl->setPixmap(scaled);
        artLbl->setScaledContents(true);
        artLbl->setStyleSheet(
            "border-radius: 12px; background: rgba(255,255,255,0.12);");
        row->addWidget(artLbl, 0, Qt::AlignVCenter);
    } else {
        auto *iconLbl = new QLabel(icon, banner);
        iconLbl->setStyleSheet("font-size: 46px; background: transparent;");
        row->addWidget(iconLbl, 0, Qt::AlignVCenter);
    }

    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(4);

    auto *titleLbl = new QLabel(title, banner);
    titleLbl->setStyleSheet(
        "font-size: 24px; font-weight: 800; color: #ffffff; background: transparent;");

    auto *subLbl = new QLabel(subtitle, banner);
    subLbl->setStyleSheet(
        "font-size: 13px; color: rgba(255, 255, 255, 0.9); background: transparent;");
    subLbl->setWordWrap(true);

    textCol->addWidget(titleLbl);
    textCol->addWidget(subLbl);
    row->addLayout(textCol, 1);

    return banner;
}
