/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2007 QMUL.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FileFinder.h"
#include "RemoteFile.h"
#include "AudioFileReaderFactory.h"
#include "DataFileReaderFactory.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QSettings>

#include <iostream>

FileFinder *
FileFinder::m_instance = 0;

FileFinder::FileFinder() :
    m_lastLocatedLocation("")
{
}

FileFinder::~FileFinder()
{
}

FileFinder *
FileFinder::getInstance()
{
    if (m_instance == 0) {
        m_instance = new FileFinder();
    }
    return m_instance;
}

QString
FileFinder::getOpenFileName(FileType type, QString fallbackLocation)
{
    QString settingsKey;
    QString lastPath = fallbackLocation;
    
    QString title = tr("Select file");
    QString filter = tr("All files (*.*)");

    switch (type) {

    case SessionFile:
        settingsKey = "sessionpath";
        title = tr("Select a session file");
        filter = tr("Sonic Visualiser session files (*.sv)\nAll files (*.*)");
        break;

    case AudioFile:
        settingsKey = "audiopath";
        title = "Select an audio file";
        filter = tr("Audio files (%1)\nAll files (*.*)")
            .arg(AudioFileReaderFactory::getKnownExtensions());
        break;

    case LayerFile:
        settingsKey = "layerpath";
        filter = tr("All supported files (%1)\nSonic Visualiser Layer XML files (*.svl)\nComma-separated data files (*.csv)\nSpace-separated .lab files (*.lab)\nMIDI files (*.mid)\nText files (*.txt)\nAll files (*.*)").arg(DataFileReaderFactory::getKnownExtensions());
        break;

    case SessionOrAudioFile:
        settingsKey = "lastpath";
        filter = tr("All supported files (*.sv %1)\nSonic Visualiser session files (*.sv)\nAudio files (%1)\nAll files (*.*)")
            .arg(AudioFileReaderFactory::getKnownExtensions());
        break;

    case AnyFile:
        settingsKey = "lastpath";
        filter = tr("All supported files (*.sv %1 %2)\nSonic Visualiser session files (*.sv)\nAudio files (%1)\nLayer files (%2)\nAll files (*.*)")
            .arg(AudioFileReaderFactory::getKnownExtensions())
            .arg(DataFileReaderFactory::getKnownExtensions());
        break;
    };

    if (lastPath == "") {
        char *home = getenv("HOME");
        if (home) lastPath = home;
        else lastPath = ".";
    } else if (QFileInfo(lastPath).isDir()) {
        lastPath = QFileInfo(lastPath).canonicalPath();
    } else {
        lastPath = QFileInfo(lastPath).absoluteDir().canonicalPath();
    }

    QSettings settings;
    settings.beginGroup("FileFinder");
    lastPath = settings.value(settingsKey, lastPath).toString();

    QString path = "";

    // Use our own QFileDialog just for symmetry with getSaveFileName below

    QFileDialog dialog;
    dialog.setFilters(filter.split('\n'));
    dialog.setWindowTitle(title);
    dialog.setDirectory(lastPath);

    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    
    if (dialog.exec()) {
        QStringList files = dialog.selectedFiles();
        if (!files.empty()) path = *files.begin();
        
        QFileInfo fi(path);
        
        if (!fi.exists()) {
            
            QMessageBox::critical(0, tr("File does not exist"),
                                  tr("File \"%1\" does not exist").arg(path));
            path = "";
            
        } else if (!fi.isReadable()) {
            
            QMessageBox::critical(0, tr("File is not readable"),
                                  tr("File \"%1\" can not be read").arg(path));
            path = "";
            
        } else if (fi.isDir()) {
            
            QMessageBox::critical(0, tr("Directory selected"),
                                  tr("File \"%1\" is a directory").arg(path));
            path = "";

        } else if (!fi.isFile()) {
            
            QMessageBox::critical(0, tr("Non-file selected"),
                                  tr("Path \"%1\" is not a file").arg(path));
            path = "";
            
        } else if (fi.size() == 0) {
            
            QMessageBox::critical(0, tr("File is empty"),
                                  tr("File \"%1\" is empty").arg(path));
            path = "";
        }                
    }

    if (path != "") {
        settings.setValue(settingsKey,
                          QFileInfo(path).absoluteDir().canonicalPath());
    }
    
    return path;
}

