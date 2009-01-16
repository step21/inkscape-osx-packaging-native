/** @file
 * @brief SVG Fonts dialog - implementation
 */
/* Authors:
 *   Felipe C. da S. Sanches <felipe.sanches@gmail.com>
 *
 * Copyright (C) 2008 Authors
 * Released under GNU GPLv2 (or later).  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_SVG_FONTS

#include <2geom/pathvector.h>
#include "document-private.h"
#include <gtkmm/notebook.h>
#include <glibmm/i18n.h>
#include <message-stack.h>
#include "selection.h"
#include <string.h>
#include "svg/svg.h"
#include "svg-fonts-dialog.h"
#include "xml/node.h"
#include "xml/repr.h"

SvgFontDrawingArea::SvgFontDrawingArea(){
    this->text = "";
    this->svgfont = NULL;
}

void SvgFontDrawingArea::set_svgfont(SvgFont* svgfont){
    this->svgfont = svgfont;
}

void SvgFontDrawingArea::set_text(Glib::ustring text){
    this->text = text;
    redraw();
}

void SvgFontDrawingArea::set_size(int x, int y){
    this->x = x;
    this->y = y;
    ((Gtk::Widget*) this)->set_size_request(x, y);
}

void SvgFontDrawingArea::redraw(){
    ((Gtk::Widget*) this)->queue_draw();
}

bool SvgFontDrawingArea::on_expose_event (GdkEventExpose */*event*/){
  if (this->svgfont){
    Glib::RefPtr<Gdk::Window> window = get_window();
    Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
    cr->set_font_face( Cairo::RefPtr<Cairo::FontFace>(new Cairo::FontFace(this->svgfont->get_font_face(), false /* does not have reference */)) );
    cr->set_font_size (this->y-20);
    cr->move_to (10, 10);
    cr->show_text (this->text.c_str());
  }
  return TRUE;
}

namespace Inkscape {
namespace UI {
namespace Dialog {

/*
Gtk::HBox* SvgFontsDialog::AttrEntry(gchar* lbl, const SPAttributeEnum attr){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    Gtk::Entry* entry = Gtk::manage(new Gtk::Entry());
    hbox->add(* entry );
    hbox->show_all();

    entry->signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_attr_changed));
    return hbox;
}
*/

SvgFontsDialog::AttrEntry::AttrEntry(SvgFontsDialog* d, gchar* lbl, const SPAttributeEnum attr){
    this->dialog = d;
    this->attr = attr;
    this->add(* Gtk::manage(new Gtk::Label(lbl)) );
    this->add(entry);
    this->show_all();

    entry.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::AttrEntry::on_attr_changed));
}

void SvgFontsDialog::AttrEntry::set_text(char* t){
    if (!t) return;
    entry.set_text(t);
}

void SvgFontsDialog::AttrEntry::on_attr_changed(){

    SPObject* o = NULL;
    for(SPObject* node = this->dialog->get_selected_spfont()->children; node; node=node->next){
        switch(this->attr){
            case SP_PROP_FONT_FAMILY:
                if (SP_IS_FONTFACE(node)){
                    o = node;
                    continue;
                }
                break;
            default:
                o = NULL;
        }
    }

    const gchar* name = (const gchar*)sp_attribute_name(this->attr);
    if(name && o) {
        SP_OBJECT_REPR(o)->setAttribute((const gchar*) name, this->entry.get_text().c_str());
        o->parent->requestModified(SP_OBJECT_MODIFIED_FLAG);

        Glib::ustring undokey = "svgfonts:";
        undokey += name;
        sp_document_maybe_done(o->document, undokey.c_str(), SP_VERB_DIALOG_SVG_FONTS,
                               _("Set SVG Font attribute"));
    }

}

Gtk::HBox* SvgFontsDialog::AttrCombo(gchar* lbl, const SPAttributeEnum /*attr*/){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    hbox->add(* Gtk::manage(new Gtk::ComboBox()) );
    hbox->show_all();
    return hbox;
}

/*
Gtk::HBox* SvgFontsDialog::AttrSpin(gchar* lbl){
    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    hbox->add(* Gtk::manage(new Gtk::Label(lbl)) );
    hbox->add(* Gtk::manage(new Gtk::SpinBox()) );
    hbox->show_all();
    return hbox;
}*/

/*** SvgFontsDialog ***/

GlyphComboBox::GlyphComboBox(){
}

void GlyphComboBox::update(SPFont* spfont){
    if (!spfont) return
//TODO: figure out why do we need to append_text("") before clearing items properly...

    this->append_text(""); //Gtk is refusing to clear the combobox when I comment out this line
    this->clear_items();

    for(SPObject* node = spfont->children; node; node=node->next){
        if (SP_IS_GLYPH(node)){
            this->append_text(((SPGlyph*)node)->unicode);
        }
    }
}

void SvgFontsDialog::on_kerning_value_changed(){
    if (!this->kerning_pair) return;
    SPDocument* document = sp_desktop_document(this->getDesktop());

    //TODO: I am unsure whether this is the correct way of calling sp_document_maybe_done
    Glib::ustring undokey = "svgfonts:hkern:k:";
    undokey += this->kerning_pair->u1->attribute_string();
    undokey += ":";
    undokey += this->kerning_pair->u2->attribute_string();

    //slider values increase from right to left so that they match the kerning pair preview
    this->kerning_pair->repr->setAttribute("k", Glib::Ascii::dtostr(get_selected_spfont()->horiz_adv_x - kerning_slider.get_value()).c_str());
    sp_document_maybe_done(document, undokey.c_str(), SP_VERB_DIALOG_SVG_FONTS, _("Adjust kerning value"));

    //populate_kerning_pairs_box();
    kerning_preview.redraw();
    _font_da.redraw();
}

void SvgFontsDialog::glyphs_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _GlyphsContextMenu.popup(event->button, event->time);
    }
}

void SvgFontsDialog::kerning_pairs_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _KerningPairsContextMenu.popup(event->button, event->time);
    }
}

void SvgFontsDialog::fonts_list_button_release(GdkEventButton* event)
{
    if((event->type == GDK_BUTTON_RELEASE) && (event->button == 3)) {
        _FontsContextMenu.popup(event->button, event->time);
    }
}

void SvgFontsDialog::create_glyphs_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::ImageMenuItem(Gtk::Stock::REMOVE));
    _GlyphsContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _GlyphsContextMenu.accelerate(parent);
}

void SvgFontsDialog::create_kerning_pairs_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::ImageMenuItem(Gtk::Stock::REMOVE));
    _KerningPairsContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _KerningPairsContextMenu.accelerate(parent);
}

void SvgFontsDialog::create_fonts_popup_menu(Gtk::Widget& parent, sigc::slot<void> rem)
{
    Gtk::MenuItem* mi = Gtk::manage(new Gtk::ImageMenuItem(Gtk::Stock::REMOVE));
    _FontsContextMenu.append(*mi);
    mi->signal_activate().connect(rem);
    mi->show();
    _FontsContextMenu.accelerate(parent);
}

void SvgFontsDialog::update_sensitiveness(){
    if (get_selected_spfont()){
        global_vbox.set_sensitive(true);
        glyphs_vbox.set_sensitive(true);
        kerning_vbox.set_sensitive(true);
    } else {
        global_vbox.set_sensitive(false);
        glyphs_vbox.set_sensitive(false);
        kerning_vbox.set_sensitive(false);
    }
}

/* Add all fonts in the document to the combobox. */
void SvgFontsDialog::update_fonts()
{
    SPDesktop* desktop = this->getDesktop();
    SPDocument* document = sp_desktop_document(desktop);
    const GSList* fonts = sp_document_get_resource_list(document, "font");

    _model->clear();
    for(const GSList *l = fonts; l; l = l->next) {
        Gtk::TreeModel::Row row = *_model->append();
        SPFont* f = (SPFont*)l->data;
        row[_columns.spfont] = f;
        row[_columns.svgfont] = new SvgFont(f);
        const gchar* lbl = f->label();
        const gchar* id = SP_OBJECT_ID(f);
        row[_columns.label] = lbl ? lbl : (id ? id : "font");
    }

    update_sensitiveness();
}

void SvgFontsDialog::on_preview_text_changed(){
    _font_da.set_text((gchar*) _preview_entry.get_text().c_str());
    _font_da.set_text(_preview_entry.get_text());
}

