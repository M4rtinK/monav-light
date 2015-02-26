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

#include "qtilerenderer.h"
#include "utils/qthelpers.h"
#include "../../utils/osm/xmlreader.h"
#include "../../utils/osm/pbfreader.h"
#ifndef NOGUI
#include "qrsettingsdialog.h"
#endif

#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QCache>
#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <time.h>
#include <stdlib.h>

#define NEED_QTILE_WRITE //Need this before including quadtile.h
#include "quadtile.h"
#include "types.h"

// Preprocessing will be split into chunks with an aim never to require more
// than this amount of RAM. This is inexact.
static unsigned long long g_memory_target = 100*1024*1024;

//Index granularity. Index is terminated at max depth, or when a part of the
//index tree contains max_index_contents ways, whichever is the sooner.
//A significant part of the preprocessing time scales with 4^MAX_INDEX_DEPTH
//#define MAX_INDEX_DEPTH 16
#define MAX_INDEX_DEPTH 15
#define MAX_INDEX_CONTENTS 100

//Split ways that cross tiles this size at the tile boundaries.
int g_waysplit_tiledepth=13;

//Removes last 8 bits from a qtile to round it down for improved storage.
#define QTILE_LB_MASK (~(0xFFULL))
//Way segments longer than this can't be stored in the database format, and
//will need an interpolated node inserted to split them.
#define LONGEST_WAY_SEG (1<<19)

#define DB_VERSION "org.hollo.quadtile.pqdb.04"


using std::vector;
using std::map;
using std::string;
using std::list;
using std::pair;

struct stats {
	 std::map<std::string, long> bytes_used;
};
struct stats g_stats;

struct osm_subrule_t {
	 const char *val;
	 osm_type_t type;
};
struct osm_rules_t {
	 const char *key;
	 const osm_subrule_t *subrules;
};

struct osm_subrule_t waterway_rules[] = {
  {"stream",           WATERWAY},
  {"river",            WATERWAY},
  {"canal",            WATERWAY},
  {"ditch",            WATERWAY},
  {"drain",            WATERWAY},
  {"riverbank",        AREA_WATER},
  {0, DONE}
};
struct osm_subrule_t railway_rules[] = {
 {"rail",              RW_RAIL},
 {"light_rail",        RW_RAIL},
 {"narrow_gauge",      RW_RAIL},
 {"tram",              RW_RAIL},
 {"monorail",          RW_RAIL},
 {"subway",            RW_RAIL},
 {"preserved",         RW_RAIL},
 {0, DONE}
};
struct osm_subrule_t landuse_rules[] = {
 {"vineyard",          AREA_NATURE},
 {"cemetery",          AREA_CEMETERY},
 {"grave_yard",        AREA_CEMETERY},
 {"residential",       AREA_RESIDENTIAL},
 {"military",          AREA_MILITARY},
 {"field",             AREA_FIELD},
 {"meadow",            AREA_MEADOW},
 {"grass",             AREA_MEADOW},
 {"conservation",      AREA_COMMON},
 {"recreation_ground", AREA_COMMON},
 {"forest",            AREA_FOREST},
 {"wood",              AREA_WOOD},
 {"retail",            AREA_RETAIL},
 {"commercial",        AREA_RETAIL},
 {"industrial",        AREA_INDUSTRIAL},
 {"railway",           AREA_INDUSTRIAL},
 {0, DONE}
};
struct osm_subrule_t leisure_rules[] = {
 {"common",            AREA_COMMON},
 {"garden",            AREA_COMMON},
 {"pitch",             AREA_COMMON},
 {"park",              AREA_PARK},
 {"nature_reserve",    AREA_NATURE},
 {0, DONE}
};
struct osm_subrule_t amenity_rules[] = {
 {"parking",           AREA_PARKING},
 {0, DONE}
};
struct osm_subrule_t tourism_rules[] = {
 {"camp_site",         AREA_CAMPSITE},
 {"attraction",        AREA_CAMPSITE},
 {"picnic_site",       AREA_CAMPSITE},
 {0, DONE}
};
struct osm_subrule_t military_rules[] = {
 {"barracks",          AREA_BARRACKS},
 {"danger_area",       AREA_DANGER_AREA},
 {0, DONE}
};
struct osm_subrule_t natural_rules[] = {
 {"wood",              AREA_WOOD},
 {"water",             AREA_WATER},
 {"heath",             AREA_COMMON},
 {"field",             AREA_FIELD},
 {0, DONE}
};
struct osm_subrule_t building_rules[] = {
 {"yes",              AREA_BUILDING},
 {0, DONE}
};
struct osm_subrule_t highway_rules[] = {
 { "pedestrian",       HW_PEDESTRIAN },
 { "path",             HW_PATH },
 { "footway",          HW_FOOTWAY },
 { "steps",            HW_STEPS },
 { "bridleway",        HW_BRIDLEWAY },
 { "cycleway",         HW_CYCLEWAY },
 { "private",          HW_PRIVATE },
 { "unsurfaced",       HW_UNSURFACED },
 { "unclassified",     HW_UNCLASSIFIED },
 { "residential",      HW_RESIDENTIAL },
 { "living_street",    HW_LIVING_STREET },
 { "service",          HW_SERVICE },
 { "tertiary",         HW_TERTIARY },
 { "tertiary_link",    HW_TERTIARY },
 { "secondary",        HW_SECONDARY },
 { "secondary_link",   HW_SECONDARY },
 { "primary",          HW_PRIMARY },
 { "primary_link",     HW_PRIMARY },
 { "trunk",            HW_TRUNK },
 { "trunk_link",       HW_TRUNK },
 { "motorway",         HW_MOTORWAY },
 { "motorway_link",    HW_MOTORWAY },
 { 0, DONE }
};
struct osm_subrule_t tracktype_rules[] = {
 { "grade1",          HW_SERVICE },
 { "grade2",          HW_UNSURFACED },
 { "grade3",          HW_PATH },
 { 0, DONE }
};

struct osm_rules_t osm_rules[] = {
  {"waterway", waterway_rules},
  {"railway", railway_rules},
  {"landuse", landuse_rules},
  {"leisure", leisure_rules},
  {"amenity", amenity_rules},
  {"tourism", tourism_rules},
  {"military", military_rules},
  {"natural", natural_rules},
  {"building", building_rules},
  {"highway", highway_rules},
  {"tracktype", tracktype_rules},
  {0, 0}
};

struct projectedxy {
	 unsigned long x, y;
};

class placename {
  public:
	 enum {CITY=0, TOWN=1, VILLAGE=2, STATION=3, SUBURB=4, HAMLET=5}  type;
	 quadtile position;
	 QString name;
	 static bool sorter(const placename &p1, const placename &p2)
		{return (p1.position<p2.position);};
};

