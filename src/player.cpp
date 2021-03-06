/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "player.h"

#include "playercontrols.h"
#include "playlistmodel.h"
#include "histogramwidget.h"
#include "videowidget.h"

#include <QMediaService>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QAudioProbe>
#include <QMediaMetaData>
#include <QtWidgets>
#include <QtGlobal>
#include <QKeyEvent>

#include "../../lsMisc/stdQt/inisettings.h"
#include "../../lsMisc/stdQt/stdQt.h"

#include "consts.h"
#include "slider_style.h"
#include "clickableSlider.h"

using namespace Consts;
using namespace AmbiesoftQt;

Player* Player::self_ = nullptr;
// static
bool Player::hook(void** pvv)
{
    QEvent* event = reinterpret_cast<QEvent*>(pvv[1]);
    switch(event->type())
    {
        case QEvent::KeyPress:
        {
            QKeyEvent* pKeyEvent = reinterpret_cast<QKeyEvent*>(event);
            qDebug() << pKeyEvent->key() << __FUNCTION__;
            if(pKeyEvent->key()==Qt::Key_Space)
            {
                qDebug() << "[space]" << __FUNCTION__;

                Q_ASSERT(Player::self_);
                Player::self_->togglePlay();

                bool* result = reinterpret_cast<bool*>(pvv[2]);
                *result = true;
                pKeyEvent->accept();
                return true;
            }
        }
        break;

        case QEvent::Wheel:
        {
            QWheelEvent *pWheelEvent = reinterpret_cast<QWheelEvent*>(event);
            Player::self_->onHookWheel(pWheelEvent);

            bool* result = reinterpret_cast<bool*>(pvv[2]);
            *result = true;
            pWheelEvent->accept();
            return true;
        }
        break;

        default:
        {}
    }
    return false;
}


