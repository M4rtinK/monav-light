/* tile-write.cpp. Writes openstreetmap map tiles using a series of rules
(defined near the top) and a pre-processed database file of roads, paths,
and areas arranged as a quad-tree in the same fashion as the tiles.

Current issues:
    No text support - would be easy to add.
*/

#include "../../utils/qthelpers.h"
#include <QCache>
#include <QMetaType>

#include <string>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <vector>
#include <map>
#include <algorithm>
#ifdef TILE_WRITE_MAIN
#include <iostream>
#define qCritical() std::cout
#else
#include <QDebug>
#endif

#include "types.h"
#include "tile-write.h"

#include "img_writer.h"
#include "quadtile.h"

#define DB_VERSION "org.hollo.quadtile.pqdb.04"

#ifdef PROFILELOGGING
#include <sys/time.h>
static char g_timelog_str[256];
static struct timeval g_timelog_tz, g_timelog_tz2;
#define TIMELOGINIT(A) {strncpy(g_timelog_str, (A), 256);\
                        gettimeofday(&g_timelog_tz, 0);}
#define TIMELOG(A)     {gettimeofday(&g_timelog_tz2,0); \
fprintf(stderr, "%s: %-25s %10ldms\n", g_timelog_str, (A), (g_timelog_tz2.tv_sec-g_timelog_tz.tv_sec)*1000 + g_timelog_tz2.tv_usec/1000 - g_timelog_tz.tv_usec/1000); g_timelog_tz = g_timelog_tz2;}
#else
#define TIMELOGINIT(A)
#define TIMELOG(A)
#endif

/* Logging stuff - integrate to MoNav*/
enum logLevel {LOG_VERBOSE, LOG_DEBUG, LOG_ERROR, LOG_CRITICAL};
static void Log(enum logLevel lvl, const char *fmt, ...)
{
    if(lvl < LOG_ERROR) return;
    va_list argp;

    va_start(argp, fmt);
    vfprintf(stdout, fmt, argp);
    va_end(argp);
}

struct coord {
    unsigned long x, y;
};
class Way {
  public:
    Way() {ncoords=0;};
    bool init(FILE *fp);
    void print();
    bool draw(ImgWriter &img, DrawingRules & rules,
              unsigned long tilex, unsigned long tiley,
              int zoom, int magnification, int pass, Roadnames &roadnames);
	osm_type_t type() {return _type;};
    static bool sort_by_type(Way w1, Way w2) {return w1._type<w2._type;};
    static void new_tile() {allcoords.clear();};

    static FILE *names_fp;
  private:
    osm_type_t _type;
    int coordi, ncoords;
    long namep;
    static std::vector<coord> allcoords; //Coords for all loaded ways.
};
std::vector<coord> Way::allcoords;
FILE *Way::names_fp=0;

//Initialise a way from the database. fp has been seeked to the start of the
//way we wish to read.
bool Way::init(FILE *fp)
{
    unsigned char buf[18];

	if(fread(buf, 3, 1, fp)!=1) {
		Log(LOG_ERROR, "Failure to read any of the way\n");
		return false;
	}
    _type = (osm_type_t) buf[0];
    int nqtiles = (int) (((( buf[1]) << 8) | buf[2]) & 0x3FF);
	int namepsize = ((buf[1] & 0xE0) >>5 );
	if(namepsize) {
		if(fread(buf, namepsize, 1, fp)!=1) {
			Log(LOG_ERROR, "Failed to read way header\n");
			return false;
		}
		namep = buf2l(buf, namepsize);
	}
	else namep = 0;

	if(fread(buf, 8, 1, fp)!=1) {
		Log(LOG_ERROR, "Failed to read way\n");
		return false;
	}
    coord c;
    c.x = buf2l(buf);
    c.y = buf2l(buf+4);
    coordi = allcoords.size();
    ncoords = 1;
    allcoords.push_back(c);
    
    //Add the cordinates
    for(int i=1;i<nqtiles;i++) {
        if(fread(buf, 4, 1, fp)!=1) {
            Log(LOG_ERROR, "Failed to read remainder of way\n");
            return false;
        }
        long dx = ((buf[0]<<8)  | buf[1]) << 4;
        long dy = ((buf[2]<<8)  | buf[3]) << 4;
        dx -= 1<<19; dy -= 1<<19;
        c.x += (dx);
        c.y += (dy);
        allcoords.push_back(c);
        ncoords++;
    }
    return true;
}