/* osm_way - a direct representation of a way in the osm file. */
class osm_way {
  public:
	osm_way() { m_type=0; nnodes=0;};
	bool valid() {
		for(int i=0; i<nnodes; i++) {
			//Check for ways that haven't had their nodes set.
			if((all_nodes.begin()+inodes+i)->y==0xFFFFFFFFUL) return false;
		}
		return true;
	};
	bool is_worth_saving(bool motorway=false);
	unsigned char type() const;
//All nodes are stored in a global vector to avoid memory fragmentation.
	static vector<struct projectedxy> all_nodes;
	int inodes, nnodes;
	long namep; //Offset to the \n terminated way name in a DB file.

	unsigned char m_type;
};
vector<struct projectedxy> osm_way::all_nodes;

/* qtile_way - an osm way split on tile boundaries with interpolated nodes
   in long segments. These are written out to temporary files shortly after
   creation, so we store nodes internally for ease of processing.*/
class qtile_way {
  public:
	qtile_way() : q(0), m_type(0) {};
	static bool create(const osm_way &w, class qtile_writer *qtw);
	bool serialise(FILE *fp);
	quadtile get_q() const {return q;};
	int get_nnodes() const {return nodes.size();};
	void print() {
		printf("%d: %llx ", (int)m_type, q);
		for(nodelist::iterator i=nodes.begin();i!=nodes.end();i++)
			printf("%lx,%lx ", i->x, i->y);
		printf("\n");
	}
  private:
	qtile_way(quadtile _q, char _type)
		: q(_q), m_type(_type){};
	qtile_way(const osm_way &w);
	void interpolate_long_ways();
	bool is_area() {return m_type<HW_PEDESTRIAN;};

//member variables
	typedef vector<struct projectedxy> nodelist;
	nodelist nodes;
	quadtile q;
	unsigned char m_type;
	long namep; //Offset to the \n terminated way name in a DB file.

//static variables
	static int splitlevel;
};
int qtile_way::splitlevel=g_waysplit_tiledepth;

/* The same as qtile_way, but, as it's used for the final processing and
   many will be loaded in memory at once for sortng, it is optimised for memory
   usage. We store all nodes in a static structure and pointers to it in
   each way. */
class qtile_way_small {
  public:
	qtile_way_small() : inodes(0), nnodes(0) {};
	bool unserialise(FILE *fp);
	static bool sorter(const qtile_way_small &qw1, const qtile_way_small &qw2)
		{return(qw1.q < qw2.q);};
	bool is_motorway();
	bool write_buf(FILE *fp);
	int buf_len();
	quadtile get_q() const {return q;};

	static void clear_all_nodes() { all_nodes.clear();};
  private:
//member variables
	quadtile q;
	unsigned char m_type;
	int inodes, nnodes;
	long namep; //Offset to the \n terminated way name in a DB file.

//static variables
	static vector<struct projectedxy> all_nodes;
};
vector<struct projectedxy> qtile_way_small::all_nodes;

/* All disk I/O.
   Writes ways to temporary files during osm import.
      Multiple temporary files are used so that each is
      small enough to load into memory for sorting.
      Ways are bucket sorted to different temporary files
      as they are written.

   During the first pass of osm import, all nodes are shown
   to this object. This allows it to guess how large the
   import will be (in order to work out how many temp files
   will be needed), and by saving a sample of nodes work
   out what area the file covers and how to assign ways
   to temporary files in roughly equal numbers.

   Finally reads the temporary files, and writes the DB and
   the index.
*/
class qtile_writer {
  public:
	qtile_writer(const QString &_dir)
		: dir(_dir), inited(false), total_nodes(0), names_fp_out(0), namep_cache(1000) {};
	void serialise(qtile_way &q);
	void write_placenames(vector<placename> &placenames);
	bool write();
	void show_node(quadtile q);
	long get_namep(const QString &name);
	static bool concat(const QString &from, const QString &to);
  private:
	bool write_index(FILE *fp, class qindexTree &qidx);
	void init();
	QString dir;
	bool inited;
	typedef vector<std::pair<QString, FILE*> > filelist;
	filelist files;
	unsigned long long total_nodes;
	vector<quadtile> sample_nodes, qtile_tmpfiles;
	vector<int> tempfile_ways, tempfile_nodes;
	FILE *names_fp_out;
	QCache<QString, long> namep_cache;
};

/* This constructor is intentionally private. A valid qtile_way meets two
   criteria - no way segments longer than LONGEST_WAY_SEG (which this function
   does), and ways split at tile zoom-size-13 boundaries. This constructor
   can only be called from qtile_way::create() which returns a list of valid
   qtile_ways created from an osm_way. */
qtile_way::qtile_way(const osm_way &w)
{
	for(int i=w.inodes; i<w.inodes+w.nnodes; i++)
		nodes.push_back(osm_way::all_nodes[i]);
	q = mux(nodes[0].x, nodes[0].y);
	m_type = w.type();
	namep = w.namep;
	interpolate_long_ways();
}

/* Insert interpolated nodes wherever the jump between two nodes is greater
	than LONGEST_WAY_SEG. */
void qtile_way::interpolate_long_ways()
{
	quadtile x, y, oldx, oldy, newx, newy;
	for(nodelist::iterator i=nodes.begin(); i!=nodes.end(); i++) {
		x = i->x; y = i->y;
		if(i!=nodes.begin()) {
			long xdiff = x-oldx, ydiff = y-oldy;
			if(labs(xdiff)>=LONGEST_WAY_SEG || labs(ydiff)>=LONGEST_WAY_SEG) {
				long xstep = (x<oldx ? 1-LONGEST_WAY_SEG : LONGEST_WAY_SEG-1);
				long ystep = (y<oldy ? 1-LONGEST_WAY_SEG : LONGEST_WAY_SEG-1);
				if(labs(xdiff) > labs(ydiff)) {
					newx = oldx + xstep;
					newy= oldy + (newx-(double)oldx) * (y-(double)oldy)
												  / (x-(double)oldx);
				} else {
					newy = oldy + ystep;
					newx=oldx + (newy-(double)oldy) * (x-(double)oldx)
												/ (y-(double)oldy);
				}
				struct projectedxy xy;
				//Round x and y individually instead of useing QTILE_LB_MASK
				xy.x = newx & (~0xFULL); xy.y = newy & (~0xFULL);
				i = nodes.insert(i, xy);
				x=newx; y=newy; //So oldx/oldy point to the new node.
			}
		}
		oldx = x; oldy = y;
	}
}


