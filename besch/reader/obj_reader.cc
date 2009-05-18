#include <string.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <sys/stat.h>
#include <direct.h>
#include <io.h>
#endif


// for the progress bar
#include "../../simcolor.h"
#include "../../simimg.h"
#include "../../simtypes.h"
#include "../../simgraph.h"

#include "../skin_besch.h"	// just for the logo
#include "../grund_besch.h"	// for the error message!
#include "../../simskin.h"

// for init of button images
#include "../../gui/components/gui_button.h"

// normal stuff
#include "../../dataobj/translator.h"
#include "../../dataobj/umgebung.h"

#include "../../utils/searchfolder.h"

#include "../../tpl/inthashtable_tpl.h"
#include "../../tpl/ptrhashtable_tpl.h"
#include "../../tpl/stringhashtable_tpl.h"
#include "../../simdebug.h"
#include "../../utils/cstring_t.h"

#include "../obj_besch.h"
#include "../obj_node_info.h"

#include "obj_reader.h"


inthashtable_tpl<obj_type, stringhashtable_tpl<obj_besch_t *> > obj_reader_t::loaded;
inthashtable_tpl<obj_type, stringhashtable_tpl< slist_tpl<obj_besch_t **> > > obj_reader_t::unresolved;
ptrhashtable_tpl<obj_besch_t **, int> obj_reader_t::fatals;

void obj_reader_t::register_reader()
{
	if(!obj_reader) {
		obj_reader =  new inthashtable_tpl<obj_type, obj_reader_t *>;
	}
	obj_reader->put(get_type(), this);
	//printf("This program can read %s objects\n", get_type_name());
}


bool obj_reader_t::init(const char *message)
{
	// search for skins first
	chdir( umgebung_t::program_dir );
	load( "skin/", translator::translate("Loading skins ...") );
	if(  umgebung_t::program_dir!=umgebung_t::user_dir  ) {
		chdir( umgebung_t::user_dir );
		load( "skin/", translator::translate("Loading skins ...") );
	}
	chdir( umgebung_t::program_dir );
	button_t::init_button_images();
	return true;
}


bool obj_reader_t::laden_abschliessen()
{
	resolve_xrefs();

	inthashtable_iterator_tpl<obj_type, obj_reader_t *> iter(obj_reader);

	while(iter.next()) {
DBG_MESSAGE("obj_reader_t::laden_abschliessen()","Checking %s objects...",iter.get_current_value()->get_type_name());

		if(!iter.get_current_value()->successfully_loaded()) {
			dbg->warning("obj_reader_t::laden_abschliessen()","... failed!");
			return false;
		}
	}

#if 0
	// disposing would be a great idea to save additional 256kB main memory ...
	if(skinverwaltung_t::biglogosymbol) {
		// this will crash simutrasn at zomming, nightmode , ...
		delete skinverwaltung_t::biglogosymbol->get_bild(0);
		delete skinverwaltung_t::biglogosymbol->get_bild(1);
		delete skinverwaltung_t::biglogosymbol->get_bild(2);
		delete skinverwaltung_t::biglogosymbol->get_bild(3);
	}
#endif

#if 0
	// clean screen
	if(drawing) {
		display_fillbox_wh( 0, display_get_height()/2-20, display_get_width(), display_get_height()/2+20, COL_BLACK, true );
	}
#endif

DBG_MESSAGE("obj_reader_t::init()", "done");
	button_t::init_button_images();
	return true;
}


bool obj_reader_t::load(const char *liste, const char *message)
{
	searchfolder_t find;
	cstring_t name = find.complete(liste, "dat");
	size_t i;
	const bool drawing=is_display_init();

	if(name.right(1) != "/") {
		// very old style ... (I think unused by now)

		FILE *listfp = fopen(name,"rt");
		if(listfp) {
			while(!feof(listfp)) {
				char buf[256];

				if(fgets(buf, 255, listfp) == 0) {
					continue;
				}

				buf[255] = '\0';

				if(*buf == '#') {
					continue;
				}
				i = strlen(buf);
				while(i && buf[i] < 32) {
					buf[i--] = '\0';
				}
				if(!i) {
					continue;
				}

				find.search(buf, "pak");
				for (searchfolder_t::const_iterator i = find.begin(), end = find.end(); i != end; ++i) {
					read_file(*i);
				}
			}
			fclose(listfp);
			return true;
		}
	}
	else {
		// Keine dat-file? dann ist liste ein Verzeichnis?

		// with nice progress indicator ...
		const int max=find.search(liste, "pak");
		int teilung=-7;
		for(long bit=1;  bit<max;  bit+=bit) {
			teilung ++;
		}
		if(teilung<0) {
			teilung = 0;
		}
		teilung = (2<<teilung)-1;

		if(drawing) {
			display_set_progress_text(message);
		}

		if(drawing  &&  skinverwaltung_t::biglogosymbol==NULL) {
			display_fillbox_wh( 0, 0, display_get_width(), display_get_height(), COL_BLACK, true );
			read_file(name+"symbol.BigLogo.pak");
DBG_MESSAGE("obj_reader_t::load()","big logo %p", skinverwaltung_t::biglogosymbol);
		}
		if(skinverwaltung_t::biglogosymbol) {
			const int w = skinverwaltung_t::biglogosymbol->get_bild(0)->get_pic()->w;
			const int h = skinverwaltung_t::biglogosymbol->get_bild(0)->get_pic()->h;
			int x = display_get_width()/2-w;
			int y = display_get_height()/4-w;
			if(y<0) {
				y = 1;
			}
			display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(0), x, y, 0, false, true);
			display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(1), x+w, y, 0, false, true);
			display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(2), x, y+h, 0, false, true);
			display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(3), x+w, y+h, 0, false, true);
