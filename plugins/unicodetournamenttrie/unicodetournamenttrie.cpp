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

#include "unicodetournamenttrie.h"
#include "utils/qthelpers.h"
#include "utils/edgeconnector.h"
#include "interfaces/iimporter.h"
#ifndef NOGUI
#include "uttsettingsdialog.h"
#endif
#include <algorithm>
#include <QMultiHash>
#include <QList>
#include <limits>

UnicodeTournamentTrie::UnicodeTournamentTrie()
{
}

UnicodeTournamentTrie::~UnicodeTournamentTrie()
{
}

QString UnicodeTournamentTrie::GetName()
{
	return "Unicode Tournament Trie";
}

int UnicodeTournamentTrie::GetFileFormatVersion()
{
	return 1;
}

UnicodeTournamentTrie::Type UnicodeTournamentTrie::GetType()
{
	return AddressLookup;
}

bool UnicodeTournamentTrie::LoadSettings( QSettings* /*settings*/ )
{
	return true;
}

bool UnicodeTournamentTrie::SaveSettings( QSettings* /*settings*/ )
{
	return true;
}

bool UnicodeTournamentTrie::Preprocess( IImporter* importer, QString dir )
{
	QString filename = fileInDirectory( dir, "Unicode Tournament Trie" );

	QFile subTrieFile( filename + "_sub" );
	QFile wayFile( filename + "_ways" );

	if ( !openQFile( &subTrieFile, QIODevice::WriteOnly ) )
		return false;
	if ( !openQFile( &wayFile, QIODevice::WriteOnly ) )
		return false;

	std::vector< IImporter::Place > inputPlaces;
	std::vector< IImporter::Address > inputAddress;
	std::vector< UnsignedCoordinate > inputWayBuffer;
	std::vector< QString > inputWayNames;
	if ( !importer->GetAddressData( &inputPlaces, &inputAddress, &inputWayBuffer, &inputWayNames ) )
		return false;

	Timer time;

	std::sort( inputAddress.begin(), inputAddress.end() );
	qDebug() << "Unicode Tournament Trie: sorted addresses by importance:" << time.restart() << "ms";

	std::vector< UnsignedCoordinate > wayBuffer;
	std::vector< utt::Node > trie( 1 );
	unsigned address = 0;

	// build address name index
	QMultiHash< unsigned, unsigned > addressByName;
	for ( ; address < inputAddress.size(); address++ ) {
		addressByName.insert( inputAddress[address].name, address );
	}

	// compute way lengths
	QList< unsigned > uniqueNames = addressByName.uniqueKeys();
	std::vector< std::pair< double, unsigned > > wayLengths;
	for ( unsigned name = 0; name < ( unsigned ) uniqueNames.size(); name++ ) {
		QList< unsigned > segments = addressByName.values( uniqueNames[name] );
		double distance = 0;
		for( unsigned segment = 0; segment < ( unsigned ) segments.size(); segment++ ) {
			const IImporter::Address segmentAddress = inputAddress[segment];
			for ( unsigned coord = 1; coord < segmentAddress.pathLength; ++coord ) {
				GPSCoordinate sourceGPS = inputWayBuffer[segmentAddress.pathID + coord - 1].ToProjectedCoordinate().ToGPSCoordinate();
				GPSCoordinate targetGPS = inputWayBuffer[segmentAddress.pathID + coord].ToProjectedCoordinate().ToGPSCoordinate();
				distance += sourceGPS.ApproximateDistance( targetGPS );
			}
		}
		wayLengths.push_back( std::pair< double, unsigned >( distance, name ) );
	}

	// sort ways by aggregate lengths
	std::sort( wayLengths.begin(), wayLengths.end() );
	std::vector< unsigned > wayImportance( uniqueNames.size() );
	for ( unsigned way = 0; way < wayLengths.size(); way++ )
		wayImportance[wayLengths[way].second] = way;
	wayLengths.clear();

	std::vector< utt::Node > subTrie( 1 );

	for ( unsigned name = 0; name < ( unsigned ) uniqueNames.size(); name++ ) {
		QList< unsigned > segments = addressByName.values( uniqueNames[name] );

		// build edge connector data structures
		std::vector< EdgeConnector< UnsignedCoordinate>::Edge > connectorEdges;
		std::vector< unsigned > resultSegments;
		std::vector< unsigned > resultSegmentDescriptions;
		std::vector< bool > resultReversed;

		for ( unsigned segment = 0; segment < ( unsigned ) segments.size(); segment++ ) {
			const IImporter::Address& segmentAddress = inputAddress[segments[segment]];
			EdgeConnector< UnsignedCoordinate >::Edge newEdge;
			newEdge.source = inputWayBuffer[segmentAddress.pathID];
			newEdge.target = inputWayBuffer[segmentAddress.pathID + segmentAddress.pathLength - 1];
			newEdge.reverseable = true;
			connectorEdges.push_back( newEdge );
		}

		EdgeConnector< UnsignedCoordinate >::run( &resultSegments, &resultSegmentDescriptions, &resultReversed, connectorEdges );

		// string places with the same name together
		unsigned nextID = 0;
		for ( unsigned segment = 0; segment < resultSegments.size(); segment++ ) {
			utt::Data subEntry;
			subEntry.start = wayBuffer.size();

			for ( unsigned description = 0; description < resultSegments[segment]; description++ ) {
				unsigned segmentID = resultSegmentDescriptions[nextID + description];
				const IImporter::Address& segmentAddress = inputAddress[segments[segmentID]];
				std::vector< UnsignedCoordinate > path;
				for ( unsigned pathID = 0; pathID < segmentAddress.pathLength; pathID++ )
					path.push_back( inputWayBuffer[pathID + segmentAddress.pathID]);
				if ( resultReversed[segmentID] )
					std::reverse( path.begin(), path.end() );
				int skipFirst = description == 0 ? 0 : 1;
				assert( skipFirst == 0 || wayBuffer.back() == path.front() );
				wayBuffer.insert( wayBuffer.end(), path.begin() + skipFirst, path.end() );
			}
			
			utt::PlaceData placeData;
			placeData.name = inputPlaces[inputAddress[segments[resultSegmentDescriptions[nextID]]].nearPlace].name;

			subEntry.length = wayBuffer.size() - subEntry.start;
			insert( &subTrie, wayImportance[name], inputWayNames[uniqueNames[name]], subEntry, placeData );

			nextID += resultSegments[segment];
		}
	}

	writeTrie( &subTrie, subTrieFile );

	assert( address == inputAddress.size() );
	qDebug() << "Unicode Tournament Trie: build tries and tournament trees:" << time.restart() << "ms";

	for ( std::vector< UnsignedCoordinate >::const_iterator i = wayBuffer.begin(), e = wayBuffer.end(); i != e; ++i ) {
		wayFile.write( ( char* ) &i->x, sizeof( i->x ) );
		wayFile.write( ( char* ) &i->y, sizeof( i->y ) );
	}
	qDebug() << "Unicode Tournament Trie: wrote ways:" << time.restart() << "ms";

	return true;
}

