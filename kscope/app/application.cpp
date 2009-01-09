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

#include <QMessageBox>
#include "application.h"
#include "mainwindow.h"
#include "managedproject.h"
#include "projectmanager.h"
#include "version.h"

namespace KScope
{

namespace App
{

/**
 * Class constructor.
 * @param  argc  The number of command-line arguments
 * @param  argv  Command-line argument list
 */
Application::Application(int& argc, char** argv)
	: QApplication(argc, argv)
{
	QCoreApplication::setOrganizationName("elad_lahav@users.sourceforge.net");
	QCoreApplication::setApplicationName("KScope");
	QCoreApplication::setApplicationVersion(AppVersion::toString());
}

/**
 * Class destructor.
 */
Application::~Application()
{
}

/**
 * Starts the application.
 * Note that this method only returns when the application terminates.
 * @return The return code of the application.
 */
int Application::run()
{
	// Create the main window.
	// We can do it on the stack, as the method does not return as long as the
	// application is running.
	MainWindow mainWnd;
	mainWnd_ = &mainWnd;
	mainWnd_->show();

	// Do application initialisation.
	// The reason for posting an event is to have a running application (event
	// loop) before starting the initialisation process. This way, the process
	// can take full advantage of the event mechanism.
	postEvent(this, new QEvent(static_cast<QEvent::Type>(AppInitEvent)));

	return exec();
}

/**
 * Displays application and version information.
 */
void Application::about()
{
	QString msg;
	QTextStream str(&msg);

	str << "Source browsing, analysis and editing\n";
	str << "Version " << applicationVersion() << "\n";
	str << "Copyright (c) 2007-2009 by Elad Lahav\n";
	str << "Distributed under the terms of the GNU Public License v2";

	QMessageBox::about(mainWnd_, applicationName(), msg);
}

/**
 * Handles custom events.
 * @param  event  The event to handle.
 */
void Application::customEvent(QEvent* event)
{
	if (event->type() == static_cast<QEvent::Type>(AppInitEvent))
		init();
}

/**
 * Performs application initialisation, after the event loop was started.
 */
void Application::init()
{
	setupEngines();

	// Parse command-line arguments.
	// TODO: Need to think some more about the options.
	QString path;
	QStringList args = arguments();
	while (!args.isEmpty()) {
		QString arg = args.takeFirst();
		if (arg.startsWith("-")) {
			switch (arg.at(1).toLatin1()) {
			case 'f':
				path = args.takeFirst();
				mainWnd_->openFile(path);
				return;

			case 'p':
				path = args.takeFirst();
				mainWnd_->loadProject(path);
				return;
			}
		}
	}

	// Get the path of the last active project.
	path = QSettings().value("Session/LastProject", "").toString();
	if (path.isEmpty())
		return;

	// Get the project's name.
	QString name = Cscope::ManagedProject(path).name();

	// Prompt the user for opening the last project.
	// TODO: Want more options on start-up (list of last projects, create new,
	// do nothing).
	if (QMessageBox::question(NULL, tr("Open Last Project"),
	                          tr("Would you like to reload '%1'?").arg(name),
	                          QMessageBox::Yes | QMessageBox::No)
	    == QMessageBox::Yes) {
		mainWnd_->loadProject(path);
	}
}

void Application::setupEngines()
{
	// TODO: We'd like a list of engines that can be iterated over in compile
	// time to generate multi-engine code.
	typedef Core::EngineConfig<Cscope::Crossref> Config;

	QSettings settings;

	// Prefix group with "Engine_" so that engines do not overrun application
	// groups by accident.
	settings.beginGroup(QString("Engine_") + Config::name());

	// Add each value under the engine group to the map of configuration
	// parameters.
	Core::KeyValuePairs params;
	QStringList keys = settings.allKeys();
	QString key;
	foreach (key, keys)
		params[key] = settings.value(key);

	settings.endGroup();

	// Apply configuration to the engine.
	Config::setConfig(params);
}

} // namespace App

} // namespace KScope
