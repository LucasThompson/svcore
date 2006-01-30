/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/*
    A waveform viewer and audio annotation editor.
    Chris Cannam, Queen Mary University of London, 2005-2006
    
    This is experimental software.  Not for distribution.
*/

/*
   This is a modified version of a source file from the Rosegarden
   MIDI and audio sequencer and notation editor, copyright 2000-2006
   Chris Cannam, distributed under the GNU General Public License.

   This file contains traces of the KCommandHistory class from the KDE
   project, copyright 2000 Werner Trobin and David Faure and
   distributed under the GNU Lesser General Public License.
*/

#ifndef _MULTI_VIEW_COMMAND_HISTORY_H_
#define _MULTI_VIEW_COMMAND_HISTORY_H_

#include <QObject>
#include <QString>

#include <stack>
#include <set>
#include <map>

class Command;
class QAction;
class QMenu;
class QToolBar;

/**
 * The MultiViewCommandHistory class stores a list of executed
 * commands and maintains Undo and Redo actions synchronised with
 * those commands.
 *
 * MultiViewCommandHistory also allows you to associate more than one
 * Undo and Redo action with the same command history, and it keeps
 * them all up-to-date at once.  This makes it effective in systems
 * where multiple views may be editing the same data at once.
 */

class MultiViewCommandHistory : public QObject
{
    Q_OBJECT

public:
    MultiViewCommandHistory();
    virtual ~MultiViewCommandHistory();

    void clear();
    
    void registerMenu(QMenu *menu);
    void registerToolbar(QToolBar *toolbar);

    void addCommand(Command *command, bool execute = true);
    
    /// Return the maximum number of items in the undo history.
    int undoLimit() { return m_undoLimit; }

    /// Set the maximum number of items in the undo history.
    void setUndoLimit(int limit);

    /// Return the maximum number of items in the redo history.
    int redoLimit() { return m_redoLimit; }

    /// Set the maximum number of items in the redo history.
    void setRedoLimit(int limit);
    
public slots:
    /**
     * Checkpoint function that should be called when the document is
     * saved.  If the undo/redo stack later returns to the point at
     * which the document was saved, the documentRestored signal will
     * be emitted.
     */
    virtual void documentSaved();

protected slots:
    void undo();
    void redo();
    void undoActivated(QAction *);
    void redoActivated(QAction *);

signals:
    /**
     * Emitted whenever a command has just been executed, whether by
     * addCommand, undo, or redo.
     */
    void commandExecuted(Command *);

    /**
     * Emitted whenever a command has just been executed, whether by
     * addCommand, undo, or redo.
     */
    void commandExecuted();

    /**
     * Emitted when the undo/redo stack has reached the same state at
     * which the documentSaved slot was last called.
     */
    void documentRestored();

private:
    QAction *m_undoAction;
    QAction *m_redoAction;
    QMenu *m_undoMenu;
    QMenu *m_redoMenu;

    std::map<QAction *, int> m_actionCounts;

    typedef std::stack<Command *> CommandStack;
    CommandStack m_undoStack;
    CommandStack m_redoStack;

    int m_undoLimit;
    int m_redoLimit;
    int m_savedAt;

    void updateActions();
//    void updateMenus();

//    void updateButtons();
//    void updateButton(bool undo, const QString &name, CommandStack &stack);
//    void updateMenu(bool undo, const QString &name, CommandStack &stack);
    void clipCommands();

    void clipStack(CommandStack &stack, int limit);
    void clearStack(CommandStack &stack);
};


#endif