QString
FileFinder::getSaveFileName(FileType type, QString fallbackLocation)
{
    QString settingsKey;
    QString lastPath = fallbackLocation;
    
    QString title = tr("Select file");
    QString filter = tr("All files (*.*)");

    switch (type) {

    case SessionFile:
        settingsKey = "savesessionpath";
        title = tr("Select a session file");
        filter = tr("Sonic Visualiser session files (*.sv)\nAll files (*.*)");
        break;

    case AudioFile:
        settingsKey = "saveaudiopath";
        title = "Select an audio file";
        title = tr("Select a file to export to");
        filter = tr("WAV audio files (*.wav)\nAll files (*.*)");
        break;

    case LayerFile:
        settingsKey = "savelayerpath";
        title = tr("Select a file to export to");
        filter = tr("Sonic Visualiser Layer XML files (*.svl)\nComma-separated data files (*.csv)\nText files (*.txt)\nAll files (*.*)");
        break;

    case SessionOrAudioFile:
        std::cerr << "ERROR: Internal error: FileFinder::getSaveFileName: SessionOrAudioFile cannot be used here" << std::endl;
        abort();

    case AnyFile:
        std::cerr << "ERROR: Internal error: FileFinder::getSaveFileName: AnyFile cannot be used here" << std::endl;
        abort();
    };

    if (lastPath == "") {
        char *home = getenv("HOME");
        if (home) lastPath = home;
        else lastPath = ".";
    } else if (QFileInfo(lastPath).isDir()) {
        lastPath = QFileInfo(lastPath).canonicalPath();
    } else {
        lastPath = QFileInfo(lastPath).absoluteDir().canonicalPath();
    }

    QSettings settings;
    settings.beginGroup("FileFinder");
    lastPath = settings.value(settingsKey, lastPath).toString();

    QString path = "";

    // Use our own QFileDialog instead of static functions, as we may
    // need to adjust the file extension based on the selected filter

    QFileDialog dialog;
    dialog.setFilters(filter.split('\n'));
    dialog.setWindowTitle(title);
    dialog.setDirectory(lastPath);

    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setConfirmOverwrite(false); // we'll do that
        
    if (type == SessionFile) {
        dialog.setDefaultSuffix("sv");
    } else if (type == AudioFile) {
        dialog.setDefaultSuffix("wav");
    }

    bool good = false;

    while (!good) {

        path = "";
        
        if (!dialog.exec()) break;
        
        QStringList files = dialog.selectedFiles();
        if (files.empty()) break;
        path = *files.begin();
        
        QFileInfo fi(path);
        
        if (type == LayerFile && fi.suffix() == "") {
            QString expectedExtension;
            QString selectedFilter = dialog.selectedFilter();
            if (selectedFilter.contains(".svl")) {
                expectedExtension = "svl";
            } else if (selectedFilter.contains(".txt")) {
                expectedExtension = "txt";
            } else if (selectedFilter.contains(".csv")) {
                expectedExtension = "csv";
            }
            if (expectedExtension != "") {
                path = QString("%1.%2").arg(path).arg(expectedExtension);
                fi = QFileInfo(path);
            }
        }
        
        if (fi.isDir()) {
            QMessageBox::critical(0, tr("Directory selected"),
                                  tr("File \"%1\" is a directory").arg(path));
            continue;
        }
        
        if (fi.exists()) {
            if (QMessageBox::question(0, tr("File exists"),
                                      tr("The file \"%1\" already exists.\nDo you want to overwrite it?").arg(path),
                                      QMessageBox::Ok,
                                      QMessageBox::Cancel) != QMessageBox::Ok) {
                continue;
            }
        }
        
        good = true;
    }
        
    if (path != "") {
        settings.setValue(settingsKey,
                          QFileInfo(path).absoluteDir().canonicalPath());
    }
    
    return path;
}