Player::Player(QWidget *parent,
               IniSettings& settings)
    : ParentClass(parent),
      settings_(settings)
{
    self_ = this;
    QInternal::registerCallback(QInternal::EventNotifyCallback,hook);

    //! [create-objs]
    m_player = new QMediaPlayer(this);
    m_player->setAudioRole(QAudio::VideoRole);
    qInfo() << "Supported audio roles:";
    for (QAudio::Role role : m_player->supportedAudioRoles())
        qInfo() << "    " << role;
    // owned by PlaylistModel
    m_playlist = new QMediaPlaylist();
    m_player->setPlaylist(m_playlist);
    //! [create-objs]

    // this->setWindowIcon(QIcon(":/images/icon.png"));
    this->setWindowIcon(QIcon(":/images/icon.ico"));

    connect(m_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &Player::metaDataChanged);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &Player::playlistPositionChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &Player::statusChanged);
    connect(m_player, &QMediaPlayer::bufferStatusChanged, this, &Player::bufferingProgress);
    connect(m_player, &QMediaPlayer::videoAvailableChanged, this, &Player::videoAvailableChanged);
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &Player::displayErrorMessage);
    connect(m_player, &QMediaPlayer::stateChanged, this, &Player::stateChanged);

    //! [2]
    m_videoWidget = new VideoWidget(this, this, m_player);
    m_player->setVideoOutput(m_videoWidget);

    m_playlistModel = new PlaylistModel(this);
    m_playlistModel->setPlaylist(m_playlist);
    //! [2]

    m_playlistView = new QListView(this);
    m_playlistView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_playlistView, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(OnPlayListContextMenu(QPoint)));
    m_playlistView->setModel(m_playlistModel);
    m_playlistView->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));

    connect(m_playlistView, &QAbstractItemView::activated, this, &Player::jump);

    m_slider = new ClickableSlider(m_player, Qt::Horizontal, this);
    m_slider->setStyle(new SliderStyle(m_slider->style()));

    m_slider->setRange(0, static_cast<int>(m_player->duration() / 1000));

    m_labelDuration = new QLabel(this);
    connect(m_slider, &QSlider::sliderMoved, this, &Player::seek);


    m_labelHistogram = new QLabel(this);
    m_labelHistogram->setText("Histogram:");
    m_videoHistogram = new HistogramWidget(this);
    m_audioHistogram = new HistogramWidget(this);
    QHBoxLayout *histogramLayout = new QHBoxLayout;
    histogramLayout->addWidget(m_labelHistogram);
    histogramLayout->addWidget(m_videoHistogram, 1);
    histogramLayout->addWidget(m_audioHistogram, 2);

    m_videoProbe = new QVideoProbe(this);
    connect(m_videoProbe, &QVideoProbe::videoFrameProbed, m_videoHistogram, &HistogramWidget::processFrame);
    m_videoProbe->setSource(m_player);

    m_audioProbe = new QAudioProbe(this);
    connect(m_audioProbe, &QAudioProbe::audioBufferProbed, m_audioHistogram, &HistogramWidget::processBuffer);
    m_audioProbe->setSource(m_player);

    QPushButton *openButton = new QPushButton(tr("Open"), this);

    connect(openButton, &QPushButton::clicked, this, &Player::open);

    m_controls = new PlayerControls(this);
    m_controls->setState(m_player->state());
    m_controls->setVolume(m_player->volume());
    m_controls->setMuted(m_controls->isMuted());

    connect(m_controls, &PlayerControls::play, m_player, &QMediaPlayer::play);
    connect(m_controls, &PlayerControls::pause, m_player, &QMediaPlayer::pause);
    connect(m_controls, &PlayerControls::stop, m_player, &QMediaPlayer::stop);
    connect(m_controls, &PlayerControls::next, m_playlist, &QMediaPlaylist::next);
    connect(m_controls, &PlayerControls::previous, this, &Player::previousClicked);
    connect(m_controls, &PlayerControls::changeVolume, m_player, &QMediaPlayer::setVolume);
    connect(m_controls, &PlayerControls::changeMuting, m_player, &QMediaPlayer::setMuted);
    connect(m_controls, &PlayerControls::changeRate, m_player, &QMediaPlayer::setPlaybackRate);
    connect(m_controls, &PlayerControls::stop, m_videoWidget, QOverload<>::of(&QVideoWidget::update));

    connect(m_player, &QMediaPlayer::stateChanged, m_controls, &PlayerControls::setState);
    connect(m_player, &QMediaPlayer::volumeChanged, m_controls, &PlayerControls::setVolume);
    connect(m_player, &QMediaPlayer::mutedChanged, m_controls, &PlayerControls::setMuted);

    m_fullScreenButton = new QPushButton(tr("FullScreen"), this);
    m_fullScreenButton->setCheckable(true);

    m_colorButton = new QPushButton(tr("Color Options..."), this);
    m_colorButton->setEnabled(false);
    connect(m_colorButton, &QPushButton::clicked, this, &Player::showColorDialog);

    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(m_videoWidget, 2);
    displayLayout->addWidget(m_playlistView);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(m_controls);
    controlLayout->addStretch(1);
    controlLayout->addWidget(m_fullScreenButton);
    controlLayout->addWidget(m_colorButton);

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_slider);
    hLayout->addWidget(m_labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    layout->addLayout(histogramLayout);
#if defined(Q_OS_QNX)
    // On QNX, the main window doesn't have a title bar (or any other decorations).
    // Create a status bar for the status information instead.
    m_statusLabel = new QLabel;
    m_statusBar = new QStatusBar;
    m_statusBar->addPermanentWidget(m_statusLabel);
    m_statusBar->setSizeGripEnabled(false); // Without mouse grabbing, it doesn't work very well.
    layout->addWidget(m_statusBar);
#endif

    // keypress?
    // this->setFocusPolicy ( Qt::StrongFocus );

    // Drag and drop
    this->setAcceptDrops(true);

    setLayout(layout);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"\
                                "Please check the media service plugins are installed."));

        m_controls->setEnabled(false);
        m_playlistView->setEnabled(false);
        openButton->setEnabled(false);
        m_colorButton->setEnabled(false);
        m_fullScreenButton->setEnabled(false);
    }

    metaDataChanged();

    serializeSettings(false);
}

Player::~Player()
{
}

bool Player::isPlayerAvailable() const
{
    return m_player->isAvailable();
}

void Player::open()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    QStringList supportedMimeTypes = m_player->supportedMimeTypes();
    if (!supportedMimeTypes.isEmpty()) {
        supportedMimeTypes.append("audio/x-m3u"); // MP3 playlists
        fileDialog.setMimeTypeFilters(supportedMimeTypes);
    }
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        addToPlaylist(fileDialog.selectedUrls());
}

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists() && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

void Player::addToPlaylist(const QList<QUrl> &urls)
{
    for (auto &url: urls) {
        if (isPlaylist(url))
            m_playlist->load(url);
        else
            m_playlist->addMedia(url);
    }
}