// Static. Takes a way from the osm file and pre-processes it to one or more
// qtile ways stored in result.
bool qtile_way::create(const osm_way &w, class qtile_writer *qtw)
{
	//Convert osm_way to a single qtile_way with interpolated segs as needed
	qtile_way qway(w);

	//Work out where to split the way.
	quadtile qmask = ~(0x3FFFFFFFFFFFFFFFLL >> (splitlevel*2));
	if(qway.is_area()) { 
		//We don't do this very cleverly - area may end up being stored in
		//places it isn't needed.
		quadtile minx=0, miny=0, maxx=0, maxy=0, x, y;
		for(nodelist::iterator i=qway.nodes.begin(); i!=qway.nodes.end(); i++) {
			x = i->x; y = i->y;
			if(minx==0 || x<minx) minx = x;
			if(miny==0 || y<miny) miny = y;
			if(maxx==0 || x>maxx) maxx = x;
			if(maxy==0 || y>maxy) maxy = y;
		}
		//The >>2 of qmask and splitlevel-1 shouldn't be needed but are.
		//must have got the maths wrong somewhere. FIXME
		quadtile top_left     = mux(minx, miny) & (qmask>>2);
		quadtile bottom_right = mux(maxx, maxy) & (qmask>>2);
		demux(top_left, &minx, &miny);
		demux(bottom_right, &maxx, &maxy);

		quadtile splittilesize = 1<<(31-splitlevel-1);
		//printf("  %lld %lld %lld\n", maxx-minx, maxy-miny, splittilesize);
		int xtiles = (maxx-minx)/splittilesize;
		int ytiles = (maxy-miny)/splittilesize;
		if(xtiles*ytiles>10000) {
			qDebug() << "Skipping very large area of type" << ((int) w.type())
					<< "which would cover" << xtiles*ytiles << "tiles. ("
					<< xtiles << "x" << ytiles << ")";
			return false;
		}
		for(x=minx; x<=maxx; x+=splittilesize) {
			for(y=miny; y<=maxy; y+=splittilesize) {
				quadtile newq = mux(x, y);
				qtile_way newarea(qway);
				newarea.q = newq;
				qtw->serialise(newarea);
			}
		}
	} else { //It's a way, not an area
		quadtile current_qtile = qway.q & qmask;
		nodelist::iterator current_qtile_start = qway.nodes.begin();
		for(nodelist::iterator i=qway.nodes.begin(); i!=qway.nodes.end(); i++) {
			quadtile iq = mux(i->x, i->y) & qmask;
			if(iq != current_qtile || i+1==qway.nodes.end()) {
				//The range current_qtile_start => i covers 1 tile. Store it.
				if(i==qway.nodes.begin()) { //Sanity check.
					qCritical() << "First node of way is out of qtile";
					return false;
				}
				qtile_way qway2(current_qtile, qway.m_type);
				qway2.namep = qway.namep;
				for(nodelist::iterator j=current_qtile_start; j!=i; j++)
					qway2.nodes.push_back(*j);
				qway2.nodes.push_back(*i); //Add the overlapping node.
				qtw->serialise(qway2);
				if(iq != current_qtile && i+1==qway.nodes.end()) {
					//Special case - last node is in a new tile. Need to add another qtile_way
					qtile_way qway3(iq, qway.m_type);
					qway3.nodes.push_back(*(i-1));
					qway3.nodes.push_back(*i);
					qway3.namep = qway.namep;
					qtw->serialise(qway3);
				}
				current_qtile_start = i-1; //-1 to overlap previous tile
				current_qtile = iq;
			}
		}
	}
	return true;
}

// Used to write to temporary file for unserialisation by qtile_way_small
// Format is q(8bytes) type(1byte) namep(4bytes) no_of_nodes(2bytes) nodes(8bytes each)
bool qtile_way::serialise(FILE *fp)
{
	if(fwrite(ll2buf(q), 8, 1, fp)!=1) return false;
	char buf[7];
	buf[0] = m_type;
	memcpy(buf+1, l2buf(namep), 4);
	memcpy(buf+5, l2buf(nodes.size())+2, 2);
	if(fwrite(buf, 7, 1, fp)!=1) return false;
	for(nodelist::iterator i=nodes.begin(); i!=nodes.end(); i++) {
		if(fwrite(l2buf(i->x), 4, 1, fp)!=1) return false;
		if(fwrite(l2buf(i->y), 4, 1, fp)!=1) return false;
	}
	return true;
}

bool qtile_way_small::unserialise(FILE *fp)
{
	unsigned char buf[8];
	if(fread(buf, 8, 1, fp)!=1) return false;
	q = buf2ll(buf);
	if(fread(buf, 7, 1, fp)!=1) return false;
	m_type = buf[0];
	namep = buf2l(buf+1);
	nnodes = (buf[5] << 8) | buf[6];
	inodes = all_nodes.size();
	for(int i=0; i<nnodes; i++) {
		if(fread(buf, 8, 1, fp)!=1) return false;
		struct projectedxy xy;
		xy.x = buf2l(buf);
		xy.y = buf2l(buf+4);
		if(i<=0x1FFF) all_nodes.push_back(xy);
	}
	if(nnodes > 0x1FFF) {
		qCritical() << "Too many nodes. Need to truncate way. nnodes =" << nnodes;
		nnodes=0x1FFF;
	}
	return true;
}

int qtile_way_small::buf_len()
{
	//buf is made up from:
		//Type         - 1 byte
		//No of coords - 2  bytes (low 13 bits).
		//                  High 3 bits == n == No of name pointer/info bits.
		//(name pointer/info) n bytes 
		//First coordinate - 8bytes
		//Offsets to subsequent nodes - 4 bytes each.
	int result = 1 + 2 + 8 + 4 * (nnodes-1);
	if(namep > 0xFFFFFF) result++;
	if(namep > 0xFFFF) result++;
	if(namep) result+=2;
	return result;
}

bool qtile_way_small::write_buf(FILE *fp)
{
	unsigned char buf[8];
	//Serialise type and no. of nodes
	buf[0] = m_type;
	if(nnodes > 0x1FFF) {printf("Ooops. Too many nodes %d\n", nnodes); exit(-1);}
	memcpy(buf+1, l2buf(nnodes)+2, 2);
	unsigned char namepsize;
	if(!namep) namepsize = 0;
	else if (namep<=0xFFFF) namepsize = 2;
	else if (namep<=0xFFFFFF) namepsize = 3;
	else namepsize = 4;
	buf[1] |= (namepsize << 5);
	if(namepsize) memcpy(buf+3, l2buf(namep) + 4 - namepsize, namepsize);
	if(fwrite(buf, 3 + namepsize, 1, fp)!=1) return false;

	//x/y coords of the first node.
	quadtile x, y, lastx, lasty;
	lastx = all_nodes[inodes].x; lasty = all_nodes[inodes].y;
	memcpy(buf, ll2buf((lastx<<32LL) | lasty), 8);
	if(fwrite(buf, 8, 1, fp)!=1) return false;

	//Now offsets to the subsequent nodes.
	for(int i=inodes+1; i<inodes+nnodes; i++) {
		x = all_nodes[i].x; y = all_nodes[i].y;

		long xdiff = x-lastx, ydiff = y-lasty;
		if(xdiff<-LONGEST_WAY_SEG || xdiff>LONGEST_WAY_SEG ||
			ydiff<-LONGEST_WAY_SEG || ydiff>LONGEST_WAY_SEG) {
				printf("Too long a segment %ld %ld\n", xdiff, ydiff);
		}
		xdiff += LONGEST_WAY_SEG;
		ydiff += LONGEST_WAY_SEG;
		xdiff >>= 4; ydiff >>= 4;
		memcpy(buf, l2buf(xdiff)+2, 2);
		memcpy(buf + 2, l2buf(ydiff)+2, 2);
		if(fwrite(buf, 4, 1, fp)!=1) return false;
		lastx = x; lasty = y;
	}
	return true;
}