//Draw this way to an img tile.
bool Way::draw(ImgWriter &img, DrawingRules & rules,
              unsigned long tilex, unsigned long tiley,
              int zoom, int magnification, int pass,
              Roadnames &roadnames)
{
    int r, g, b;
    double width;
    bool polygon;
    bool use_roadname=false;

    if(pass==0 && zoom >= 17 && type() >= HW_RESIDENTIAL && type() < HW_MOTORWAY) use_roadname = true;
    roadname road;
    if(!rules.get_rule(_type, zoom, pass, &r, &g, &b, &width, &polygon)) return false;
    img.SetPen(r, g, b, width * magnification);

    int oldx=0, oldy=0;
    
    ImgWriter::coord *c=NULL;
    if(polygon) {
        c = new ImgWriter::coord[ncoords];
    }
    int j=0;
    for(std::vector<coord>::iterator i=allcoords.begin() + coordi;
        i!=allcoords.begin() + coordi + ncoords; i++){
        int newx, newy;

        //At low level zooms we just draw the start and end of each line.
        if(zoom<10 && i!=allcoords.begin() + coordi)
            i = allcoords.begin() + coordi + ncoords -1;
        //The coords are in the range 0 to 1ULL<<31
        //We want to divide by 2^31 and multiply by 2^zoom and multiply
        //by 2^8 (=256 - the no of pixels in a tile) to get to pixel
        //scale at current zoom. We do this with a shift as it's faster
        //and performance sensitive on profiling. Note we can't do
        //(i->x - tilex) >> (31-8-zoom) as if(tilex>i->x) we'll be shifting
        //a negative number. We shift each part separately, and then cast to a 
        //signed number before subtracting.
        newx = (long) ((i->x) >> (31-8-zoom)) - (long) (tilex >> (31-8-zoom));
        newy = (long) ((i->y) >> (31-8-zoom)) - (long) (tiley >> (31-8-zoom));
		  newx *= magnification;
		  newy *= magnification;
        if(use_roadname) {
			road.coords.push_back(newx);
			road.coords.push_back(newy);
        }
        if(!polygon && i!=allcoords.begin() + coordi) {
            img.DrawLine(oldx, oldy, newx, newy);
        }
        if(polygon) {c[j].x = newx; c[j++].y = newy;}
        oldx=newx; oldy=newy;
    }
    if(polygon) {
        img.FillPoly(c, ncoords);
        delete [] c;
    }
    if(use_roadname && names_fp && namep) {
		bool xlt=true, ylt=true, xgt=true, ygt=true;
		for(std::vector<int>::iterator i = road.coords.begin();
				i!=road.coords.end() && i+1!=road.coords.end(); i+=2) {
			if(*i >-100) xlt = false;
			if(*i <356) xgt = false;
			if(*(i+1) >-100) ylt = false;
			if(*(i+1) <356) ygt = false;
		}
		if(xlt || ylt || xgt || ygt) {
			//All points are off the tile in one direction. discard.
		} else {
			if(fseek(names_fp, namep, SEEK_SET)!=-1) {
				char buf[512];
				if(fgets(buf, 511, names_fp)) {
					if(strlen(buf)) buf[strlen(buf)-1] = 0;
					road.name = QString::fromUtf8(buf);
				}
			}
			roadnames.push_back(road);
			return true;
		}
	}
	return true;
}

