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

#ifndef BLOCKCACHE_H_INCLUDED
#define BLOCKCACHE_H_INCLUDED

#include <QFile>
#include <QHash>
#include <limits>
#include <QtDebug>

// Block must have member function / variables:
// variable id => block id
// function void load( const unsigned char* buffer )
template< class Block >
class BlockCache{

public:

	BlockCache()
	{
		m_cache = NULL;
		m_LRU = NULL;
		m_blocks = NULL;
	}

	bool load( const QString& filename, int cacheBlocks, unsigned blockSize )
	{
		m_cacheBlocks = cacheBlocks;
		m_blockSize = blockSize;
		m_inputFile.setFileName( filename );
		if ( !m_inputFile.open( QIODevice::ReadOnly | QIODevice::Unbuffered ) ) {
			qCritical() << "failed to open file:" << m_inputFile.fileName();
			return false;
		}

		m_cache = new unsigned char[( m_cacheBlocks + 1 ) * m_blockSize];
		m_LRU = new LRUEntry[m_cacheBlocks];
		m_blocks = new Block[m_cacheBlocks];

		m_firstLoaded = -1;
		m_lastLoaded = -1;
		m_loadedCount = 0;

		return true;
	}

	void unload ( )
	{
		m_inputFile.close();
		if ( m_cache != NULL )
			delete[] m_cache;
		if ( m_LRU != NULL )
			delete[] m_LRU;
		if ( m_blocks != NULL )
			delete[] m_blocks;
		m_cache = NULL;
		m_LRU = NULL;
		m_blocks = NULL;
		m_index.clear();
	}

	const Block* getBlock( unsigned block )
	{
		int cacheID = m_index.value( block, -1 );
		if ( cacheID == -1 )
			return loadBlock( block );

		useBlock( cacheID );
		return m_blocks + cacheID;
	}

private:

	const Block* loadBlock( unsigned block )
	{
		int freeBlock = m_loadedCount;
		// cache is full => select least recently used block
		if ( m_loadedCount == m_cacheBlocks ) {
			assert ( m_lastLoaded != -1 );
			freeBlock = m_lastLoaded;
			m_index.remove( m_blocks[freeBlock].id );
			useBlock( freeBlock );
		} else {
			//insert into the front of the list
			m_LRU[freeBlock].previousLoaded = -1;
			m_LRU[freeBlock].nextLoaded = m_firstLoaded;
			if ( m_firstLoaded != -1 )
				m_LRU[m_firstLoaded].previousLoaded = freeBlock;
			if ( m_lastLoaded == -1 )
				m_lastLoaded = freeBlock;
			m_firstLoaded = freeBlock;
			m_loadedCount++;
		}

		//load block
		m_inputFile.seek( ( long long ) block * m_blockSize );
		m_inputFile.read( ( char* ) m_cache + freeBlock * m_blockSize, m_blockSize );
		m_blocks[freeBlock].load( block, m_cache + freeBlock * m_blockSize );
		m_index[block] = freeBlock;

		return m_blocks + freeBlock;
	}

	void useBlock( int cacheID )
	{
		assert( m_firstLoaded != -1 );
		if ( m_firstLoaded == cacheID )
			return;

		LRUEntry& block = m_LRU[cacheID];

		//remove block from the list to put it into the front
		if ( block.nextLoaded != -1 )
			m_LRU[block.nextLoaded].previousLoaded = block.previousLoaded;
		else
			m_lastLoaded = block.previousLoaded;

		m_LRU[block.previousLoaded].nextLoaded = block.nextLoaded;

		// insert block into the front
		m_LRU[m_firstLoaded].previousLoaded = cacheID;
		block.nextLoaded = m_firstLoaded;
		block.previousLoaded = -1;
		m_firstLoaded = cacheID;
	}

	struct LRUEntry{
		int nextLoaded;
		int previousLoaded;
	};

	Block* m_blocks;
	LRUEntry* m_LRU;
	unsigned char* m_cache;
	int m_firstLoaded;
	int m_lastLoaded;
	int m_loadedCount;
	int m_cacheBlocks;
	unsigned m_blockSize;
	QFile m_inputFile;
	QHash< unsigned, int > m_index;

};

#endif // BLOCKCACHE_H_INCLUDED