#if 0
			display_free_all_images_above( skinverwaltung_t::biglogosymbol->get_bild_nr(0) );
#endif
		}

		if(  grund_besch_t::ausserhalb==NULL  ) {
			// defining the pak tile witdh ....
			read_file(name+"ground.Outside.pak");
			if(grund_besch_t::ausserhalb==NULL) {
				dbg->error("obj_reader_t::load()","ground.Outside.pak not found, cannot guess tile size! (driving on left will not work!)");
			}
		}

DBG_MESSAGE("obj_reader_t::load()", "reading from '%s'", (const char*)name);
		uint n = 0;
		for (searchfolder_t::const_iterator i = find.begin(), end = find.end(); i != end; ++i, n++) {
			read_file(*i);
			if ((n & teilung) == 0 && drawing) {
				display_progress(n, max);
			}
		}

		return find.begin()!=find.end();
	}
	return false;
}


void obj_reader_t::read_file(const char *name)
{
	// Hajo: added trace
	DBG_DEBUG("obj_reader_t::read_file()", "filename='%s'", name);

	FILE *fp = fopen(name, "rb");

	if(fp) {
		sint32 n = 0;

		// This is the normal header reading code
		int c;
		do {
			c = fgetc(fp);
			n ++;
		} while(!feof(fp) && c != 0x1a);

		if(feof(fp)) {
			// Hajo: added error check
			dbg->error("obj_reader_t::read_file()",	"unexpected end of file after %d bytes while reading '%s'!",n, name);
		}
		else {
//			DBG_DEBUG("obj_reader_t::read_file()", "skipped %d header bytes", n);
		}

		// Compiled Verison
		uint32 version = 0;
		char dummy[4], *p;
		p = dummy;

		n = fread(dummy, 4, 1, fp);
		version = decode_uint32(p);

		DBG_DEBUG("obj_reader_t::read_file()", "read %d blocks, file version is %x", n, version);

		if(version <= COMPILER_VERSION_CODE) {
			obj_besch_t *data = NULL;
			read_nodes(fp, data, 0 );
		}
		else {
			DBG_DEBUG("obj_reader_t::read_file()","version of '%s' is too old, %d instead of %d", version, COMPILER_VERSION_CODE, name);
		}
		fclose(fp);
	}
	else {
		// Hajo: added error check
		dbg->error("obj_reader_t::read_file()", "reading '%s' failed!", name);
	}
}


void obj_reader_t::read_nodes(FILE* fp, obj_besch_t*& data, int register_nodes)
{
	obj_node_info_t node;
	char load_dummy[8], *p;

	p = load_dummy;
	fread(p, 8, 1, fp);
	node.type = decode_uint32(p);
	node.children = decode_uint16(p);
	node.size = decode_uint16(p);

	obj_reader_t *reader = obj_reader->get(static_cast<obj_type>(node.type));
	if(reader) {

//DBG_DEBUG("obj_reader_t::read_nodes()","Reading %.4s-node of length %d with '%s'",	reinterpret_cast<const char *>(&node.type),	node.size,	reader->get_type_name());
		data = reader->read_node(fp, node);
		for(int i = 0; i < node.children; i++) {
			read_nodes(fp, data->node_info[i], register_nodes+1);
		}

//DBG_DEBUG("obj_reader_t","registering with '%s'", reader->get_type_name());
		if(register_nodes<2  ||  node.type!=obj_cursor) {
			// since many buildings are with cursors that do not need registration
			reader->register_obj(data);
		}
	}
	else {
		// no reader found ...
		dbg->warning("obj_reader_t::read_nodes()","skipping unknown %.4s-node\n",reinterpret_cast<const char *>(&node.type));
		fseek(fp, node.size, SEEK_CUR);
		for(int i = 0; i < node.children; i++) {
			skip_nodes(fp);
		}
		data = NULL;
	}
}


