#ifndef gui_ifc_selection_listener_h
#define gui_ifc_selection_listener_h


class gui_komponente_t;

/**
 * Interface f�r Lauscher auf Komponenten mit selektierbaren Eintr�gen
 * @param c die Komponente auf der gew�hlt wurde
 * @param index der ausgew�hlte Index
 * @see scrolled_list_t
 * @author Hj. Malthaner
 * @version $Revision: 1.2 $
 */
class selection_listener_t
{
public:
    virtual void eintrag_gewaehlt(gui_komponente_t *c, int index) = 0;
};

#endif
