#include "../simdebug.h"

#include "bruecke_besch.h"



/*
 *  member function:
 *      bruecke_besch_t::gib_simple()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index f�r einfaches Br�ckenst�ck bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_simple(ribi_t::ribi ribi)
{
    return (ribi & ribi_t::nordsued) ? NS_Segment : OW_Segment;
}


// dito for pillars
bruecke_besch_t::img_t bruecke_besch_t::gib_pillar(ribi_t::ribi ribi)
{
    return (ribi & ribi_t::nordsued) ? NS_Pillar : OW_Pillar;
}


/*
 *  member function:
 *      bruecke_besch_t::gib_start()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index f�r klassischen Hangstart �ck bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_start(ribi_t::ribi ribi)
{
    switch(ribi) {
    case ribi_t::nord:	return N_Start;
    case ribi_t::sued:	return S_Start;
    case ribi_t::ost:	return O_Start;
    case ribi_t::west:	return W_Start;
    default:		return (img_t)-1;
    }
}


/*
 *  member function:
 *      bruecke_besch_t::gib_rampe()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Richtigen Index f�r Rampenstart �ck bestimmen
 *
 *  Return type:
 *      bruecke_besch_t::img_t
 *
 *  Argumente:
 *      ribi_t::ribi ribi
 */
bruecke_besch_t::img_t bruecke_besch_t::gib_rampe(ribi_t::ribi ribi)
{
    switch(ribi) {
    case ribi_t::nord:	return N_Rampe;
    case ribi_t::sued:	return S_Rampe;
    case ribi_t::ost:	return O_Rampe;
    case ribi_t::west:	return W_Rampe;
    default:		return (img_t)-1;
    }
}