/* DrawingRules - load from rendering.qrr */
DrawingRules::DrawingRules(const std::string &filename) :
	filename( filename )
{
    load_rules();
}

bool DrawingRules::get_rule(osm_type_t type, int zoom, int pass,
                  int *r, int *g, int *b, double *width, bool *polygon)
{
    if(pass<0 || pass>1) return false;

    for(std::vector<struct DrawingRule>::iterator rule = drawing_rules.begin();
       rule!=drawing_rules.end(); rule++) {
       if(type >= rule->type_from && type <= rule->type_to && 
           zoom>=rule->zoom_from && zoom<=rule->zoom_to) {
            if(rule->red[pass]==-1) return false;
            *r = rule->red[pass];
            *g = rule->green[pass];
            *b = rule->blue[pass];
            *width = rule->width[pass];
            *polygon = rule->polygon;
            return true;
        }
    }

    return false;
}

bool DrawingRules::load_rules()
{
    static std::map<std::string, osm_type_t> type_table;
    type_table["AREA_PARK"] = AREA_PARK;
    type_table["AREA_CAMPSITE"] = AREA_CAMPSITE;
    type_table["AREA_NATURE"] = AREA_NATURE;
    type_table["AREA_CEMETERY"] = AREA_CEMETERY;
    type_table["AREA_RESIDENTIAL"] = AREA_RESIDENTIAL;
    type_table["AREA_BARRACKS"] = AREA_BARRACKS;
    type_table["AREA_MILITARY"] = AREA_MILITARY;
    type_table["AREA_FIELD"] = AREA_FIELD;
    type_table["AREA_DANGER_AREA"] = AREA_DANGER_AREA;
    type_table["AREA_MEADOW"] = AREA_MEADOW;
    type_table["AREA_COMMON"] = AREA_COMMON;
    type_table["AREA_FOREST"] = AREA_FOREST;
    type_table["AREA_WATER"] = AREA_WATER;
    type_table["AREA_WOOD"] = AREA_WOOD;
    type_table["AREA_RETAIL"] = AREA_RETAIL;
    type_table["AREA_INDUSTRIAL"] = AREA_INDUSTRIAL;
    type_table["AREA_PARKING"] = AREA_PARKING;
    type_table["AREA_BUILDING"] = AREA_BUILDING;
    type_table["HW_PEDESTRIAN"] = HW_PEDESTRIAN;
    type_table["HW_PATH"] = HW_PATH;
    type_table["HW_FOOTWAY"] = HW_FOOTWAY;
    type_table["HW_STEPS"] = HW_STEPS;
    type_table["HW_BRIDLEWAY"] = HW_BRIDLEWAY;
    type_table["HW_CYCLEWAY"] = HW_CYCLEWAY;
    type_table["HW_PRIVATE"] = HW_PRIVATE;
    type_table["HW_UNSURFACED"] = HW_UNSURFACED;
    type_table["HW_UNCLASSIFIED"] = HW_UNCLASSIFIED;
    type_table["HW_RESIDENTIAL"] = HW_RESIDENTIAL;
    type_table["HW_LIVING_STREET"] = HW_LIVING_STREET;
    type_table["HW_SERVICE"] = HW_SERVICE;
    type_table["HW_TERTIARY"] = HW_TERTIARY;
    type_table["HW_SECONDARY"] = HW_SECONDARY;
    type_table["HW_PRIMARY"] = HW_PRIMARY;
    type_table["HW_TRUNK"] = HW_TRUNK;
    type_table["HW_MOTORWAY"] = HW_MOTORWAY;
    type_table["RW_RAIL"] = RW_RAIL;
    type_table["WATERWAY"] = WATERWAY;
    type_table["PLACE_TOWN"] = PLACE_TOWN;

    drawing_rules.clear();
    std::map<std::string, long> colourMap;
    char buf[4096];
	 FILE *fp = fopen(filename.c_str(), "r");
    if(!fp) {
		  qCritical() << "Can't find rendering rules:" << filename.c_str();
        return false;
    }
    while(fgets(buf, 4095, fp)) {
        if(strlen(buf)<1) return false;
        buf[strlen(buf)-1]=0;
        std::vector<std::string> tokens;
        tokenise(buf, tokens);
        if(tokens.size()<5) continue;
        int i=1;
        if(tokens[0] == "Colour:") {
            if(tokens.size()!=5) continue;
            int red, green, blue;
            i++;
            if(!sscanf(tokens[i++].c_str(), "%i", &red)) continue;
            if(!sscanf(tokens[i++].c_str(), "%i", &green)) continue;
            if(!sscanf(tokens[i++].c_str(), "%i", &blue)) continue;
            Log(LOG_DEBUG, "%s: %d, %d, %d\n", tokens[1].c_str(), red, green, blue);
            colourMap[tokens[1]] = (red << 16) | (green << 8)  | (blue + 1);
        }
        else if(tokens[0]=="Line:") {
            if(tokens.size()<7) continue;
            DrawingRule r;
            memset(&r, 0, sizeof(DrawingRule));
            r.red[1] = -1; //Disable 2nd pass by default.
            r.type_from = type_table[tokens[i++]];
            r.type_to   = type_table[tokens[i++]];
            r.polygon = false;
            r.zoom_from = atoi(tokens[i++].c_str());
            r.zoom_to = atoi(tokens[i++].c_str());
            std::string tmp = tokens[i++];
            if(colourMap[tmp]) {
                //Colour is stored +1 so we can store black (0) and find it.
                long col = colourMap[tmp]-1; 
                r.red[0] = col >> 16;
                r.green[0] = (col >> 8) & 0xFF;
                r.blue[0] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[0])) continue;
            }
            r.width[0] = atof(tokens[i++].c_str());
            if(i== (int) tokens.size()) {
                drawing_rules.push_back(r);
                continue;
            }
            tmp = tokens[i++];
            if(colourMap[tmp]) {
                long col = colourMap[tmp]-1; 
                r.red[1] = col >> 16;
                r.green[1] = (col >> 8) & 0xFF;
                r.blue[1] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[1])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[1])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[1])) continue;
            }
            r.width[1] = atof(tokens[i++].c_str());
            drawing_rules.push_back(r);
        }
        else if(tokens[0]=="Area:") {
            if(tokens.size()<5) continue;
            DrawingRule r;
            memset(&r, 0, sizeof(DrawingRule));
            r.type_from = type_table[tokens[i++]];
            r.type_to   = r.type_from;
            r.polygon = true;
            r.zoom_from = atoi(tokens[i++].c_str());
            r.zoom_to = atoi(tokens[i++].c_str());
            std::string tmp = tokens[i++];
            if(colourMap[tmp]) {
                //Colour is stored +1 so we can store black (0) and find it.
                long col = colourMap[tmp]-1; 
                r.red[0] = col >> 16;
                r.green[0] = (col >> 8) & 0xFF;
                r.blue[0] = (col & 0xFF);
            } else {
                if(!sscanf(tmp.c_str(), "%i", &r.red[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.green[0])) continue;
                if(!sscanf(tokens[i++].c_str(), "%i", &r.blue[0])) continue;
            }
            r.width[0] = -1;
            r.red[1] = -1;
            drawing_rules.push_back(r);
        }
    }
    fclose(fp);
    return true;
}

