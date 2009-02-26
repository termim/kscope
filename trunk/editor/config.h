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

#ifndef __EDITOR_CONFIG_H__
#define __EDITOR_CONFIG_H__

#include <QObject>
#include <QSettings>
#include <qscilexercpp.h>
#include <qscilexermakefile.h>
#include <qscilexerbash.h>

namespace KScope
{

namespace Editor
{

class Editor;
class LexerStyleModel;

/**
 * A special lexer used to create a common default style for all other lexers.
 * This class serves two purposes:
 * 1. Allows style properties to be inherited by all lexers (e.g., for setting
 *    a single, common font);
 * 2. Provides a default lexer for files that are not handled by any of the
 *    pre-defined lexers.
 * @author Elad Lahav
 */
class CommonLexer : public QsciLexer
{
public:
	/**
	 * Class constructor.
	 * @param  parent Parent object
	 */
	CommonLexer(QObject* parent) : QsciLexer(parent) {}

	/**
	 * Class destructor.
	 */
	~CommonLexer() {}

	/**
	 * @return A string identifying the language handled by the lexer
	 */
	const char* language() const { return tr("Common").toLatin1(); }

	/**
	 * @return A string identifying the lexer
	 */
	const char* lexer() const { return tr("Common").toLatin1(); }

	/**
	 * Provides a name for the given style ID.
	 * @param  style The style ID
	 * @return The name of the style, or an empty string if the style does not
	 *         exist
	 */
	QString description(int style) const {
		if (style == 0)
			return tr("Default");

		return QString();
	}

	/**
	 * @return The ID of the default style for this lexer
	 */
	int defaultStyle() const { return 0; }
};

/**
 * Manages editor configuration.
 * @author Elad Lahav
 */
class Config : public QObject
{
	Q_OBJECT

public:
	Config(QObject* parent = NULL);
	~Config();

	void load(const QSettings&);
	void store(QSettings&) const;
	void apply(Editor*) const;
	QsciLexer* lexer(const QString&) const;

	typedef QList<QsciLexer*> LexerList;

private:
	/**
	 * Whether to highlight the current line.
	 */
	bool hlCurLine_;

	/**
	 * Whether to show line numbers in the margin.
	 */
	bool marginLineNumbers_;

	/**
	 * Whether to use tabs for indentation.
	 */
	bool indentTabs_;

	/**
	 * The tab width, in characters.
	 */
	int tabWidth_;

	/**
	 * The common defaults lexers.
	 */
	CommonLexer* commonLexer_;

	/**
	 * C/C++ lexer.
	 */
	QsciLexerCPP* cppLexer_;

	/**
	 * Makefile lexer.
	 */
	QsciLexerMakefile* makeLexer_;

	/**
	 * Shell script lexer.
	 */
	QsciLexerBash* bashLexer_;

	/**
	 * A list of the above lexers for batch operations.
	 * All objects in this list must inherit from both QsciLexer and
	 * LexerExInterface.
	 */
	LexerList lexers_;

	/**
	 * Used to configure lexer styles.
	 */
	LexerStyleModel* styleModel_;

	/**
	 * Maps file name patterns to lexers.
	 */
	struct {
		struct Pair {
			QRegExp re_;
			QsciLexer* lexer_;

			Pair(const QString& pattern)
				: re_(pattern, Qt::CaseSensitive, QRegExp::Wildcard),
				  lexer_(NULL) {}

			void operator=(QsciLexer* lexer) { lexer_ = lexer; }
		};

		QList<Pair> map_;

		Pair& operator[](const QString& pattern) {
			map_.append(Pair(pattern));
			return map_.last();
		}

		QsciLexer* find(const QString& text) const {
			QList<Pair>::ConstIterator itr;
			for (itr = map_.begin(); itr != map_.end(); ++itr) {
				if ((*itr).re_.exactMatch(text))
					return (*itr).lexer_;
			}
			return NULL;
		}

		void clear() { map_.clear(); }
	} lexerMap_;

	friend class ConfigDialog;

	void fromEditor(Editor* editor = NULL);

	template<class T>
	static inline void loadValue(const QSettings& settings, T& val,
	                      const QString& key, const T& defVal) {
		QVariant var = settings.value(key, defVal);
		val = var.value<T>();
	}
};

} // namespace Editor

} // namespace KScope

#endif // __EDITOR_CONFIG_H__