void SvgFontsDialog::on_kerning_pair_selection_changed(){
    SPGlyphKerning* kern = get_selected_kerning_pair();
    if (!kern) {
        kerning_preview.set_text("");
        return;
    }
    Glib::ustring str;
    str += kern->u1->sample_glyph();
    str += kern->u2->sample_glyph();

    kerning_preview.set_text(str);
    this->kerning_pair = kern;

    //slider values increase from right to left so that they match the kerning pair preview
    kerning_slider.set_value(get_selected_spfont()->horiz_adv_x - kern->k);
}

void SvgFontsDialog::update_global_settings_tab(){
    SPFont* font = get_selected_spfont();
    if (!font) return;

    SPObject* obj;
    for (obj=font->children; obj; obj=obj->next){
        if (SP_IS_FONTFACE(obj)){
            _familyname_entry->set_text(((SPFontFace*) obj)->font_family);
        }
    }
}

void SvgFontsDialog::on_font_selection_changed(){
    SPFont* spfont = this->get_selected_spfont();
    if (!spfont) return;

    SvgFont* svgfont = this->get_selected_svgfont();
    first_glyph.update(spfont);
    second_glyph.update(spfont);
    kerning_preview.set_svgfont(svgfont);
    _font_da.set_svgfont(svgfont);
    _font_da.redraw();

    double set_width = spfont->horiz_adv_x;
    setwidth_spin.set_value(set_width);

    kerning_slider.set_range(0, set_width);
    kerning_slider.set_draw_value(false);
    kerning_slider.set_value(0);

    update_global_settings_tab();
    populate_glyphs_box();
    populate_kerning_pairs_box();
    update_sensitiveness();
}

void SvgFontsDialog::on_setwidth_changed(){
    SPFont* spfont = this->get_selected_spfont();
    if (spfont){
        spfont->horiz_adv_x = setwidth_spin.get_value();
        //TODO: tell cairo that the glyphs cache has to be invalidated
        //    The current solution is to recreate the whole cairo svgfont.
        //    This is not a good solution to the issue because big fonts will result in poor performance.
        update_glyphs();
    }
}

SPGlyphKerning* SvgFontsDialog::get_selected_kerning_pair()
{
    Gtk::TreeModel::iterator i = _KerningPairsList.get_selection()->get_selected();
    if(i)
        return (*i)[_KerningPairsListColumns.spnode];
    return NULL;
}

SvgFont* SvgFontsDialog::get_selected_svgfont()
{
    Gtk::TreeModel::iterator i = _FontsList.get_selection()->get_selected();
    if(i)
        return (*i)[_columns.svgfont];
    return NULL;
}

SPFont* SvgFontsDialog::get_selected_spfont()
{
    Gtk::TreeModel::iterator i = _FontsList.get_selection()->get_selected();
    if(i)
        return (*i)[_columns.spfont];
    return NULL;
}

SPGlyph* SvgFontsDialog::get_selected_glyph()
{
    Gtk::TreeModel::iterator i = _GlyphsList.get_selection()->get_selected();
    if(i)
        return (*i)[_GlyphsListColumns.glyph_node];
    return NULL;
}

Gtk::VBox* SvgFontsDialog::global_settings_tab(){
    _familyname_entry = new AttrEntry(this, (gchar*) _("Family Name:"), SP_PROP_FONT_FAMILY);

    global_vbox.pack_start(*_familyname_entry, false, false);
/*    global_vbox->add(*AttrCombo((gchar*) _("Style:"), SP_PROP_FONT_STYLE));
    global_vbox->add(*AttrCombo((gchar*) _("Variant:"), SP_PROP_FONT_VARIANT));
    global_vbox->add(*AttrCombo((gchar*) _("Weight:"), SP_PROP_FONT_WEIGHT));
*/

//Set Width (horiz_adv_x):
    Gtk::HBox* setwidth_hbox = Gtk::manage(new Gtk::HBox());
    setwidth_hbox->add(*Gtk::manage(new Gtk::Label(_("Set width:"))));
    setwidth_hbox->add(setwidth_spin);

    setwidth_spin.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_setwidth_changed));
    setwidth_spin.set_range(0, 4096);
    setwidth_spin.set_increments(10, 100);
    global_vbox.pack_start(*setwidth_hbox, false, false);

    return &global_vbox;
}