// Return true if we are to be saved in the motorway (low zoom level) db.
bool qtile_way_small::is_motorway()
{
	 if(m_type<=HW_SECONDARY && m_type>=HW_PEDESTRIAN) return false;
	 if(m_type <= HW_PEDESTRIAN) return false;
	 if(m_type == WATERWAY) return false;
	 return true;
}


class qindexTree {
	 public:
		  qindexTree();
		  void addIndex(quadtile _q, long long _offset);
		  void addIndex_r(quadtile _q, long long _offset, int entries=1, bool deferred=false);
		  void print(FILE *fp=stdout);
		  void increase_offsets(long off) {
				if(offset!=-1) offset += off;
				for(int i=0;i<4;i++) if(child[i]) child[i]->increase_offsets(off);
		  };
		  void deleteRecursive();

	 private:
		  bool contains(quadtile testq) {
				return((testq & (~qmask))==q);
		  };
		  qindexTree(quadtile _q, quadtile _qmask, int _level, qindexTree *_parent = NULL);
		  void recursive_fill_nulls();
		  void subprint(FILE *fp, int *index);

		  qindexTree *child[4];
		  qindexTree *parent;
		  long long q, qmask, lastq;
		  int level;
		  long long offset;
		  int my_index;
		  int contents;
		  int deferred_contents;
		  static int index_entry_size;
};
int qindexTree::index_entry_size=24; //No. of bytes each index entry takes up.

/* Default constructor - used for the top node of the tree only. */
qindexTree::qindexTree()
{
	 for(int i=0; i<4; i++) child[i]=0;
	 q = 0; lastq=0; deferred_contents = 0;
	 qmask = 0x3FFFFFFFFFFFFFFFLL;
	 //binary_printf(q); printf("\n");
	 level = 0;
	 offset = -1; contents=0; my_index = -1;
	 parent = NULL;
}

/* Private constructor for adding a child node */
qindexTree::qindexTree(quadtile _q, quadtile _qmask, int _level, qindexTree *_parent)
	 : parent(_parent), q(_q), qmask(_qmask), level(_level)
{
	 for(int i=0; i<4; i++) child[i]=0;
	 offset = -1; contents = 0; my_index = -1; lastq=0; deferred_contents=0;

	 //printf("New quadtile: %llx %llx %d\n", q, qmask, level);
	 //binary_printf(q); printf("\n");
	 //binary_printf(qmask); printf("\n");
}

/* Fill any NULL children with a qindexTree structure with no ways. */
void qindexTree::recursive_fill_nulls()
{
	 if(level >= MAX_INDEX_DEPTH) return; //Maximum depth of tree.
	 for(int i=0;i<4;i++) {
		  if(!child[i]) {
				quadtile newq = q | (((long long)i) << (60-(level*2)));
				child[i] = new qindexTree(newq, qmask >> 2, level+1, this);
		  }
	 }
}

/* deletes the whole tree */
void qindexTree::deleteRecursive()
{
	if ( level >= MAX_INDEX_DEPTH )
		return;

	for ( int i = 0; i < 4; i++ ) {
		if ( child[i] != NULL )
			child[i]->deleteRecursive();
		delete child[i];
	}
}

/* Calls the recursive addIndex function if it will make a difference. */
void qindexTree::addIndex(quadtile _q, long long _offset)
{
	 quadtile _qmask = ~(0x3FFFFFFFFFFFFFFFLL >> (2*MAX_INDEX_DEPTH));
	 if((lastq & _qmask)==(_q & _qmask)) {
		  deferred_contents++;
	 } else {
		  if(deferred_contents) addIndex_r(lastq, 0, deferred_contents, true);
		  addIndex_r(_q, _offset);
		  deferred_contents = 0;
	 }
	 lastq = _q;
}

void qindexTree::addIndex_r(quadtile _q, long long _offset, int entries, bool deferred)
{
	 //if(level==0) {
		  //printf("\nNew addIndex for %llx ", _q);
		  //binary_printf(_q); printf("\n");
	 //}
	 if(!contains(_q)) {
		  printf("ERROR - asked to add an index for a qtile we don't contain\n");
		  exit(-1);
	 }

	 contents+=entries;
	 if(offset==-1) {
		  if(deferred) {printf("ERROR - addIndex optimisation broken down\n"); exit(-1);}
		  offset = _offset;
	 }
	 else if(_offset < offset && !deferred) {
		  printf("ERROR - addIndex called out of order: %llx < %llx\n", _offset, offset);
	 }

	 if(level >= MAX_INDEX_DEPTH) return; //Maximum depth of tree.

	 for(int i=0; i<4; i++) {
		  if(!child[i]) {
				quadtile newq = q | (((long long)i) << (60-(level*2)));
				child[i] = new qindexTree(newq, qmask >> 2, level+1, this);
		  }

		  if(child[i]->contains(_q)) return child[i]->addIndex_r(_q, _offset, entries, deferred);
	 }
	 printf("Error - we (%llx, %d) contain %llx, but none of our children claim it\n", q, level, _q);
	 //binary_printf(q); printf("\n"); binary_printf(_q); printf("\n");
	 exit(-1);
}

void qindexTree::print(FILE *fp)
{
	 int index=0;

	 recursive_fill_nulls();
	 subprint(NULL, &index);

	 //Would need to go to 4 bytes in storage format.
	 if(index > 255*255*255 - 3) throw("Too many index entries\n");

	 //Change offsets to be from beginning of the file (taking into account the
	 //index size).
	 long index_bytes = index_entry_size*index + 3;
	 increase_offsets(index_bytes);

	 unsigned char *s = ll2buf(index);
	 if(fwrite(s+5, 3, 1, fp)!=1) throw("Failed write\n");
	 subprint(fp, NULL);
}

/*Called in two passes. Pass 1 fp==NULL, pass 2 index==NULL.
  On pass 1 we do everything except write the file, and index the
  qindexTree structures with the order in which they will be written.
  On pass 2 we write the file, using the index of the child as the
  pointer when serialising child[4].
  index 0xFFFFFF means we have too few ways ourself, and haven't bothered
  with children. 0xFFFFFE means the child has no ways and doesn't exist.*/
