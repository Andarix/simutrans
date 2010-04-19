#include "../../utils/cstring_t.h"
#include "obj_pak_exception.h"
#include "obj_writer.h"
#include "obj_node.h"
#include "../obj_besch.h"


uint32 obj_node_t::free_offset;	    // next free offset in file

#define OBJ_NODE_INFO_SIZE (10)

obj_node_t::obj_node_t(obj_writer_t* writer, int size, obj_node_t* parent)
{
	this->parent = parent;

	desc.type = writer->get_type();
	desc.children = 0;
	desc.size = size;
	write_offset = free_offset + OBJ_NODE_INFO_SIZE;	// put size of dis here!
	free_offset = write_offset + desc.size;
}


void obj_node_t::write(FILE* fp)
{
	fseek(fp, write_offset - OBJ_NODE_INFO_SIZE, SEEK_SET);
	uint32 type = endian_uint32(&desc.type);
	uint16 children = endian_uint16(&desc.children);
	uint32 size = endian_uint32(&desc.size);
	fwrite(&type, 4, 1, fp);
	fwrite(&children, 2, 1, fp);
	fwrite(&size, 4, 1, fp);
	if (parent) {
		parent->desc.children++;
	}
}


void obj_node_t::write_data(FILE* fp, const void* data)
{
	write_data_at(fp, data, 0, desc.size);
}


void obj_node_t::write_data_at(FILE* fp, const void* data, int offset, int size)
{
	if (offset < 0 || size < 0 || offset + size > desc.size) {
		char reason[1024];
		sprintf(reason, "invalid parameters (offset=%d, size=%d, obj_size=%d)", offset, size, desc.size);
		throw obj_pak_exception_t("obj_node_t", reason);
	}
	fseek(fp, write_offset + offset, SEEK_SET);
	fwrite(data, size, 1, fp);
}

void obj_node_t::write_uint8(FILE* fp, uint8 data, int offset)
{
	this->write_data_at(fp, &data, offset, 1);
}

void obj_node_t::write_uint16(FILE* fp, uint16 data, int offset)
{
	uint16 data2 = endian_uint16(&data);
	this->write_data_at(fp, &data2, offset, 2);
}

void obj_node_t::write_uint32(FILE* fp, uint32 data, int offset)
{
	uint32 data2 = endian_uint32(&data);
	this->write_data_at(fp, &data2, offset, 4);
}