void
SvgFontsDialog::populate_glyphs_box()
{
    if (!_GlyphsListStore) return;
    _GlyphsListStore->clear();

    SPFont* spfont = this->get_selected_spfont();
    _glyphs_observer.set(spfont);

    for(SPObject* node = spfont->children; node; node=node->next){
        if (SP_IS_GLYPH(node)){
            Gtk::TreeModel::Row row = *(_GlyphsListStore->append());
            row[_GlyphsListColumns.glyph_node] = (SPGlyph*) node;
            row[_GlyphsListColumns.glyph_name] = ((SPGlyph*) node)->glyph_name;
            row[_GlyphsListColumns.unicode] = ((SPGlyph*) node)->unicode;
        }
    }
}

void
SvgFontsDialog::populate_kerning_pairs_box()
{
    if (!_KerningPairsListStore) return;
    _KerningPairsListStore->clear();

    SPFont* spfont = this->get_selected_spfont();

    for(SPObject* node = spfont->children; node; node=node->next){
        if (SP_IS_HKERN(node)){
            Gtk::TreeModel::Row row = *(_KerningPairsListStore->append());
            row[_KerningPairsListColumns.first_glyph] = ((SPGlyphKerning*) node)->u1->attribute_string().c_str();
            row[_KerningPairsListColumns.second_glyph] = ((SPGlyphKerning*) node)->u2->attribute_string().c_str();
            row[_KerningPairsListColumns.kerning_value] = ((SPGlyphKerning*) node)->k;
            row[_KerningPairsListColumns.spnode] = (SPGlyphKerning*) node;
        }
    }
}

SPGlyph *new_glyph(SPDocument* document, SPFont *font, const int count)
{
    g_return_val_if_fail(font != NULL, NULL);
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(document);

    // create a new glyph
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:glyph");

    std::ostringstream os;
    os << _("glyph") << " " << count;
    repr->setAttribute("glyph-name", os.str().c_str());

    // Append the new glyph node to the current font
    SP_OBJECT_REPR(font)->appendChild(repr);
    Inkscape::GC::release(repr);

    // get corresponding object
    SPGlyph *g = SP_GLYPH( document->getObjectByRepr(repr) );

    g_assert(g != NULL);
    g_assert(SP_IS_GLYPH(g));

    return g;
}

void SvgFontsDialog::update_glyphs(){
    SPFont* font = get_selected_spfont();
    if (!font) return;
    populate_glyphs_box();
    populate_kerning_pairs_box();
    first_glyph.update(font);
    second_glyph.update(font);
    get_selected_svgfont()->refresh();
    _font_da.redraw();
}

void SvgFontsDialog::add_glyph(){
    const int count = _GlyphsListStore->children().size();
    SPDocument* doc = sp_desktop_document(this->getDesktop());
    /* SPGlyph* glyph =*/ new_glyph(doc, get_selected_spfont(), count+1);

    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Add glyph"));

    update_glyphs();
}

void SvgFontsDialog::set_glyph_description_from_selected_path(){
    SPDesktop* desktop = this->getDesktop();
    if (!desktop) {
        g_warning("SvgFontsDialog: No active desktop");
        return;
    }

    Inkscape::MessageStack *msgStack = sp_desktop_message_stack(desktop);
    SPDocument* doc = sp_desktop_document(desktop);
    Inkscape::Selection* sel = sp_desktop_selection(desktop);
    if (sel->isEmpty()){
        char *msg = _("Select a <b>path</b> to define the curves of a glyph");
        msgStack->flash(Inkscape::ERROR_MESSAGE, msg);
        return;
    }

    Inkscape::XML::Node* node = (Inkscape::XML::Node*) g_slist_nth_data((GSList *)sel->reprList(), 0);
    if (!node) return;//TODO: should this be an assert?
    if (!node->matchAttributeName("d") || !node->attribute("d")){
        char *msg = _("The selected object does not have a <b>path</b> description.");
        msgStack->flash(Inkscape::ERROR_MESSAGE, msg);
        return;
    } //TODO: //Is there a better way to tell it to to the user?

    Geom::PathVector pathv = sp_svg_read_pathv(node->attribute("d"));

    //This matrix flips the glyph vertically
    Geom::Matrix m(Geom::Coord(1),Geom::Coord(0),Geom::Coord(0),Geom::Coord(-1),Geom::Coord(0),Geom::Coord(0));
    pathv*=m;
    //then we offset it
    pathv+=Geom::Point(Geom::Coord(0),Geom::Coord(get_selected_spfont()->horiz_adv_x));

    SPGlyph* glyph = get_selected_glyph();
    if (!glyph){
        char *msg = _("No glyph selected in the SVGFonts dialog.");
        msgStack->flash(Inkscape::ERROR_MESSAGE, msg);
        return;
    }
    glyph->repr->setAttribute("d", (char*) sp_svg_write_path (pathv));
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Set glyph curves"));

    update_glyphs();
}