/* Static helper function */
void DrawingRules::tokenise(const std::string &input, std::vector<std::string> &output)
{
	 const char *s = input.c_str();
	 char* buffer = new char[strlen( s ) + 1];
	 strcpy( buffer, s );
	 const char *p = strtok(buffer, " ");
    while(p) {
        output.push_back(p);
        p = strtok(NULL, " ");
    }
	 delete buffer;
}

/* A recursive index class into the quadtile way database. */
class qindex {
friend class qindex_e;
 public:
	qindex(FILE *_fp) : fp(_fp) {};
	bool read_header();
	long long get_index(quadtile q, int _level, int *_nways);
	bool read_entry(int entry_no, int _level, class qindex_e *result);
 private:
	QCache<int, class qindex_e> cache;
	int nitems;
	int max_safe_zoom;
	FILE *fp;
	long index_start;
};

/* An entry in the qindex */
class qindex_e {
friend class qindex;
  public:
	qindex_e(qindex *_qidx) : qidx(_qidx)  {};
	bool load(int _level);
	bool get_child_id(int childno, int *id) {
		if(child[childno] >= 0xFFFFFE) return false;
		*id = child[childno];
		return true;
	};
	bool is_leaf() {
		for(int i=0;i<4;i++)
			if(child[i] < 0xFFFFFE) return false;
		return true;
	};
	static const int qindex_e_size = 24;
  private:
	class qindex *qidx;
	long child[4];
	//long long q;
	long long offset;
	int nways;
	int level;
};
const int qindex_e::qindex_e_size;

