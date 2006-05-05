/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _TEMP_DIRECTORY_H_
#define _TEMP_DIRECTORY_H_

#include <QString>
#include <QMutex>

#include <exception>

/**
 * A class that manages the creation and removal of a temporary
 * directory tree to store data during the program run.  There is one
 * root temporary directory for the program, created on demand and
 * deleted when the program exits.
 *
 * This class is thread safe.
 */

class TempDirectory
{
public:
    static TempDirectory *instance();
    
    virtual ~TempDirectory();

    class DirectoryCreationFailed : virtual public std::exception
    {
    public:
        DirectoryCreationFailed(QString directory) throw();
        virtual DirectoryCreationFailed::~DirectoryCreationFailed() throw() { }
        virtual const char *what() const throw();

    protected:
        QString m_directory;
    };

    /**
     * Create the root temporary directory if necessary, and return
     * its path.  Throw DirectoryCreationFailed if the directory
     * cannot be created.
     */
    QString getPath();

    /** 
     * Create an immediate subdirectory of the root temporary
     * directory of the given name, if it doesn't already exist, and
     * return its path.  Throw DirectoryCreationFailed if the
     * directory cannot be created.
     */
    QString getSubDirectoryPath(QString subdir);

    /**
     * Delete the temporary directory (before exiting).
     */
    void cleanup();

protected:
    TempDirectory();

    QString createTempDirectoryIn(QString inDir);
    void cleanupDirectory(QString tmpDir);
    void cleanupAbandonedDirectories(QString svDir);

    QString m_tmpdir;
    QMutex m_mutex;

    static TempDirectory *m_instance;
};


#endif