void UnicodeTournamentTrie::insert( std::vector< utt::Node >* trie, unsigned importance, const QString& name, utt::Data data, utt::PlaceData placeData )
{
	unsigned node = 0;
	QString lowerName = name.toLower();
	int position = 0;
	bool moreImportant = false;
	while ( position < lowerName.length() ) {
		bool found = false;
		for ( int c = 0; c < ( int ) trie->at( node ).labelList.size(); c++ ) {
			utt::Label& label = (*trie)[node].labelList[c];
			if ( label.string[0] == lowerName[position] ) {
				int diffPos = 0;
				int minLength = std::min( label.string.length(), lowerName.length() - position );
				for ( ; diffPos < minLength; diffPos++ )
					if ( label.string[diffPos] != lowerName[position + diffPos] )
						break;

				if ( diffPos != label.string.length() ) {
					utt::Label newEdge;
					newEdge.importance = label.importance;
					newEdge.index = label.index;
					newEdge.string = label.string.mid( diffPos );

					label.string = label.string.left( diffPos );
					label.index = trie->size();
					node = label.index;

					if ( label.importance < importance ) {
						label.importance = importance;
						moreImportant = true;
					}

					trie->push_back( utt::Node() ); //invalidates label reference!!!
					trie->back().labelList.push_back( newEdge );
				} else {
					node = label.index;
					if ( label.importance < importance ) {
						label.importance = importance;
						moreImportant = true;
					}
				}

				position += diffPos;
				found = true;
				break;
			}
		}

		if ( position == lowerName.length() )
			found = true;

		if ( !found ) {
			utt::Label label;
			label.string = lowerName.mid( position );
			label.index = trie->size();
			label.importance = importance;
			(*trie)[node].labelList.push_back( label );

			node = trie->size();
			trie->push_back( utt::Node() );
			break;
		}
	}

	if ( !moreImportant ) {
		(*trie)[node].dataList.push_back( data );
		(*trie)[node].placeDataList.push_back( placeData );
	} else {
		(*trie)[node].dataList.insert( (*trie)[node].dataList.begin(), data );
		(*trie)[node].placeDataList.insert( (*trie)[node].placeDataList.begin(), placeData );
	}
}

void UnicodeTournamentTrie::writeTrie( std::vector< utt::Node >* trie, QFile& file )
{
	if ( trie->size() == 0 )
		return;

	size_t position = 0;
	std::vector< unsigned > index( trie->size() );
	std::vector< unsigned > stack;
	std::vector< unsigned > order;
	stack.push_back( 0 );
	while ( !stack.empty() ) {
		unsigned node = stack.back();
		stack.pop_back();
		order.push_back( node );

		index[node] = position;
		position += (*trie)[node].GetSize();

		std::sort( (*trie)[node].labelList.begin(), (*trie)[node].labelList.end() );
		for ( int i = (*trie)[node].labelList.size() - 1; i >= 0; i-- )
			stack.push_back( (*trie)[node].labelList[i].index );
	}

	for ( int i = 0; i < ( int ) trie->size(); i++ ) {
		for ( int c = 0; c < ( int ) (*trie)[i].labelList.size(); c++ ) {
			(*trie)[i].labelList[c].index = index[(*trie)[i].labelList[c].index];
		}
	}
	assert( order.size() == trie->size() );

	char* buffer = new char[position];

	position = 0;
	for ( int i = 0; i < ( int ) order.size(); i++ ) {
		unsigned node = order[i];
		(*trie)[node].Write( buffer + position );
		utt::Node testElement;
		testElement.Read( buffer + position );
		assert( testElement == (*trie)[node] );
		position += (*trie)[node].GetSize();
	}
	file.write( buffer, position );

	delete[] buffer;
}

#ifndef NOGUI
bool UnicodeTournamentTrie::GetSettingsWindow( QWidget** window )
{
	*window =  new UTTSettingsDialog();
	return true;
}

bool UnicodeTournamentTrie::FillSettingsWindow( QWidget* /*window*/ )
{
	return true;
}

bool UnicodeTournamentTrie::ReadSettingsWindow( QWidget* /*window*/ )
{
	return true;
}

#endif
