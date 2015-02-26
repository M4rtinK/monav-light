#ifndef TILE_WRITE_H
#define TILE_WRITE_H

#include <string>
#include <stdio.h>
#include <vector>
#include <QObject>
#include <QString>
#include "types.h"

struct placename {
    double tilex, tiley; // fractional x/y tile coords.
    typedef enum {CITY=0, TOWN=1, VILLAGE=2, STATION=3, SUBURB=4, HAMLET=5} types;
    types type;
    QString name;
};

struct roadname {
	QString name;
	//In pixels from the top of the tile.
	std::vector<int> coords;
};
typedef std::vector<struct roadname> Roadnames;

class DrawingRules {
  public:
    struct DrawingRule {
        osm_type_t type_from; //List of types the rule
        osm_type_t type_to;   //applies to
        bool polygon;
        int zoom_from, zoom_to;
        int red[2], green[2], blue[2];
        double width[2];
    };
	 DrawingRules(const std::string &filename);
    bool get_rule(osm_type_t type, int zoom, int pass,
                  int *r, int *g, int *b, double *width, bool *polygon);
  private:
    static void tokenise(const std::string &input, std::vector<std::string> &output);
    bool load_rules();

	 std::string filename;
    std::vector<struct DrawingRule> drawing_rules;
};

class QtileDb {
public:
	QtileDb() : qidx(NULL), _fp(NULL) {};
    bool init(const std::string &_filename);
	bool query_index(int x, int y, int zoom, int *nways);
	FILE *open_next_file();
	FILE *fp() const {return(_fp);};

private:
	std::string filename;
	class qindex *qidx;
	FILE *_fp;
};

class TileWriter : public QObject {
	Q_OBJECT

public slots:

	bool draw_image( QString filename, int x, int y, int zoom, int magnification);

signals:

	void image_finished( int x, int y, int zoom, int magnification, QByteArray data, Roadnames roadnames);

public:
	TileWriter( QString dir );
	virtual ~TileWriter() {}

	void get_placenames(int x, int y, int zoom, int drawzoom, std::vector<struct placename> &result);
private:
	static bool need_next_pass(int type1, int type2);
	class ImgWriter *img;
	QtileDb db[3];
	DrawingRules dr;
};

#endif