void qindexTree::subprint(FILE *fp, int *index)
{
	 //if(contents==0) return;
	 if(index) {
		  my_index = *index;
		  (*index)++;
	 }
	 if(fp) {
		unsigned char *s;
		s = ll2buf(offset);
		if(offset<0 && contents) printf("Offset <0 : %lld, %d\n", offset, contents);
		if(fwrite(s, 8, 1, fp)!=1) throw("Failed write\n");
		s = ll2buf(contents);
		if(fwrite(s+4, 4, 1, fp)!=1) throw("Failed write\n");
	 }
	 if(parent && parent->contents < MAX_INDEX_CONTENTS) {
	 //Need to terminate the tree on parents contents not ours. We could
	 //contain no ways while our parent contains many thousands, so we
	 //need substantiating as an entry with 0 contents.
		  if(fp) {
				unsigned char idx_buf[3*4];
				memset(idx_buf, 255, 3*4);
				if(fwrite(idx_buf, 3, 4, fp)!=4) throw("Failed write\n");
		  }
		  return;
	 }
	 if(fp) {
		  for(int i=0;i<4; i++) {
				if(child[i]) {
					 unsigned char *s = ll2buf(child[i]->my_index);
					 if(fwrite(s+5, 3, 1, fp)!=1) throw("Failed write\n");
				} else { //Child doesn't exist
					 unsigned char idx_buf[] = {0xFF, 0xFF, 0xFE};
					 if(fwrite(idx_buf, 3, 1, fp)!=1) throw("Failed write\n");
				}
		  }
	 }
	 for(int i=0;i<4; i++) if(child[i]) child[i]->subprint(fp, index);
}

bool osm_way::is_worth_saving(bool motorway)
{
	 if(m_type==0) return false;
	 if(motorway && m_type<=HW_SECONDARY && m_type>=HW_PEDESTRIAN) return false;
	 if(motorway && m_type <= HW_PEDESTRIAN) return false; //Trial of this
	 if(motorway && m_type == WATERWAY) return false; //Trial of this
	 return true;
}

// Serialise q to one of the temporary files (in a bucket sort).
void qtile_writer::serialise(qtile_way &q)
{
	if(!inited) init();
	unsigned int i;
	for(i=0;i<qtile_tmpfiles.size() && qtile_tmpfiles[i]<q.get_q(); i++);
	q.serialise(files[i].second);
	tempfile_ways[i]++;
	tempfile_nodes[i]+=q.get_nnodes();
}

//no_of_tmpfiles is an empirical guess to end up with all ways.???.tmp files
//less than g_memory_target in size. The factor of 600 is very conservative
//(50 is probably more accurate), but there is little extra cost here.
void qtile_writer::init()
{
	inited = true;
	int no_of_tmpfiles = total_nodes * 600ULL / g_memory_target;
	if(no_of_tmpfiles<1) no_of_tmpfiles=1;
	qDebug() << "Qtile: Initialising qtile_writer for input file with" <<
				total_nodes << "nodes" << "using" << no_of_tmpfiles <<
				"temporary files with a sample size of" << sample_nodes.size();
	for(int i=0;i<no_of_tmpfiles;i++) {
		char buf[50];
		sprintf(buf, "/ways.%03d.tmp", i);
		QString filename = dir + buf;
		FILE *fp = fopen(filename.toAscii(), "w");
		files.push_back(std::pair<QString, FILE*>(filename, fp));
		tempfile_ways.push_back(0);
		tempfile_nodes.push_back(0);
	}
	std::sort(sample_nodes.begin(), sample_nodes.end());
	for(int i=1; i<no_of_tmpfiles; i++) {
		qtile_tmpfiles.push_back(sample_nodes[sample_nodes.size()*i/no_of_tmpfiles]);
	}
	//for(unsigned int i=0;i<qtile_tmpfiles.size();i++) printf("Qtile_tmpfiles cutoff @ %llx\n", qtile_tmpfiles[i]);
}

//Called once for every node in the osm file on the first pass. We use it to
//set up parameters for the bucket sort.
void qtile_writer::show_node(quadtile q)
{
	total_nodes++;
	if((rand() % 10000)==0) sample_nodes.push_back(q);
}

//Return a pointer into the name file for this name. We use a cache to
//try and save memory on common names.
long qtile_writer::get_namep(const QString &name)
{
	if(!name.size()) return 0;
	if(!names_fp_out) {
		names_fp_out = fopen((dir + "/ways.names.txt").toAscii(), "w");
		if(!names_fp_out) return 0;
		fprintf(names_fp_out, "Qtile renderer names list version 0.1\n");
	}
	long result, *lp;
	lp = namep_cache[name];
	if(lp) return *lp;
	result = ftell(names_fp_out);
	if(result==-1) return 0;
	fprintf(names_fp_out, "%s\n", name.toUtf8().constData());
	namep_cache.insert(name, new long(result));
	return result;
}

//#define USE_EXISTING_TEMP_FILES 63 //For debugging.
//Read all nodes from the temp files in turn, sort them, and write them into
//the final database.
bool qtile_writer::write()
{
#ifdef USE_EXISTING_TEMP_FILES //For debugging.
	for(int i=0;i<USE_EXISTING_TEMP_FILES;i++) {
		char buf[50];
		sprintf(buf, "/ways.%03d.tmp", i);
		QString filename = dir + buf;
		files.push_back(std::pair<QString, FILE*>(filename, NULL));
		tempfile_ways.push_back(0);
		tempfile_nodes.push_back(0);
	}
#endif
	FILE *fp_out = fopen((dir + "/ways.all.qdb").toAscii(), "w");
	FILE *mway_fp_out = fopen((dir + "/ways.motorway.qdb").toAscii(), "w");
	qindexTree qidx, mway_index;
	long long file_offset=0, mway_file_offset=0; 

	int i=0;
	for(filelist::iterator f = files.begin(); f!=files.end(); f++) {
		//Open and read into memory and sort each temporary file in turn.
		if(f->second) fclose(f->second);
		f->second = fopen(f->first.toAscii(), "r");
		qDebug() << "Qtile: opening file " << ++i << "/" << files.size();
		if(!f->second) continue;

		qtile_way_small::clear_all_nodes();
		vector<qtile_way_small> qways;
		qtile_way_small qway;
		try {
			while(qway.unserialise(f->second))
				qways.push_back(qway);
		} catch(std::bad_alloc) {
			qCritical() << "Bad alloc with " << qways.size() << " ways. Some ways will be missing from db";
		}
		qDebug() << "Qtile:   sorting " << i << "/" << files.size();
		std::sort(qways.begin(), qways.end(), qtile_way_small::sorter);
		qDebug() << "Qtile:   writing from " << i << "/" << files.size();
		//Write the sorted ways to the main, (and maybe the motorway) db.
		for(vector<qtile_way_small>::iterator qws = qways.begin(); qws!=qways.end(); qws++) {
			if(!qws->write_buf(fp_out)) continue;
			qidx.addIndex(qws->get_q(), file_offset);
			file_offset += qws->buf_len();
			if(qws->is_motorway()) {
				if(qws->write_buf(mway_fp_out)) {
					mway_index.addIndex(qws->get_q(), mway_file_offset);
					mway_file_offset += qws->buf_len();
				}
			}
		}
		qDebug() << "Qtile:   done writing from " <<  i << "/" << files.size();
		fclose(f->second);
		f->second = 0;
	}

	fclose(fp_out);
	fclose(mway_fp_out);
	if(names_fp_out) fclose(names_fp_out);

	//Delete temporary files.
	for(filelist::iterator f = files.begin(); f!=files.end(); f++) {
		if(f->second) fclose(f->second);
		QFile::remove(f->first);
	}

	//Write the indexes, and concatenate the ways onto the end.
	fp_out = fopen((dir + "/ways.all.pqdb").toAscii(), "w");
	write_index(fp_out, qidx);
	fclose(fp_out);
	fp_out = fopen((dir + "/ways.motorway.pqdb").toAscii(), "w");
	write_index(fp_out, mway_index);
	fclose(fp_out);
	concat(dir + "/ways.all.qdb", dir + "/ways.all.pqdb");
	concat(dir + "/ways.motorway.qdb", dir + "/ways.motorway.pqdb");

	//Delete temporary files.
	QFile::remove(dir + "/ways.all.qdb");
	QFile::remove(dir + "/ways.motorway.qdb");
	return true;
}

