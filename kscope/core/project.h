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

#ifndef __CORE_PROJECT_H
#define __CORE_PROJECT_H

#include <QSettings>
#include <QDir>
#include <QtDebug>
#include "engine.h"
#include "codebase.h"
#include "exception.h"

namespace KScope
{

namespace Core
{

/**
 * Abstract base-class for projects.
 * A project consists of a code base, and an engine that indexes it.
 * @author Elad Lahav
 */
class ProjectBase
{
public:
	ProjectBase() {}
	virtual ~ProjectBase() {}

	/**
	 * Parameters for to configuring projects.
	 */
	struct Params {
		/**
		 * The main project path.
		 * The interpretation of this value is implementation-specific, but it
		 * is expected that projects will store configuration and data files
		 * in a directory corresponding to this path.
		 */
		QString projPath_;

		/**
		 * The name of the project (to be displayed to the user).
		 */
		QString name_;

		/**
		 * The root path of the code base.
		 */
		QString rootPath_;

		/**
		 * A string used to initialise the engine (implementation-dependent).
		 */
		QString engineString_;

		/**
		 * A string used to initialise the code base (implementation-dependent).
		 */
		QString codebaseString_;
	};

	/**
	 * Opens the project.
	 */
	virtual void open() = 0;

	/**
	 * Creates a new project.
	 * @param  params  Project parameters
	 */
	virtual void create(const Params& params) = 0;

	/**
	 * Closes the project.
	 */
	virtual void close() = 0;

	/**
	 * @return The name of the project
	 */
	virtual QString name() const = 0;

	/**
	 * @return The root path for the code base
	 */
	virtual QString rootPath() const = 0;

	/**
	 * @return Pointer to the engine
	 */
	virtual Engine* engine() = 0;

	/**
	 * @return Pointer to the code base
	 */
	virtual Codebase* codebase() = 0;
};

/**
 * Class template for projects using a standard configuration file.
 * This is expected to be the (parameterised) base class for most project
 * implementations.
 * @author Elad Lahav
 */
template<class EngineT, class CodebaseT>
class Project : public ProjectBase
{
public:
	/**
	 * Class constructor.
	 * Attempts to read the name of the project from the given configuration
	 * file.
	 * This given file may not exist (e.g., if this object is used to create a
	 * new project). The constructor will still succeed, but open() will fail.
	 * @param  confFileName  The name of the configuration file
	 * @param  projPath      The path of the directory holding the configuration
	 *                       file (may be empty for new projects)
	 */
	Project(const QString& configFileName, const QString& projPath = QString())
		: configFileName_(configFileName), loaded_(false), open_(false) {
		load(projPath);
	}

	/**
	 * Class destructor.
	 */
	virtual ~Project() {}

	/**
	 * Opens the project.
	 * Initialises the code base and engine.
	 * The configuration file must have been successfully loaded in the
	 * constructor, or by a call to create(), for this method to succeed.
	 * @throw Exception
	 */
	virtual void open() {
		// Nothing to do if the project is already open.
		if (open_)
			return;

		// Make sure the configuration parameters were loaded.
		if (!loaded_)
			throw new Exception("Project parameters were not loaded");

		try {
			// Prepare the engine.
			engine_.open(params_.engineString_);

			// Load the code base.
			codebase_.load(params_.codebaseString_);
		}
		catch (Exception* e) {
			throw e;
		}

		open_ = true;
		qDebug() << "Project opened (EngineString='" << params_.engineString_
		         << "' CodebaseString='" << params_.codebaseString_ << "')";
	}

	/**
	 * Creates a new project.
	 * Note that the project is not open after it has been created.
	 * @param  params  Project parameters
	 * @throw  Exception
	 */
	virtual void create(const Params& params) {
		if (open_ || loaded_)
			throw new Exception("Cannot overwrite an existing project");

		// Make sure the directory to contain the new configuration file
		// exists. Create it if necessary.
		QDir dir(params.projPath_);
		if (!dir.exists() && !dir.mkpath(params.projPath_)) {
			throw new Exception(QString("Failed to create the directory '%1'")
			                    .arg(params.projPath_));
		}

		// Do not overwrite an existing project file.
		if (dir.exists(configFileName_)) {
			throw new Exception(QString("Cannot overwrite an existing "
			                            "project file '%1'")
			                    .arg(dir.filePath(configFileName_)));
		}

		// Copy the given parameters.
		params_ = params;

		// Write the configuration file.
		QSettings projConfig(configPath(), QSettings::IniFormat);
		projConfig.beginGroup("Project");
		projConfig.setValue("Name", params_.name_);
		projConfig.setValue("RootPath", params_.rootPath_);
		projConfig.setValue("EngineString", params_.engineString_);
		projConfig.setValue("CodebaseString", params_.codebaseString_);
		projConfig.endGroup();
	}

	/**
	 * Marks the project as closed.
	 * Note, however, that the configuration parameters are still loaded.
	 */
	virtual void close() {
		open_ = false;
	}

	/**
	 * @return The name of the project
	 */
	virtual QString name() const { return params_.name_; }

	/**
	 * @return The root path for the project's code base
	 */
	virtual QString rootPath() const { return params_.rootPath_; }

	/**
	 * @return A pointer to the engine object
	 */
	virtual Engine* engine() { return &engine_; }

	/**
	 * @return A pointer to the code base object
	 */
	virtual Codebase* codebase() { return &codebase_; }

	/**
	 * Retrieves a copy of the current configuration parameters.
	 * @param  params  An object into which current values are copied
	 */
	void getCurrentParams(Params& params) const {
		params = params_;
	}

protected:
	/**
	 * The name of the project's configuration file.
	 * The configuration file resides under the project path stored in the
	 * configuration parameters.
	 */
	QString configFileName_;

	/**
	 * Configuration parameters.
	 */
	Params params_;

	/**
	 * Whether the project parameters were loaded from the configuration file.
	 */
	bool loaded_;

	/**
	 * Whether the project is open.
	 */
	bool open_;

	/**
	 * The indexing engine.
	 */
	EngineT engine_;

	/**
	 * The code base.
	 */
	CodebaseT codebase_;

	/**
	 * Reads project settings from the configuration file.
	 */
	virtual void load(const QString& projPath) {
		qDebug() << __func__ << projPath << configFileName_;

		// Do nothing if the project file does not exist (needs to be created).
		QFileInfo fi(QDir(projPath).filePath(configFileName_));
		if (!fi.exists() || !fi.isReadable())
			return;

		// Store the project path.
		// This is the directory holding the configuration file.
		params_.projPath_ = projPath;
		if (!params_.projPath_.endsWith("/"))
			params_.projPath_ += "/";

		// Load parameters.
		QSettings projConfig(configPath(), QSettings::IniFormat);
		projConfig.beginGroup("Project");
		params_.name_ = projConfig.value("Name", "").toString();
		params_.rootPath_ = projConfig.value("RootPath", "/").toString();
		params_.engineString_ = projConfig.value("EngineString").toString();
		params_.codebaseString_ = projConfig.value("CodebaseString").toString();
		projConfig.endGroup();

		loaded_ = true;
		qDebug() << "Project loaded (name='" << params_.name_ << "')";
	}

	inline QString configPath() {
		return params_.projPath_ + configFileName_;
	}
};

}

}

#endif /* __CORE_PROJECT_H */
