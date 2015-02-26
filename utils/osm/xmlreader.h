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

#ifndef XMLREADER_H
#define XMLREADER_H

#include "ientityreader.h"
#include "bz2input.h"
#include <libxml/xmlreader.h>
#include <clocale>
#include <QHash>

class XMLReader: public IEntityReader {

public:

	XMLReader()
	{
            m_oldLocale = setlocale( LC_NUMERIC, NULL );
            setlocale( LC_NUMERIC, "C" );
	}

	virtual bool open( QString filename )
	{
		if ( filename.endsWith( ".bz2" ) )
			m_inputReader = getBz2Reader( filename.toUtf8().constData() );
		else
			m_inputReader = xmlNewTextReaderFilename( filename.toUtf8().constData() );

		if ( m_inputReader == NULL ) {
			qCritical() << "failed to open XML reader";
			return false;
		}

		return true;
	}

	virtual ~XMLReader()
	{
		if ( m_inputReader != NULL )
			xmlFreeTextReader( m_inputReader );
                setlocale( LC_NUMERIC, m_oldLocale );
	}

	virtual void setNodeTags( QStringList tags )
	{
		for ( int i = 0; i < tags.size(); i++ )
			m_nodeTags.insert( tags[i], i );
	}

	virtual void setWayTags( QStringList tags )
	{
		for ( int i = 0; i < tags.size(); i++ )
			m_wayTags.insert( tags[i], i );
	}

	virtual void setRelationTags( QStringList tags )
	{
		for ( int i = 0; i < tags.size(); i++ )
			m_relationTags.insert( tags[i], i );
	}

	virtual EntityType getEntitiy( Node* node, Way* way, Relation* relation )
	{
		assert( node != NULL );
		assert( way != NULL );
		assert( relation != NULL );

		while ( xmlTextReaderRead( m_inputReader ) == 1 ) {
			const int type = xmlTextReaderNodeType( m_inputReader );

			if ( type != 1 ) // 1 is Element
				continue;

			xmlChar* currentName = xmlTextReaderName( m_inputReader );
			if ( currentName == NULL )
				continue;

			if ( xmlStrEqual( currentName, ( const xmlChar* ) "node" ) == 1 ) {
				readNode( node );
				xmlFree( currentName );
				return EntityNode;
			}

			if ( xmlStrEqual( currentName, ( const xmlChar* ) "way" ) == 1 ) {
				readWay( way );
				xmlFree( currentName );
				return EntityWay;
			}

			if ( xmlStrEqual( currentName, ( const xmlChar* ) "relation" ) == 1 ) {
				readRelation( relation );
				xmlFree( currentName );
				return EntityRelation;
			}

			xmlFree( currentName );
		}

		return EntityNone;
	}

protected:

	void readNode( Node* node )
	{
		node->tags.clear();
		xmlChar* attribute = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "id" );
		if ( attribute != NULL ) {
			node->id =  atoi( ( const char* ) attribute );
			xmlFree( attribute );
		}
		attribute = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "lat" );
		if ( attribute != NULL ) {
			node->coordinate.latitude =  atof( ( const char* ) attribute );
			xmlFree( attribute );
		}
		attribute = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "lon" );
		if ( attribute != NULL ) {
			node->coordinate.longitude =  atof( ( const char* ) attribute );
			xmlFree( attribute );
		}

		if ( xmlTextReaderIsEmptyElement( m_inputReader ) != 1 ) {
			const int depth = xmlTextReaderDepth( m_inputReader );

			while ( xmlTextReaderRead( m_inputReader ) == 1 ) {
				const int childType = xmlTextReaderNodeType( m_inputReader );

				if ( childType != 1 && childType != 15 ) // element begin / end
					continue;

				const int childDepth = xmlTextReaderDepth( m_inputReader );
				xmlChar* childName = xmlTextReaderName( m_inputReader );

				if ( childName == NULL )
					continue;

				if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "node" ) == 1 ) {
					xmlFree( childName );
					break;
				}

				if ( childType != 1 ) {
					xmlFree( childName );
					continue;
				}

				if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
					xmlChar* key = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "k" );
					xmlChar* value = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "v" );

					if ( key != NULL && value != NULL ) {
						int tagID = m_nodeTags.value( QString( ( const char* ) key ), -1 );
						if ( tagID != -1 ) {
							Tag tag;
							tag.key = tagID;
							tag.value = QString::fromUtf8( ( const char* ) value );
							node->tags.push_back( tag );
						}
					}

					if ( key != NULL )
						xmlFree( key );
					if ( value != NULL )
						xmlFree( value );
				}

				xmlFree( childName );
			}
		}
	}

	void readWay( Way* way )
	{
		way->tags.clear();
		way->nodes.clear();

		xmlChar* attribute = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "id" );
		if ( attribute != NULL ) {
			way->id =  atoi( ( const char* ) attribute );
			xmlFree( attribute );
		}

		if ( xmlTextReaderIsEmptyElement( m_inputReader ) != 1 ) {
			const int depth = xmlTextReaderDepth( m_inputReader );

			while ( xmlTextReaderRead( m_inputReader ) == 1 ) {
				const int childType = xmlTextReaderNodeType( m_inputReader );

				if ( childType != 1 && childType != 15 ) // element begin / end
					continue;

				const int childDepth = xmlTextReaderDepth( m_inputReader );
				xmlChar* childName = xmlTextReaderName( m_inputReader );

				if ( childName == NULL )
					continue;

				if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "way" ) == 1 ) {
					xmlFree( childName );
					break;
				}

				if ( childType != 1 ) {
					xmlFree( childName );
					continue;
				}

				if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
					xmlChar* key = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "k" );
					xmlChar* value = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "v" );

					if ( key != NULL && value != NULL ) {
						int tagID = m_wayTags.value( QString( ( const char* ) key ), -1 );
						if ( tagID != -1 ) {
							Tag tag;
							tag.key = tagID;
							tag.value = QString::fromUtf8( ( const char* ) value );
							way->tags.push_back( tag );
						}
					}

					if ( key != NULL )
						xmlFree( key );
					if ( value != NULL )
						xmlFree( value );
				} else if ( xmlStrEqual( childName, ( const xmlChar* ) "nd" ) == 1 ) {
					xmlChar* ref = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "ref" );
					if ( ref != NULL ) {
						way->nodes.push_back( atoi( ( const char* ) ref ) );
						xmlFree( ref );
					}
				}

				xmlFree( childName );
			}
		}
	}

	void readRelation( Relation* relation )
	{
		relation->tags.clear();

		xmlChar* attribute = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "id" );
		if ( attribute != NULL ) {
			relation->id =  atoi( ( const char* ) attribute );
			xmlFree( attribute );
		}

		if ( xmlTextReaderIsEmptyElement( m_inputReader ) != 1 ) {
			const int depth = xmlTextReaderDepth( m_inputReader );

			while ( xmlTextReaderRead( m_inputReader ) == 1 ) {
				const int childType = xmlTextReaderNodeType( m_inputReader );

				if ( childType != 1 && childType != 15 ) // element begin / end
					continue;

				const int childDepth = xmlTextReaderDepth( m_inputReader );
				xmlChar* childName = xmlTextReaderName( m_inputReader );

				if ( childName == NULL )
					continue;

				if ( depth == childDepth && childType == 15 && xmlStrEqual( childName, ( const xmlChar* ) "relation" ) == 1 ) {
					xmlFree( childName );
					break;
				}

				if ( childType != 1 ) {
					xmlFree( childName );
					continue;
				}

				if ( xmlStrEqual( childName, ( const xmlChar* ) "tag" ) == 1 ) {
					xmlChar* key = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "k" );
					xmlChar* value = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "v" );

					if ( key != NULL && value != NULL ) {
						int tagID = m_relationTags.value( QString( ( const char* ) key ), -1 );
						if ( tagID != -1 ) {
							Tag tag;
							tag.key = tagID;
							tag.value = QString::fromUtf8( ( const char* ) value );
							relation->tags.push_back( tag );
						}
					}

					if ( key != NULL )
						xmlFree( key );
					if ( value != NULL )
						xmlFree( value );
				} else if ( xmlStrEqual( childName, ( const xmlChar* ) "member" ) == 1 ) {
					xmlChar* type = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "type" );
					xmlChar* ref = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "ref" );
					xmlChar* role = xmlTextReaderGetAttribute( m_inputReader, ( const xmlChar* ) "role" );

					if ( type != NULL && ref != NULL && role != NULL ) {
						RelationMember member;
						bool validType = false;
						if ( xmlStrEqual( type, ( const xmlChar* ) "node" ) == 1 ) {
							member.type = RelationMember::Node;
							validType = true;
						} else if ( xmlStrEqual( type, ( const xmlChar* ) "way" ) == 1 ) {
							member.type = RelationMember::Way;
							validType = true;
						} else if ( xmlStrEqual( type, ( const xmlChar* ) "relation" ) == 1 ) {
							member.type = RelationMember::Relation;
							validType = true;
						}

						if ( validType ) {
							member.ref = atoi( ( const char* ) ref );
							member.role = QString::fromUtf8( ( const char* ) role );
							relation->members.push_back( member );
						}
					}

					if ( type != NULL )
						xmlFree( type );
					if ( ref != NULL )
						xmlFree( ref );
					if ( role != NULL )
						xmlFree( role );
				}

				xmlFree( childName );
			}
		}
	}

	xmlTextReaderPtr m_inputReader;
	QHash< QString, int > m_nodeTags;
	QHash< QString, int > m_wayTags;
	QHash< QString, int > m_relationTags;

        const char* m_oldLocale;
};

#endif // XMLREADER_H