bool qtile_writer::concat(const QString &from, const QString &to)
{
	QFile infile(from), outfile(to);
	if(!infile.open(QIODevice::ReadOnly)) {
		qCritical() << "Failed to open from file: " << from;
		return false;
	}
	if(!outfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
		qCritical() << "Failed to open to file: " << to;
		return false;
	}
	while(!infile.atEnd()) outfile.write(infile.read(8196));
	return true;
}

bool qtile_writer::write_index(FILE *fp, qindexTree &qidx)
{
	qDebug() << "Qtile: writing index";
	char tmp[100];
	time_t t;
	time(&t);
	struct tm *_tm = gmtime(&t);
	sprintf(tmp, "%s depth=%d %04d-%02d-%02d\n", DB_VERSION, g_waysplit_tiledepth,
						_tm->tm_year+1900, _tm->tm_mon+1, _tm->tm_mday);
	fprintf(fp, "%s", tmp);

	qidx.increase_offsets(strlen(tmp));
	qidx.print(fp);
	qidx.deleteRecursive();
	return true;
}

unsigned char osm_way::type() const
{
	 return m_type;
}

/* *****************************************
 *             XML Parsing
 * *****************************************/
struct node {
	 int id;
	 unsigned long x, y;
};

class OSMReader {
  public:
	 OSMReader(class qtile_writer *qtw);
	 bool load(const QString &filename);
	 vector<class osm_way> &get_ways() {return ways;};
	 vector<placename> &get_places() {return placenames;};
	 void delete_ways();

  private:
	 bool load_xml(const QString &filename, int pass=0);
	 void add_node(IEntityReader::Node &node);
	 bool add_way(IEntityReader::Way &w);
	 bool get_node(int id, unsigned long *x, unsigned long *y);
	void free_all_memory();
	bool write_ways_to_temp_file();
	bool load_ways_from_temp_file();

	 static bool nodesorter(const struct node &n1, const struct node &n2);

	 vector<struct node> nodes;
	 vector<class osm_way> ways;
	typedef std::vector<pair<int,int> > nodes_to_load_t;
	nodes_to_load_t nodes_to_load;
	vector<placename> placenames;
	int pass;
	int current_ways;
	bool read_all_ways;
	int total_nodes, total_ways;
	static int ways_per_chunk; 
	class qtile_writer *m_qtw;
};
int OSMReader::ways_per_chunk = 600000;

OSMReader::OSMReader(class qtile_writer *qtw)
	: m_qtw(qtw)
{
	current_ways=0;

	//Empirically ways.size()*10 ~= all_nodes.size()
	//sizeof(class osm_way)==12bytes, all_nodes==8, nodes_to_load==8
	qDebug() << "Qtile: Reserving memory (in MB) for ways, nodes_to_load, and all_nodes" << g_memory_target * 0.07/(1024*1024) << g_memory_target * 0.465/(1024*1024) << g_memory_target * 0.465/(1024*1024);
	ways.reserve(g_memory_target * 0.07 / 12);
	nodes_to_load.reserve(g_memory_target * 0.465 / 8);
	osm_way::all_nodes.reserve(g_memory_target * 0.465 / 8);
}

void OSMReader::free_all_memory()
{
	vector<struct node>().swap( nodes );
	vector<class osm_way>().swap( ways );
	nodes_to_load_t().swap( nodes_to_load );
	vector<placename>().swap( placenames );
	vector<struct projectedxy>().swap( osm_way::all_nodes);
}

bool OSMReader::load(const QString &filename)
{
	Timer timer;
    qDebug() << "Qtile: Loading osm file:";
	int chunk=0;
	current_ways = 0;
	total_nodes = 0;
	total_ways = 0;
	read_all_ways=false;
#ifndef USE_EXISTING_TEMP_FILES
	while(!read_all_ways) {
		qDebug() << "Qtile: pass " << chunk+1;
		if(chunk) qDebug() << "Qtile:    loading " << nodes_to_load.size() << \
                           " nodes for the " << ways.size() << \
                           " ways in the previous pass";
		else qDebug() << "Qtile:    loading placenames";
		if(!load_xml(filename, 1)) return false;
		std::sort(nodes_to_load.begin(), nodes_to_load.end());
		chunk++;
	}
	qDebug() << "Qtile: pass " << chunk+1 << " (final pass - reading nodes only)";
	if(chunk) qDebug() << "Qtile:    loading " << nodes_to_load.size() << \
			" nodes for the " << ways.size() << \
			" ways in the previous pass";
	if(!load_xml(filename, 1)) return false;
#endif

	qDebug() << "Qtile: Loaded osm file: " << timer.restart() << "ms";

	qDebug() << "Qtile: Freeing memory";
	free_all_memory();
	qDebug() << "Qtile: Freed memory: " << timer.restart() << "ms";
	return true;
}

bool OSMReader::write_ways_to_temp_file()
{
	qDebug() << "Qtile:    writing ways to temp file";
	for(vector<class osm_way>::iterator way = ways.begin(); way!=ways.end();way++){
		if(way->valid()) {
			qtile_way::create(*way, m_qtw);
		}
	}
	//total_nodes += osm_way::all_nodes.size();
	//total_ways += ways.size();
	qDebug() << "Qtile:    clearing memory";
	ways.clear();
	nodes_to_load.clear();
	osm_way::all_nodes.clear();
	return true;
}

/* pass=0 does a single pass, read all nodes to memory, then read ways.
   pass=1 is called repeatedly. 
       The first time it is called it reads as many ways as it can fit in
       the reserved memory.
       Subsequent calls it reads the nodes for these ways, and writes
       the ways to a temp file. It then reads the next batch of ways. */