obj_besch_t *obj_reader_t::read_node(FILE *fp, obj_node_info_t &node)
{
	obj_besch_t* besch = new(node.size) obj_besch_t();
	besch->node_info = new obj_besch_t*[node.children];

	if(node.size>0) {
		// not 32/64 Bit compatible for everything but char!
		dbg->warning("obj_reader_t::read_node()","native called on type %.4s (size %i), will break on 64Bit if type!=ASCII",reinterpret_cast<const char *>(&node.type),node.size);
		fread(besch + 1, node.size, 1, fp);
	}

	return besch;
}


void obj_reader_t::skip_nodes(FILE *fp)
{
	obj_node_info_t node;
	char load_dummy[8], *p;

	p = load_dummy;
	fread(p, 8, 1, fp);
	node.type = decode_uint32(p);
	node.children = decode_uint16(p);
	node.size = decode_uint16(p);
//DBG_DEBUG("obj_reader_t::skip_nodes", "type %.4s (size %d)",reinterpret_cast<const char *>(&node.type),node.size);

	fseek(fp, node.size, SEEK_CUR);
	for(int i = 0; i < node.children; i++) {
		skip_nodes(fp);
	}
}


void obj_reader_t::delete_node(obj_besch_t *data)
{
	delete [] data->node_info;
	delete data;
}


void obj_reader_t::resolve_xrefs()
{
	slist_tpl<obj_besch_t *> xref_nodes;
	inthashtable_iterator_tpl<obj_type, stringhashtable_tpl<slist_tpl<obj_besch_t **> > > xreftype_iter(unresolved);

	while(xreftype_iter.next()) {
		stringhashtable_iterator_tpl<slist_tpl<obj_besch_t **> > xrefname_iter(xreftype_iter.access_current_value());

		while(xrefname_iter.next()) {
			obj_besch_t *obj_loaded = NULL;

			if(strlen(xrefname_iter.get_current_key()) > 0) {
				stringhashtable_tpl<obj_besch_t *> *objtype_loaded = loaded.access(xreftype_iter.get_current_key());
				if(objtype_loaded) {
					obj_loaded = objtype_loaded->get(xrefname_iter.get_current_key());
				}
				/*if(!objtype_loaded || !obj_loaded) {
				dbg->fatal("obj_reader_t::resolve_xrefs", "cannot resolve '%4.4s-%s'",&xreftype_iter.get_current_key(), xrefname_iter.get_current_key());
				}*/
			}

			slist_iterator_tpl<obj_besch_t **> xref_iter(xrefname_iter.access_current_value());
			while(xref_iter.next()) {
			if(!obj_loaded && fatals.get(xref_iter.get_current())) {
				dbg->fatal("obj_reader_t::resolve_xrefs", "cannot resolve '%4.4s-%s'",	&xreftype_iter.get_current_key(), xrefname_iter.get_current_key());
				}
				// delete old xref-node
				xref_nodes.append(*xref_iter.get_current());
				*xref_iter.get_current() = obj_loaded;
			}
		}
	}

	while (!xref_nodes.empty()) {
		delete_node(xref_nodes.remove_first());
	}

	loaded.clear();
	unresolved.clear();
	fatals.clear();
}


void obj_reader_t::obj_for_xref(obj_type type, const char *name, obj_besch_t *data)
{
	stringhashtable_tpl<obj_besch_t *> *objtype_loaded = loaded.access(type);

	if(!objtype_loaded) {
		loaded.put(type, stringhashtable_tpl<obj_besch_t *>());
		objtype_loaded = loaded.access(type);
	}
	if(!objtype_loaded->get(name)) {
		objtype_loaded->put(name, data);
	}
}


void obj_reader_t::xref_to_resolve(obj_type type, const char *name, obj_besch_t **dest, bool fatal)
{
	stringhashtable_tpl< slist_tpl<obj_besch_t **> > *typeunresolved = unresolved.access(type);

	if(!typeunresolved) {
		unresolved.put(type, stringhashtable_tpl< slist_tpl<obj_besch_t **> >());
		typeunresolved = unresolved.access(type);
	}
	slist_tpl<obj_besch_t **> *list = typeunresolved->access(name);
	if(!list) {
		typeunresolved->put(name, slist_tpl<obj_besch_t **>());
		list = typeunresolved->access(name);
	}
	list->insert(dest);
	if(fatal) {
		fatals.put(dest, 1);
	}
}
