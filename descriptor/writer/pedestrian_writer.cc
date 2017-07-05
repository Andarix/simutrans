#include <string>
#include "../../dataobj/tabfile.h"
#include "obj_node.h"
#include "text_writer.h"
#include "imagelist_writer.h"
#include "imagelist2d_writer.h"
#include "pedestrian_writer.h"


void pedestrian_writer_t::write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj)
{
	int i;

	obj_node_t node(this, 8, &parent);

	write_head(fp, node, obj);

	uint16 const distribution_weight = obj.get_int("distributionweight", 1);

	static const char* const dir_codes[] = {
		"s", "w", "sw", "se", "n", "e", "ne", "nw"
	};
	slist_tpl<std::string> keys;
	slist_tpl<slist_tpl<std::string> > keys_animated;
	std::string str;


	char buf[40];
	// test for animation images
	uint16 is_animated = 0;
	for (i = 0; i < 8; i++) {
		sprintf(buf, "image[%s][0]", dir_codes[i]);
		str = obj.get(buf);
		is_animated += !str.empty();
	}

	for (i = 0; i < 8; i++) {
		if (is_animated) {
			for (uint16 j = 0; j<500; j++) {
				keys_animated.append();
				sprintf(buf, "image[%s][%d]", dir_codes[i], j);
				str = obj.get(buf);
				printf("%s : %s\n", buf, str.c_str());
				if (str.empty()) {
					break;
				}
				keys_animated.at(i).append(str);
			}
		}
		else {
			sprintf(buf, "image[%s]", dir_codes[i]);
			str = obj.get(buf);
			keys.append(str);
		}
	}

	uint16 steps_per_frame = is_animated ? max(obj.get_int("steps_per_frame", 1), 1) : 0;

	if (is_animated) {
		imagelist2d_writer_t::instance()->write_obj(fp, node, keys_animated);
	}
	else {
		imagelist_writer_t::instance()->write_obj(fp, node, keys);
	}

	uint16 offset = obj.get_int("offset", 20);


	// Hajo: Version needs high bit set as trigger -> this is required
	//       as marker because formerly nodes were unversionend
	uint16 version = 0x8001;
	node.write_uint16(fp, version,             0);
	node.write_uint16(fp, distribution_weight, 2);
	node.write_uint16(fp, steps_per_frame,     4);
	node.write_uint16(fp, offset,              6);

	node.write(fp);
}