bool OSMReader::load_xml(const QString &filename, int _pass)
{
	pass = _pass;
	IEntityReader* reader = NULL;
	if ( filename.endsWith( "osm.bz2" ) || filename.endsWith( ".osm" ) )
		reader = new XMLReader();
	else if ( filename.endsWith( ".pbf" ) )
		reader = new PBFReader();

	if ( reader == NULL ) {
		qCritical() << "File format not supported";
		return false;
	}

	 if(!reader->open(filename)) {
		  fprintf(stderr, "Failed to open file\n");
		  delete reader;
		  return false;
	 }

	 QStringList list;
	 list.push_back("name");
	 for(osm_rules_t *rule=osm_rules; rule->subrules;rule++)
		  list.push_back(rule->key);
	 reader->setWayTags(list);
	 list.clear();
	if(pass==0 || current_ways==0) {
		list.push_back("place");
		list.push_back("name");
		list.push_back("railway");
		reader->setNodeTags(list);
	}

	IEntityReader::EntityType etype;
	IEntityReader::Node n; IEntityReader::Way w; IEntityReader::Relation r;
	int ways_seen = 0, ways_loaded=0;
	bool first_way = true;
	 do {
		  etype = reader->getEntitiy(&n, &w, &r);
		  switch(etype) {
		  case IEntityReader::EntityNode:
				add_node(n);
				break;
		  case IEntityReader::EntityWay:
				if(first_way) {
					first_way = false;
					if(placenames.size()) m_qtw->write_placenames(placenames);
					if(ways.size()) write_ways_to_temp_file();
					if(read_all_ways) {
						delete reader;
						return true;
					}
					qDebug() << "Qtile:    loading ways"
							<< current_ways << "->";
				}
				ways_seen++;
				if(ways_seen > current_ways) {
					if(!add_way(w)) {
						delete reader;
						read_all_ways = false;
						return true;
					}
					ways_loaded++;
					current_ways++;
				}
				break;
		  case IEntityReader::EntityRelation:
				//Not interested in relations.
				break;
		  case IEntityReader::EntityNone:
				break;
		  }
	} while(etype!=IEntityReader::EntityNone);
	delete reader;
	read_all_ways = true;
	return true;
}

bool OSMReader::nodesorter(const struct node &n1, const struct node &n2)
{
	 return n1.id < n2.id;
}

//Return true unless we are out of memory.
bool OSMReader::add_way(IEntityReader::Way &way)
{
	 if(way.nodes.size()<2) {
		  //printf("Excluding zero length way\n");
		  return true;
	 }
	if(ways.capacity() - ways.size() < 100) return false;
	if(osm_way::all_nodes.size() + way.nodes.size()
				- osm_way::all_nodes.capacity() < 100) return false;
	 class osm_way w;
	 w.m_type = 0;
	 std::string stats_key="";
	 // FIXME earlier code had tags in a map. MoNav's parsing delivers them
	 // in vector<Tag>. Temporary fix is to recreate the tagMap...
	 std::map<QString, QString> tagMap;
	 for(std::vector<IEntityReader::Tag>::iterator i = way.tags.begin();
				i!=way.tags.end(); i++) {
		if(!i->value.size()) continue;
		 if(i->key) tagMap[osm_rules[i->key - 1].key] = i->value;
		  else tagMap["name"] = i->value;
	 }
	 for(osm_rules_t *rules=osm_rules; rules->subrules && !w.m_type;rules++) {
		  const char *key = rules->key;
		  std::string val(tagMap[key].toAscii());
		  for(const osm_subrule_t *srules = rules->subrules;
				srules->val && !w.m_type; srules++) {
				if(val==srules->val) {
					 w.m_type = srules->type;
					 stats_key = std::string(rules->key) + ":" + std::string(tagMap[key].toAscii());
				}
		  }
	 }

	 if(!w.is_worth_saving()) return true;

	 unsigned long x, y;
	 if(!get_node(*way.nodes.begin(), &x, &y))  return true;
	w.inodes = osm_way::all_nodes.size();
	for(std::vector<unsigned int>::iterator i = way.nodes.begin();
			i != way.nodes.end(); i++) {
		//quadtile nq = get_node(*i);
		projectedxy xy;
		if(!get_node(*i, &xy.x, &xy.y)) return true;
	    if(nodes.size()==0) //We are doing a 2 pass - record the nodes we need.
			nodes_to_load.push_back(pair<int,int>(*i, osm_way::all_nodes.size()));
		osm_way::all_nodes.push_back(xy);
		w.nnodes++;
	}
    w.namep = m_qtw->get_namep(tagMap["name"]);
	ways.push_back(w);
	if((ways.size()%1000000)==0)
		qDebug() << "Qtile:       " << ways.size() << " ways with " << osm_way::all_nodes.size() << "nodes";
	return true;
}

void OSMReader::add_node(IEntityReader::Node &node)
{
	struct node n;
	n.id=node.id;
	ll2pxy(node.coordinate.latitude, node.coordinate.longitude, &n.x, &n.y);
	n.x = n.x & (~0xFULL);
	n.y = n.y & (~0xFULL);

	if(pass==0 || current_ways==0) { //Placenames on first iteration only
		m_qtw->show_node(mux(n.x, n.y));
		std::map<QString, QString> tagMap;
		for(std::vector<IEntityReader::Tag>::iterator i = node.tags.begin();
				i!=node.tags.end(); i++) {
			if(!i->value.size()) continue;
			switch(i->key) {
			case 0:
				tagMap["place"] = i->value;
				break;
			case 1:
				tagMap["name"] = i->value;
				break;
			case 2:
				tagMap["railway"] = i->value;
				break;
			}
		}
		if((tagMap["place"].size() > 0 || tagMap["railway"]=="station")
				&& tagMap["name"].size() > 0) {
			placename place;
			if(tagMap["place"]=="city") place.type = placename::CITY;
			else if(tagMap["place"]=="town") place.type = placename::TOWN;
			else if(tagMap["place"]=="village") place.type = placename::VILLAGE;
			else if(tagMap["place"]=="suburb") place.type = placename::SUBURB;
			else if(tagMap["place"]=="hamlet") place.type = placename::HAMLET;
			else if(tagMap["railway"]=="station") place.type = placename::STATION;
			else return;
			place.name = tagMap["name"];
			place.position = mux(n.x, n.y);
			placenames.push_back(place);
		}
	}

	if(pass==1) {
		nodes_to_load_t::iterator i;
		i = std::lower_bound(nodes_to_load.begin(), nodes_to_load.end(), pair<int,int>(n.id, 0));
		while(i!=nodes_to_load.end() && i->first==n.id) {
			osm_way::all_nodes[i->second].x = n.x;
			osm_way::all_nodes[i->second].y = n.y;
			i++;
		};
		return;
	}
	nodes.push_back(n);


         if((nodes.size()%1000000)==0)
                 qDebug() << nodes.size() << " nodes loaded " << (nodes.size() * sizeof(struct node) / (1024*1024)) << "MB. Capacity is " << (nodes.capacity() * sizeof(struct node) / (1024*1024)) << "MB";

}