bool qindex::read_header()
{
    Log(LOG_DEBUG, "Load index\n");
    if(!fp) return false;
    char tmp[100];
    if(!fgets(tmp, 100, fp)) return false;
    if(strncmp(tmp, DB_VERSION, strlen(DB_VERSION))) {
        Log(LOG_ERROR, "Not a DB file, or wrong version\n");
        return false;
    }
    char *s=strstr(tmp, "depth=");
    if(s) max_safe_zoom = atoi(s+6);
    else {
        Log(LOG_ERROR, "Can't read maximum safe zoom\n");
        return false;
    }

    index_start = strlen(tmp)+3;
    unsigned char buf[8];
    memset(buf, 0, 8);
    if(fread(buf+5, 3, 1, fp)!=1) {
        Log(LOG_ERROR, "Failed to read index file\n");
        return false;
    }
    nitems = (int) buf2ll(buf);
    Log(LOG_DEBUG, "Index with %d items\n", nitems);
    return true;
}

//Load the entry_no index entry from the index file into result. We cache this
//using QCache and entry_no as a key.
bool qindex::read_entry(int entry_no, int _level, qindex_e *result)
{
	if(entry_no >= 0xFFFFFE) {
		Log(LOG_CRITICAL, "read_entry asked to read a non-existant entry\n");
		return false;
	}
	qindex_e * c = cache[entry_no];
	if(c) {
		*result = *c;
		return true;
	}

	result->qidx = this;
	if(fseek(fp, entry_no * qindex_e::qindex_e_size + index_start, SEEK_SET)==-1) {
		Log(LOG_ERROR, "Failed to seek to index entry\n");
		return false;
	}
	if(result->load(_level)){
		c = new qindex_e(*result);
		cache.insert(entry_no, c);
		return true;
	}
	else return false;
}

/*Load a single index entry from disk*/
bool qindex_e::load(int _level)
{
	level = _level;
	unsigned char buf[qindex_e_size];
	if(fread(buf, qindex_e_size, 1, qidx->fp)!=1) {
		Log(LOG_ERROR, "Failed read of index file\n");
		return false;
	}
	offset = buf2ll(buf);
	nways = buf2l(buf+8);

	for(int j=0;j<4;j++)
		child[j] = buf2l(buf+12+j*3, 3);
	return true;
}

