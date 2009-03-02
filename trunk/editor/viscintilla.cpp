/***************************************************************************
 *   Copyright (C) 2007-2009 by Elad Lahav
 *   elad_lahav@users.sourceforge.net
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ***************************************************************************/

#include <QDebug>
#include "viscintilla.h"

namespace KScope
{

namespace Editor
{

/**
 * Handles cursor movement commands.
 * Commands that depend on movement, such as delete and copy, should inherit
 * from this class and reimplement action().
 * @author Elad Lahav
 */
struct MoveCommand : public ViScintilla::Command
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		// Abort early on unhandled keys.
		if ((event->modifiers() & ~Qt::ShiftModifier) != Qt::NoModifier)
			return NotHandled;

		if (event->text().length() > 1)
			return NotHandled;

		// Translate keys.
		char key;
		switch (event->key()) {
		case Qt::Key_Left:
			key = 'h';
			break;

		case Qt::Key_Right:
			key = 'l';
			break;

		case Qt::Key_Up:
			key = 'k';
			break;

		case Qt::Key_Down:
			key = 'j';
			break;

		default:
			if (event->text().length() == 1)
				key = event->text()[0].toLatin1();
			else
				return NotHandled;
		}

		// Get the current position.
		int curLine, curColumn, newLine, newColumn;
		editor->getCursorPosition(&curLine, &curColumn);

		switch (key) {
		case '0':
			if (seq.isEmpty()) {
				// Move to the beginning of the line.
				newLine = curLine;
				newColumn = 0;
				break;
			}
			// Fall through...
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			seq.append(event->text());
			return Continue;

		case 'l':
			newLine = curLine;
			newColumn = curColumn + stringToMultiplier(seq);
			break;

		case 'h':
			newLine = curLine;
			newColumn = curColumn - stringToMultiplier(seq);
			break;

		case 'k':
			newLine = curLine - stringToMultiplier(seq);
			newColumn = curColumn;
			break;

		case 'j':
			newLine = curLine + stringToMultiplier(seq);
			newColumn = curColumn;
			break;

		case 'w':
			break;

		case '$':
			newLine = curLine;
			newColumn = editor->lineLength(curLine) - 1;
			break;

		default:
			return NotHandled;
		}

		action(editor, curLine, curColumn, newLine, newColumn);

		// On multi-key sequences, add the last character so that the entire
		// sequence can be displayed.
		if (!seq.isEmpty())
			seq.append(key);

		return Done;
	}

	QString name() const { return "Move"; }

	virtual void action(ViScintilla* editor, int curLine, int curColumn,
	                    int newLine, int newColumn) {
		(void)curLine;
		(void)curColumn;
		editor->setCursorPosition(newLine, newColumn);
	}

	static int stringToMultiplier(const QString& str) {
		if (str.isEmpty())
			return 1;

		return str.toInt();
	}
};

/**
 * Copies (yanks) text.
 * Supported variants include:
 * - yy: Copy line,
 * - y<NUMBER>y: Copy <NUMBER> lines,
 * - y<MOVE_COMMAND>: Copy from the current position to the new one, based
 *   on the move command used.
 * @author Elad Lahav
 */
struct YankCommand : public MoveCommand
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		// Check for the first "y".
		if (seq.isEmpty()) {
			if (event->text() == "y") {
				seq = "y";
				return Continue;
			}

			return NotHandled;
		}

		// Non-empty sequence that begins with a "d".
		// A second "d" means delete the number of lines stored in the
		// multiplier.
		if (event->text() == "y")
		{
			// Get the current line number.
			int line, col;
			editor->getCursorPosition(&line, &col);

			// Get the number of lines to delete.
			int lines = stringToMultiplier(seq.mid(1));

			// Delete the lines.
			action(editor, line, 0, line + lines, 0);
			seq.append("y");
			return Done;
		}

		// Not a "y", use the move command to interpret the key.
		return MoveCommand::processKey(event, editor, seq);
	}

	QString name() const { return "Yank"; }

	virtual void action(ViScintilla* editor, int curLine, int curColumn,
	                    int newLine, int newColumn) {
		editor->setSelection(curLine, curColumn, newLine, newColumn);
		editor->copy();
	}
};

/**
 * Cuts text.
 * Supported variants include:
 * - dd: Cut line,
 * - d<NUMBER>d: Cut <NUMBER> lines,
 * - d<MOVE_COMMAND>: Copy from the current position to the new one, based
 *   on the move command used.
 * @author Elad Lahav
 */