void Player::setCustomAudioRole(const QString &role)
{
    m_player->setCustomAudioRole(role);
}

void Player::durationChanged(qint64 duration)
{
    m_duration = duration / 1000;
    m_slider->setMaximum(m_duration);
}

void Player::positionChanged(qint64 progress)
{
    if (!m_slider->isSliderDown())
        m_slider->setValue(progress / 1000);

    updateDurationInfo(progress / 1000);
}

void Player::metaDataChanged()
{
    if (m_player->isMetaDataAvailable()) {
        QString trackInfo;
        QString artist = m_player->metaData(QMediaMetaData::AlbumArtist).toString();
        QString title = m_player->metaData(QMediaMetaData::Title).toString();
        if( !artist.isEmpty())
        {
            trackInfo = artist;
            if(!title.isEmpty())
            {
                trackInfo += " - ";
                trackInfo += title;
            }
        }
        else if(!title.isEmpty())
        {
            trackInfo = title;
        }
        else
        {
            trackInfo = QFileInfo(m_player->currentMedia().canonicalUrl().path()).fileName();
        }
        setTrackInfo(trackInfo);

        if (m_coverLabel) {
            QUrl url = m_player->metaData(QMediaMetaData::CoverArtUrlLarge).value<QUrl>();

            m_coverLabel->setPixmap(!url.isEmpty()
                                    ? QPixmap(url.toString())
                                    : QPixmap());
        }
    }
}

void Player::previousClicked()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if (m_player->position() <= 5000)
        m_playlist->previous();
    else
        m_player->setPosition(0);
}

void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
        m_player->play();
    }
}

void Player::playlistPositionChanged(int currentItem)
{
    clearHistogram();
    m_playlistView->setCurrentIndex(m_playlistModel->index(currentItem, 0));
}

void Player::seek(int seconds)
{
    m_player->setPosition(seconds * 1000);
}


void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(m_player->bufferStatus()));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(m_player->bufferStatus()));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void Player::stateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::StoppedState)
        clearHistogram();
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
            status == QMediaPlayer::BufferingMedia ||
            status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(int progress)
{
    if (m_player->mediaStatus() == QMediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(progress));
    else
        setStatusInfo(tr("Buffering %1%").arg(progress));
}

void Player::videoAvailableChanged(bool available)
{
    if (!available) {
        disconnect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget, &QVideoWidget::setFullScreen);
        disconnect(m_videoWidget, &QVideoWidget::fullScreenChanged, m_fullScreenButton, &QPushButton::setChecked);
        m_videoWidget->setFullScreen(false);
    } else {
        connect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget, &QVideoWidget::setFullScreen);
        connect(m_videoWidget, &QVideoWidget::fullScreenChanged, m_fullScreenButton, &QPushButton::setChecked);

        if (m_fullScreenButton->isChecked())
            m_videoWidget->setFullScreen(true);
    }
    m_colorButton->setEnabled(available);
}

void Player::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

void Player::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

