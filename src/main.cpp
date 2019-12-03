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

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QStyle>

#include "../../lsMisc/stdQt/stdQt.h"
#include "../../lsMisc/stdQt/settings.h"

#include "consts.h"
#include "player.h"
#include "application.h"

using namespace Consts;
using namespace AmbiesoftQt;

void noMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)
    Q_UNUSED(msg)
}

int main(int argc, char *argv[])
{

    QCoreApplication::setOrganizationName(ORGANIZATION);
    QCoreApplication::setOrganizationDomain(APPDOMAIN);
    QCoreApplication::setApplicationName(APPNAME);
    QCoreApplication::setApplicationVersion(APPVERSION);

    Application theApp(argc, argv);

#ifdef QT_NO_DEBUG
    if ( !theApp.arguments().contains(QLatin1String("--with-debug") ))
    {
        qInstallMessageHandler(noMessageOutput);
    }
#endif

    QCommandLineParser parser;
    QCommandLineOption customAudioRoleOption("custom-audio-role",
                                             "Set a custom audio role for the player.",
                                             "role");
    parser.setApplicationDescription(APPNAME);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(customAudioRoleOption);
    parser.addPositionalArgument("url", "The URL(s) to open.");
    parser.process(theApp);


    bool bExit = false;
    Q_ASSERT(isLegalFilePath(theApp.organizationName()));
    Q_ASSERT(isLegalFilePath(theApp.applicationName()));
    QString inifile = getInifile(bExit,
                                 theApp.organizationName(),
                                 theApp.applicationName());
    if(bExit)
        return 1;
    QScopedPointer<IniSettings> settings(inifile.isEmpty() ?
                                          new IniSettings(QApplication::organizationName(), QApplication::applicationName()) :
                                          new IniSettings(inifile));
    if(!settings->isAccessible())
    {
        Alert(nullptr,
              QObject::tr("\"%1\" is not accessible.").arg(settings->fileName()));
        return 1;
    }


    // style:  "windows", "windowsvista", "fusion", or "macintosh".
    qDebug () << "CurrentStyle: " << QApplication::style()->objectName() << __FUNCTION__;
    QString style = settings->valueString(KEY_STYLE);
    if(style.isEmpty())
        style = "fusion";
    if(!QApplication::setStyle(style))
    {
        Alert(nullptr, QObject::tr("Failed to set style") + ": " + style);
    }


    Player player(nullptr, *settings);

    if (parser.isSet(customAudioRoleOption))
        player.setCustomAudioRole(parser.value(customAudioRoleOption));

    if (!parser.positionalArguments().isEmpty() && player.isPlayerAvailable()) {
        QList<QUrl> urls;
        for (auto &a: parser.positionalArguments())
            urls.append(QUrl::fromUserInput(a, QDir::currentPath(), QUrl::AssumeLocalFile));
        player.addToPlaylist(urls);
    }

    player.show();
    return theApp.exec();
}
