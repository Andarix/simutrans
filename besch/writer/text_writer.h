#ifndef __TEXT_WRITER_H
#define __TEXT_WRITER_H


#include <stdio.h>

#include "obj_writer.h"
#include "../objversion.h"


class cstring_t;
class obj_node_t;


class text_writer_t : public obj_writer_t {
    static text_writer_t the_instance;

    text_writer_t() { register_writer(false); }
public:
    static text_writer_t *instance() { return &the_instance; }

    virtual obj_type get_type() const { return obj_text; }
    virtual const char *get_type_name() const { return "text"; }

    void dump_node(FILE *infp, const obj_node_info_t &node);
    void write_obj(FILE *fp, obj_node_t &parent, const char *text);
};

#endif
