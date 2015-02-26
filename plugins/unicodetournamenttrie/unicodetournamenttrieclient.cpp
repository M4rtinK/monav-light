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

#include "unicodetournamenttrieclient.h"
#include "utils/qthelpers.h"
#include <QtDebug>
#include <algorithm>
#ifndef NOGUI
 #include <QMessageBox>
#endif

UnicodeTournamentTrieClient::UnicodeTournamentTrieClient()
{
	trieFile = NULL;
	subTrieFile = NULL;
	dataFile = NULL;
	trieData = NULL;
	subTrieData = NULL;
}

UnicodeTournamentTrieClient::~UnicodeTournamentTrieClient()
{

}

QString UnicodeTournamentTrieClient::GetName()
{
	return "Unicode Tournament Trie";
}

void UnicodeTournamentTrieClient::SetInputDirectory( const QString& dir )
{
	directory = dir;
}

void UnicodeTournamentTrieClient::ShowSettings()
{
#ifndef NOGUI
	QMessageBox::information( NULL, "Settings", "No settings available" );
#endif
}

bool UnicodeTournamentTrieClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

bool UnicodeTournamentTrieClient::LoadData()
{
	UnloadData();
	QString filename = fileInDirectory( directory, "Unicode Tournament Trie" );
	trieFile = new QFile( filename + "_main" );
	subTrieFile = new QFile( filename + "_sub" );
	dataFile = new QFile( filename + "_ways" );

	if ( !openQFile( trieFile, QIODevice::ReadOnly ) )
		return false;
	if ( !openQFile( subTrieFile, QIODevice::ReadOnly ) )
		return false;
	if ( !openQFile( dataFile, QIODevice::ReadOnly ) )
		return false;

	trieData = ( char* ) trieFile->map( 0, trieFile->size() );
	subTrieData = ( char* ) subTrieFile->map( 0, subTrieFile->size() );

	if ( trieData == NULL ) {
		qDebug( "Failed to Memory Map trie data" );
		return false;
	}
	if ( subTrieData == NULL ) {
		qDebug( "Failed to Memory Map sub trie data" );
		return false;
	}

	return true;
}

bool UnicodeTournamentTrieClient::UnloadData()
{
	if ( trieFile != NULL )
		delete trieFile;
	trieFile = NULL;
	if ( subTrieFile != NULL )
		delete subTrieFile;
	subTrieFile = NULL;
	if ( dataFile != NULL )
		delete dataFile;
	dataFile = NULL;

	return true;
}

bool UnicodeTournamentTrieClient::find( const char* trie, unsigned* resultNode, QString* missingPrefix, QString prefix )
{
	unsigned node = *resultNode;
	for ( int i = 0; i < ( int ) prefix.length(); ) {
		utt::Node element;
		element.Read( trie + node );
		bool found = false;
		for ( int c = 0; c < ( int ) element.labelList.size(); c++ ) {
			const utt::Label& label = element.labelList[c];
			bool equal = true;
			for ( int subIndex = 0; subIndex < ( int ) label.string.length(); ++subIndex ) {
				if ( i + subIndex >= ( int ) prefix.length() ) {
					*missingPrefix = label.string.mid( subIndex );
					break;
				}
				if ( label.string[subIndex] != prefix[i + subIndex] ) {
					equal = false;
					break;
				}
			}
			if ( !equal )
				continue;

			i += label.string.length();
			node = label.index;
			found = true;
			break;

		}
		if ( !found )
			return false;
	}

	*resultNode = node;
	return true;
}

