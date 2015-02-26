/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include "interfaces/iconsolesettings.h"

// parses the command line for a given set of data sinks
// each data sink can provide a list of settings and
// the parser tries to match the command line to it
class CommandLineParser
{

public:

	CommandLineParser();
	~CommandLineParser();

	// register a new data sink
	// if the a setting entry of the data sink clashes with previous settings
	// all conflicting settings are inaccesible for the user
	void registerDataSink( IConsoleSettings* dataSink );

	// unregisters all data sinks
	void clean();

	// tries to parse command line arguments
	// will fail if a data sink fails to set a settings
	// or if a ambigious setting was used
	bool parse( bool ignoreMissing = false );

	bool displayHelp();

private:

	struct PrivateImplementation;

	PrivateImplementation* d;
};

#endif // COMMANDLINEPARSER_H