/*This gets a node by id from the nodes list. Assume no nodes are added
  after first call to this function. On first call we sort nodes and then
  do a binary search. This uses less memory and works out faster than using
  a std::map for nodes*/
bool OSMReader::get_node(int id, unsigned long *x, unsigned long *y)
{
	static bool sorted_nodes=false;
	if(nodes.size()==0) { //Just save the id.
		*x = (unsigned long) id; *y=0xFFFFFFFFUL;
		return true;
	}
	if(!sorted_nodes) std::sort(nodes.begin(), nodes.end(), nodesorter);
	sorted_nodes=true;

	int i, bottom, top;
	bottom = 0;
	top = nodes.size();
	do {
		i = (top+bottom)/2;
		if(top-bottom==1) i=bottom;
		if(nodes[i].id==id) {
				*x = nodes[i].x; *y = nodes[i].y;
				return true;
		}
		if(top-bottom==1) {
			i=top;
			if(nodes[i].id!=id) {
				qDebug() << "Can't find node " << id;
				return false;
			}
			*x = nodes[i].x; *y = nodes[i].y;
			return true;
		}

		if(nodes[i].id<id) bottom = i;
		else top=i;

	} while(top!=bottom);
	qDebug() << "Can't find node " << id;
	return(false);
}

void OSMReader::delete_ways()
{
	ways.clear();
	osm_way::all_nodes.clear();
	std::vector<class osm_way>().swap( ways );
	std::vector<struct projectedxy>().swap( osm_way::all_nodes);
}

QtileRenderer::QtileRenderer()
{
        Q_INIT_RESOURCE(rendering_rules);
}

QtileRenderer::~QtileRenderer()
{
}

QString QtileRenderer::GetName()
{
	return "Qtile Renderer";
}

int QtileRenderer::GetFileFormatVersion()
{
	return 2;
}

QtileRenderer::Type QtileRenderer::GetType()
{
	return Renderer;
}

bool QtileRenderer::LoadSettings( QSettings* settings )
{
        m_settings.rulesFile = ":/rendering_rules/default.qrr";
	if ( settings == NULL )
		return false;
	settings->beginGroup( "Qtile Renderer" );
	m_settings.inputFile = settings->value( "input" ).toString();
	m_settings.rulesFile = settings->value( "rulesFile" ).toString();
	m_settings.unused = settings->value( "unused" ).toBool();
	settings->endGroup();
        if(m_settings.rulesFile.size()==0)
            m_settings.rulesFile = ":/rendering_rules/default.qrr";

	return true;
}

bool QtileRenderer::SaveSettings( QSettings* settings )
{
	if ( settings == NULL )
		return false;
	settings->beginGroup( "Qtile Renderer" );
	settings->setValue( "input", m_settings.inputFile );
	settings->setValue( "rulesFile", m_settings.rulesFile );
	settings->setValue( "unused", m_settings.unused );
	settings->endGroup();

	return true;
}

bool QtileRenderer::Preprocess( IImporter*, QString dir )
{
	m_qtw = new qtile_writer(dir);
	m_osr = new OSMReader(m_qtw);

	qDebug() << "Qtile renderer preprocessing";
	QString ofile_name = dir + "/rendering.qrr";
	QFile infile(m_settings.rulesFile), outfile(ofile_name);
	if(!infile.open(QIODevice::ReadOnly)) {
		qCritical() << "Failed to open rendering rules: " << m_settings.rulesFile;
		return false;
	}
	if(!outfile.open(QIODevice::WriteOnly)) {
		qCritical() << "Failed to open rendering rules output file: " << ofile_name;
		return false;
	}
	while(!infile.atEnd()) outfile.write(infile.readLine());
	infile.close(); outfile.close();
	m_osr->load(m_settings.inputFile);
	delete m_osr; //m_osr has written to m_qtw. Make memory for sorting.
	m_qtw->write();
	qDebug() << "Qtile: preprocessing finished";

	delete m_qtw;
	return true;
}

void qtile_writer::write_placenames(vector<placename> &placenames)
{
	qDebug() << "Qtile:        writing place names";
	std::sort(placenames.begin(), placenames.end(), placename::sorter);

	long file_offset = 0;
	qindexTree qidx;
	for(vector<placename>::iterator i = placenames.begin();
		i!=placenames.end(); i++) {
		int len = i->name.toUtf8().size();
		if(len>100) len=100;
		qidx.addIndex(i->position, file_offset);
		file_offset += len+10;
	}
	QString outfile = dir;
	outfile += "/places.pqdb";
	FILE *place_fp = fopen(outfile.toAscii(), "wb");
	char buf[115];
	time_t t;
	time(&t);
	struct tm *_tm = gmtime(&t);
	sprintf(buf, "%s depth=%d places %04d-%02d-%02d\n", DB_VERSION, 
		g_waysplit_tiledepth, _tm->tm_year+1900, _tm->tm_mon+1,
		_tm->tm_mday);
	fprintf(place_fp, "%s", buf);

	qidx.increase_offsets(strlen(buf));
	qidx.print(place_fp);
	qidx.deleteRecursive();

	for(vector<placename>::iterator i = placenames.begin();
		i!=placenames.end(); i++) {
		memcpy(buf, ll2buf(i->position), 8);
		int len = i->name.toUtf8().size();
		if(len>100) len=100;
		buf[8] = len>100 ? 100 : len;
		buf[9] = (unsigned char) i->type;
		strncpy(buf+10, i->name.toUtf8().constData(), 100);
		fwrite(buf, len+10, 1, place_fp);
	}
	fclose(place_fp);
	vector<placename>().swap( placenames );
}


#ifndef NOGUI
bool QtileRenderer::GetSettingsWindow( QWidget** window )
{
	*window = new QRSettingsDialog();
	return true;
}

bool QtileRenderer::FillSettingsWindow( QWidget* window )
{
	QRSettingsDialog* settings = qobject_cast< QRSettingsDialog* >( window );
	if ( settings == NULL )
		return false;

	return settings->readSettings( m_settings );
}

bool QtileRenderer::ReadSettingsWindow( QWidget* window )
{
	QRSettingsDialog* settings = qobject_cast< QRSettingsDialog* >( window );
	if ( settings == NULL )
		return false;

	return settings->fillSettings( &m_settings );
}
#endif

QString QtileRenderer::GetModuleName()
{
	return GetName();
}

bool QtileRenderer::GetSettingsList( QVector< Setting >* settings )
{
	settings->push_back( Setting( "", "qtile-input", "osm/osm.pbf input file", "filename" ) );
	settings->push_back( Setting( "", "qtile-rendering-rules", "rendering rules file", "filename" ) );
	return true;
}

bool QtileRenderer::SetSetting( int id, QVariant data )
{
	switch( id ) {
	case 0 :
		m_settings.inputFile = data.toString();
		break;
	case 1 :
		m_settings.rulesFile = data.toString();
		break;
	default:
		return false;
	}

	return true;
}

Q_EXPORT_PLUGIN2( qtilerenderer, QtileRenderer )