//Get the offset into the DB file where we can read the first way from
//quadtile _q (masked with level - ie. the first way in the same level
//_level map tile as _q
long long qindex::get_index(quadtile _q, int _level, int *_nways)
{
	*_nways = 0;
	if(_level > max_safe_zoom) _level = max_safe_zoom;

	qindex_e qnode(this), qchild(this);
	if(!read_entry(0, 0, &qnode)) return -1;

	for(int i = 0; i <= _level; i++) {
		if(_level == qnode.level || qnode.is_leaf()) { //We have the answer
			*_nways = qnode.nways;
			return qnode.offset;
		}
		//The next two most significant binary digits of _q which we haven't yet
		//used tell us which child contains _q.
		int childno = (_q & (0x3LL << (60-i*2))) >> (60-i*2);
		int id;
		if(!qnode.get_child_id(childno, &id)) { //The child is empty
			*_nways = 0;
			return 0;
		}
		if(!read_entry(id, qnode.level+1, &qchild)) return -1;
		qnode = qchild;
	}
	Log(LOG_CRITICAL, "Ooops. Reached end of qindex::get_index\n");
	*_nways = 0;
	return 0;
}

bool QtileDb::init(const std::string &_filename)
{
	filename = _filename;
	_fp = fopen(filename.c_str(), "rb");
	if(!_fp) return false;

	qidx = new qindex(_fp);
	if(!qidx->read_header()) {
		fclose(_fp);
		_fp=NULL;
		delete qidx;
		qidx=NULL;
		return false;
	}
	Log(LOG_DEBUG, "Opened %s\n", filename.c_str());
	return true;
}

TileWriter::TileWriter( QString dir )
	 : dr(fileInDirectory( dir, "rendering.qrr" ).toLocal8Bit().constData() )
{
	img = new ImgWriter;
	db[0].init(fileInDirectory( dir, "ways.all.pqdb" ).toLocal8Bit().constData());
	db[1].init(fileInDirectory( dir, "ways.motorway.pqdb" ).toLocal8Bit().constData());
	db[2].init(fileInDirectory( dir, "places.pqdb" ).toLocal8Bit().constData());

	qRegisterMetaType<Roadnames>("Roadnames");
	Way::names_fp = fopen(fileInDirectory( dir, "ways.names.txt" ).toLocal8Bit().constData(), "rb");
}

/*Static function. Way types that are drawn similarly (eg.
HIGHWAY_RESIDENTIAL / HIGHWAY_LIVING_STREET) should have their first passes
drawn together then second passes to make the pics look right. */
bool TileWriter::need_next_pass(int type1, int type2)
{
    if(type1==type2) return false;
    if(type1 > HW_UNSURFACED && type1<HW_SECONDARY &&
       type2 > HW_UNSURFACED && type2<HW_SECONDARY) return false;
    return true;
}

//For tile x/y/zoom use the index to seek to the relavent part of the db file,
//and return in nways the number of ways needed to load. Factored out from
//draw_image() as it's also needed for place names.
bool QtileDb::query_index(int x, int y, int zoom, int *nways)
{
    if(!_fp) return false;
    //Work out which quadtile to load.
    long z = 1UL << zoom;
    double xf = ((double)x)/z;
    double yf = ((double)y)/z;
    quadtile q;
    q = xy2q(xf, yf);

    Log(LOG_VERBOSE, "Finding seek point\n");
    //Seek to the relevant point in the db.
    long long offset = qidx->get_index(q , zoom, nways);
    if(*nways==0) return true;
	if(offset==-1) return false;
	Log(LOG_DEBUG, "Offset = %llx, nways=%d\n", offset, *nways);

	if(offset==-1 || fseek(fp(), offset, SEEK_SET)==-1) {
		Log(LOG_ERROR, "Failed to find index\n");
		return false;
	}

    return true;
}