void SvgFontsDialog::missing_glyph_description_from_selected_path(){
    SPDesktop* desktop = this->getDesktop();
    if (!desktop) {
        g_warning("SvgFontsDialog: No active desktop");
        return;
    }

    Inkscape::MessageStack *msgStack = sp_desktop_message_stack(desktop);
    SPDocument* doc = sp_desktop_document(desktop);
    Inkscape::Selection* sel = sp_desktop_selection(desktop);
    if (sel->isEmpty()){
        char *msg = _("Select a <b>path</b> to define the curves of a glyph");
        msgStack->flash(Inkscape::ERROR_MESSAGE, msg);
        return;
    }

    Inkscape::XML::Node* node = (Inkscape::XML::Node*) g_slist_nth_data((GSList *)sel->reprList(), 0);
    if (!node) return;//TODO: should this be an assert?
    if (!node->matchAttributeName("d") || !node->attribute("d")){
        char *msg = _("The selected object does not have a <b>path</b> description.");
        msgStack->flash(Inkscape::ERROR_MESSAGE, msg);
        return;
    } //TODO: //Is there a better way to tell it to to the user?

    Geom::PathVector pathv = sp_svg_read_pathv(node->attribute("d"));

    //This matrix flips the glyph vertically
    Geom::Matrix m(Geom::Coord(1),Geom::Coord(0),Geom::Coord(0),Geom::Coord(-1),Geom::Coord(0),Geom::Coord(0));
    pathv*=m;
    //then we offset it
//  pathv+=Geom::Point(Geom::Coord(0),Geom::Coord(get_selected_spfont()->horiz_adv_x));
    pathv+=Geom::Point(Geom::Coord(0),Geom::Coord(1000));//TODO: use here the units-per-em attribute?

    SPObject* obj;
    for (obj = get_selected_spfont()->children; obj; obj=obj->next){
        if (SP_IS_MISSING_GLYPH(obj)){
            obj->repr->setAttribute("d", (char*) sp_svg_write_path (pathv));
            sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Set glyph curves"));
        }
    }

    update_glyphs();
}

void SvgFontsDialog::glyph_name_edit(const Glib::ustring&, const Glib::ustring& str){
    Gtk::TreeModel::iterator i = _GlyphsList.get_selection()->get_selected();
    if (!i) return;

    SPGlyph* glyph = (*i)[_GlyphsListColumns.glyph_node];
    glyph->repr->setAttribute("glyph-name", str.c_str());

    SPDocument* doc = sp_desktop_document(this->getDesktop());
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Edit glyph name"));

    update_glyphs();
}

void SvgFontsDialog::glyph_unicode_edit(const Glib::ustring&, const Glib::ustring& str){
    Gtk::TreeModel::iterator i = _GlyphsList.get_selection()->get_selected();
    if (!i) return;

    SPGlyph* glyph = (*i)[_GlyphsListColumns.glyph_node];
    glyph->repr->setAttribute("unicode", str.c_str());

    SPDocument* doc = sp_desktop_document(this->getDesktop());
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Set glyph unicode"));

    update_glyphs();
}

void SvgFontsDialog::remove_selected_font(){
    SPFont* font = get_selected_spfont();

    sp_repr_unparent(font->repr);
    SPDocument* doc = sp_desktop_document(this->getDesktop());
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Remove font"));

    update_fonts();
}

void SvgFontsDialog::remove_selected_glyph(){
    if(!_GlyphsList.get_selection()) return;

    Gtk::TreeModel::iterator i = _GlyphsList.get_selection()->get_selected();
    if(!i) return;

    SPGlyph* glyph = (*i)[_GlyphsListColumns.glyph_node];
    sp_repr_unparent(glyph->repr);

    SPDocument* doc = sp_desktop_document(this->getDesktop());
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Remove glyph"));

    update_glyphs();
}

void SvgFontsDialog::remove_selected_kerning_pair(){
    if(!_KerningPairsList.get_selection()) return;

    Gtk::TreeModel::iterator i = _KerningPairsList.get_selection()->get_selected();
    if(!i) return;

    SPGlyphKerning* pair = (*i)[_KerningPairsListColumns.spnode];
    sp_repr_unparent(pair->repr);

    SPDocument* doc = sp_desktop_document(this->getDesktop());
    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Remove kerning pair"));

    update_glyphs();
}

Gtk::VBox* SvgFontsDialog::glyphs_tab(){
    _GlyphsList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &SvgFontsDialog::glyphs_list_button_release));
    create_glyphs_popup_menu(_GlyphsList, sigc::mem_fun(*this, &SvgFontsDialog::remove_selected_glyph));

    Gtk::HBox* missing_glyph_hbox = Gtk::manage(new Gtk::HBox());
    Gtk::Label* missing_glyph_label = Gtk::manage(new Gtk::Label(_("Missing Glyph:")));
    missing_glyph_hbox->pack_start(*missing_glyph_label, false,false);
    missing_glyph_hbox->pack_start(missing_glyph_button, false,false);
    missing_glyph_button.set_label(_("From selection..."));
    missing_glyph_button.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::missing_glyph_description_from_selected_path));
    glyphs_vbox.pack_start(*missing_glyph_hbox, false,false);

    glyphs_vbox.add(_GlyphsListScroller);
    _GlyphsListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _GlyphsListScroller.set_size_request(-1, 290);//It seems that is does not work. Why? I want a box with larger height
    _GlyphsListScroller.add(_GlyphsList);
    _GlyphsListStore = Gtk::ListStore::create(_GlyphsListColumns);
    _GlyphsList.set_model(_GlyphsListStore);
    _GlyphsList.append_column_editable(_("Glyph Name"), _GlyphsListColumns.glyph_name);
    _GlyphsList.append_column_editable(_("Unicode"), _GlyphsListColumns.unicode);

    Gtk::HBox* hb = Gtk::manage(new Gtk::HBox());
    add_glyph_button.set_label(_("Add Glyph"));
    add_glyph_button.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::add_glyph));

    hb->pack_start(add_glyph_button, false,false);
    hb->pack_start(glyph_from_path_button, false,false);

    glyphs_vbox.pack_start(*hb, false, false);
    glyph_from_path_button.set_label(_("Get curves from selection..."));
    glyph_from_path_button.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::set_glyph_description_from_selected_path));

    dynamic_cast<Gtk::CellRendererText*>( _GlyphsList.get_column_cell_renderer(0))->signal_edited().connect(
        sigc::mem_fun(*this, &SvgFontsDialog::glyph_name_edit));

    dynamic_cast<Gtk::CellRendererText*>( _GlyphsList.get_column_cell_renderer(1))->signal_edited().connect(
        sigc::mem_fun(*this, &SvgFontsDialog::glyph_unicode_edit));

    _glyphs_observer.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::update_glyphs));

    return &glyphs_vbox;
}

