/***************************************************************************
 *   Copyright (C) 2007-2008 by Elad Lahav
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

#ifndef __CORE_ENGINE_H
#define __CORE_ENGINE_H

#include <QObject>
#include "globals.h"

namespace KScope
{

namespace Core
{

/**
 * Abstract base class for symbol databases.
 * @author Elad Lahav
 */
class Engine : public QObject
{
	Q_OBJECT

public:
	Engine(QObject* parent = 0) : QObject(parent) {}
	virtual ~Engine() {}

	struct Controlled {
		virtual void stop() = 0;
	};

	/**
	 * Represents a single thread of execution in a database.
	 * Engine operations are expected to execute asynchronously. Therefore,
	 * methods of the Engine class that invoke such operations (query and build)
	 * take an Engine::Connection object as a parameter. This object is used to
	 * convey information on the executing operation, including its progress and
	 * results.
	 * From the caller's end, this object can be used to stop an on-going
	 * operation.
	 * @author  Elad Lahav
	 */
	struct Connection {
		void setCtrlObject(Controlled* ctrlObject) { ctrlObject_ = ctrlObject; }
		void stop() { ctrlObject_->stop(); }

		virtual void onDataReady(const Core::LocationList& locList) = 0;
		virtual void onFinished() = 0;
		virtual void onAborted() = 0;
		virtual void onProgress(const QString& text, uint cur, uint total) = 0;

	protected:
		Controlled* ctrlObject_;
	};

public slots:
	/**
	 * Makes the database available for querying.
	 * @param  openString  Implementation-specific string
	 * @return true if successful, false otherwise
	 */
	virtual bool open(const QString& openString) = 0;

	/**
	 * Starts a query.
	 * @param  conn    Used for communication with the ongoing operation
	 * @param  query   The query to execute
	 * @return true if the query was started successfully, false otherwise
	 */
	virtual bool query(Connection& conn, const Query& query) const = 0;

	/**
	 * (Re)builds the symbols database.
	 * @param  conn    Used for communication with the ongoing operation
	 * @return true if the operation was started successfully, false otherwise
	 */
	virtual bool build(Connection&) const = 0;
};

}

}

#endif // __CORE_ENGINE_H