//The main entry point into the class. Draw the map tile described by x, y
//and zoom. The tile is drawn into memory and the raw image data emitted as
//a signal once drawn. If _imgname is not empty the image will be saved into
//a file (used for debugging/developing only).
bool TileWriter::draw_image(QString _imgname, int x, int y, int zoom, int magnification)
{
	Roadnames roadnames;
    TIMELOGINIT("Draw_image");
    //FILE *fp = fopen(filename.c_str(), "rb");
    int current_db = zoom>12 ? 0 : 1;
	 Log(LOG_DEBUG, "Write %s, %d, %d, %d\n", _imgname.toUtf8().constData(), x, y, zoom);

    int nways;
    if(!db[current_db].query_index(x, y, zoom, &nways)) return false;
    TIMELOG("Querying index");

    //Initialise the image.
    Log(LOG_VERBOSE, "Initialising image with %d ways\n", nways);
	 img->NewImage(256 * magnification, 256 * magnification, _imgname.toLocal8Bit().constData() );
    img->SetBG(242, 238, 232);
    TIMELOG("Image initialisation");

    //load the ways.
    Way::new_tile();
    Log(LOG_VERBOSE, "Loading ways\n");
    Way s;
    typedef std::vector<Way> WayList;
    WayList waylist;

    for(int i=0; i<nways;i++) {
        Way s;
        if(!s.init(db[current_db].fp())) continue;
        waylist.push_back(s);
    }
    TIMELOG("Loading ways");

    if(waylist.size()==0) { //We de-reference waylist.begin() later.
        Log(LOG_VERBOSE, "Empty tile\n");
        if(_imgname.size()) img->Save();
        return true;
    }

    //Sort by way-type, with least important roads first. This stops minor roads
    //intruding on the colour of major roads that they join.
    Log(LOG_VERBOSE, "Sorting ways\n");
    std::sort(waylist.begin(), waylist.end(), Way::sort_by_type);
    TIMELOG("Sorting ways");
    
    unsigned long itilex = x << (31-zoom), itiley = y << (31-zoom);
    //This is a pain. We have to do both passes of one type before moving on to
    //the next type.
    Log(LOG_VERBOSE, "Drawing ways\n");
    int current_type = waylist.begin()->type();
    std::vector<Way>::iterator cur_type_start, i, j;
    cur_type_start = waylist.begin();
    for(i=waylist.begin(); i!=waylist.end(); i++) {
        if(need_next_pass(i->type(), current_type)) { //Do the second pass.
            for(j=cur_type_start; j!=i; j++) 
                if(!j->draw(*img, dr, itilex, itiley, zoom, magnification, 1, roadnames)) break;
            current_type = i->type();
            cur_type_start = i;
        }
        i->draw(*img, dr, itilex, itiley, zoom, magnification, 0, roadnames);
    }
    //Do second pass for the last type.
    for(j=cur_type_start; j!=waylist.end(); j++)
        if(!j->draw(*img, dr, itilex, itiley, zoom, magnification, 1, roadnames)) break;
    TIMELOG("Drawing");
    
    Log(LOG_VERBOSE, "Saving img\n");
    if(_imgname.size()) img->Save();
    Log(LOG_VERBOSE, "Done\n");
    TIMELOG("Saving");
    
    //for(i=waylist.begin();i!=waylist.end();i++) delete *i;

	emit image_finished( x, y, zoom, magnification, QByteArray( ( const char* ) img->get_img_data(), 256 * magnification * 256 * magnification * 3 ), roadnames );
    TIMELOG("Emitting result");
    return true;
}

//x,y,zoom refer to an osm map tile. Return all placenames within this
//tile. The data will be drawn at actualzoom, so adjust tilex/tiley and
//select which places to draw based on this.
void TileWriter::get_placenames(int x, int y, int zoom, int actualzoom,
			std::vector<struct placename> &result)
{

    int placenamedb = 2;
    int nways;
    if(!db[placenamedb].query_index(x, y, zoom, &nways)) return;
    char buf[256];
    unsigned char ubuf[10];
    for(int i=0; i<nways;i++) {
        if(fread(ubuf, 10, 1, db[placenamedb].fp())!=1) return;

        struct placename p;
        p.type = (placename::types) ubuf[9];
        int namelen = ubuf[8];
        quadtile q = buf2ll(ubuf);
        quadtile x, y;
        demux(q, &x, &y);
        p.tilex = (double) x / (double) (1ULL<<(31-actualzoom));
        p.tiley = (double) y / (double) (1ULL<<(31-actualzoom));

        if(fread(buf, namelen, 1, db[placenamedb].fp())!=1) return;

        buf[namelen]=0;
        if(p.type>=placename::HAMLET && actualzoom<13) continue;
        if(p.type>=placename::SUBURB && actualzoom<12) continue;
        if(p.type>=placename::STATION && actualzoom<13) continue;
        if(p.type>=placename::VILLAGE && actualzoom<12) continue;
        if(p.type>=placename::TOWN && actualzoom<9) continue;

        p.name = QString::fromUtf8(buf);
        result.push_back(p);
    }
}

