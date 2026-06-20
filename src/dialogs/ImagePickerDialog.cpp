#include "ImagePickerDialog.h"
#include "DockerManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QStackedWidget>
#include <QFrame>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEvent>
#include <QMouseEvent>
#include <QLineEdit>

namespace {

const char *INPUT_STYLE = R"(
    QLineEdit, QComboBox {
        border: 2px solid #e2e8f0;
        border-radius: 8px;
        padding: 8px 12px;
        font-size: 13px;
        color: #1e293b;
        background: #f8fafc;
        min-height: 20px;
    }
    QLineEdit:focus, QComboBox:focus { border-color: #6366f1; background: #fff; }
    QComboBox::drop-down { border: none; width: 20px; }
    QComboBox QAbstractItemView {
        background: #fff; color: #1e293b;
        selection-background-color: #6366f1;
        selection-color: #fff;
        border: 1px solid #e2e8f0;
        border-radius: 4px; outline: none;
    }
)";

const char *RADIO_STYLE = R"(
    QRadioButton {
        font-size: 13px; font-weight: 600; color: #1e293b;
        padding: 10px 14px; border-radius: 8px;
        background: transparent;
    }
    QRadioButton:hover { background: #f1f5f9; }
    QRadioButton:checked { background: #eef2ff; color: #4f46e5; }
    QRadioButton::indicator { width: 16px; height: 16px; }
    QRadioButton::indicator:unchecked {
        border: 2px solid #cbd5e1; border-radius: 8px; background: white;
    }
    QRadioButton::indicator:checked {
        border: 2px solid #6366f1; border-radius: 8px; background: #6366f1;
    }
)";

} // namespace

ImagePickerDialog::ImagePickerDialog(DockerManager *docker,
                                      const QString  &currentImage,
                                      QWidget        *parent)
    : QDialog(parent), m_docker(docker), m_currentImage(currentImage)
{
    m_net = new QNetworkAccessManager(this);
    m_tagDebounce = new QTimer(this);
    m_tagDebounce->setSingleShot(true);
    m_tagDebounce->setInterval(400);
    connect(m_tagDebounce, &QTimer::timeout, this, &ImagePickerDialog::reloadHubTags);

    setWindowTitle("Source de l'image Docker");
    setFixedWidth(520);
    setModal(true);
    setStyleSheet("QDialog { background: #f8fafc; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Header ────────────────────────────────────────────────────────────
    auto *header = new QWidget(this);
    header->setStyleSheet("background: #1e1b4b;");
    auto *hlay = new QHBoxLayout(header);
    hlay->setContentsMargins(24, 20, 24, 20);
    auto *hIcon  = new QLabel("🐳", header);
    hIcon->setStyleSheet("font-size: 28px; background: transparent;");
    auto *hTitle = new QLabel("Source de l'image Docker", header);
    hTitle->setStyleSheet("font-size: 16px; font-weight: 700; color: white; background: transparent;");
    auto *hSub   = new QLabel("Choisissez d'où télécharger l'image de ce serveur", header);
    hSub->setStyleSheet("font-size: 12px; color: #a5b4fc; background: transparent;");
    auto *hTCol  = new QVBoxLayout;
    hTCol->setSpacing(2);
    hTCol->addWidget(hTitle);
    hTCol->addWidget(hSub);
    hlay->addWidget(hIcon);
    hlay->addSpacing(12);
    hlay->addLayout(hTCol, 1);
    root->addWidget(header);

    // ── Body ──────────────────────────────────────────────────────────────
    auto *body = new QWidget(this);
    auto *blay = new QVBoxLayout(body);
    blay->setContentsMargins(24, 20, 24, 16);
    blay->setSpacing(12);

    // Source radio buttons
    auto *sourceCard = new QFrame(body);
    sourceCard->setStyleSheet("QFrame { background: white; border-radius: 12px; }");
    auto *sclay = new QVBoxLayout(sourceCard);
    sclay->setContentsMargins(8, 8, 8, 8);
    sclay->setSpacing(2);

    m_sourceGroup = new QButtonGroup(this);
    auto *rbHub   = new QRadioButton("🌐  Docker Hub / Registry public", sourceCard);
    auto *rbLocal = new QRadioButton("💻  Image locale (déjà téléchargée)", sourceCard);
    auto *rbPriv  = new QRadioButton("🔒  Registry privé (avec authentification)", sourceCard);
    rbHub->setChecked(true);
    for (auto *rb : {rbHub, rbLocal, rbPriv}) rb->setStyleSheet(RADIO_STYLE);
    m_sourceGroup->addButton(rbHub,   0);
    m_sourceGroup->addButton(rbLocal, 1);
    m_sourceGroup->addButton(rbPriv,  2);
    sclay->addWidget(rbHub);
    sclay->addWidget(rbLocal);
    sclay->addWidget(rbPriv);
    blay->addWidget(sourceCard);

    // Source-specific stacked widget
    m_srcStack = new QStackedWidget(body);
    m_srcStack->addWidget(buildHubPage());
    m_srcStack->addWidget(buildLocalPage());
    m_srcStack->addWidget(buildPrivatePage());
    blay->addWidget(m_srcStack);

    // Preview label
    auto *sep = new QFrame(body);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: #e2e8f0; border: none;");
    sep->setFixedHeight(1);
    blay->addWidget(sep);

    m_previewLabel = new QLabel(body);
    m_previewLabel->setStyleSheet(
        "font-size: 12px; color: #64748b; background: #f1f5f9;"
        "border-radius: 8px; padding: 8px 12px;");
    m_previewLabel->setWordWrap(true);
    blay->addWidget(m_previewLabel);

    root->addWidget(body, 1);

    // ── Footer buttons ───────────────────────────────────────────────────
    auto *footer = new QWidget(this);
    footer->setStyleSheet("background: white; border-top: 1px solid #e2e8f0;");
    auto *flay = new QHBoxLayout(footer);
    flay->setContentsMargins(24, 14, 24, 14);

    auto *cancelBtn = new QPushButton("Annuler", footer);
    cancelBtn->setFixedHeight(38);
    cancelBtn->setStyleSheet(R"(
        QPushButton { background: #f1f5f9; color: #64748b; border: none;
                      border-radius: 8px; font-size: 13px; font-weight: 600; padding: 0 20px; }
        QPushButton:hover { background: #e2e8f0; }
    )");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_applyBtn = new QPushButton("✓  Appliquer", footer);
    m_applyBtn->setFixedHeight(38);
    m_applyBtn->setStyleSheet(R"(
        QPushButton { background: #6366f1; color: white; border: none;
                      border-radius: 8px; font-size: 13px; font-weight: 700; padding: 0 24px; }
        QPushButton:hover { background: #4f46e5; }
        QPushButton:disabled { background: #94a3b8; }
    )");
    m_applyBtn->setCursor(Qt::PointingHandCursor);
    connect(m_applyBtn, &QPushButton::clicked, this, &QDialog::accept);

    flay->addStretch();
    flay->addWidget(cancelBtn);
    flay->addSpacing(8);
    flay->addWidget(m_applyBtn);
    root->addWidget(footer);

    // Connections
    connect(m_sourceGroup, &QButtonGroup::idClicked,
            this, &ImagePickerDialog::onSourceChanged);

    updatePreview();
    reloadHubTags();   // pre-fill tags for the image we opened with
}

// ── Page builders ─────────────────────────────────────────────────────────────

QWidget *ImagePickerDialog::buildHubPage()
{
    auto *w    = new QWidget;
    auto *form = new QFormLayout(w);
    form->setContentsMargins(0, 8, 0, 4);
    form->setSpacing(10);

    auto mkLbl = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("font-size: 12px; font-weight: 600; color: #374151;");
        return l;
    };

    // Split the incoming "repository:tag" into its two parts.
    QString initialRepo = m_currentImage;
    QString initialTag;
    {
        int slash = m_currentImage.lastIndexOf('/');
        int colon = m_currentImage.lastIndexOf(':');
        if (colon > slash) {   // a ':' that is part of the tag, not a registry port
            initialRepo = m_currentImage.left(colon);
            initialTag  = m_currentImage.mid(colon + 1);
        }
    }

    m_hubImage = new QLineEdit(w);
    m_hubImage->setText(initialRepo);
    m_hubImage->setStyleSheet(INPUT_STYLE);
    m_hubImage->setPlaceholderText("ex: itzg/minecraft-server");
    connect(m_hubImage, &QLineEdit::textChanged, this, &ImagePickerDialog::updatePreview);
    // Reload the tag list (debounced) whenever the repository changes.
    connect(m_hubImage, &QLineEdit::textChanged,
            this, &ImagePickerDialog::scheduleHubTagReload);

    // Tag combo — auto-populated from the registry, but still editable so a
    // tag that is not listed (or a brand-new one) can be typed by hand.
    m_hubTag = new QComboBox(w);
    m_hubTag->setEditable(true);
    m_hubTag->setStyleSheet(INPUT_STYLE);
    if (!initialTag.isEmpty())
        m_hubTag->setCurrentText(initialTag);
    connect(m_hubTag, &QComboBox::currentTextChanged, this, &ImagePickerDialog::updatePreview);
    // A single click in the editable field should drop the version list down,
    // not just place the text cursor. Watch its line edit for mouse presses.
    if (m_hubTag->lineEdit())
        m_hubTag->lineEdit()->installEventFilter(this);

    auto *reloadBtn = new QPushButton("🔄", w);
    reloadBtn->setFixedSize(38, 38);
    reloadBtn->setCursor(Qt::PointingHandCursor);
    reloadBtn->setToolTip("Recharger les tags depuis le registry");
    reloadBtn->setStyleSheet(R"(
        QPushButton { background: #e0e7ff; color: #4f46e5; border: none;
                      border-radius: 8px; font-size: 14px; font-weight: 600; }
        QPushButton:hover { background: #c7d2fe; }
    )");
    connect(reloadBtn, &QPushButton::clicked, this, &ImagePickerDialog::reloadHubTags);

    auto *tagRow    = new QWidget(w);
    auto *tagRowLay = new QHBoxLayout(tagRow);
    tagRowLay->setContentsMargins(0, 0, 0, 0);
    tagRowLay->setSpacing(8);
    tagRowLay->addWidget(m_hubTag, 1);
    tagRowLay->addWidget(reloadBtn);

    m_hubTagStatus = new QLabel(w);
    m_hubTagStatus->setStyleSheet("font-size: 11px; color: #94a3b8;");
    m_hubTagStatus->setWordWrap(true);

    auto *note = new QLabel(
        "ℹ  Les tags sont chargés automatiquement depuis Docker Hub. "
        "Si la liste est vide, vous pouvez saisir un tag manuellement "
        "(par défaut <b>latest</b>).", w);
    note->setStyleSheet("font-size: 11px; color: #94a3b8;");
    note->setWordWrap(true);

    form->addRow(mkLbl("Image (repository)"), m_hubImage);
    form->addRow(mkLbl("Tag"),                tagRow);
    form->addRow("",                          m_hubTagStatus);
    form->addRow("",                          note);
    return w;
}

QWidget *ImagePickerDialog::buildLocalPage()
{
    auto *w   = new QWidget;
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 8, 0, 4);
    lay->setSpacing(8);

    auto mkLbl = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("font-size: 12px; font-weight: 600; color: #374151;");
        return l;
    };

    m_localStatus = new QLabel("Cliquez sur «Charger» pour lister les images locales.", w);
    m_localStatus->setStyleSheet("font-size: 12px; color: #94a3b8;");

    m_localCombo = new QComboBox(w);
    m_localCombo->setStyleSheet(INPUT_STYLE);
    m_localCombo->setEnabled(false);
    connect(m_localCombo, &QComboBox::currentTextChanged,
            this, &ImagePickerDialog::updatePreview);

    auto *loadBtn = new QPushButton("🔄  Charger les images locales", w);
    loadBtn->setFixedHeight(36);
    loadBtn->setCursor(Qt::PointingHandCursor);
    loadBtn->setStyleSheet(R"(
        QPushButton { background: #e0e7ff; color: #4f46e5; border: none;
                      border-radius: 8px; font-size: 12px; font-weight: 600; }
        QPushButton:hover { background: #c7d2fe; }
    )");
    connect(loadBtn, &QPushButton::clicked, this, &ImagePickerDialog::loadLocalImages);

    lay->addWidget(mkLbl("Images disponibles localement"));
    lay->addWidget(m_localStatus);
    lay->addWidget(m_localCombo);
    lay->addWidget(loadBtn);
    return w;
}

QWidget *ImagePickerDialog::buildPrivatePage()
{
    auto *w    = new QWidget;
    auto *form = new QFormLayout(w);
    form->setContentsMargins(0, 8, 0, 4);
    form->setSpacing(10);

    auto mkLbl = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("font-size: 12px; font-weight: 600; color: #374151;");
        return l;
    };

    m_privRegistry = new QLineEdit(w);
    m_privRegistry->setPlaceholderText("ex: registry.monentreprise.com:5000");
    m_privRegistry->setStyleSheet(INPUT_STYLE);
    connect(m_privRegistry, &QLineEdit::textChanged, this, &ImagePickerDialog::updatePreview);

    m_privImage = new QLineEdit(w);
    m_privImage->setPlaceholderText("ex: monprojet/monimage:latest");
    m_privImage->setStyleSheet(INPUT_STYLE);
    connect(m_privImage, &QLineEdit::textChanged, this, &ImagePickerDialog::updatePreview);

    m_privUser = new QLineEdit(w);
    m_privUser->setPlaceholderText("Nom d'utilisateur");
    m_privUser->setStyleSheet(INPUT_STYLE);

    m_privPass = new QLineEdit(w);
    m_privPass->setEchoMode(QLineEdit::Password);
    m_privPass->setPlaceholderText("Mot de passe / Token");
    m_privPass->setStyleSheet(INPUT_STYLE);

    auto *note = new QLabel(
        "🔐  Un <b>docker login</b> sera exécuté automatiquement avant le pull "
        "avec les identifiants fournis.", w);
    note->setStyleSheet("font-size: 11px; color: #94a3b8;");
    note->setWordWrap(true);

    form->addRow(mkLbl("URL du registry"), m_privRegistry);
    form->addRow(mkLbl("Image / Tag"),     m_privImage);
    form->addRow(mkLbl("Utilisateur"),     m_privUser);
    form->addRow(mkLbl("Mot de passe"),    m_privPass);
    form->addRow("", note);
    return w;
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void ImagePickerDialog::onSourceChanged(int id)
{
    m_srcStack->setCurrentIndex(id);
    if (id == 0 && m_hubTag && m_hubTag->count() == 0)
        reloadHubTags();
    updatePreview();
}

void ImagePickerDialog::loadLocalImages()
{
    m_localStatus->setText("⏳ Chargement des images locales…");
    m_localCombo->setEnabled(false);
    m_localCombo->clear();

    DockerManager *docker = m_docker;
    auto *thread = QThread::create([this, docker]() {
        QStringList imgs = docker->listLocalImages();
        QMetaObject::invokeMethod(this, [this, imgs]() {
            if (imgs.isEmpty()) {
                m_localStatus->setText("⚠  Aucune image locale trouvée.");
            } else {
                m_localCombo->addItems(imgs);
                m_localCombo->setEnabled(true);
                m_localStatus->setText(
                    QString("%1 image(s) disponible(s) localement.").arg(imgs.size()));
                m_localLoaded = true;
                updatePreview();
            }
        }, Qt::QueuedConnection);
    });
    thread->start();
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
}

QString ImagePickerDialog::hubRepository() const
{
    QString repo = m_hubImage ? m_hubImage->text().trimmed() : QString();
    if (repo.isEmpty()) return {};

    // Docker Hub only — strip an explicit docker.io host if the user typed one.
    if (repo.startsWith("docker.io/"))          repo = repo.mid(10);
    else if (repo.startsWith("registry-1.docker.io/")) repo = repo.mid(21);

    // Official images (no namespace) live under the "library/" namespace.
    if (!repo.contains('/')) repo = "library/" + repo;
    return repo;
}

void ImagePickerDialog::scheduleHubTagReload()
{
    // Only meaningful while the Hub source is active.
    if (m_sourceGroup && m_sourceGroup->checkedId() == 0)
        m_tagDebounce->start();
}

void ImagePickerDialog::reloadHubTags()
{
    if (!m_hubTag) return;

    const QString repo = hubRepository();
    if (repo.isEmpty()) {
        if (m_hubTagStatus) m_hubTagStatus->setText("");
        return;
    }

    // Cancel any in-flight request so late replies cannot clobber newer ones.
    if (m_tagReply) {
        m_tagReply->abort();
        m_tagReply->deleteLater();
        m_tagReply = nullptr;
    }

    if (m_hubTagStatus)
        m_hubTagStatus->setText("⏳ Chargement des tags depuis Docker Hub…");

    QUrl url(QString("https://hub.docker.com/v2/repositories/%1/tags").arg(repo));
    QUrlQuery q;
    q.addQueryItem("page_size", "100");
    q.addQueryItem("ordering", "last_updated");
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "GameServerManager");
    m_tagReply = m_net->get(req);

    connect(m_tagReply, &QNetworkReply::finished, this, [this, repo]() {
        QNetworkReply *reply = m_tagReply;
        m_tagReply = nullptr;
        if (!reply) return;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            if (m_hubTagStatus)
                m_hubTagStatus->setText("⚠  Impossible de charger les tags ("
                                        + reply->errorString() + ").");
            return;
        }

        const auto doc  = QJsonDocument::fromJson(reply->readAll());
        const auto arr  = doc.object().value("results").toArray();
        QStringList tags;
        for (const auto &v : arr) {
            const QString name = v.toObject().value("name").toString();
            if (!name.isEmpty()) tags << name;
        }

        // Preserve whatever the user currently has typed/selected.
        const QString keep = m_hubTag->currentText().trimmed();
        m_hubTag->blockSignals(true);
        m_hubTag->clear();
        m_hubTag->addItems(tags);
        if (!keep.isEmpty()) m_hubTag->setCurrentText(keep);
        else if (!tags.isEmpty()) m_hubTag->setCurrentIndex(0);
        m_hubTag->blockSignals(false);

        if (m_hubTagStatus) {
            m_hubTagStatus->setText(tags.isEmpty()
                ? "⚠  Aucun tag trouvé pour ce dépôt sur Docker Hub."
                : QString("✓  %1 tag(s) disponible(s).").arg(tags.size()));
        }
        updatePreview();
    });
}

bool ImagePickerDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (m_hubTag && m_hubTag->lineEdit() && obj == m_hubTag->lineEdit()
        && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            // Make sure the list is fresh, then drop it down.
            if (m_hubTag->count() == 0)
                reloadHubTags();
            m_hubTag->showPopup();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void ImagePickerDialog::updatePreview()
{
    QString img = selectedImage();
    if (img.isEmpty()) {
        m_previewLabel->setText("⚠  Aucune image sélectionnée.");
    } else if (requiresLogin()) {
        m_previewLabel->setText(
            QString("🔒  docker login %1  →  docker pull <b>%2</b>")
            .arg(loginRegistry(), img));
    } else {
        m_previewLabel->setText(
            QString("🐳  docker pull <b>%1</b>").arg(img));
    }
    m_applyBtn->setEnabled(!img.isEmpty());
}

// ── Accessors ─────────────────────────────────────────────────────────────────

QString ImagePickerDialog::selectedImage() const
{
    int src = m_sourceGroup->checkedId();
    if (src == 0) {
        QString repo = m_hubImage ? m_hubImage->text().trimmed() : m_currentImage;
        if (repo.isEmpty()) return {};
        QString tag = m_hubTag ? m_hubTag->currentText().trimmed() : QString();
        return tag.isEmpty() ? repo : repo + ":" + tag;
    }
    if (src == 1) return m_localCombo ? m_localCombo->currentText().trimmed() : QString();
    if (src == 2) {
        QString reg = m_privRegistry ? m_privRegistry->text().trimmed() : QString();
        QString img = m_privImage    ? m_privImage->text().trimmed()    : QString();
        if (reg.isEmpty() || img.isEmpty()) return {};
        return reg + "/" + img;
    }
    return {};
}

bool ImagePickerDialog::requiresLogin() const
{
    return m_sourceGroup && m_sourceGroup->checkedId() == 2
           && m_privUser && !m_privUser->text().trimmed().isEmpty();
}

QString ImagePickerDialog::loginRegistry() const
{
    return m_privRegistry ? m_privRegistry->text().trimmed() : QString();
}

QString ImagePickerDialog::loginUsername() const
{
    return m_privUser ? m_privUser->text().trimmed() : QString();
}

QString ImagePickerDialog::loginPassword() const
{
    return m_privPass ? m_privPass->text() : QString();
}