void
FileFinder::registerLastOpenedFilePath(FileType type, QString path)
{
    QString settingsKey;

    switch (type) {
    case SessionFile:
        settingsKey = "sessionpath";
        break;

    case AudioFile:
        settingsKey = "audiopath";
        break;

    case LayerFile:
        settingsKey = "layerpath";
        break;

    case SessionOrAudioFile:
        settingsKey = "lastpath";
        break;

    case AnyFile:
        settingsKey = "lastpath";
        break;
    }

    if (path != "") {
        QSettings settings;
        settings.beginGroup("FileFinder");
        path = QFileInfo(path).absoluteDir().canonicalPath();
        settings.setValue(settingsKey, path);
        settings.setValue("lastpath", path);
    }
}
    
QString
FileFinder::find(FileType type, QString location, QString lastKnownLocation)
{
    if (QFileInfo(location).exists()) return location;

    if (RemoteFile::canHandleScheme(QUrl(location))) {
        RemoteFile rf(location);
        bool available = rf.isAvailable();
        rf.deleteLocalFile();
        if (available) return location;
    }

    QString foundAt = "";

    if ((foundAt = findRelative(location, lastKnownLocation)) != "") {
        return foundAt;
    }

    if ((foundAt = findRelative(location, m_lastLocatedLocation)) != "") {
        return foundAt;
    }

    return locateInteractive(type, location);
}

QString
FileFinder::findRelative(QString location, QString relativeTo)
{
    if (relativeTo == "") return "";

    std::cerr << "Looking for \"" << location.toStdString() << "\" next to \""
              << relativeTo.toStdString() << "\"..." << std::endl;

    QString fileName;
    QString resolved;

    if (RemoteFile::canHandleScheme(QUrl(location))) {
        fileName = QUrl(location).path().section('/', -1, -1,
                                                 QString::SectionSkipEmpty);
    } else {
        fileName = QFileInfo(location).fileName();
    }

    if (RemoteFile::canHandleScheme(QUrl(relativeTo))) {
        resolved = QUrl(relativeTo).resolved(fileName).toString();
        RemoteFile rf(resolved);
        if (!rf.isAvailable()) resolved = "";
        std::cerr << "resolved: " << resolved.toStdString() << std::endl;
        rf.deleteLocalFile();
    } else {
        resolved = QFileInfo(relativeTo).dir().filePath(fileName);
        if (!QFileInfo(resolved).exists() ||
            !QFileInfo(resolved).isFile() ||
            !QFileInfo(resolved).isReadable()) {
            resolved = "";
        }
    }
            
    return resolved;
}

QString
FileFinder::locateInteractive(FileType type, QString thing)
{
    QString question;
    if (type == AudioFile) {
        question = tr("Audio file \"%1\" could not be opened.\nDo you want to locate it?");
    } else {
        question = tr("File \"%1\" could not be opened.\nDo you want to locate it?");
    }

    QString path = "";
    bool done = false;

    while (!done) {

        int rv = QMessageBox::question
            (0, 
             tr("Failed to open file"),
             question.arg(thing),
             tr("Locate file..."),
             tr("Use URL..."),
             tr("Cancel"),
             0, 2);
        
        switch (rv) {

        case 0: // Locate file

            if (QFileInfo(thing).dir().exists()) {
                path = QFileInfo(thing).dir().canonicalPath();
            }
            
            path = getOpenFileName(type, path);
            done = (path != "");
            break;

        case 1: // Use URL
        {
            bool ok = false;
            path = QInputDialog::getText
                (0, tr("Use URL"),
                 tr("Please enter the URL to use for this file:"),
                 QLineEdit::Normal, "", &ok);

            if (ok && path != "") {
                RemoteFile rf(path);
                if (rf.isAvailable()) {
                    done = true;
                } else {
                    QMessageBox::critical
                        (0, tr("Failed to open location"),
                         tr("URL \"%1\" could not be opened").arg(path));
                    path = "";
                }
                rf.deleteLocalFile();
            }
            break;
        }

        case 2: // Cancel
            path = "";
            done = true;
            break;
        }
    }

    if (path != "") m_lastLocatedLocation = path;
    return path;
}