struct CutCommand : public MoveCommand
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		// Check for the first "d".
		if (seq.isEmpty()) {
			if (event->text() == "d") {
				seq = "d";
				return Continue;
			}

			return NotHandled;
		}

		// Non-empty sequence that begins with a "d".
		// A second "d" means delete the number of lines stored in the
		// multiplier.
		if (event->text() == "d")
		{
			// Get the current line number.
			int line, col;
			editor->getCursorPosition(&line, &col);

			// Get the number of lines to delete.
			int lines = stringToMultiplier(seq.mid(1));

			// Delete the lines.
			action(editor, line, 0, line + lines, 0);
			seq.append("d");
			return Done;
		}

		// Not a "d", use the move command to interpret the key.
		return MoveCommand::processKey(event, editor, seq);
	}

	QString name() const { return "Cut"; }

	virtual void action(ViScintilla* editor, int curLine, int curColumn,
	                    int newLine, int newColumn) {
		editor->setSelection(curLine, curColumn, newLine, newColumn);
		editor->cut();
	}
};

/**
 * Pastes text from the clipboard.
 * @author Elad Lahav
 */
struct PasteCommand : public ViScintilla::Command
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		(void)seq;

		if (event->text() == "p") {
			editor->paste();
			return Done;
		}

		return NotHandled;
	}

	QString name() const { return "Paste"; }
};

/**
 * Aborts Vi mode.
 * Different keys place the cursor in different locations:
 * - i: Current position
 * - a: Next column
 * - I: Beginning of current line
 * - A: End of current line
 * @author Elad Lahav
 */
struct InsertCommand : public ViScintilla::Command
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		(void)seq;

		// Abort early on unhandled keys.
		if ((event->modifiers() & ~Qt::ShiftModifier) != Qt::NoModifier)
			return NotHandled;

		if (event->text().length() > 1)
			return NotHandled;

		// Get the current cursor position.
		int line, col;
		editor->getCursorPosition(&line, &col);

		char key = event->text()[0].toLatin1();
		switch (key) {
		case 'i':
			return Exit;

		case 'I':
			col = 0;
			break;

		case 'a':
			col++;
			break;

		case 'A':
			col = editor->lineLength(line) - 1;
			break;

		default:
			return NotHandled;
		}

		// Move the cursor and exit ExMode.
		editor->setCursorPosition(line, col);
		return Exit;
	}

	QString name() const { return "Insert"; }
};

/**
 * Performs the undo (u) and redo (Ctrl+r) actions.
 * @author Elad Lahav
 */
struct UndoRedoCommand : public ViScintilla::Command
{
	ProcessResult processKey(QKeyEvent* event, ViScintilla* editor,
	                         QString& seq) {
		(void)seq;

		// Get the current cursor position.
		if (event->text() == "u") {
			editor->undo();
			return Done;
		}
		else if ((event->key() == Qt::Key_R)
		         && (event->modifiers() == Qt::ControlModifier)) {
			editor->redo();
			return Done;
		}

		return NotHandled;
	}

	QString name() const { return "UndoRedo"; }
};

ViScintilla::CommandHash ViScintilla::commandHash_;

/**
 * Class constructor.
 * @param  parent Parent widget
 */
ViScintilla::ViScintilla(QWidget* parent)
	: QsciScintilla(parent), mode_(Disabled), curCommand_(NULL)
{
}

/**
 * Class destructor.
 */
ViScintilla::~ViScintilla()
{
}

/**
 * Changes the edit mode.
 * @param  mode The new mode to set
 */
void ViScintilla::setEditMode(EditMode mode)
{
	if (mode != mode_) {
		// Set the new mode.
		qDebug() << __func__ << mode;
		mode_ = mode;

		// Change the caret style.
		// TODO: Handle overwrite in disabled/insert mode.
		switch (mode_) {
		case Disabled:
		case InsertMode:
			SendScintilla(SCI_SETCARETSTYLE, CARETSTYLE_LINE);
			break;

		case NormalMode:
		case VisualMode:
			SendScintilla(SCI_SETCARETSTYLE, CARETSTYLE_BLOCK);
			break;
		}

		// Notify of the change.
		emit editModeChanged(mode_);
	}
}

/**
 * Handles a key press.
 * If Vi compatibility is disabled or the current editing mode is insert, the
 * event is handled by Scintilla. Otherwise, the key is intercepted and
 * interpreted by the Vi command structure.
 * @param  event The key event
 */
