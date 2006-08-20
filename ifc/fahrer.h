/* fahrer.h
 *
 * Interface f�r Verbindung von Fahrzeugen mit der Route
 *
 * 15.01.00, Hj. Malthaner
 */

#ifndef fahrer_h
#define fahrer_h

#ifndef boden_wege_weg_h
#include "../boden/wege/weg.h"
#endif

class grund_t;

/**
 * Interface f�r Verbindung von Fahrzeugen mit der Route.
 *
 * @author Hj. Malthaner, 15.01.00
 * @version $Revision: 1.8 $
 */
class fahrer_t
{
public:
    virtual bool ist_befahrbar(const grund_t* ) const = 0;

    /**
     * Ermittelt die f�r das Fahrzeug geltenden Richtungsbits,
     * abh�ngig vom Untergrund.
     *
     * @author Hj. Malthaner, 03.01.01
     */
    virtual ribi_t::ribi gib_ribi(const grund_t* ) const = 0;

    virtual weg_t::typ gib_wegtyp() const = 0;
};


#endif