void Player::displayErrorMessage()
{
    setStatusInfo(m_player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60,
                          currentInfo % 60, (currentInfo * 1000) % 1000);
        QTime totalTime((m_duration / 3600) % 60, (m_duration / 60) % 60,
                        m_duration % 60, (m_duration * 1000) % 1000);
        QString format = "mm:ss";
        if (m_duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    m_labelDuration->setText(tStr);
}

void Player::showColorDialog()
{
    if (!m_colorDialog) {
        QSlider *brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessSlider->setRange(-100, 100);
        brightnessSlider->setValue(m_videoWidget->brightness());
        connect(brightnessSlider, &QSlider::sliderMoved, m_videoWidget, &QVideoWidget::setBrightness);
        connect(m_videoWidget, &QVideoWidget::brightnessChanged, brightnessSlider, &QSlider::setValue);

        QSlider *contrastSlider = new QSlider(Qt::Horizontal);
        contrastSlider->setRange(-100, 100);
        contrastSlider->setValue(m_videoWidget->contrast());
        connect(contrastSlider, &QSlider::sliderMoved, m_videoWidget, &QVideoWidget::setContrast);
        connect(m_videoWidget, &QVideoWidget::contrastChanged, contrastSlider, &QSlider::setValue);

        QSlider *hueSlider = new QSlider(Qt::Horizontal);
        hueSlider->setRange(-100, 100);
        hueSlider->setValue(m_videoWidget->hue());
        connect(hueSlider, &QSlider::sliderMoved, m_videoWidget, &QVideoWidget::setHue);
        connect(m_videoWidget, &QVideoWidget::hueChanged, hueSlider, &QSlider::setValue);

        QSlider *saturationSlider = new QSlider(Qt::Horizontal);
        saturationSlider->setRange(-100, 100);
        saturationSlider->setValue(m_videoWidget->saturation());
        connect(saturationSlider, &QSlider::sliderMoved, m_videoWidget, &QVideoWidget::setSaturation);
        connect(m_videoWidget, &QVideoWidget::saturationChanged, saturationSlider, &QSlider::setValue);

        QFormLayout *layout = new QFormLayout;
        layout->addRow(tr("Brightness"), brightnessSlider);
        layout->addRow(tr("Contrast"), contrastSlider);
        layout->addRow(tr("Hue"), hueSlider);
        layout->addRow(tr("Saturation"), saturationSlider);

        QPushButton *button = new QPushButton(tr("Close"));
        layout->addRow(button);

        m_colorDialog = new QDialog(this);
        m_colorDialog->setWindowTitle(tr("Color Options"));
        m_colorDialog->setLayout(layout);

        connect(button, &QPushButton::clicked, m_colorDialog, &QDialog::close);
    }
    m_colorDialog->show();
}

void Player::clearHistogram()
{
    QMetaObject::invokeMethod(m_videoHistogram, "processFrame", Qt::QueuedConnection, Q_ARG(QVideoFrame, QVideoFrame()));
    QMetaObject::invokeMethod(m_audioHistogram, "processBuffer", Qt::QueuedConnection, Q_ARG(QAudioBuffer, QAudioBuffer()));
}

void Player::showEvent(QShowEvent *event)
{
    ParentClass::showEvent(event);

    m_player->setPlaybackRate(m_controls->playbackRate());
    m_player->play();

}
void Player::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    serializeSettings(true);
    // WaitingCursor wc;

    ParentClass::closeEvent(event);
}
void Player::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}
void Player::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}
void Player::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}
void Player::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls())
        return;

    addToPlaylist(mimeData->urls());

    event->acceptProposedAction();
}

//void Player::keyPressEvent(QKeyEvent *event)
//{
//    qDebug() <<event->key() << __FUNCTION__;
//    ParentClass::keyPressEvent(event);
//}
void Player::togglePlay()
{
    switch(m_player->state())
    {
    case QMediaPlayer::PlayingState:
        m_player->pause();
        break;
    case QMediaPlayer::PausedState:
        m_player->play();
        break;
    case QMediaPlayer::StoppedState:
        break;
    }
}

void Player::onHookWheel(QWheelEvent *pWheelEvent)
{
    int delta = 5000;
    m_player->setPosition(m_player->position() +
                          (pWheelEvent->delta() > 0 ? -delta:delta));
}
void Player::serializeSettings(const bool bWrite)
{
    // geometry
    if(bWrite)
    {
        settings_.setValue(KEY_GEOMETRY, saveGeometry());
    }
    else
    {
        restoreGeometry(settings_.value(KEY_GEOMETRY).toByteArray());
    }


    // volume
    if(bWrite)
    {
        settings_.setValue(KEY_VOLUME, m_controls->volume());
    }
    else
    {
        QVariant volume = settings_.value(KEY_VOLUME);
        if( volume.isValid())
            m_controls->setVolume(volume.toInt());
    }

    if(bWrite)
        settings_.sync();
}

void Player::OnPlayListContextMenu(const QPoint &pos)
{
    // Create menu and insert some actions
    QMenu myMenuItemArea;
    myMenuItemArea.addAction(tr("&Open Directory"), this, SLOT(onOpenDirectory()));

    // Show context menu at handling position
    myMenuItemArea.exec(m_playlistView->mapToGlobal(pos));
}
void Player::onOpenDirectory()
{
//    QString file = m_playlistView->currentIndex().data().toString();
    QModelIndex currentIndex = m_playlistView->currentIndex();
//    QString itemText = index.data(Qt::DisplayRole).toString();
//    qDebug() << itemText;

    QUrl location = m_playlist->media(currentIndex.row()).canonicalUrl();
    QString fullpath = location.toLocalFile();
//    qDebug() << aa;

    if(!showInGraphicalShell(this, fullpath))
        Alert(this, tr("Failed to open directory."));
}