int UnicodeTournamentTrieClient::getSuggestion( const char* trie, QStringList* resultNames, unsigned node, int count, const QString prefix )
{
	std::vector< Suggestion > candidates( 1 );
	candidates[0].index = node;
	candidates[0].prefix = prefix;
	candidates[0].importance = std::numeric_limits< unsigned >::max();

	while( count > 0 && candidates.size() > 0 ) {
		const Suggestion next = candidates[0];
		candidates[0] = candidates.back();
		candidates.pop_back();

		utt::Node element;
		element.Read( trie + next.index );
		bool isThis = true;
		for ( std::vector< utt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c ) {
			assert( c->importance <= next.importance );
			if ( c->importance == next.importance )
				isThis = false;
		}
		if ( isThis && element.dataList.size() > 0 ) {
			assert( next.prefix.length() > 0 );
			QString suggestion = next.prefix[0].toUpper();
			for ( int i = 1; i < ( int ) next.prefix.length(); ++i ) {
				if ( suggestion[i - 1] == ' ' || suggestion[i - 1] == '-' )
					suggestion += next.prefix[i].toUpper();
				else
					suggestion += next.prefix[i];
			}
			resultNames->push_back( suggestion );
			count--;
		}
		for ( std::vector< utt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c ) {
			Suggestion nextEntry;
			nextEntry.prefix = next.prefix + c->string;
			nextEntry.index = c->index;
			nextEntry.importance = c->importance;
			candidates.push_back( nextEntry );
		}
		std::sort( candidates.begin(), candidates.end() );
		if ( ( int ) candidates.size() > count )
			candidates.resize( count );
	}

	return count;
}

bool UnicodeTournamentTrieClient::GetPlaceSuggestions( const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions )
{
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();

	if ( !find( trieData, &node, &prefix, name ) )
		return false;

	if ( prefix.length() == 0 ) {
		utt::Node element;
		element.Read( trieData + node );
		for ( std::vector< utt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c )
			inputSuggestions->push_back( input + c->string );
	}
	else {
		inputSuggestions->push_back( input + prefix );
	}
	getSuggestion( trieData, suggestions, node, amount, name + prefix );
	std::sort( inputSuggestions->begin(), inputSuggestions->end() );
	return true;
}

bool UnicodeTournamentTrieClient::GetStreetSuggestions( int placeID, const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions )
{
	if ( placeID < 0 )
		return false;
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();

	if ( !find( subTrieData + placeID, &node, &prefix, name ) )
		return false;

	if ( prefix.length() == 0 ) {
		utt::Node element;
		element.Read( subTrieData + placeID + node );
		for ( std::vector< utt::Label >::const_iterator c = element.labelList.begin(), e = element.labelList.end(); c != e; ++c )
			inputSuggestions->push_back( input + c->string );
	}
	else {
		inputSuggestions->push_back( input + prefix );
	}
	getSuggestion( subTrieData + placeID, suggestions, node, amount, name + prefix );
	std::sort( inputSuggestions->begin(), inputSuggestions->end() );
	return true;
}

bool UnicodeTournamentTrieClient::GetPlaceData( QString input, QVector< int >* placeIDs, QVector< UnsignedCoordinate >* placeCoordinates )
{
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();
	if ( !find( trieData, &node, &prefix, name ) )
		return false;

	utt::Node element;
	element.Read( trieData + node );

	for ( std::vector< utt::Data >::const_iterator i = element.dataList.begin(), e = element.dataList.end(); i != e; ++i ) {
		utt::CityData data;
		data.Read( subTrieData + i->start );
		placeCoordinates->push_back( data.coordinate );
		placeIDs->push_back( i->start + data.GetSize() );
	}

	return placeIDs->size() != 0;
}

bool UnicodeTournamentTrieClient::GetStreetData( int placeID, QString input, QVector< int >* segmentLength, QVector< UnsignedCoordinate >* coordinates )
{
	if ( placeID < 0 )
		return false;
	unsigned node = 0;
	QString prefix;
	QString name = input.toLower();
	if ( !find( subTrieData + placeID, &node, &prefix, name ) )
		return false;

	utt::Node element;
	element.Read( subTrieData + placeID + node );

	for ( std::vector< utt::Data >::const_iterator i = element.dataList.begin(), e = element.dataList.end(); i != e; ++i ) {
		unsigned* buffer = new unsigned[i->length * 2];
		dataFile->seek( i->start * sizeof( unsigned ) * 2 );
		dataFile->read( ( char* ) buffer, i->length * 2 * sizeof( unsigned ) );
		for ( unsigned start = 0; start < i->length; ++start ) {
			UnsignedCoordinate temp;
			temp.x = buffer[start * 2];
			temp.y = buffer[start * 2 + 1];
			coordinates->push_back( temp );
		}
		delete[] buffer;
		segmentLength->push_back( coordinates->size() );
	}

	return segmentLength->size() != 0;
}

Q_EXPORT_PLUGIN2(unicodetournamenttrieclient, UnicodeTournamentTrieClient)
