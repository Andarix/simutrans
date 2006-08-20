#ifndef dings_gebaeundefundament_h
#define dings_gebaeundefundament_h



/**
 * Geb�ude brauchen einen ebenen Untergrund. Diese Klasse liefert
 * den notwendigen Sockel, wenn Geb�ude am Hang gebaut werden.
 *
 * @version $Revision: 1.8 $
 * @author Hj. Maltahner
 */
class gebaeudefundament_t : public ding_t
{
public:
    gebaeudefundament_t(karte_t *welt, loadsave_t *file);
    gebaeudefundament_t(karte_t *welt, koord3d pos, spieler_t *sp);

    /**
     * @return Gibt den Namen des Objekts zur�ck.
     */
    const char *gib_name() const {return "Gebaeudefundament";};

    /**
     * @return Gibt den typ des Objekts zur�ck.
     */
    inline enum ding_t::typ gib_typ() const {return gebaeudefundament;};

    void zeige_info() {};  // fundamente sind fuer den Spieler teil des Gebaeudes

    /**
     * Wird nach dem Laden der Welt aufgerufen - �blicherweise benutzt
     * um das Aussehen des Dings an Boden und Umgebung anzupassen
     *
     * @author Hj. Malthaner
     */
    virtual void laden_abschliessen();


    void * operator new(size_t s);
    void operator delete(void *p);
};


#endif