void ViScintilla::keyPressEvent(QKeyEvent* event)
{
	switch (mode_) {
	case Disabled:
		// Let Scintilla handle all events.
		QsciScintilla::keyPressEvent(event);
		return;

	case InsertMode:
		// Intercept the ESC key for entering normal mode.
		if ((event->key() == Qt::Key_Escape)
		    && (event->modifiers() == Qt::NoModifier)) {
			setEditMode(NormalMode);
			event->setAccepted(true);
		}
		else {
			// Anything but ESC.
			QsciScintilla::keyPressEvent(event);
		}
		return;

	case NormalMode:
	case VisualMode:
		break;
	}

	// Process the key event.
	Command::ProcessResult result;

	if (curCommand_) {
		// If a command is in progress, let it process the the key.
		result = curCommand_->processKey(event, this, cmdSequence_);
	}
	else {
		// This is the first key in a new command.
		// Check which command is associated with this key.
		result = Command::NotHandled;
		cmdSequence_ = "";
		QList<Command*> cmdList = commandHash_.values(event->key());
		foreach (Command* cmd, cmdList) {
			if ((result = cmd->processKey(event, this, cmdSequence_))
			    != Command::NotHandled) {
				curCommand_ = cmd;
				qDebug() << "Command:" << cmd->name();
				break;
			}
		}
	}

	switch (result) {
	case Command::Continue:
		// A command is in progress.
		break;

	case Command::Exit:
		// Return to insert mode.
		setEditMode(InsertMode);
		// Fall through...

	case Command::Done:
		curCommand_ = NULL;
		// Do not clear the sequence, so that it is available for display.
		break;

	case Command::NotHandled:
		// Unrecognised command sequence.
		curCommand_ = NULL;
		emit message(tr("Bad sequence: %1").arg(cmdSequence_), 2000);
		cmdSequence_ = "";
		return event->setAccepted(false);
	}

	// Report a non-empty sequence, so that it can be displayed.
	if (!cmdSequence_.isEmpty())
		emit message(cmdSequence_, 1000);

	event->setAccepted(true);
}

/**
 * Class constructor.
 * Creates the command objects, and puts them in the key-to-command hash.
 */
ViScintilla::CommandHash::CommandHash()
{
	MoveCommand* moveCmd = new MoveCommand;
	YankCommand* yankCmd = new YankCommand;
	CutCommand* cutCmd = new CutCommand;
	PasteCommand* pasteCmd = new PasteCommand;
	InsertCommand* insCmd = new InsertCommand;
	UndoRedoCommand* undoCmd = new UndoRedoCommand;

	cmdList_ << moveCmd << yankCmd << cutCmd << pasteCmd << insCmd << undoCmd;

	insert(Qt::Key_0, moveCmd);
	insert(Qt::Key_1, moveCmd);
	insert(Qt::Key_2, moveCmd);
	insert(Qt::Key_3, moveCmd);
	insert(Qt::Key_4, moveCmd);
	insert(Qt::Key_5, moveCmd);
	insert(Qt::Key_6, moveCmd);
	insert(Qt::Key_7, moveCmd);
	insert(Qt::Key_8, moveCmd);
	insert(Qt::Key_9, moveCmd);
	insert(Qt::Key_L, moveCmd);
	insert(Qt::Key_H, moveCmd);
	insert(Qt::Key_K, moveCmd);
	insert(Qt::Key_J, moveCmd);
	insert(Qt::Key_W, moveCmd);
	insert(Qt::Key_Left, moveCmd);
	insert(Qt::Key_Right, moveCmd);
	insert(Qt::Key_Up, moveCmd);
	insert(Qt::Key_Down, moveCmd);
	insert(Qt::Key_Dollar, moveCmd);
	insert(Qt::Key_Y, yankCmd);
	insert(Qt::Key_D, cutCmd);
	insert(Qt::Key_P, pasteCmd);
	insert(Qt::Key_I, insCmd);
	insert(Qt::Key_A, insCmd);
	insert(Qt::Key_U, undoCmd);
	insert(Qt::Key_R, undoCmd);
}

/**
 * Class destructor.
 * Deletes the command objects.
 */
ViScintilla::CommandHash::~CommandHash()
{
	foreach (Command* cmd, cmdList_)
		delete cmd;
}

} // namespace Editor

} // namespace KScope
