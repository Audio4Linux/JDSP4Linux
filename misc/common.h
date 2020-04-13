/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */
#ifndef COMMON_H
#define COMMON_H
#include <QDir>
#include <QMenu>
#include <string>
#include <algorithm>
#include <cmath>

static std::string default_config = "enable=true\nanalogmodelling_enable=false\nanalogmodelling_tubedrive=10000\nbass_enable=false\nbass_mode=1000\nbass_filtertype=1\nbass_freq=65\nstereowide_enable=false\nbs2b_enable=false\ncompression_enable=false\ncompression_pregain=20\ncompression_threshold=-60\ncompression_knee=40\ncompression_ratio=-20\ncompression_attack=1\ncompression_release=88\ntone_enable=false\ntone_filtertype=0\ntone_eq=\"0;0;0;0;0;0;0;0;0;0;0;0;0;0;0\"\nmasterswitch_limthreshold=0\nmasterswitch_limrelease=50\nddc_enable=false\nddc_file=\"none\"\nbs2b_fcut=300\nbs2b_feed=78\nstereowide_mcoeff=1000\nstereowide_scoeff=400\nheadset_enable=false\nheadset_osf=1\nheadset_lpf_input=18000\nheadset_lpf_bass=300\nheadset_lpf_damp=10000\nheadset_lpf_output=18000\nheadset_reflection_amount=0.5\nheadset_reflection_width=0.6\nheadset_reflection_factor=1.2\nheadset_finaldry=-7.0\nheadset_finalwet=-8.0\nheadset_width=0.9\nheadset_wet=-8.0\nheadset_bassboost=0.1\nheadset_lfo_spin=0.4\nheadset_lfo_wander=0.3\nheadset_decay=1.2\nheadset_delay=0\nconvolver_enable=false\nconvolver_gain=0\nconvolver_file=\"...\"\n";
static inline bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}
static float translate(int value,int leftMin,int leftMax,float rightMin,float rightMax){
    float leftSpan = leftMax - leftMin;
    float rightSpan = rightMax - rightMin;
    float valueScaled = float(value - leftMin) / float(leftSpan);
    return rightMin + (valueScaled * rightSpan);
}

static inline QString pathAppend(const QString& path1, const QString& path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

static QString findDirOfFile(QString file){
    return QFileInfo(file).absoluteDir().absolutePath();
}

static QString chopFirstLastChar(QString i){
    if(i.size() > 2){
        i.remove(0,1);
        i.chop(1);
    }
    return i;
}

template<typename TReal>
static bool isApproximatelyEqual(TReal a, TReal b, TReal tolerance = std::numeric_limits<TReal>::epsilon())
{
    TReal diff = std::fabs(a - b);
    if (diff <= tolerance)
        return true;

    if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
        return true;

    return false;
}

namespace MenuIO {
static QString buildString(QMenu* menu){
    QString out;
    for(auto action : menu->actions()){
        if(action->isSeparator())
            out += "separator;";
        else if(action->menu())
            out += action->menu()->property("tag").toString() + ";";
        else
            out += action->property("tag").toString() + ";";
    }
    return out;
}
static QMenu* buildMenu(QMenu* options, QString input){
    QMenu* out = new QMenu();
    for(auto item : input.split(";")){
        if(item == "separator")
            out->addSeparator();
        else if(item.startsWith("menu")){
            for(auto action : options->actions()){
                if(action->menu())
                    if(action->menu()->property("tag") == item){
                        out->addMenu(action->menu());
                        continue;
                    }
            }
        }
        else{
            for(auto action : options->actions()){
                if(action->property("tag") == item){
                    out->addAction(action);
                    continue;
                }
            }
        }
    }
    return out;
}
}
#endif // COMMON_H