void SvgFontsDialog::add_kerning_pair(){
    if (first_glyph.get_active_text() == "" ||
        second_glyph.get_active_text() == "") return;

    //look for this kerning pair on the currently selected font
    this->kerning_pair = NULL;
    for(SPObject* node = this->get_selected_spfont()->children; node; node=node->next){
        //TODO: It is not really correct to get only the first byte of each string.
        //TODO: We should also support vertical kerning
        if (SP_IS_HKERN(node) && ((SPGlyphKerning*)node)->u1->contains((gchar) first_glyph.get_active_text().c_str()[0])
            && ((SPGlyphKerning*)node)->u2->contains((gchar) second_glyph.get_active_text().c_str()[0]) ){
            this->kerning_pair = (SPGlyphKerning*)node;
            continue;
        }
    }

    if (this->kerning_pair) return; //We already have this kerning pair

    SPDocument* document = sp_desktop_document(this->getDesktop());
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(document);

    // create a new hkern node
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:hkern");

    repr->setAttribute("u1", first_glyph.get_active_text().c_str());
    repr->setAttribute("u2", second_glyph.get_active_text().c_str());
    repr->setAttribute("k", "0");

    // Append the new hkern node to the current font
    SP_OBJECT_REPR(get_selected_spfont())->appendChild(repr);
    Inkscape::GC::release(repr);

    // get corresponding object
    this->kerning_pair = SP_HKERN( document->getObjectByRepr(repr) );

    sp_document_done(document, SP_VERB_DIALOG_SVG_FONTS, _("Add kerning pair"));
}

Gtk::VBox* SvgFontsDialog::kerning_tab(){
    _KerningPairsList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &SvgFontsDialog::kerning_pairs_list_button_release));
    create_kerning_pairs_popup_menu(_KerningPairsList, sigc::mem_fun(*this, &SvgFontsDialog::remove_selected_kerning_pair));

//Kerning Setup:
    kerning_vbox.add(*Gtk::manage(new Gtk::Label(_("Kerning Setup:"))));
    Gtk::HBox* kerning_selector = Gtk::manage(new Gtk::HBox());
    kerning_selector->add(*Gtk::manage(new Gtk::Label(_("1st Glyph:"))));
    kerning_selector->add(first_glyph);
    kerning_selector->add(*Gtk::manage(new Gtk::Label(_("2nd Glyph:"))));
    kerning_selector->add(second_glyph);
    kerning_selector->add(add_kernpair_button);
    add_kernpair_button.set_label(_("Add pair"));
    add_kernpair_button.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::add_kerning_pair));
    _KerningPairsList.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_kerning_pair_selection_changed));
    kerning_slider.signal_value_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_kerning_value_changed));

    kerning_vbox.pack_start(*kerning_selector, false,false);

    kerning_vbox.add(_KerningPairsListScroller);
    _KerningPairsListScroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
    _KerningPairsListScroller.add(_KerningPairsList);
    _KerningPairsListStore = Gtk::ListStore::create(_KerningPairsListColumns);
    _KerningPairsList.set_model(_KerningPairsListStore);
    _KerningPairsList.append_column(_("First Unicode range"), _KerningPairsListColumns.first_glyph);
    _KerningPairsList.append_column(_("Second Unicode range"), _KerningPairsListColumns.second_glyph);
//    _KerningPairsList.append_column_numeric_editable(_("Kerning Value"), _KerningPairsListColumns.kerning_value, "%f");

    kerning_vbox.add((Gtk::Widget&) kerning_preview);

    Gtk::HBox* kerning_amount_hbox = Gtk::manage(new Gtk::HBox());
    kerning_vbox.pack_start(*kerning_amount_hbox, false,false);
    kerning_amount_hbox->add(*Gtk::manage(new Gtk::Label(_("Kerning value:"))));
    kerning_amount_hbox->add(kerning_slider);

    kerning_preview.set_size(300 + 20, 150 + 20);
    _font_da.set_size(150 + 20, 50 + 20);

    return &kerning_vbox;
}

SPFont *new_font(SPDocument *document)
{
    g_return_val_if_fail(document != NULL, NULL);

    SPDefs *defs = (SPDefs *) SP_DOCUMENT_DEFS(document);

    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(document);

    // create a new font
    Inkscape::XML::Node *repr;
    repr = xml_doc->createElement("svg:font");

    //By default, set the horizontal advance to 1024 units
    repr->setAttribute("horiz-adv-x", "1024");

    // Append the new font node to defs
    SP_OBJECT_REPR(defs)->appendChild(repr);

    //create a missing glyph
    Inkscape::XML::Node *fontface;
    fontface = xml_doc->createElement("svg:font-face");
    fontface->setAttribute("units-per-em", "1024");
    repr->appendChild(fontface);

    //create a missing glyph
    Inkscape::XML::Node *mg;
    mg = xml_doc->createElement("svg:missing-glyph");
    mg->setAttribute("d", "M0,0h1020v1024h-1020z");
    repr->appendChild(mg);

    // get corresponding object
    SPFont *f = SP_FONT( document->getObjectByRepr(repr) );

    g_assert(f != NULL);
    g_assert(SP_IS_FONT(f));
    Inkscape::GC::release(mg);
    Inkscape::GC::release(repr);
    return f;
}

