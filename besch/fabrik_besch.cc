#include "fabrik_besch.h"
#include "xref_besch.h"
#include "../network/checksum.h"


void field_class_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(production_per_field);
	chk->input(storage_capacity);
	chk->input(spawn_weight);
}


void field_group_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(probability);
	chk->input(max_fields);
	chk->input(min_fields);
	chk->input(field_classes);
	for(uint16 i=0; i<field_classes; i++) {
		const field_class_desc_t *fc = get_field_class(i);
		fc->calc_checksum(chk);
	}
}


void factory_supplier_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(capacity);
	chk->input(supplier_count);
	chk->input(consumption);
	chk->input(get_ware()->get_name());
}


void factory_product_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input(capacity);
	chk->input(factor);
	chk->input(get_ware()->get_name());
}


void factory_desc_t::calc_checksum(checksum_t *chk) const
{
	chk->input((uint8)placement);
	chk->input(productivity);
	chk->input(range);
	chk->input(chance);
	chk->input(color);
	chk->input(supplier_count);
	chk->input(product_count);
	chk->input(fields);
	chk->input(pax_level);
	chk->input(electricity_producer);
	chk->input(expand_probability);
	chk->input(expand_minimum);
	chk->input(expand_range);
	chk->input(expand_times);
	chk->input(electric_boost);
	chk->input(pax_boost);
	chk->input(mail_boost);
	chk->input(electric_amount);
	chk->input(pax_demand);
	chk->input(mail_demand);

	for (uint8 i=0; i<supplier_count; i++) {
		const factory_supplier_desc_t *supp = get_supplier(i);
		supp->calc_checksum(chk);
	}

	for (uint8 i=0; i<product_count; i++) {
		const factory_product_desc_t *prod = get_product(i);
		prod->calc_checksum(chk);
	}

	const field_group_desc_t *field_group = get_field_group();
	if (field_group) {
		field_group->calc_checksum(chk);
	}

	get_building()->calc_checksum(chk);
}