//Stuff for ease of profiling
//Compile with:
//   g++ -DPROFILELOGGING -DTILE_WRITE_MAIN -O3 -pg -o tile-write tile-write.cpp -lagg
//
//   expects data files in current dir. Either run with no args to time
//   building the compiled in area, 1 arg for building the whole area without any
//   time spent saving/converting image formats, or with 3 args of x, y, z (to display
//   tile) or 4 args: x,y,z,i to draw the tile i times for profiling.
#ifdef TILE_WRITE_MAIN
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

void get_tile(double lat, double lon, int zoom, int *x, int *y)
{
    double mlat = (( 1-log(tan(lat*M_PI/180) + (1/cos(lat*M_PI/180)))/M_PI)/2);
    double mlon = (lon+180.0)/360.0;
    *x = mlon * pow(2, zoom);
    *y = mlat * pow(2, zoom);
}


int main(int argc, char *argv[])
{
    TileWriter t("./");

    struct timeval tv1, tv2;
    long totalmsecs[18] = {0};
    int maxmsecs[18] = {0}, ntiles[18] = {0};
    int total_tiles=0, max_tiles=10000000, i;
    if(argc==1 || argc==2) {
        char buf[1024];
        for(int z=5; z<=16; z++) {
            sprintf(buf, "mkdir %d >/dev/null 2>&1", z); i=system(buf);
            int x1, y1, x2, y2;
            //get_tile(58, -6, z, &x1, &y1);
            //get_tile(50, 2, z, &x2, &y2);
            get_tile(51.6, -0.4, z, &x1, &y1);
            get_tile(51.2, 0.1, z, &x2, &y2);
            for(int x=x1; x<=x2; x++) {
                sprintf(buf, "mkdir %d/%d >/dev/null 2>&1", z, x); i=system(buf);
                for(int y=y1; y<=y2; y++) {
                    sprintf(buf, "%d/%d/%d.png", z, x, y);
                    printf("%s\n", buf);
                    gettimeofday(&tv1, 0);
                    t.draw_image(argc==1 ? buf : "", x, y, z, 1);
                    gettimeofday(&tv2, 0);
                    int msecs = (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
                    if(msecs > maxmsecs[z]) maxmsecs[z] = msecs;
                    totalmsecs[z] += msecs;
                    ntiles[z] ++;
                    if(++total_tiles > max_tiles) goto stats;
                }
            }
        }
        goto stats;
    } else {
        int x = atoi(argv[2]);
        int y = atoi(argv[3]);
        int z = atoi(argv[1]);

        if(argc==4) {
            t.draw_image("test.png", x, y, z, 1);
            int i = system("eog test.png");
        } else {
            int count = atoi(argv[4]);
            for(int i=0;i<count;i++) {
                t.draw_image("", x, y, z, 1);
            }
        }
    }
    return 0;

    stats:
    for(int z=0;z<18;z++) {
        if(!ntiles[z]) continue;
        printf(" Zoom lvl: %2d, n =%5d, total = %5ldms, avg = %3ldms, max = %3dms\n",
               z, ntiles[z], totalmsecs[z], totalmsecs[z]/ntiles[z], maxmsecs[z]);
    }
    return 0;
}
#endif /* TILE_WRITE_MAIN */