void set_font_family(SPFont* font, char* str){
    if (!font) return;
    SPObject* obj;
    for (obj=font->children; obj; obj=obj->next){
        if (SP_IS_FONTFACE(obj)){
            obj->repr->setAttribute("font-family", str);
        }
    }

    sp_document_done(font->document, SP_VERB_DIALOG_SVG_FONTS, _("Set font family"));
}

void SvgFontsDialog::add_font(){
    SPDocument* doc = sp_desktop_document(this->getDesktop());
    SPFont* font = new_font(doc);

    const int count = _model->children().size();
    std::ostringstream os, os2;
    os << _("font") << " " << count;
    font->setLabel(os.str().c_str());

    os2 << "SVGFont " << count;
    SPObject* obj;
    for (obj=font->children; obj; obj=obj->next){
        if (SP_IS_FONTFACE(obj)){
            obj->repr->setAttribute("font-family", os2.str().c_str());
        }
    }

    update_fonts();
//    select_font(font);

    sp_document_done(doc, SP_VERB_DIALOG_SVG_FONTS, _("Add font"));
}

SvgFontsDialog::SvgFontsDialog()
 : UI::Widget::Panel("", "/dialogs/svgfonts", SP_VERB_DIALOG_SVG_FONTS), _add(Gtk::Stock::NEW)
{
    _add.signal_clicked().connect(sigc::mem_fun(*this, &SvgFontsDialog::add_font));

    Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox());
    Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox());

    vbox->pack_start(_FontsList);
    vbox->pack_start(_add, false, false);
    hbox->add(*vbox);
    hbox->add(_font_settings);
    _getContents()->add(*hbox);

//List of SVGFonts declared in a document:
    _model = Gtk::ListStore::create(_columns);
    _FontsList.set_model(_model);
    _FontsList.append_column_editable(_("_Font"), _columns.label);
    _FontsList.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_font_selection_changed));

    this->update_fonts();

    Gtk::Notebook *tabs = Gtk::manage(new Gtk::Notebook());
    tabs->set_scrollable();

    tabs->append_page(*global_settings_tab(), _("_Global Settings"), true);
    tabs->append_page(*glyphs_tab(), _("_Glyphs"), true);
    tabs->append_page(*kerning_tab(), _("_Kerning"), true);

    _font_settings.add(*tabs);

//Text Preview:
    _preview_entry.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::on_preview_text_changed));
    _getContents()->add((Gtk::Widget&) _font_da);
    _preview_entry.set_text("Sample Text");
    _font_da.set_text("Sample Text");

    Gtk::HBox* preview_entry_hbox = Gtk::manage(new Gtk::HBox());
    _getContents()->add(*preview_entry_hbox);
    preview_entry_hbox->add(*Gtk::manage(new Gtk::Label(_("Preview Text:"))));
    preview_entry_hbox->add(_preview_entry);

    _FontsList.signal_button_release_event().connect_notify(sigc::mem_fun(*this, &SvgFontsDialog::fonts_list_button_release));
    create_fonts_popup_menu(_FontsList, sigc::mem_fun(*this, &SvgFontsDialog::remove_selected_font));

    _defs_observer.set(SP_DOCUMENT_DEFS(sp_desktop_document(this->getDesktop())));
    _defs_observer.signal_changed().connect(sigc::mem_fun(*this, &SvgFontsDialog::update_fonts));

    _getContents()->show_all();
}

SvgFontsDialog::~SvgFontsDialog(){}

} // namespace Dialog
} // namespace UI
} // namespace Inkscape

#endif //#ifdef ENABLE_SVG_FONTS

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :