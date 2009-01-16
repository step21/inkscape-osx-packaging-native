#define INKSCAPE_LPE_ROUGH_HATCHES_CPP
/** \file
 * LPE Curve Stitching implementation, used as an example for a base starting class
 * when implementing new LivePathEffects.
 *
 */
/*
 * Authors:
 *   JF Barraud.
*
* Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-rough-hatches.h"

#include "sp-item.h"
#include "sp-path.h"
#include "svg/svg.h"
#include "xml/repr.h"

#include <2geom/path.h>
#include <2geom/piecewise.h>
#include <2geom/sbasis.h>
#include <2geom/sbasis-math.h>
#include <2geom/sbasis-geometric.h>
#include <2geom/bezier-to-sbasis.h>
#include <2geom/sbasis-to-bezier.h>
#include <2geom/d2.h>
#include <2geom/matrix.h>

#include "ui/widget/scalar.h"
#include "libnr/nr-values.h"

namespace Inkscape {
namespace LivePathEffect {

using namespace Geom;

//------------------------------------------------
// Some goodies to navigate through curve's levels.
//------------------------------------------------
struct LevelCrossing{
    Point pt;
    double t;
    bool sign;
    bool used;
    std::pair<unsigned,unsigned> next_on_curve;
    std::pair<unsigned,unsigned> prev_on_curve;
};
struct LevelCrossingOrder {
    bool operator()(LevelCrossing a, LevelCrossing b) {
        return a.pt[Y] < b.pt[Y];
    }
};
struct LevelCrossingInfo{
    double t;
    unsigned level;
    unsigned idx;
};
struct LevelCrossingInfoOrder {
    bool operator()(LevelCrossingInfo a, LevelCrossingInfo b) {
        return a.t < b.t;
    }
};

typedef std::vector<LevelCrossing> LevelCrossings;

std::vector<double>
discontinuities(Piecewise<D2<SBasis> > const &f){
    std::vector<double> result;
    if (f.size()==0) return result;
    result.push_back(f.cuts[0]);
    Point prev_pt = f.segs[0].at1();
    //double old_t  = f.cuts[0];
    for(unsigned i=1; i<f.size(); i++){
        if ( f.segs[i].at0()!=prev_pt){
            result.push_back(f.cuts[i]);
            //old_t = f.cuts[i];
            //assert(f.segs[i-1].at1()==f.valueAt(old_t));
        }
        prev_pt = f.segs[i].at1();
    }
    result.push_back(f.cuts.back());
    //assert(f.segs.back().at1()==f.valueAt(old_t));
    return result;
}

class LevelsCrossings: public std::vector<LevelCrossings>{
public:
    LevelsCrossings():std::vector<LevelCrossings>(){};
    LevelsCrossings(std::vector<std::vector<double> > const &times,
                    Piecewise<D2<SBasis> > const &f,
                    Piecewise<SBasis> const &dx){
        
        for (unsigned i=0; i<times.size(); i++){
            LevelCrossings lcs;
            for (unsigned j=0; j<times[i].size(); j++){
                LevelCrossing lc;
                lc.pt = f.valueAt(times[i][j]);
                lc.t = times[i][j];
                lc.sign = ( dx.valueAt(times[i][j])>0 );
                lc.used = false;
                lcs.push_back(lc);
            }
            std::sort(lcs.begin(), lcs.end(), LevelCrossingOrder());
            push_back(lcs);
        }
        //Now create time ordering.
        std::vector<LevelCrossingInfo>temp;
        for (unsigned i=0; i<size(); i++){
            for (unsigned j=0; j<(*this)[i].size(); j++){
                LevelCrossingInfo elem;
                elem.t = (*this)[i][j].t;
                elem.level = i;
                elem.idx = j;
                temp.push_back(elem);
            }
        }
        std::sort(temp.begin(),temp.end(),LevelCrossingInfoOrder());
        std::vector<double> jumps = discontinuities(f);
        unsigned jump_idx = 0;
        unsigned first_in_comp = 0;
        for (unsigned i=0; i<temp.size(); i++){
            unsigned lvl = temp[i].level, idx = temp[i].idx;
            if ( i == temp.size()-1 || temp[i+1].t > jumps[jump_idx+1]){
                std::pair<unsigned,unsigned>next_data(temp[first_in_comp].level,temp[first_in_comp].idx);
                (*this)[lvl][idx].next_on_curve = next_data;
                first_in_comp = i+1;
                jump_idx += 1;
            }else{
                std::pair<unsigned,unsigned> next_data(temp[i+1].level,temp[i+1].idx);
                (*this)[lvl][idx].next_on_curve = next_data;
            }
        }

        for (unsigned i=0; i<size(); i++){
            for (unsigned j=0; j<(*this)[i].size(); j++){
                std::pair<unsigned,unsigned> next = (*this)[i][j].next_on_curve;
                (*this)[next.first][next.second].prev_on_curve = std::pair<unsigned,unsigned>(i,j);
            }
        }
#if 0
        std::cout<<"\n";
        for (unsigned i=0; i<temp.size()-1; i++){
            std::cout<<temp[i].level<<","<<temp[i].idx<<" -> ";
        }
        std::cout<<"\n";
        for (unsigned i=0; i<size(); i++){
            for (unsigned j=0; j<(*this)[i].size(); j++){
                std::cout<<"level:"<<i<<", idx:"<<j<<" -  ";
                std::cout<<"next:"<<(*this)[i][j].next_on_curve.first<<",";
                std::cout<<(*this)[i][j].next_on_curve.second<<" - ";
                std::cout<<"prev:"<<(*this)[i][j].prev_on_curve.first<<",";
                std::cout<<(*this)[i][j].prev_on_curve.second<<"\n";
            }
        }
#endif
    }

    void findFirstUnused(unsigned &level, unsigned &idx){
        level = size();
        idx = 0;
        for (unsigned i=0; i<size(); i++){
            for (unsigned j=0; j<(*this)[i].size(); j++){
                if (!(*this)[i][j].used){
                    level = i;
                    idx = j;
                    return;
                }
            }
        }
    }
    //set indexes to point to the next point in the "snake walk"
    //follow_level's meaning: 
    //  0=yes upward
    //  1=no, last move was upward,
    //  2=yes downward
    //  3=no, last move was downward.
    void step(unsigned &level, unsigned &idx, int &direction){
        if ( direction % 2 == 0 ){
            if (direction == 0) {
                if ( idx >= (*this)[level].size()-1 || (*this)[level][idx+1].used ) {
                    level = size();
                    return;
                }
                idx += 1;
            }else{
                if ( idx <= 0  || (*this)[level][idx-1].used ) {
                    level = size();
                    return;
                }
                idx -= 1;
            }
            direction += 1;
            return;
        }
        double t = (*this)[level][idx].t;
        double sign = ((*this)[level][idx].sign ? 1 : -1);
        double next_t = t;
        //level += 1;
        direction = (direction + 1)%4;
        if (level == size()){
            return;
        }

        std::pair<unsigned,unsigned> next;
        if ( sign > 0 ){
            next = (*this)[level][idx].next_on_curve;
        }else{
            next = (*this)[level][idx].prev_on_curve;
        }

        if ( level+1 != next.first || (*this)[next.first][next.second].used ) {
            level = size();
            return;
        }
        level = next.first;
        idx = next.second;

/*********************
        //look for next time on the same level
        for (unsigned j=0; j<(*this)[level].size(); j++){
            double tj = (*this)[level][j].t;
            if ( sign*(tj-t) > 0 ){
                if( next_t == t ||  sign*(tj-next_t)<0 ){
                    next_t = tj;
                    idx = j;
                }
            }
        }
        if ( next_t == t ){//not found? look at max/min time in this component, as time is "periodic".
            for (unsigned j=0; j<(*this)[level].size(); j++){
                double tj = (*this)[level][j].t;
                if ( -sign*(tj-next_t) > 0 ){
                    next_t = tj;
                    idx = j;
                }
            }
        }
        if ( next_t == t ){//still not found? houch! this should not happen.
            level = size();
            return;
        }
        if ( (*this)[level][idx].used ) {
            level = size();
            return;
        }
*************************/
        return;
    }
};

//-------------------------------------------------------
// Bend a path...
//-------------------------------------------------------

Piecewise<D2<SBasis> > bend(Piecewise<D2<SBasis> > const &f, Piecewise<SBasis> bending){
    D2<Piecewise<SBasis> > ff = make_cuts_independent(f);
    ff[X] += compose(bending, ff[Y]);
    return sectionize(ff);
}

//--------------------------------------------------------
// The RoughHatches lpe.
//--------------------------------------------------------
LPERoughHatches::LPERoughHatches(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    dist_rdm(_("Dist randomness"), _("Variation of dist between hatches, in %."), "dist_rdm", &wr, this, 75),
    growth(_("Growth"), _("Growth of distance between hatches."), "growth", &wr, this, 0.),
    scale_tf(_("Start smothness (front side)"), _("MISSING DESCRIPTION"), "scale_tf", &wr, this, 1.),
    scale_tb(_("Start smothness (back side)"), _("MISSING DESCRIPTION"), "scale_tb", &wr, this, 1.),
    scale_bf(_("End smothness (front side)"), _("MISSING DESCRIPTION"), "scale_bf", &wr, this, 1.),
    scale_bb(_("End smothness (back side)"), _("MISSING DESCRIPTION"), "scale_bb", &wr, this, 1.),
    top_edge_variation(_("Start edge variance"), _("The amount of random jitter to move the hatches start"), "top_edge_variation", &wr, this, 0),
    bot_edge_variation(_("End edge variance"), _("The amount of random jitter to move the hatches end"), "bot_edge_variation", &wr, this, 0),
    top_tgt_variation(_("Start tangential variance"), _("The amount of random jitter to move the hatches start along the boundary"), "top_tgt_variation", &wr, this, 0),
    bot_tgt_variation(_("End tangential variance"), _("The amount of random jitter to move the hatches end along the boundary"), "bot_tgt_variation", &wr, this, 0),
    top_smth_variation(_("Start smoothness variance"), _("Randomness of the smoothness of the U turn at hatches start"), "top_smth_variation", &wr, this, 0),
    bot_smth_variation(_("End spacing variance"), _("Randomness of the smoothness of the U turn at hatches end"), "bot_smth_variation", &wr, this, 0),
    fat_output(_("Generate thick/thin path"), _("Simulate a stroke of varrying width"), "fat_output", &wr, this, true),
    do_bend(_("Bend hatches"), _("Add a global bend to the hatches (slower)"), "do_bend", &wr, this, true),
    stroke_width_top(_("Stroke width (start side)"), _("Width at hatches 'start'"), "stroke_width_top", &wr, this, 1.),
    stroke_width_bot(_("Stroke width (end side)"), _("Width at hatches 'end'"), "stroke_width_bot", &wr, this, 1.),
    front_thickness(_("Front thickness (%)"), _("MISSING DESCRIPTION"), "front_thickness", &wr, this, 1.),
    back_thickness(_("Back thickness (%)"), _("MISSING DESCRIPTION"), "back_thickness", &wr, this, .25),
    bender(_("Global bending"), _("Relative position to ref point defines global bending direction and amount"), "bender", &wr, this, NULL, Geom::Point(-5,0)),
    direction(_("Hatches width and dir"), _("Defines hatches frequency and direction"), "direction", &wr, this, Geom::Point(50,0))
{
    registerParameter( dynamic_cast<Parameter *>(&direction) );
    registerParameter( dynamic_cast<Parameter *>(&do_bend) );
    registerParameter( dynamic_cast<Parameter *>(&bender) );
    registerParameter( dynamic_cast<Parameter *>(&dist_rdm) );
    registerParameter( dynamic_cast<Parameter *>(&growth) );
    registerParameter( dynamic_cast<Parameter *>(&top_edge_variation) );
    registerParameter( dynamic_cast<Parameter *>(&bot_edge_variation) );
    registerParameter( dynamic_cast<Parameter *>(&top_tgt_variation) );
    registerParameter( dynamic_cast<Parameter *>(&bot_tgt_variation) );
    registerParameter( dynamic_cast<Parameter *>(&scale_tf) );
    registerParameter( dynamic_cast<Parameter *>(&scale_tb) );
    registerParameter( dynamic_cast<Parameter *>(&scale_bf) );
    registerParameter( dynamic_cast<Parameter *>(&scale_bb) );
    registerParameter( dynamic_cast<Parameter *>(&top_smth_variation) );
    registerParameter( dynamic_cast<Parameter *>(&bot_smth_variation) );
    registerParameter( dynamic_cast<Parameter *>(&fat_output) );
    registerParameter( dynamic_cast<Parameter *>(&stroke_width_top) );
    registerParameter( dynamic_cast<Parameter *>(&stroke_width_bot) );
    registerParameter( dynamic_cast<Parameter *>(&front_thickness) );
    registerParameter( dynamic_cast<Parameter *>(&back_thickness) );

    //hatch_dist.param_set_range(0.1, NR_HUGE);
    growth.param_set_range(-0.95, NR_HUGE);
    dist_rdm.param_set_range(0, 99.);
    stroke_width_top.param_set_range(0,  NR_HUGE);
    stroke_width_bot.param_set_range(0,  NR_HUGE);
    front_thickness.param_set_range(0, NR_HUGE);
    back_thickness.param_set_range(0, NR_HUGE);

    concatenate_before_pwd2 = true;
    show_orig_path = true;
}

LPERoughHatches::~LPERoughHatches()
{

}

Geom::Piecewise<Geom::D2<Geom::SBasis> > 
LPERoughHatches::doEffect_pwd2 (Geom::Piecewise<Geom::D2<Geom::SBasis> > const & pwd2_in){

    Piecewise<D2<SBasis> > result;
    
    Piecewise<D2<SBasis> > transformed_pwd2_in = pwd2_in;
    Piecewise<SBasis> tilter;//used to bend the hatches
    Matrix bend_mat;//used to bend the hatches

    if (do_bend.get_value()){
        Point bend_dir = -rot90(unit_vector(direction.getOrigin() - bender));
        double bend_amount = L2(direction.getOrigin() - bender);
        bend_mat = Matrix(-bend_dir[Y], bend_dir[X], bend_dir[X], bend_dir[Y],0,0);
        transformed_pwd2_in = pwd2_in * bend_mat;
        tilter = Piecewise<SBasis>(shift(Linear(bend_amount),1));
        OptRect bbox = bounds_exact( transformed_pwd2_in );
        if (not(bbox)) return pwd2_in;
        tilter.setDomain((*bbox)[Y]);
        transformed_pwd2_in = bend(transformed_pwd2_in, tilter);
        transformed_pwd2_in = transformed_pwd2_in * bend_mat.inverse();
    }
    hatch_dist = Geom::L2(direction.getVector())/5;
    Point hatches_dir = rot90(unit_vector(direction.getVector()));
    Matrix mat(-hatches_dir[Y], hatches_dir[X], hatches_dir[X], hatches_dir[Y],0,0);
    transformed_pwd2_in = transformed_pwd2_in * mat;
        
    std::vector<std::vector<Point> > snakePoints;
    snakePoints = linearSnake(transformed_pwd2_in);
    if ( snakePoints.size() > 0 ){
        Piecewise<D2<SBasis> >smthSnake = smoothSnake(snakePoints); 
        smthSnake = smthSnake*mat.inverse();
        if (do_bend.get_value()){
            smthSnake = smthSnake*bend_mat;
            smthSnake = bend(smthSnake, -tilter);
            smthSnake = smthSnake*bend_mat.inverse();
        }
        return (smthSnake);
    }
    return pwd2_in;
}

//------------------------------------------------
// Generate the levels with random, growth...
//------------------------------------------------
std::vector<double>
LPERoughHatches::generateLevels(Interval const &domain){
    std::vector<double> result;
    double x = domain.min() + double(hatch_dist)/2.;
    double step = double(hatch_dist);
    double scale = 1+(hatch_dist*growth/domain.extent());
    while (x < domain.max()){
        result.push_back(x);
        double rdm = 1;
        if (dist_rdm.get_value() != 0) 
            rdm = 1.+ double((2*dist_rdm - dist_rdm.get_value()))/100.;
        x+= step*rdm;
        step*=scale;//(1.+double(growth));
    }
    return result;
}


//-------------------------------------------------------
// Walk through the intersections to create linear hatches
//-------------------------------------------------------
std::vector<std::vector<Point> > 
LPERoughHatches::linearSnake(Piecewise<D2<SBasis> > const &f){

    std::vector<std::vector<Point> > result;

    Piecewise<SBasis> x = make_cuts_independent(f)[X];
    //Rque: derivative is computed twice in the 2 lines below!!
    Piecewise<SBasis> dx = derivative(x);
    OptInterval range = bounds_exact(x);

    if (not range) return result;
    std::vector<double> levels = generateLevels(*range);
    std::vector<std::vector<double> > times;
    times = multi_roots(x,levels);

//TODO: fix multi_roots!!!*****************************************
//remove doubles :-(
    std::vector<std::vector<double> > cleaned_times(levels.size(),std::vector<double>());
    for (unsigned i=0; i<times.size(); i++){
        if ( times[i].size()>0 ){
            double last_t = times[i][0]-1;//ugly hack!!
            for (unsigned j=0; j<times[i].size(); j++){
                if (times[i][j]-last_t >0.000001){
                    last_t = times[i][j];
                    cleaned_times[i].push_back(last_t);
                }
            }
        }
    }
    times = cleaned_times;
//     for (unsigned i=0; i<times.size(); i++){
//         std::cout << "roots on level "<<i<<": ";
//         for (unsigned j=0; j<times[i].size(); j++){
//             std::cout << times[i][j] <<" ";
//         }
//         std::cout <<"\n";
//     }
//*******************************************************************
    LevelsCrossings lscs(times,f,dx);
    unsigned i,j;
    lscs.findFirstUnused(i,j);
    std::vector<Point> result_component;
    while ( i < lscs.size() ){ 
        int dir = 0;
        while ( i < lscs.size() ){
            result_component.push_back(lscs[i][j].pt);
            lscs[i][j].used = true;
            lscs.step(i,j, dir);
        }
        result.push_back(result_component);
        result_component = std::vector<Point>();
        lscs.findFirstUnused(i,j);
    }
    return result;
}

//-------------------------------------------------------
// Smooth the linear hatches according to params...
//-------------------------------------------------------
Piecewise<D2<SBasis> > 
LPERoughHatches::smoothSnake(std::vector<std::vector<Point> > const &linearSnake){

    Piecewise<D2<SBasis> > result;
    for (unsigned comp=0; comp<linearSnake.size(); comp++){
        if (linearSnake[comp].size()>=2){
            bool is_top = true;//Inversion here; due to downward y? 
            Point last_pt = linearSnake[comp][0];
            Point last_top = linearSnake[comp][0];
            Point last_bot = linearSnake[comp][0];
            Point last_hdle = linearSnake[comp][0];
            Point last_top_hdle = linearSnake[comp][0];
            Point last_bot_hdle = linearSnake[comp][0];
            Geom::Path res_comp(last_pt);
            Geom::Path res_comp_top(last_pt);
            Geom::Path res_comp_bot(last_pt);
            unsigned i=1;
            while( i+1<linearSnake[comp].size() ){
                Point pt0 = linearSnake[comp][i];
                Point pt1 = linearSnake[comp][i+1];
                Point new_pt = (pt0+pt1)/2;
                double scale_in = (is_top ? scale_tf : scale_bf );
                double scale_out = (is_top ? scale_tb : scale_bb );
                if (is_top){
                    if (top_edge_variation.get_value() != 0) 
                        new_pt[Y] += double(top_edge_variation)-top_edge_variation.get_value()/2.;
                    if (top_tgt_variation.get_value() != 0) 
                        new_pt[X] += double(top_tgt_variation)-top_tgt_variation.get_value()/2.;
                    if (top_smth_variation.get_value() != 0) {
                        scale_in*=(100.-double(top_smth_variation))/100.;
                        scale_out*=(100.-double(top_smth_variation))/100.;
                    }
                }else{
                    if (bot_edge_variation.get_value() != 0) 
                        new_pt[Y] += double(bot_edge_variation)-bot_edge_variation.get_value()/2.;
                    if (bot_tgt_variation.get_value() != 0) 
                        new_pt[X] += double(bot_tgt_variation)-bot_tgt_variation.get_value()/2.;
                    if (bot_smth_variation.get_value() != 0) {
                        scale_in*=(100.-double(bot_smth_variation))/100.;
                        scale_out*=(100.-double(bot_smth_variation))/100.;
                    }
                }
                Point new_hdle_in  = new_pt + (pt0-pt1) * (scale_in /2.);
                Point new_hdle_out = new_pt - (pt0-pt1) * (scale_out/2.);
                
                if ( fat_output.get_value() ){
                    double scaled_width = double((is_top ? stroke_width_top : stroke_width_bot))/(pt1[X]-pt0[X]);
                    Point hdle_offset = (pt1-pt0)*scaled_width;
                    Point inside = new_pt;
                    Point inside_hdle_in;
                    Point inside_hdle_out;
                    inside[Y]+= double((is_top ? -stroke_width_top : stroke_width_bot));
                    inside_hdle_in  = inside + (new_hdle_in -new_pt) + hdle_offset * double((is_top ? front_thickness : back_thickness));
                    inside_hdle_out = inside + (new_hdle_out-new_pt) - hdle_offset * double((is_top ? back_thickness : front_thickness));
                    //TODO: find a good way to handle limit cases (small smthness, large stroke).
                    //if (inside_hdle_in[X]  > inside[X]) inside_hdle_in = inside;
                    //if (inside_hdle_out[X] < inside[X]) inside_hdle_out = inside;
                    
                    if (is_top){
                        res_comp_top.appendNew<CubicBezier>(last_top_hdle,new_hdle_in,new_pt);
                        res_comp_bot.appendNew<CubicBezier>(last_bot_hdle,inside_hdle_in,inside);
                        last_top_hdle = new_hdle_out;
                        last_bot_hdle = inside_hdle_out;
                    }else{
                        res_comp_top.appendNew<CubicBezier>(last_top_hdle,inside_hdle_in,inside);
                        res_comp_bot.appendNew<CubicBezier>(last_bot_hdle,new_hdle_in,new_pt);
                        last_top_hdle = inside_hdle_out;
                        last_bot_hdle = new_hdle_out;
                    }
                }else{
                    res_comp.appendNew<CubicBezier>(last_hdle,new_hdle_in,new_pt);
                }
            
                last_hdle = new_hdle_out;
                i+=2;
                is_top = !is_top;
            }
            if ( i<linearSnake[comp].size() )
                if ( fat_output.get_value() ){
                    res_comp_top.appendNew<CubicBezier>(last_top_hdle,linearSnake[comp][i],linearSnake[comp][i]);
                    res_comp_bot.appendNew<CubicBezier>(last_bot_hdle,linearSnake[comp][i],linearSnake[comp][i]);
                }else{
                    res_comp.appendNew<CubicBezier>(last_hdle,linearSnake[comp][i],linearSnake[comp][i]);
                }
            if ( fat_output.get_value() ){
                res_comp = res_comp_bot;
                res_comp.append(res_comp_top.reverse(),Geom::Path::STITCH_DISCONTINUOUS);
            }    
            result.concat(res_comp.toPwSb());
        }
    }
    return result;
}

void
LPERoughHatches::doBeforeEffect (SPLPEItem */*lpeitem*/)
{
    using namespace Geom;
    top_edge_variation.resetRandomizer();
    bot_edge_variation.resetRandomizer();
    top_tgt_variation.resetRandomizer();
    bot_tgt_variation.resetRandomizer();
    top_smth_variation.resetRandomizer();
    bot_smth_variation.resetRandomizer();
    dist_rdm.resetRandomizer();

    //original_bbox(lpeitem);
}


void
LPERoughHatches::resetDefaults(SPItem * item)
{
    Geom::OptRect bbox = item->getBounds(Geom::identity(), SPItem::GEOMETRIC_BBOX);
    Geom::Point origin(0.,0.);
    Geom::Point vector(50.,0.);
    if (bbox) {
        origin = bbox->midpoint();
        vector = Geom::Point((*bbox)[X].extent()/4, 0.);
        top_edge_variation.param_set_value( (*bbox)[Y].extent()/10, 0 );
        bot_edge_variation.param_set_value( (*bbox)[Y].extent()/10, 0 );
    }
    direction.set_and_write_new_values(origin, vector);
    bender.param_set_and_write_new_value( origin + Geom::Point(5,0) );
    hatch_dist = Geom::L2(vector)/5;
}


} //namespace LivePathEffect
} /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :