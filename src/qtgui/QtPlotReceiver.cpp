#include "ResourcePreview.hpp"
#include "QtPlotReceiver.hpp"
#include "CameraAlignment.hpp"
#include "SegmentArc.hpp"
#include <sstream>
#include <iomanip>
#include <locale>
#include "TableColumns.hpp"
#include "PlotWidget.hpp"
#include "BodyFixedPoint.hpp"
#include "CelestialBody.hpp"
#include "Spacecraft.hpp"
#include "Hardware.hpp"
#include "FieldOfView.hpp"
#include "ConicalFOV.hpp"
#include "RectangularFOV.hpp"
#include "CustomFOV.hpp"
#include <osg/Matrixd>
#include "SolarSystem.hpp"
#include "FileManager.hpp"
#include "CoordinateConverter.hpp"
#include "Moderator.hpp"
#include "MessageInterface.hpp"
#include "GmatDefaults.hpp"
#include "GmatCommand.hpp"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileInfo>
#include <algorithm>
#include <cmath>

namespace {
QString text(const std::string &value) { return QString::fromStdString(value); }
QColor rgb(UnsignedInt color) { return QColor::fromRgb(static_cast<QRgb>(color & 0xffffff)); }
constexpr double degrees = 57.2957795130823208768;
PlotSensorMask sensorMaskGeometry(const QtSensorMaskSetting &definition)
{
   auto *moderator=Moderator::Instance();
   auto *hardware=dynamic_cast<Hardware *>(moderator->GetConfiguredObject(definition.hardware.toStdString()));
   if (!hardware) throw std::runtime_error((definition.name+": unknown sensor Hardware "+definition.hardware).toStdString());
   const auto fovName=hardware->GetRefObjectName(Gmat::FIELD_OF_VIEW);
   auto *fov=dynamic_cast<FieldOfView *>(moderator->GetConfiguredObject(fovName));
   if (!fov) throw std::runtime_error((definition.name+": Hardware needs a FieldOfView").toStdString());
   PlotSensorMask mask; mask.name=definition.name; mask.source=definition.source; mask.hardware=definition.hardware;
   mask.label=definition.label; mask.automaticLength=definition.automaticLength; mask.length=definition.length;
   mask.color=rgb(fov->GetColor()); mask.color.setAlpha(static_cast<int>(fov->GetAlpha()));
   auto *source=moderator->GetConfiguredObject(definition.source.toStdString());
   if (!source && moderator->GetSolarSystemInUse()) source=moderator->GetSolarSystemInUse()->GetBody(definition.source.toStdString());
   if (auto *body=dynamic_cast<CelestialBody *>(source)) mask.sourceRadius=body->GetEquatorialRadius();
   const auto location=hardware->GetLocation(),direction=hardware->GetDirection(),second=hardware->GetSecondDirection();
   osg::Vec3d forward(direction[0],direction[1],direction[2]),up(second[0],second[1],second[2]);
   auto finite=[](const osg::Vec3d &value) { return std::isfinite(value.x()) && std::isfinite(value.y()) && std::isfinite(value.z()); };
   if (!finite(forward) || !finite(up) || !std::isfinite(forward.length()) || !std::isfinite(up.length()) || forward.normalize()<1e-12 || up.normalize()<1e-12 || (forward^up).length()<1e-12)
      throw std::runtime_error((definition.name+": Hardware direction and second direction must form a finite nondegenerate mount").toStdString());
   osg::Matrixd mount; mount.makeLookAt(osg::Vec3d(),forward,up);
   const auto rotation=mount.getRotate().inverse();
   const auto axis=rotation*osg::Vec3d(0,0,-1);
   mask.axis={axis.x(),axis.y(),axis.z()};
   for (int i=0;i<3;++i) mask.origin[i]=location[i];
   if (!PlotModel::usableOrbitPosition(mask.origin[0],mask.origin[1],mask.origin[2])) throw std::runtime_error((definition.name+": Hardware location is not renderable").toStdString());
   auto vertex=[&](double x,double y) {
      const auto body=rotation*osg::Vec3d(x,y,-1);
      if (!PlotModel::usableOrbitPosition(body.x(),body.y(),body.z())) throw std::runtime_error((definition.name+": FOV boundary is not renderable").toStdString());
      mask.boundary.append({body.x(),body.y(),body.z()});
   };
   if (auto *conical=dynamic_cast<ConicalFOV *>(fov)) {
      mask.drawEdges=false; const auto angles=conical->GetMaskConeAngles();
      if (angles.GetSize()!=1) throw std::runtime_error((definition.name+": ConicalFOV has no cone angle").toStdString());
      const double radius=std::tan(angles[0]);
      for (int i=0;i<120;++i) { const double clock=6.2831853071795864769*i/120; vertex(radius*std::cos(clock),radius*std::sin(clock)); }
   } else if (auto *rectangular=dynamic_cast<RectangularFOV *>(fov)) {
      const double width=std::tan(rectangular->GetAngleWidth()),height=std::tan(rectangular->GetAngleHeight());
      vertex(width,height); vertex(-width,height); vertex(-width,-height); vertex(width,-height);
   } else if (auto *custom=dynamic_cast<CustomFOV *>(fov)) {
      // Initialize only a detached copy. Its copy constructor has no path side
      // effect, and file/interpolated mask angles are frozen before Run/replay.
      CustomFOV copy(*custom);
      if (!copy.Initialize()) throw std::runtime_error((definition.name+": CustomFOV did not initialize").toStdString());
      const auto cones=copy.GetMaskConeAngles(),clocks=copy.GetMaskClockAngles();
      if (cones.GetSize()!=clocks.GetSize() || cones.GetSize()<3) throw std::runtime_error((definition.name+": CustomFOV needs paired cone/clock vertices").toStdString());
      for (int i=0;i<cones.GetSize();++i) { const auto radius=std::tan(cones[i]); vertex(radius*std::cos(clocks[i]),radius*std::sin(clocks[i])); }
   } else throw std::runtime_error((definition.name+": supported mask shapes are ConicalFOV, RectangularFOV and CustomFOV").toStdString());
   return mask;
}

std::deque<PlotCamera> &cameraHistory(PlotModel &data,int index)
{
   if (index>=0) return index==0 ? data.cameras : data.cameraViews[index].cameras;
   const int selected=-index-1;
   return selected==0 ? data.resetView->cameras : data.cameraViews[selected].reset->cameras;
}
}
void QtPlotReceiver::validateCameraReferences(const QMap<QString,QtCameraSetting> &settings)
{
   auto *moderator=Moderator::Instance();
   for (auto it=settings.cbegin();it!=settings.cend();++it) {
      auto validateTrajectory=[&](const QString &name,bool allowRoot=true) {
         if (name.isEmpty()) return;
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (!plot || !plot->IsOfType("OrbitView")) throw std::runtime_error("Plot display/camera metadata requires an OrbitView");
         if (allowRoot && name=="CoordinateSystem") return;
         const auto &objects=plot->GetStringArrayParameter("Add");
         if (std::find(objects.begin(),objects.end(),name.toStdString())==objects.end())
            throw std::runtime_error((it.key()+": plot display/camera object must be in Add: "+name).toStdString());
      };
      // A body camera can reference an undrawn SpacePoint. OrbitView already
      // resolves primary references and sunlight independently of Add; named
      // secondary cameras resolve their objects at each published sample.
      auto validateBody=[&](const QString &name) {
         if (name.isEmpty()) return;
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (!plot || !plot->IsOfType("OrbitView")) throw std::runtime_error("Plot display/camera metadata requires an OrbitView");
         auto *object=moderator->GetConfiguredObject(name.toStdString());
         if (!object) object=moderator->GetSolarSystemInUse()->GetBody(name.toStdString());
         if (!dynamic_cast<SpacePoint *>(object)) throw std::runtime_error((it.key()+": automatic body camera needs a space point: "+name).toStdString());
      };
      if (it->stars) {
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (!plot || !plot->IsOfType("OrbitView")) throw std::runtime_error("Star display metadata requires an OrbitView");
         (void)qtStars(qtStarsJson(it->stars));
         auto validateCatalog=[](const QString &catalog) {
            if (FileManager::Instance()->FindPath(catalog.toStdString(),"STAR_PATH",true,true,false).empty())
               throw std::runtime_error(("Star catalog not found through STAR_PATH: "+catalog).toStdString());
         };
         // The legacy catalog setter validates retained custom input even when
         // a preset currently uses its own catalog; retain its raw spelling.
         validateCatalog(it->stars->catalog);
         if (it->stars->preset!="Custom") validateCatalog("inp_StarsHYGv3.txt");
      }
      for (const auto &mask:it->sensorMasks) {
         validateTrajectory(mask.source,false);
         auto *source=moderator->GetConfiguredObject(mask.source.toStdString());
         if (!source) source=moderator->GetSolarSystemInUse()->GetBody(mask.source.toStdString());
         if (!dynamic_cast<SpacePoint *>(source)) throw std::runtime_error((mask.name+": sensor mask source must be a plotted space point").toStdString());
         // Imported masks may use global Hardware lookup, as OFScene does.
         // The UI can offer the source's attached Hardware/FOV pairs without
         // imposing that narrower picker restriction on valid imported masks.
         (void)sensorMaskGeometry(mask);
      }
      for (const auto &vector:it->vectors) {
         for (const auto &name:vector.type=="Body-Fixed" ? QStringList{vector.source} : QStringList{vector.source,vector.destination}) {
            auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
            auto *object=moderator->GetConfiguredObject(name.toStdString());
            if (!object) object=moderator->GetSolarSystemInUse()->GetBody(name.toStdString());
            if (!plot || !plot->IsOfType("OrbitView") || !dynamic_cast<SpacePoint *>(object))
               throw std::runtime_error((it.key()+" vector "+vector.name+": unknown source/destination space point "+name).toStdString());
         }
      }
      for (const auto &name:it->objectLabels.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectTrajectories.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectCenters.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectEndpoints.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectAxes.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectGrids.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectXYPlanes.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectSegmentLabels.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectVelocities.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectMarkerSizes.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectLineWidths.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectFontSizes.keys()) validateTrajectory(name,false);
      for (const auto &name:it->objectFontPositions.keys()) validateTrajectory(name,false);
      auto validateSegment=[&](const QString &frame) {
         if (frame.isEmpty()) return;
         const auto objectName=frame.section('.',0,0),provider=frame.section('.',1,1);
         validateTrajectory(objectName);
         auto *object=moderator->GetConfiguredObject(objectName.toStdString());
         if (!object || !object->IsOfType(Gmat::SPACECRAFT)) throw std::runtime_error("Segment camera needs a plotted spacecraft");
         bool found=false; QSet<GmatCommand *> visited; QVector<GmatCommand *> pending{moderator->GetFirstCommand()};
         while (!pending.empty()) {
            auto *command=pending.takeLast(); if (!command || visited.contains(command)) continue; visited.insert(command);
            pending.append(command->GetNext());
            for (int branch=0;auto *child=command->GetChildCommand(branch);++branch) pending.append(child);
            if (command->GetTypeName()!="Propagate") continue;
            const auto summary=command->GetSummaryName();
            const auto identity=!summary.empty() && summary!="Unnamed" ? summary : command->GetName();
            if (identity!=provider.toStdString()) continue;
            const auto propagators=command->GetStringArrayParameter(command->GetParameterID("Propagator"));
            for (int group=0;group<static_cast<int>(propagators.size());++group) {
               const auto &names=command->GetStringArrayParameter(command->GetParameterID("Spacecraft"),group);
               for (const auto &name:names) {
                  if (name==objectName.toStdString()) { found=true; continue; }
                  auto *group=moderator->GetConfiguredObject(name);
                  if (group && group->IsOfType(Gmat::FORMATION)) {
                     const auto &members=group->GetStringArrayParameter("Add");
                     if (std::find(members.begin(),members.end(),objectName.toStdString())!=members.end()) found=true;
                  }
               }
            }
         }
         if (!found) throw std::runtime_error((it.key()+": segment camera has no named Propagate for "+frame).toStdString());
      };
      validateSegment(it->segmentFrame);
      if (!it->segmentFrame.isEmpty()) {
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (plot->GetStringParameter("ViewPointRefType")=="Vector" || plot->GetStringParameter("ViewPointReference")!=it->segmentFrame.section('.',0,0).toStdString())
            throw std::runtime_error("Segment camera must match ViewPointReference");
      }
      validateTrajectory(it->automaticTrajectory);
      validateBody(it->automaticBody);
      if (!it->automaticBody.isEmpty()) {
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (plot->GetStringParameter("ViewPointRefType")=="Vector" || plot->GetStringParameter("ViewPointReference")!=it->automaticBody.toStdString())
            throw std::runtime_error("Automatic body camera must match ViewPointReference");
      }
      if (it->bodyRelative || it->lookAtRotation) {
         auto *plot=moderator->GetConfiguredObject(it.key().toStdString());
         if (!plot || !plot->IsOfType("OrbitView") || (it->bodyRelative && plot->GetStringParameter("ViewPointRefType")=="Vector") ||
             plot->GetStringParameter("ViewPointVectorType")!="Vector")
            throw std::runtime_error((it.key()+": relative camera needs a vector ViewPointVector; body-relative mode also needs an object ViewPointReference").toStdString());
      }
      auto validateView=[&](const auto &view) {
      validateSegment(view.segmentFrame);
      validateTrajectory(view.automaticTrajectory);
      validateBody(view.automaticBody);
      for (const auto &name:{view.reference,view.target}) {
         if (name.isEmpty() || name=="CoordinateSystem") continue;
         auto *object=moderator->GetConfiguredObject(name.toStdString());
         if (!object) object=moderator->GetSolarSystemInUse()->GetBody(name.toStdString());
         if (!dynamic_cast<SpacePoint *>(object)) throw std::runtime_error((it.key()+" camera: unknown space point "+name).toStdString());
      }
      };
      if (it->reset) validateView(*it->reset);
      for (const auto &view:it->views) { validateView(view); if (view.reset) validateView(*view.reset); }
   }
}
QtPlotReceiver::QtPlotReceiver(QMdiArea *area) : workspace(area) {}
QtPlotReceiver::~QtPlotReceiver() { changed = {}; replayChanged = {}; clear(); }
bool QtPlotReceiver::hasReplayDisplays() const
{
   for (const auto &entry:entries) if (entry.data->kind!=PlotModel::Kind::Table) return true;
   return false;
}
void QtPlotReceiver::setReplayPosition(int value)
{
   sharedReplayPosition=std::clamp(value,0,1000);
   for (auto &entry:entries) if (entry.widget) {
      entry.widget->setSharedReplayMode(sharedReplayMode);
      entry.widget->setSharedReplayPosition(*sharedReplayPosition);
   }
   if (replayChanged) replayChanged();
}
void QtPlotReceiver::setReplayMode(PlotReplayMode mode)
{
   sharedReplayMode=mode;
   for (auto &entry:entries) if (entry.widget) {
      entry.widget->setSharedReplayMode(mode);
      if (sharedReplayPosition) entry.widget->setSharedReplayPosition(*sharedReplayPosition);
      else entry.widget->refresh();
   }
   if (replayChanged) replayChanged();
}
void QtPlotReceiver::releaseSharedReplay()
{
   sharedReplayPosition.reset();
   if (replayChanged) replayChanged();
}
QtPlotReceiver::Entry *QtPlotReceiver::find(const std::string &name)
{
   auto it=entries.find(text(name)); return it==entries.end() ? nullptr : &it.value();
}
void QtPlotReceiver::clear(bool resetViews)
{
   for (auto it=entries.cbegin();it!=entries.cend();++it)
      if (it->data->kind==PlotModel::Kind::Orbit && it->data->userView) savedViews.insert(it.key(),*it->data->userView);
   if (resetViews) savedViews.clear();
   for (auto &entry : entries) if (entry.window) delete entry.window.data();
   entries.clear(); warnings.clear(); sharedReplayPosition.reset();
   if (replayChanged) replayChanged();
   if (changed) changed();
}
void QtPlotReceiver::missionFinished()
{
   // A disabled subscriber may omit its end-of-run callback. Every terminal
   // outcome finalizes the retained (possibly partial) display history, including
   // a viewer currently closed. This does not imply a successful mission.
   for (auto &entry:entries) {
      if (entry.data->kind!=PlotModel::Kind::Orbit && entry.data->kind!=PlotModel::Kind::GroundTrack) continue;
      entry.data->endOfRun=true;
      refresh(entry,true);
   }
}
QStringList QtPlotReceiver::names() const { return entries.keys(); }
std::shared_ptr<const PlotModel> QtPlotReceiver::model(const QString &name) const
{
   auto it=entries.constFind(name); return it==entries.cend() ? nullptr : it->data;
}
bool QtPlotReceiver::show(const QString &name)
{
   auto it=entries.find(name); if (it==entries.end()) return false;
   auto &entry=it.value();
   // Close removes an MDI child before its deferred destruction. A live
   // mission may service another Open action during that interval.
   if (entry.window && !workspace->subWindowList().contains(entry.window.data())) {
      entry.window.clear(); entry.widget.clear(); entry.table.clear();
   }
   const bool created=!entry.window;
   if (created) {
      if (entry.data->kind==PlotModel::Kind::Table) {
         entry.table=new QTableWidget;
         configureTableColumns(entry.table);
         entry.table->setEditTriggers(QAbstractItemView::NoEditTriggers);
         entry.table->setRowCount(entry.cells.size());
         int columns=0; for (const auto &row:entry.cells) columns=std::max(columns,static_cast<int>(row.size()));
         entry.table->setColumnCount(columns);
         for (int r=0;r<entry.cells.size();++r) for (int c=0;c<entry.cells[r].size();++c) {
            const auto &cell=entry.cells[r][c];
            auto *item=new QTableWidgetItem(cell.text);
            item->setForeground(cell.foreground); item->setBackground(cell.background);
            entry.table->setItem(r,c,item);
         }
         fitTableColumns(entry.table);
         entry.window=workspace->addSubWindow(entry.table);
      } else {
         entry.widget=new PlotWidget(entry.data);
         entry.widget->replayRequested=[this] { releaseSharedReplay(); };
         entry.widget->skyRequested=[this,name] { if (auto *current=find(name.toStdString())) loadSkyCatalogs(*current,name.toStdString()); };
         entry.widget->setSharedReplayMode(sharedReplayMode);
         if (sharedReplayPosition) entry.widget->setSharedReplayPosition(*sharedReplayPosition);
         entry.widget->setProtectedPaths([this] { return protectedPaths ? protectedPaths() : QStringList(); });
         if (saveProjection) entry.widget->setProjectionSaver([this,name](bool perspective,double fov) { return saveProjection(name,perspective,fov); });
         entry.widget->setFocusPolicy(Qt::StrongFocus);
         entry.window=workspace->addSubWindow(entry.widget);
      }
      entry.window->setAttribute(Qt::WA_DeleteOnClose);
      entry.window->setWindowTitle(name);
      entry.window->setProperty("plotName",name);
      entry.window->resize(700,480);
   }
   // show() alone leaves a minimized MDI child minimized. Activation also
   // needs an explicit raise when another child covers this plot.
   if (entry.window->isMinimized()) entry.window->showNormal();
   else entry.window->show();
   if (created) {
      // QMdiArea's next cascade position can lie beyond the viewport after
      // several closes/reopens. Keep newly created viewer controls reachable.
      const auto bounds=workspace->viewport()->rect();
      entry.window->resize(std::min(entry.window->width(),bounds.width()),std::min(entry.window->height(),bounds.height()));
      entry.window->move(std::clamp(entry.window->x(),0,std::max(0,bounds.width()-entry.window->width())),
                         std::clamp(entry.window->y(),0,std::max(0,bounds.height()-entry.window->height())));
   }
   workspace->setActiveSubWindow(entry.window);
   entry.window->raise();
   entry.window->widget()->setFocus(); return true;
}
QtPlotReceiver::Entry &QtPlotReceiver::create(const std::string &name, PlotModel::Kind kind,
      Real x, Real y, Real w, Real h, bool maximized)
{
   remove(name);
   auto &entry=entries[text(name)]; entry.data=std::make_shared<PlotModel>(kind); entry.data->title=text(name);
   if (kind==PlotModel::Kind::Orbit && cameraSettings.contains(text(name))) {
      const auto setting=cameraSettings.value(text(name));
      for (const auto &definition:setting.vectors) {
         PlotVector vector; vector.name=definition.name; vector.source=definition.source; vector.destination=definition.destination;
         vector.label=definition.label; vector.bodyFixed=definition.type=="Body-Fixed"; vector.automaticLength=definition.automaticLength;
         vector.length=definition.length; vector.color=rgb(definition.color); vector.color.setAlphaF(.8);
         vector.start=definition.start; vector.direction=definition.direction; entry.data->vectors.append(vector);
      }
      entry.data->perspective=setting.perspective; entry.data->fieldOfView=setting.fieldOfView;
      entry.data->automaticTrajectory=setting.automaticTrajectory;
      entry.data->automaticBody=setting.automaticBody;
      entry.data->segmentFrame=setting.segmentFrame;
      // OF's root has no visible axes. Its override radius is stored in a
      // float bounding sphere; with LookAt it instead uses the unit fallback.
      const auto automaticRadius=[](const auto &camera) {
         if (!camera.segmentFrame.isEmpty()) return 1.0;
         return camera.automaticTrajectory=="CoordinateSystem" ? (camera.lookAtRotation ? 1.0 :
            static_cast<double>(static_cast<float>(12*GmatSolarSystemDefaults::PLANET_EQUATORIAL_RADIUS[GmatSolarSystemDefaults::EARTH]))) : 0.0;
      };
      entry.data->automaticRadius=automaticRadius(setting);
      const auto resetHistory=[&](const QtCameraReset &reset) {
         return PlotCameraReset{{},reset.automaticTrajectory,automaticRadius(reset),reset.automaticBody,reset.segmentFrame};
      };
      if (setting.reset) entry.data->resetView=resetHistory(*setting.reset);
      if (!setting.views.isEmpty()) {
         entry.data->cameraViews.append({setting.primaryName.isEmpty() ? QString("Script camera") : setting.primaryName,setting.perspective,setting.fieldOfView,{},setting.automaticTrajectory,automaticRadius(setting),setting.automaticBody,setting.segmentFrame});
         for (const auto &view:setting.views) {
            entry.data->cameraViews.append({view.name,view.perspective,view.fieldOfView,{},view.automaticTrajectory,automaticRadius(view),view.automaticBody,view.segmentFrame});
            if (view.reset) entry.data->cameraViews.back().reset=resetHistory(*view.reset);
         }
      }
   }
   show(text(name));
   const auto bounds=workspace->viewport()->rect();
   const int width=std::clamp(static_cast<int>((w>0 && w<=1 ? w : .6)*bounds.width()),200,std::max(200,bounds.width()));
   const int height=std::clamp(static_cast<int>((h>0 && h<=1 ? h : .6)*bounds.height()),160,std::max(160,bounds.height()));
   const int left=std::clamp(static_cast<int>((std::isfinite(x) ? std::clamp(x,0.0,1.0) : 0)*bounds.width()),0,std::max(0,bounds.width()-width));
   const int top=std::clamp(static_cast<int>((std::isfinite(y) ? std::clamp(y,0.0,1.0) : 0)*bounds.height()),0,std::max(0,bounds.height()-height));
   entry.window->showNormal(); entry.window->setGeometry(left,top,width,height);
   if (maximized) entry.window->showMaximized();
   entry.automaticGeometry=!maximized && x==0 && y==0 && w==0 && h==0;
   if (entry.automaticGeometry) {
      QList<Entry *> automatic;
      // A user move/resize ends automatic placement for that existing view.
      for (auto &candidate:entries) if (candidate.automaticGeometry && candidate.window) {
         if (!candidate.automaticRect.isNull() && candidate.window->geometry()!=candidate.automaticRect)
            candidate.automaticGeometry=false;
         else automatic.append(&candidate);
      }
      // Unspecified subscriber positions should expose every initial viewer.
      // Equal (0,0) fallbacks let the last GroundTrack cover the OrbitView.
      const int columns=static_cast<int>(std::ceil(std::sqrt(automatic.size())));
      const int rows=(automatic.size()+columns-1)/columns;
      for (int i=0;i<automatic.size();++i) {
         auto &candidate=*automatic[i];
         const int column=i%columns,row=i/columns;
         const int x0=column*bounds.width()/columns,y0=row*bounds.height()/rows;
         const int x1=(column+1)*bounds.width()/columns,y1=(row+1)*bounds.height()/rows;
         candidate.automaticRect=QRect(x0,y0,x1-x0,y1-y0);
         candidate.window->setGeometry(candidate.automaticRect);
         candidate.window->raise();
      }
   }
   // Restoring normal geometry can reactivate the formerly maximized script.
   // Activate the plot only after its final window state has been applied.
   workspace->setActiveSubWindow(entry.window);
   entry.window->raise();
   entry.window->widget()->setFocus();
   if (changed) changed(); return entry;
}
void QtPlotReceiver::refresh(Entry &entry, bool force)
{
   if (entry.xyRedrawSuspended) return;
   if (++entry.data->pendingUpdates >= entry.data->updateFrequency || force) {
      entry.data->pendingUpdates=0;
      if (entry.widget) {
         entry.widget->refresh();
         if (sharedReplayPosition) entry.widget->setSharedReplayPosition(*sharedReplayPosition);
      }
   }
}
bool QtPlotReceiver::remove(const std::string &name)
{
   auto it=entries.find(text(name)); if (it==entries.end()) return false;
   if (it->data->kind==PlotModel::Kind::Orbit && it->data->userView) savedViews.insert(it.key(),*it->data->userView);
   if (it->window) delete it->window.data(); entries.erase(it); if (changed) changed(); return true;
}
void QtPlotReceiver::warn(const std::string &name, const std::string &option)
{
   const QString key=text(name+":"+option); if (warnings.contains(key)) return;
   warnings.insert(key); MessageInterface::ShowMessage("Qt plot '%s': %s is not yet supported.\n",name.c_str(),option.c_str());
}
bool QtPlotReceiver::CreateGlPlotWindow(const std::string &name,const std::string &oldName,Real x,Real y,Real w,Real h,bool maximized,Integer redrawPoints)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,currentView==GmatPlot::GROUND_TRACK_PLOT ? PlotModel::Kind::GroundTrack : PlotModel::Kind::Orbit,x,y,w,h,maximized);
   entry.data->redrawPoints=std::max(0,static_cast<int>(redrawPoints));
   if (entry.data->kind==PlotModel::Kind::GroundTrack) { entry.data->xLabel="Longitude (deg)"; entry.data->yLabel="Latitude (deg)"; }
   return true;
}
void QtPlotReceiver::SetGlSolarSystem(const std::string &name, SolarSystem *solarSystem)
{
   if (auto *entry=find(name)) entry->solarSystem=solarSystem;
}
void QtPlotReceiver::SetGlObject(const std::string &name,const StringArray &names,const std::vector<SpacePoint *> &points)
{
   auto *entry=find(name); if (!entry) return;
   entry->objects=names; entry->points=points;
   const auto setting=cameraSettings.value(text(name));
   // Retain physical bounds for a secondary body camera whose reference is
   // absent from the drawn objects. Replay must not consult engine pointers.
   const auto cameraRadius=[&](const QString &body,const QString &segment) {
      if (body.isEmpty()) return 0.0;
      if (!segment.isEmpty()) return 1.0;
      SpacePoint *point=nullptr;
      for (auto *candidate:points) if (candidate && text(candidate->GetName())==body) { point=candidate; break; }
      if (!point && entry->solarSystem) point=entry->solarSystem->GetBody(body.toStdString());
      if (auto *celestial=dynamic_cast<CelestialBody *>(point)) return celestial->GetEquatorialRadius();
      return 1.0;
   };
   if (!setting.automaticBody.isEmpty()) entry->data->automaticRadius=cameraRadius(setting.automaticBody,setting.segmentFrame);
   for (int index=0;index<setting.views.size();++index) if (!setting.views[index].automaticBody.isEmpty())
      entry->data->cameraViews[index+1].automaticRadius=cameraRadius(setting.views[index].automaticBody,setting.views[index].segmentFrame);
   if (setting.reset && !setting.reset->automaticBody.isEmpty()) entry->data->resetView->automaticRadius=cameraRadius(setting.reset->automaticBody,setting.reset->segmentFrame);
   for (int index=0;index<setting.views.size();++index) if (setting.views[index].reset && !setting.views[index].reset->automaticBody.isEmpty())
      entry->data->cameraViews[index+1].reset->automaticRadius=cameraRadius(setting.views[index].reset->automaticBody,setting.views[index].reset->segmentFrame);
   for (size_t i=0;i<names.size();++i) {
      auto &curve=entry->data->curves[static_cast<int>(i)]; curve.name=text(names[i]);
      const auto setting=cameraSettings.value(text(name));
      if (setting.objectLabels.contains(curve.name)) curve.importedLabel=setting.objectLabels.value(curve.name);
      curve.centerMarker=setting.objectCenters.value(curve.name,false);
      curve.endpointMarkers=setting.objectEndpoints.value(curve.name,false);
      curve.objectAxes=setting.objectAxes.value(curve.name,false);
      curve.objectGrid=setting.objectGrids.value(curve.name,false);
      curve.objectXYPlane=setting.objectXYPlanes.value(curve.name,false);
      curve.objectVelocity=setting.objectVelocities.value(curve.name,false);
      curve.segmentLabels=setting.objectSegmentLabels.value(curve.name,false);
      curve.orbitMarkerSize=setting.objectMarkerSizes.value(curve.name,10);
      curve.importedLineWidth=setting.objectLineWidths.contains(curve.name) ? std::optional<double>(setting.objectLineWidths.value(curve.name)) : std::nullopt;
      curve.importedFontSize=setting.objectFontSizes.contains(curve.name) ? std::optional<quint32>(setting.objectFontSizes.value(curve.name)) : std::nullopt;
      curve.importedFontPosition=setting.objectFontPositions.value(curve.name);
      if (i<points.size() && points[i]) {
         curve.color=rgb(points[i]->GetCurrentOrbitColor());
         if (auto *body=dynamic_cast<CelestialBody *>(points[i])) {
            curve.radius=body->GetEquatorialRadius();
            curve.texturePath=text(body->GetStringParameter(body->GetParameterID("TextureMapFullPath")));
            if (!curve.texturePath.isEmpty()) curve.texturePath=QFileInfo(curve.texturePath).absoluteFilePath();
            curve.modelPath=text(body->GetStringParameter(body->GetParameterID("3DModelFileFullPath")));
            if (!curve.modelPath.isEmpty()) curve.modelPath=QFileInfo(curve.modelPath).absoluteFilePath();
            for (int axis=0;axis<3;++axis) {
               const std::string suffix(1,"XYZ"[axis]);
               curve.modelOffset[axis]=body->GetRealParameter(body->GetParameterID("3DModelOffset"+suffix));
               curve.modelRotation[axis]=body->GetRealParameter(body->GetParameterID("3DModelRotation"+suffix));
            }
            curve.modelScale=body->GetRealParameter(body->GetParameterID("3DModelScale"));
         }
         if (auto *spacecraft=dynamic_cast<Spacecraft *>(points[i])) {
            curve.modelPath=text(spacecraft->GetModelFileFullPath());
            if (!curve.modelPath.isEmpty()) curve.modelPath=QFileInfo(curve.modelPath).absoluteFilePath();
            const char *axes[]={"X","Y","Z"};
            for (int axis=0;axis<3;++axis) {
               curve.modelOffset[axis]=spacecraft->GetRealParameter(spacecraft->GetParameterID(std::string("ModelOffset")+axes[axis]));
               curve.modelRotation[axis]=spacecraft->GetRealParameter(spacecraft->GetParameterID(std::string("ModelRotation")+axes[axis]));
            }
            curve.modelScale=spacecraft->GetRealParameter(spacecraft->GetParameterID("ModelScale"));
         }
      }
   }
   entry->data->sensorMasks.clear();
   if (entry->data->kind==PlotModel::Kind::Orbit) for (const auto &definition:setting.sensorMasks) {
      try { entry->data->sensorMasks.append(sensorMaskGeometry(definition)); }
      catch (BaseException &error) { warn(name,(definition.name+": "+text(error.GetFullMessage())+" (mask hidden)").toStdString()); }
      catch (const std::exception &error) { warn(name,(definition.name+": "+QString::fromUtf8(error.what())+" (mask hidden)").toStdString()); }
   }
}
void QtPlotReceiver::SetGlCoordSystem(const std::string &name,CoordinateSystem *internal,CoordinateSystem *view,CoordinateSystem *viewUp)
{
   if (auto *entry=find(name)) { entry->internal=internal; entry->view=view; entry->viewUp=viewUp; entry->data->coordinates=view ? text(view->GetName()) : QString(); }
}
void QtPlotReceiver::SetGl2dDrawingOption(const std::string &name,const std::string &,const std::string &map,Integer footprint)
{
   SetGroundTrackOption(name,"TextureMap",map);
   if (auto *entry=find(name)) entry->data->footprints=footprint!=0;
}
void QtPlotReceiver::SetGl3dDrawingOption(const std::string &name,bool labels,bool ec,bool xy,bool wire,bool axes,bool grid,bool sun,bool,bool useInitial,bool stars,bool constellations,Integer count)
{
   if (auto *entry=find(name)) {
      auto &data=*entry->data;
      entry->useInitialView=useInitial;
      data.labels=labels; data.axes=axes; data.grid=grid;
      const auto drawing=cameraSettings.value(text(name));
      data.axesLabels=drawing.axesLabels.value_or(true); data.frameLabel=drawing.frameLabel.value_or(false);
      data.xyPlane=xy; data.eclipticPlane=ec; data.wireframe=wire; data.sunLine=sun;
      data.constellationsEnabled=constellations;
      data.starsEnabled=stars; data.starCount=static_cast<int>(std::clamp<Integer>(count,0,std::numeric_limits<int>::max()));
      data.starOptions.reset(); data.printBackground=false;
      if (drawing.stars) {
         const auto &options=*drawing.stars;
         data.starOptions=PlotStarOptions{};
         if (options.preset=="Custom") {
            data.starOptions=PlotStarOptions{options.minMagnitude,options.maxMagnitude,options.minPixels,options.maxPixels,options.minDimRatio};
            data.starCount=options.customCount;
         } else {
            data.starCount=40000;
            if (options.preset=="Virtual Reality") { data.starOptions->maxPixels=5; data.starOptions->minDimRatio=.1; }
            if (options.preset=="Printer") { data.printBackground=true; data.starsEnabled=false; }
         }
      }
      loadSkyCatalogs(*entry,name);
   }
}
void QtPlotReceiver::loadSkyCatalogs(Entry &entry,const std::string &name)
{
   auto &data=*entry.data;
   if (data.constellationsEnabled && !data.constellationCatalogLoaded) {
      data.constellationCatalogLoaded=true;
      try {
         data.constellationCatalog=ConstellationCatalog::read(text(FileManager::Instance()->FindPath("","CONSTELLATION_FILE",true,false,true)));
      } catch (BaseException &error) { data.constellationCatalog.error=text(error.GetFullMessage()); }
      if (!data.constellationCatalog.error.isEmpty())
         MessageInterface::ShowMessage("Qt OrbitView '%s': cannot load constellations: %s\n",name.c_str(),data.constellationCatalog.error.toStdString().c_str());
   }
   if (data.starsEnabled && !data.starCatalogLoaded) {
      data.starCatalogLoaded=true;
      try {
         const auto stars=cameraSettings.value(text(name)).stars;
         const auto path=stars ? FileManager::Instance()->FindPath((stars->preset=="Custom" ? stars->catalog : QString("inp_StarsHYGv3.txt")).toStdString(),"STAR_PATH",true,true,false)
            : FileManager::Instance()->FindPath("","STAR_FILE",true,false,true);
         data.starCatalog=StarCatalog::read(text(path),data.starOptions.has_value());
      } catch (BaseException &error) { data.starCatalog.error=text(error.GetFullMessage()); }
      if (!data.starCatalog.error.isEmpty())
         MessageInterface::ShowMessage("Qt OrbitView '%s': cannot load star catalog: %s\n",name.c_str(),data.starCatalog.error.toStdString().c_str());
      else if (data.starCatalog.rejectedLines)
         MessageInterface::ShowMessage("Qt OrbitView '%s': skipped %d invalid star catalog lines.\n",name.c_str(),data.starCatalog.rejectedLines);
   }
}
void QtPlotReceiver::SetGl3dViewOption(const std::string &name,SpacePoint *reference,SpacePoint *position,SpacePoint *direction,Real scale,
      const Rvector3 &referenceVector,const Rvector3 &positionVector,const Rvector3 &directionVector,const std::string &upAxis,
      bool referenceIsVector,bool positionIsVector,bool directionIsVector)
{
   if (auto *entry=find(name)) {
      entry->cameraReference=reference; entry->cameraPosition=position; entry->cameraDirection=direction;
      entry->referenceIsVector=referenceIsVector; entry->positionIsVector=positionIsVector; entry->directionIsVector=directionIsVector;
      entry->cameraScale=scale; entry->upVector={0,0,0};
      const auto axis=upAxis.empty() ? 'Z' : upAxis.back(); const int index=axis=='X' ? 0 : axis=='Y' ? 1 : 2;
      entry->upVector[index]=!upAxis.empty() && upAxis.front()=='-' ? -1 : 1;
      if (const auto setting=cameraSettings.value(text(name));setting.up) entry->upVector=*setting.up;
      for (int i=0;i<3;++i) { entry->referenceVector[i]=referenceVector[i]; entry->positionVector[i]=positionVector[i]; entry->directionVector[i]=directionVector[i]; }
      entry->data->scriptedCamera=true;
      if (entry->widget) {
         if (!entry->useInitialView && savedViews.contains(text(name))) entry->widget->restoreView(savedViews.value(text(name)));
         else entry->widget->canvas()->scriptView();
      }
   }
}
void QtPlotReceiver::SetGlDrawOrbitFlag(const std::string &name,const std::vector<bool> &flags)
{
   if (auto *entry=find(name)) for (size_t i=0;i<flags.size();++i) { auto &curve=entry->data->curves[static_cast<int>(i)]; const auto settings=cameraSettings.value(text(name)); curve.lines=settings.objectTrajectories.value(curve.name,flags[i]); }
}
void QtPlotReceiver::SetGlShowObjectFlag(const std::string &name,const std::vector<bool> &flags)
{
   if (auto *entry=find(name)) for (size_t i=0;i<flags.size();++i) entry->data->curves[static_cast<int>(i)].showObject=flags[i];
}
void QtPlotReceiver::SetGlUpdateFrequency(const std::string &name,Integer frequency) { if (auto *entry=find(name)) entry->data->updateFrequency=std::max(1,static_cast<int>(frequency)); }
bool QtPlotReceiver::IsThere(const std::string &name) { auto *entry=find(name); return entry && entry->window && entry->window->isVisible(); }
bool QtPlotReceiver::InitializeGlPlot(const std::string &name) { return find(name)!=nullptr; }
bool QtPlotReceiver::RefreshGlPlot(const std::string &name) { if (auto *entry=find(name)) { refresh(*entry,true); return true; } return false; }
bool QtPlotReceiver::DeleteGlPlot(const std::string &name) { return remove(name); }
bool QtPlotReceiver::SetGlEndOfRun(const std::string &name) { if (auto *entry=find(name)) { entry->data->endOfRun=true; refresh(*entry,true); return true; } return false; }
void QtPlotReceiver::SetMaxGlDataPoints(const std::string &name,Integer count) { if (auto *entry=find(name)) { entry->data->maxPoints=static_cast<int>(std::clamp<Integer>(count,1,std::numeric_limits<int>::max())); entry->data->trim(); if (cameraSettings.value(text(name)).solverLastN) entry->trialHistory.synchronize(*entry->data); } }
bool QtPlotReceiver::UpdateGlPlot(const std::string &name,const std::string &,const StringArray &names,const Real &epoch,
      const RealArray &x,const RealArray &y,const RealArray &z,const RealArray &vx,const RealArray &vy,const RealArray &vz,
      const ColorMap &colors,const ColorMap &targetColors,bool solving,Integer,bool update,bool drawing,bool)
{
   auto *entry=find(name); if (!entry) return false;
   auto availability=entry->dataAvailability;
   if (!entry->arcMetadataOnly) entry->dataAvailability.reset();
   if (availability) {
      // Arc poses contain only requested/present spacecraft; the real callback
      // contains the complete spacecraft list associated with this mask.
      bool matches=availability->epoch==epoch &&
         (entry->arcMetadataOnly || availability->objects.size()==static_cast<int>(names.size()));
      for (const auto &object:names) matches=matches && availability->objects.contains(text(object));
      if (!matches) { availability.reset(); warn(name,"stale or mismatched orbit data availability (ignored)"); }
   }
   const auto absent=[&](const QString &object) {
      return availability && availability->objects.contains(object) && !availability->objects.value(object);
   };
   auto &data=*entry->data; if (!entry->arcMetadataOnly) ++data.frame;
   auto storeCamera=[&](int index,PlotCamera camera) {
      if (entry->arcMetadataOnly) { entry->preparedArcCameras[index]=camera; return; }
      auto &history=cameraHistory(data,index);
      history.push_back(camera);
      while (history.size()>static_cast<size_t>(data.maxPoints)) history.pop_front();
   };
   auto updateCameras=[&](bool segmentPass) {
   if (segmentPass && entry->replayArcCamera) {
      for (auto it=entry->replayArcCameras.cbegin();it!=entry->replayArcCameras.cend();++it) {
         auto camera=it.value(); camera.frame=data.frame; storeCamera(it.key(),camera);
      }
      return;
   }
   if (data.kind==PlotModel::Kind::Orbit && data.scriptedCamera) {
      auto segmentPoint=[&](const QString &frame) -> const PlotPoint * {
         if (frame.isEmpty()) return nullptr;
         return firstRegularArcPose(data,frame);
      };
      auto resolve=[&](bool vector,SpacePoint *object,const std::array<double,3> &value) {
         auto checked=[](const std::array<double,3> &position) {
            if (!PlotModel::usableOrbitPosition(position[0],position[1],position[2]))
               throw std::runtime_error("Unavailable or unrenderable camera position");
            return position;
         };
         if (vector) return checked(value);
         if (!object) throw std::runtime_error("Missing camera reference object");
         if (absent(text(object->GetName()))) throw std::runtime_error("Camera reference was absent from this publication");
         const auto found=std::find(names.begin(),names.end(),object->GetName());
         if (found!=names.end()) {
            const auto index=static_cast<size_t>(found-names.begin());
            if (index<x.size() && index<y.size() && index<z.size()) return checked({x[index],y[index],z[index]});
         }
         auto state=object->GetMJ2000State(epoch);
         if (entry->internal && entry->view && entry->internal!=entry->view) {
            Rvector6 converted; CoordinateConverter converter;
            converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
         }
         return checked({state[0],state[1],state[2]});
      };
      const auto settings=cameraSettings.value(text(name));
      auto transform=[&](SpacePoint *object,const std::array<double,3> &origin,const std::array<double,3> &target,bool aligned,bool shortest,const PlotPoint *pose=nullptr) {
         Rmatrix33 frame;
         if (pose) {
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) frame(row,col)=pose->bodyToView[row*3+col];
         } else if (object) {
            Rmatrix33 viewToBase;
            if (entry->view) {
               entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
               viewToBase=entry->view->GetLastRotationMatrix();
            }
            auto bodyToBase=object->GetAttitude(epoch);
            if (object->IsOfType(Gmat::SPACECRAFT)) bodyToBase=bodyToBase.Transpose();
            frame=viewToBase.Transpose()*bodyToBase;
         }
         const auto direction=frame.Transpose()*Rvector3(target[0]-origin[0],target[1]-origin[1],target[2]-origin[2]);
         const auto alignment=aligned ? cameraAlignment(osg::Vec3d(direction[0],direction[1],direction[2]),shortest) : osg::Quat();
         return [frame,alignment](const std::array<double,3> &vector) {
            const auto local=alignment*osg::Vec3d(vector[0],vector[1],vector[2]);
            const auto world=frame*Rvector3(local.x(),local.y(),local.z());
            return std::array<double,3>{world[0],world[1],world[2]};
         };
      };
      const auto *primaryPose=segmentPass ? segmentPoint(settings.segmentFrame) : nullptr;
      if (settings.segmentFrame.isEmpty()!=segmentPass && (!segmentPass || primaryPose)) try {
         PlotCamera camera; camera.frame=data.frame; camera.solver=solving;
         const auto reference=primaryPose ? std::array<double,3>{primaryPose->x,primaryPose->y,primaryPose->z} : resolve(entry->referenceIsVector,entry->cameraReference,entry->referenceVector);
         const auto target=primaryPose && !settings.lookAtRotation ? reference : resolve(entry->directionIsVector,entry->cameraDirection,entry->directionVector);
         if ((settings.bodyRelative || settings.lookAtRotation) && !entry->positionIsVector)
            throw std::runtime_error("Relative camera position must be a vector");
         if (settings.bodyRelative && !entry->cameraReference) throw std::runtime_error("Missing body-relative camera reference");
         const auto orient=transform(settings.bodyRelative ? entry->cameraReference : nullptr,reference,target,settings.lookAtRotation,settings.shortestAngle,settings.bodyRelative ? primaryPose : nullptr);
         const auto position=orient(resolve(entry->positionIsVector,entry->cameraPosition,entry->positionVector));
         camera.target=settings.lookAtRotation ? reference : target;
         const auto orientedUp=orient(entry->upVector);
         Rvector3 up(orientedUp[0],orientedUp[1],orientedUp[2]);
         if (!settings.bodyRelative && !settings.lookAtRotation && entry->viewUp && entry->view && entry->viewUp!=entry->view) {
            entry->viewUp->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
            const auto upToBase=entry->viewUp->GetLastRotationMatrix();
            entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
            up=entry->view->GetLastRotationMatrix().Transpose()*upToBase*up;
         }
         if (settings.centerOffset) {
            const auto offset=orient(*settings.centerOffset);
            for (int i=0;i<3;++i) camera.target[i]+=offset[i];
         }
         for (int i=0;i<3;++i) {
            camera.eye[i]=reference[i]+entry->cameraScale*position[i]; camera.up[i]=up[i];
            if (!std::isfinite(camera.eye[i]) || !std::isfinite(camera.target[i]) || !std::isfinite(camera.up[i]))
               throw std::runtime_error("Nonfinite camera coordinates");
         }
         if (!PlotModel::usableOrbitPosition(camera.eye[0],camera.eye[1],camera.eye[2]) ||
             !PlotModel::usableOrbitPosition(camera.target[0],camera.target[1],camera.target[2]))
            throw std::runtime_error("Unrenderable camera coordinates");
         if (std::hypot(camera.eye[0]-camera.target[0],camera.eye[1]-camera.target[1],camera.eye[2]-camera.target[2])<1e-9)
            throw std::runtime_error("Camera eye and target coincide");
         storeCamera(0,camera);
      } catch (BaseException &) { warn(name,"unresolved scripted camera (using available camera or manual view)"); }
      catch (const std::exception &) { warn(name,"invalid scripted camera (using available camera or manual view)"); }
      auto captureView=[&](const auto &view,int key,const QString &label) {
         if (view.segmentFrame.isEmpty()==segmentPass) return;
         const auto *pose=segmentPass ? segmentPoint(view.segmentFrame) : nullptr;
         if (segmentPass && !pose) return; // Named segment has not arrived yet.
         try {
            auto findObject=[&](const QString &objectName) -> SpacePoint * {
               if (objectName.isEmpty() || objectName=="CoordinateSystem") return nullptr;
               SpacePoint *object=nullptr;
               for (auto *point:entry->points) if (point && text(point->GetName())==objectName) { object=point; break; }
               if (!object && entry->solarSystem) object=entry->solarSystem->GetBody(objectName.toStdString());
               if (!object) object=dynamic_cast<SpacePoint *>(Moderator::Instance()->GetInternalObject(objectName.toStdString()));
               return object;
            };
            auto position=[&](const QString &objectName) {
               if (objectName.isEmpty() || objectName=="CoordinateSystem") return std::array<double,3>{};
               return resolve(false,findObject(objectName),{});
            };
            const auto origin=pose ? std::array<double,3>{pose->x,pose->y,pose->z} : position(view.reference);
            const auto target=view.target.isEmpty() ? origin : position(view.target);
            auto *reference=view.bodyRelative ? findObject(view.reference) : nullptr;
            if (view.bodyRelative && !reference) throw std::runtime_error("Missing body-relative camera reference");
            const auto orient=transform(reference,origin,target,view.lookAtRotation,view.shortestAngle,view.bodyRelative ? pose : nullptr);
            const auto eye=orient(view.eye),center=orient(view.center);
            PlotCamera camera; camera.frame=data.frame; camera.solver=solving;
            camera.up=orient(view.up);
            for (int axis=0;axis<3;++axis) {
               camera.eye[axis]=origin[axis]+eye[axis];
               camera.target[axis]=(view.lookAtRotation ? origin[axis] : target[axis])+center[axis];
               if (!std::isfinite(camera.eye[axis]) || !std::isfinite(camera.target[axis]) || !std::isfinite(camera.up[axis])) throw std::runtime_error("Nonfinite camera position");
            }
            if (!PlotModel::usableOrbitPosition(camera.eye[0],camera.eye[1],camera.eye[2]) ||
                !PlotModel::usableOrbitPosition(camera.target[0],camera.target[1],camera.target[2]))
               throw std::runtime_error("Unrenderable camera coordinates");
            if (std::hypot(camera.eye[0]-camera.target[0],camera.eye[1]-camera.target[1],camera.eye[2]-camera.target[2])<1e-9)
               throw std::runtime_error("Camera eye and target coincide");
            storeCamera(key,camera);
         } catch (...) { warn(name,("unresolved camera "+label+" (using its available history or manual view)").toStdString()); }
      };
      if (settings.reset) captureView(*settings.reset,-1,settings.primaryName+" default");
      for (int index=0;index<settings.views.size();++index) {
         const auto &view=settings.views[index]; captureView(view,index+1,view.name);
         if (view.reset) captureView(*view.reset,-index-2,view.name+" default");
      }
   }
   };
   if (entry->arcMetadataOnly) { updateCameras(true); return true; }
   updateCameras(false);
   Rvector6 sunState;
   const bool hasSun=data.kind==PlotModel::Kind::Orbit && entry->solarSystem && entry->internal && entry->view;
   if (hasSun) {
      sunState=entry->solarSystem->GetBody("Sun")->GetMJ2000State(epoch);
      if (entry->internal!=entry->view) {
         Rvector6 converted; CoordinateConverter converter;
         converter.Convert(epoch,sunState,entry->internal,converted,entry->view); sunState=converted;
      }
   }
   if (!entry->ignoreTimeSequence && data.frame>1 && epoch<data.lastEpoch) data.breakLines();
   for (size_t i=0;i<entry->objects.size();++i) {
      const auto &object=entry->objects[i];
      auto &curve=data.curves[static_cast<int>(i)];
      const auto &palette=solving ? targetColors : colors;
      const auto color=palette.find(object); if (color!=palette.end()) curve.color=rgb(color->second);
      if (absent(text(object))) { curve.breakNext=true; continue; }
      auto found=std::find(names.begin(),names.end(),object);
      double px=0,py=0,pz=0; std::array<double,3> velocity{}; bool hasVelocity=false;
      if (found!=names.end()) {
         size_t index=static_cast<size_t>(found-names.begin());
         if (index>=x.size() || index>=y.size() || index>=z.size()) { curve.breakNext=true; continue; }
         // OrbitPlot::BufferOrbitData already converted these into view CS.
         px=x[index]; py=y[index]; pz=z[index];
         if (index<vx.size() && index<vy.size() && index<vz.size()) {
            velocity={vx[index],vy[index],vz[index]};
            hasVelocity=std::all_of(velocity.begin(),velocity.end(),PlotModel::usableValue);
         }
      } else if (i<entry->points.size() && entry->points[i] && !entry->points[i]->IsOfType(Gmat::SPACECRAFT)) {
         auto state=entry->points[i]->GetMJ2000State(epoch);
         if (entry->internal && entry->view && entry->internal!=entry->view) {
            Rvector6 converted; CoordinateConverter converter;
            converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
         }
         px=state[0]; py=state[1]; pz=state[2];
         velocity={state[3],state[4],state[5]};
         hasVelocity=std::all_of(velocity.begin(),velocity.end(),PlotModel::usableValue);
      } else { curve.breakNext=true; continue; }
      if (!PlotModel::usableOrbitPosition(px,py,pz)) { curve.breakNext=true; continue; }
      if (data.kind==PlotModel::Kind::GroundTrack) {
         if (curve.radius>0 || (px==0 && py==0 && pz==0)) continue;
         const double lon=std::atan2(py,px)*degrees, lat=std::atan2(pz,std::hypot(px,py))*degrees;
         data.append(static_cast<int>(i),lon,lat,0,epoch,drawing,solving);
      } else {
         data.append(static_cast<int>(i),px,py,pz,epoch,drawing,solving);
         if (!curve.points.empty() && curve.points.back().frame==data.frame) {
            curve.points.back().hasSun=hasSun;
            curve.points.back().viewVelocity=velocity;
            curve.points.back().hasVelocity=hasVelocity;
            for (int axis=0;axis<3;++axis) curve.points.back().sunPosition[axis]=sunState[axis];
         }
         if (!curve.points.empty() && curve.points.back().frame==data.frame &&
             i<entry->points.size() && entry->points[i]) {
            // Celestial-body GetAttitude returns body-fixed to MJ2000Eq.
            // The coordinate system rotation maps plot coordinates to that base.
            Rmatrix33 viewToBase;
            if (entry->view) {
               entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true);
               viewToBase=entry->view->GetLastRotationMatrix();
            }
            auto attitude=entry->points[i]->GetAttitude(epoch);
            // Spacecraft attitude maps inertial to body, unlike celestial bodies.
            if (entry->points[i]->IsOfType(Gmat::SPACECRAFT)) attitude=attitude.Transpose();
            const auto rotation=viewToBase.Transpose()*attitude;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
               curve.points.back().bodyToView[row*3+col]=rotation(row,col);
               curve.points.back().inertialToView[row*3+col]=viewToBase(col,row);
            }
         }
      }
   }
   for (auto &curve:data.curves) if (!curve.points.empty() && curve.points.back().frame==data.frame) {
      auto &point=curve.points.back(); point.provider=entry->provider;
      const auto arcs=data.regularArcs.constFind(curve.name);
      if (!solving && arcs!=data.regularArcs.cend() && !arcs->isEmpty() && arcs->back().providers.contains(entry->arcProvider))
         point.regularArc=arcs->back().id;
   }
   if (data.kind==PlotModel::Kind::Orbit && data.active) for (auto &vector:data.vectors) {
      PlotVectorSample sample; sample.source.frame=data.frame; sample.source.epoch=epoch; sample.source.solver=solving;
      try {
         auto findObject=[&](const QString &objectName) -> SpacePoint * {
            for (auto *point:entry->points) if (point && text(point->GetName())==objectName) return point;
            if (entry->solarSystem) if (auto *body=entry->solarSystem->GetBody(objectName.toStdString())) return body;
            return dynamic_cast<SpacePoint *>(Moderator::Instance()->GetInternalObject(objectName.toStdString()));
         };
         auto position=[&](SpacePoint *object,const QString &objectName) {
            if (absent(objectName)) throw std::runtime_error("Vector space point was absent from this publication");
            const auto found=std::find(names.begin(),names.end(),objectName.toStdString());
            std::array<double,3> value{};
            if (found!=names.end()) {
               const auto index=static_cast<size_t>(found-names.begin());
               if (index>=x.size() || index>=y.size() || index>=z.size()) throw std::runtime_error("Vector state array is incomplete");
               value={x[index],y[index],z[index]};
            } else {
               if (!object) throw std::runtime_error("Vector space point is unavailable");
               auto state=object->GetMJ2000State(epoch);
               if (entry->internal && entry->view && entry->internal!=entry->view) {
                  Rvector6 converted; CoordinateConverter converter;
                  converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
               }
               value={state[0],state[1],state[2]};
            }
            if (!PlotModel::usableOrbitPosition(value[0],value[1],value[2])) throw std::runtime_error("Vector position is unavailable or unrenderable");
            return value;
         };
         auto *source=findObject(vector.source);
         if (!source) throw std::runtime_error("Vector source is unavailable");
         const auto origin=position(source,vector.source);
         sample.source.x=origin[0]; sample.source.y=origin[1]; sample.source.z=origin[2];
         bool retainedPose=false;
         for (const auto &curve:data.curves) if (curve.name==vector.source && !curve.points.empty() && curve.points.back().frame==data.frame) {
            sample.source=curve.points.back(); sample.sourceRadius=curve.radius>0 ? curve.radius : 1; retainedPose=true; break;
         }
         if (!retainedPose) {
            if (auto *body=dynamic_cast<CelestialBody *>(source)) sample.sourceRadius=body->GetEquatorialRadius();
            Rmatrix33 viewToBase;
            if (entry->view) { entry->view->ToBaseSystem(A1Mjd(epoch),Rvector6(),true); viewToBase=entry->view->GetLastRotationMatrix(); }
            auto attitude=source->GetAttitude(epoch);
            if (source->IsOfType(Gmat::SPACECRAFT)) attitude=attitude.Transpose();
            const auto rotation=viewToBase.Transpose()*attitude;
            for (int row=0;row<3;++row) for (int col=0;col<3;++col) sample.source.bodyToView[row*3+col]=rotation(row,col);
         }
         for (auto value:sample.source.bodyToView) if (!std::isfinite(value)) throw std::runtime_error("Vector source orientation is nonfinite");
         if (!vector.bodyFixed) sample.destination=position(findObject(vector.destination),vector.destination);
      } catch (...) {
         sample.valid=false;
         warn(name,("unresolved vector "+vector.name+" (hidden at unavailable samples)").toStdString());
      }
      // An invalid sample suppresses this frame instead of replaying a stale
      // direction from an earlier epoch. Trim and solver reset match cameras.
      vector.samples.push_back(sample);
      while (vector.samples.size()>static_cast<size_t>(data.maxPoints)) vector.samples.pop_front();
   }
   // Segment references use positions and attitude captured with this sample.
   updateCameras(true);
   entry->replayArcCamera=false; entry->replayArcCameras.clear();
   if (cameraSettings.value(text(name)).solverLastN) entry->trialHistory.observe(data);
   if (update) refresh(*entry,true); return true;
}
bool QtPlotReceiver::TakeGlAction(const std::string &name,const std::string &action)
{
   if (action=="ClearObjects" && qtResourcePreviewCleanupActive()) return true;
   auto *entry=find(name); if (!entry) return false;
   if (action=="NeedsOrbitDataAvailability") return true;
   if (action.compare(0,22,"OrbitDataAvailability:")==0) {
      // A malformed replacement must not leave an older valid mask armed.
      entry->dataAvailability.reset();
      std::istringstream stream(action.substr(22)); stream.imbue(std::locale::classic());
      OrbitDataAvailability sample; size_t count;
      if (!(stream>>sample.epoch>>count) || !PlotModel::usableValue(sample.epoch) || count>entry->objects.size()) return false;
      for (size_t i=0;i<count;++i) {
         std::string object; int present;
         if (!(stream>>std::quoted(object)>>present) || object.empty() || (present!=0 && present!=1) || sample.objects.contains(text(object))) return false;
         sample.objects.insert(text(object),present!=0);
      }
      stream>>std::ws; if (!stream.eof()) return false;
      entry->dataAvailability=std::move(sample); return true;
   }
   if (action=="NeedsOrbitSolverLifecycle") return cameraSettings.value(text(name)).solverLastN.has_value();
   if (action.compare(0,25,"FinalizeOrbitSolverState:")==0) {
      const auto lastN=cameraSettings.value(text(name)).solverLastN;
      if (!lastN || *lastN<0) return false;
      OrbitTrialHistory::Stage stage;
      const auto state=action.substr(25);
      if (state=="SOLVING") stage=OrbitTrialHistory::Stage::Solving;
      else if (state=="SOLVEDPASS") stage=OrbitTrialHistory::Stage::SolvedPass;
      else if (state=="REGULAR") stage=OrbitTrialHistory::Stage::Regular;
      else return false;
      QSet<QString> spacecraft;
      for (auto *object:entry->points) if (object && object->IsOfType(Gmat::SPACECRAFT)) spacecraft.insert(text(object->GetName()));
      if (entry->trialHistory.finalize(*entry->data,stage,quint64(2)*static_cast<quint64>(*lastN),spacecraft)) refresh(*entry,true);
      return true;
   }
   if (action=="NeedsOrbitArcMetadata") {
      const auto setting=cameraSettings.value(text(name));
      if (setting.solverLastN) return true;
      for (bool enabled:setting.objectSegmentLabels) if (enabled) return true;
      if (!setting.segmentFrame.isEmpty()) return true;
      for (const auto &view:setting.views) if (!view.segmentFrame.isEmpty()) return true;
      return false;
   }
   if (action.compare(0,18,"NeedsOrbitArcPose:")==0) {
      const auto object=text(action.substr(18));
      const auto setting=cameraSettings.value(text(name));
      if (setting.objectSegmentLabels.value(object,false)) return true;
      if (setting.segmentFrame.section('.',0,0)==object) return true;
      for (const auto &view:setting.views) if (view.segmentFrame.section('.',0,0)==object) return true;
      return false;
   }
   if (action.compare(0,17,"SetOrbitProvider:")==0) {
      std::istringstream stream(action.substr(17)); stream.imbue(std::locale::classic());
      std::string identity; int trial; long long sequence;
      if (!(stream>>identity>>trial>>sequence)) return false;
      std::string provider; std::getline(stream,provider); if (!provider.empty()) provider.erase(0,1);
      entry->arcProvider=text(identity); entry->arcTrial=trial!=0; entry->provider=text(provider); return true;
   }
   if (action=="BeginOrbitArcSample") {
      entry->arcPublishedPoses.clear(); entry->preparedArcCameras.clear(); return true;
   }
   if (action.compare(0,13,"OrbitArcPose:")==0) {
      if (entry->arcTrial) return true;
      std::istringstream stream(action.substr(13)); stream.imbue(std::locale::classic());
      std::string object; PlotPoint pose;
      if (!(stream>>object>>pose.epoch>>pose.x>>pose.y>>pose.z)) return false;
      for (auto &value:pose.bodyToView) if (!(stream>>value) || !std::isfinite(value)) return false;
      for (auto &value:pose.inertialToView) if (!(stream>>value) || !std::isfinite(value)) return false;
      if (!PlotModel::usableOrbitPosition(pose.x,pose.y,pose.z) || !PlotModel::usableValue(pose.epoch)) return false;
      pose.frame=entry->data->frame; pose.provider=entry->provider;
      entry->arcPublishedPoses[text(object)]=pose;
      recordRegularArcPose(*entry->data,text(object),entry->arcProvider,entry->provider,pose); return true;
   }
   if (action=="PrepareOrbitArcCameras") {
      // Bodies have no spacecraft publisher arrays. Capture their label endpoints
      // at the same raw publication epoch, before collection/MaxPlotPoints, using
      // the ordinary view conversion. Their regular trajectories merge under the
      // first provider name, as in OF; trials never acquire regular labels.
      if (!entry->arcTrial && entry->dataAvailability) {
         const auto setting=cameraSettings.value(text(name));
         const double epoch=entry->dataAvailability->epoch;
         bool hasPublishedSpacecraft=false;
         for (bool present:entry->dataAvailability->objects) hasPublishedSpacecraft=hasPublishedSpacecraft || present;
         for (auto *object:entry->points) {
            if (!object || object->IsOfType(Gmat::SPACECRAFT) || !setting.objectSegmentLabels.value(text(object->GetName()),false)) continue;
            if (entry->view && entry->view->GetOriginName()==object->GetName() && entry->view->AreAxesOfType("BodyFixedAxes")) continue;
            try {
               auto state=object->GetMJ2000State(epoch);
               if (entry->internal && entry->view && entry->internal!=entry->view) {
                  Rvector6 converted; CoordinateConverter converter;
                  converter.Convert(epoch,state,entry->internal,converted,entry->view); state=converted;
               }
               if (!PlotModel::usableOrbitPosition(state[0],state[1],state[2])) continue;
               PlotPoint pose; pose.epoch=epoch; pose.frame=entry->data->frame; pose.provider=entry->provider;
               pose.x=state[0]; pose.y=state[1]; pose.z=state[2];
               auto &arcs=entry->data->regularArcs[text(object->GetName())];
               if (arcs.isEmpty()) recordRegularArcPose(*entry->data,text(object->GetName()),entry->arcProvider,hasPublishedSpacecraft ? entry->provider : QString(),pose);
               else {
                  auto &arc=arcs.front(); arc.providers.insert(entry->arcProvider);
                  if (pose.epoch<arc.first.epoch) arc.first=pose;
                  if (pose.epoch>arc.last.epoch) arc.last=pose;
               }
            } catch (...) { warn(name,"unavailable body segment label position (retaining its available history)"); }
         }
      }
      if (entry->arcTrial || entry->arcPublishedPoses.isEmpty()) return true;
      StringArray objects; RealArray x,y,z,empty;
      double epoch=0;
      for (auto it=entry->arcPublishedPoses.cbegin();it!=entry->arcPublishedPoses.cend();++it) {
         objects.push_back(it.key().toStdString()); x.push_back(it->x); y.push_back(it->y); z.push_back(it->z); epoch=it->epoch;
      }
      entry->arcMetadataOnly=true; entry->preparedArcCameras.clear();
      try { UpdateGlPlot(name,"",objects,epoch,x,y,z,empty,empty,empty,{}, {},false,0,false,true,false); }
      catch (...) { entry->arcMetadataOnly=false; warn(name,"unavailable segment camera metadata (retaining its available history)"); return false; }
      entry->arcMetadataOnly=false; return true;
   }
   if (action.compare(0,19,"SaveOrbitArcCamera:")==0) {
      auto cameras=entry->preparedArcCameras;
      for (auto &camera:cameras) camera.solver=entry->arcTrial;
      entry->bufferedArcCameras[text(action.substr(19)).toInt()]=std::move(cameras); return true;
   }
   if (action.compare(0,21,"ReplayOrbitArcCamera:")==0) {
      entry->replayArcCamera=true; entry->replayArcCameras=entry->bufferedArcCameras.take(text(action.substr(21)).toInt()); return true;
   }
   if (action=="FinalizeOrbitArc") {
      if (entry->arcTrial) return true;
      for (auto &arcs:entry->data->regularArcs) if (!arcs.isEmpty()) arcs.back().finalized=true;
      for (auto it=entry->preparedArcCameras.cbegin();it!=entry->preparedArcCameras.cend();++it) {
         auto camera=it.value(); camera.frame=entry->data->frame; camera.solver=false;
         auto &history=cameraHistory(*entry->data,it.key());
         if (!history.empty() && history.back().frame==camera.frame && !history.back().solver) history.back()=camera;
         else history.push_back(camera);
         while (history.size()>static_cast<size_t>(entry->data->maxPoints)) history.pop_front();
      }
      refresh(*entry,true); return true;
   }
   if (action.compare(0,16,"SetDataProvider:")==0) { entry->provider=text(action.substr(16)); return true; }
   if (action=="PenUp") { entry->data->penDown=false; entry->data->breakLines(); }
   else if (action=="PenDown") entry->data->penDown=true;
   else if (action=="ToggleOff") { entry->data->active=false; entry->data->breakLines(); }
   else if (action=="ToggleOn") entry->data->active=true;
   else if (action=="ClearObjects") { entry->trialHistory.clear(); entry->dataAvailability.reset(); entry->arcProvider.clear(); entry->provider.clear(); entry->arcTrial=false; entry->arcMetadataOnly=false; entry->data->regularArcs.clear(); entry->data->nextRegularArc=0; entry->arcPublishedPoses.clear(); entry->preparedArcCameras.clear(); entry->bufferedArcCameras.clear(); entry->replayArcCameras.clear(); entry->replayArcCamera=false; entry->objects.clear(); entry->points.clear(); entry->data->sensorMasks.clear(); entry->data->curves.clear(); entry->data->cameras.clear(); for (auto &vector:entry->data->vectors) vector.samples.clear(); if (entry->data->resetView) entry->data->resetView->cameras.clear(); for (auto &view:entry->data->cameraViews) { view.cameras.clear(); if (view.reset) view.reset->cameras.clear(); } }
   else if (action=="ClearSolverData") {
      entry->dataAvailability.reset();
      if (cameraSettings.value(text(name)).solverLastN) {
         entry->trialHistory.clearTrials(*entry->data);
         refresh(*entry,true); return true;
      }
      entry->trialHistory.clear();
      auto clearSolver=[](std::deque<PlotCamera> &cameras) {
         cameras.erase(std::remove_if(cameras.begin(),cameras.end(),[](const PlotCamera &camera) { return camera.solver; }),cameras.end());
      };
      for (auto &vector:entry->data->vectors)
         vector.samples.erase(std::remove_if(vector.samples.begin(),vector.samples.end(),[](const PlotVectorSample &sample) { return sample.source.solver; }),vector.samples.end());
      clearSolver(entry->data->cameras);
      if (entry->data->resetView) clearSolver(entry->data->resetView->cameras);
      for (auto &view:entry->data->cameraViews) { clearSolver(view.cameras); if (view.reset) clearSolver(view.reset->cameras); }
      for (auto &curve:entry->data->curves) {
         curve.points.erase(std::remove_if(curve.points.begin(),curve.points.end(),[](const PlotPoint &point) { return point.solver; }),curve.points.end());
         curve.breakNext=true;
      }
   } else if (action=="IgnoreTimeSequence") entry->ignoreTimeSequence=true;
   else if (action=="ClearData") { entry->trialHistory.clear(); entry->dataAvailability.reset(); entry->arcProvider.clear(); entry->provider.clear(); entry->arcTrial=false; entry->arcMetadataOnly=false; entry->arcPublishedPoses.clear(); entry->preparedArcCameras.clear(); entry->bufferedArcCameras.clear(); entry->replayArcCameras.clear(); entry->replayArcCamera=false; entry->data->clear(); entry->solverBreaks.clear(); entry->solverScope.clear(); }
   else { warn(name,action); return false; }
   refresh(*entry,true); return true;
}

bool QtPlotReceiver::CreateXyPlotWindow(const std::string &name,const std::string &oldName,Real x,Real y,Real w,Real h,bool maximized,const std::string &title,const std::string &xlabel,const std::string &ylabel,bool grid,bool)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::XY,x,y,w,h,maximized);
   entry.data->title=text(title.empty() ? name : title); entry.data->xLabel=text(xlabel); entry.data->yLabel=text(ylabel); entry.data->grid=grid; return true;
}
bool QtPlotReceiver::DeleteXyPlot(const std::string &name) { return remove(name); }
bool QtPlotReceiver::AddXyPlotCurve(const std::string &name,int index,const std::string &title,UnsignedInt color)
{
   auto *entry=find(name); if (!entry || index<0) return false;
   auto &curve=entry->data->curves[index]; curve.name=text(title); curve.color=rgb(color); curve.markerType=index%10; return true;
}
bool QtPlotReceiver::DeleteAllXyPlotCurves(const std::string &name,const std::string &) { if (auto *entry=find(name)) { entry->data->curves.clear(); return true; } return false; }
bool QtPlotReceiver::DeleteXyPlotCurve(const std::string &name,int index) { if (auto *entry=find(name)) return entry->data->curves.remove(index)>0; return false; }
void QtPlotReceiver::ClearXyPlotData(const std::string &name) { if (auto *entry=find(name)) { entry->data->clear(); entry->solverBreaks.clear(); entry->solverScope.clear(); refresh(*entry,true); } }
void QtPlotReceiver::XyPlotPenUp(const std::string &name) { if (auto *entry=find(name)) { entry->data->penDown=false; entry->data->breakLines(); } }
void QtPlotReceiver::XyPlotPenDown(const std::string &name) { if (auto *entry=find(name)) entry->data->penDown=true; }
void QtPlotReceiver::XyPlotDarken(const std::string &name,Integer factor,Integer index,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { for (size_t i=0;i<c.points.size();++i) if (index<0 || static_cast<Integer>(i)>=index) c.points[i].color=c.points[i].color.darker(std::max(100,static_cast<int>(factor))); });
}
void QtPlotReceiver::XyPlotLighten(const std::string &name,Integer factor,Integer index,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { for (size_t i=0;i<c.points.size();++i) if (index<0 || static_cast<Integer>(i)>=index) c.points[i].color=c.points[i].color.lighter(std::max(100,static_cast<int>(factor))); });
}
void QtPlotReceiver::XyPlotMarkPoint(const std::string &name,Integer index,Integer curve) {
   curves(name,curve,[&](PlotCurve &c) {
      const auto position=index<0 ? static_cast<Integer>(c.points.size())-1 : index;
      if (position>=0 && position<static_cast<Integer>(c.points.size())) c.points[position].highlighted=true;
   });
}
void QtPlotReceiver::XyPlotMarkBreak(const std::string &name,Integer index,Integer curve)
{
   auto *entry=find(name); if (!entry) return;
   curves(name,curve,[&](PlotCurve &c) {
      const size_t position=index<0 ? c.points.size() : std::min(c.points.size(),static_cast<size_t>(index));
      const auto frame=position<c.points.size() ? c.points[position].frame : c.points.empty() ? 0 : c.points.back().frame+1;
      if (!entry->solverScope.isEmpty()) entry->solverBreaks[entry->solverScope][c.name]=frame;
      if (!c.breaks.contains(frame)) c.breaks.append(frame);
      c.breakNext=true;
   });
}
void QtPlotReceiver::XyPlotClearFromBreak(const std::string &name,Integer which,Integer end,Integer curve)
{
   auto *entry=find(name); if (!entry) return;
   curves(name,curve,[&](PlotCurve &c) {
      if (c.breaks.isEmpty()) return;
      int index=which<0 ? c.breaks.size()-1 : static_cast<int>(which);
      if (which<0 && !entry->solverScope.isEmpty()) {
         const auto anchors=entry->solverBreaks.value(entry->solverScope);
         if (!anchors.contains(c.name)) return;
         const auto anchor=anchors.value(c.name);
         index=c.breaks.indexOf(anchor);
         if (index<0) {
            // Retention may prune an older parent anchor while preserving the
            // inner one. Its stored frame still identifies all trial samples.
            index=0; while (index<c.breaks.size() && c.breaks[index]<anchor) ++index;
            c.breaks.insert(index,anchor);
         }
      }
      if (index<0 || index>=c.breaks.size()) return;
      const auto frame=c.breaks[index];
      const auto last=end>=0 && end<c.breaks.size() ? c.breaks[end] : std::numeric_limits<quint64>::max();
      if (last<frame) return;
      c.points.erase(std::remove_if(c.points.begin(),c.points.end(),[&](const PlotPoint &point) { return point.frame>=frame && point.frame<last; }),c.points.end());
      for (auto &point:c.points) if (point.frame>=last) { point.connect=false; break; }
      if (end<0) c.breaks.resize(index+1); // Retain the anchor for the next solver iteration.
      else if (end>index) c.breaks.remove(index+1,end-index);
      c.breakNext=true;
   });
}
void QtPlotReceiver::XyPlotChangeColor(const std::string &name,Integer index,UnsignedInt color,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) { c.color=rgb(color); if (index>=0) for (size_t i=index;i<c.points.size();++i) c.points[i].color=c.color; });
}
void QtPlotReceiver::XyPlotChangeMarker(const std::string &name,Integer index,Integer marker,int curve) {
   curves(name,curve,[&](PlotCurve &c) {
      const int type=marker<0 ? c.markerType : marker%10;
      for (size_t i=std::max<Integer>(0,index);i<c.points.size();++i) c.points[i].marker=type;
      c.markerType=type;
   });
}
void QtPlotReceiver::XyPlotChangeWidth(const std::string &name,Integer,Integer width,int curve) { curves(name,curve,[&](PlotCurve &c) { c.width=std::clamp(static_cast<int>(width),1,20); }); }
void QtPlotReceiver::XyPlotChangeStyle(const std::string &name,Integer,Integer style,int curve) { curves(name,curve,[&](PlotCurve &c) { if (style>=0) c.lineStyle=style; }); }
void QtPlotReceiver::XyPlotRescale(const std::string &name) { if (auto *entry=find(name)) if (entry->widget) entry->widget->canvas()->fit(); }
void QtPlotReceiver::XyPlotCurveSettings(const std::string &name,bool lines,Integer width,Integer style,bool markers,Integer size,Integer marker,bool hiLo,Integer curve)
{
   curves(name,curve,[&](PlotCurve &c) {
      c.lines=lines; c.width=std::clamp(static_cast<int>(width),1,20); if (style>=0) c.lineStyle=style;
      c.markers=markers; c.markerSize=std::clamp(static_cast<int>(size),1,30); c.errorBars=hiLo;
      if (marker>=0) c.markerType=marker%10;
      for (auto &point:c.points) point.marker=c.markerType;
   });
}
void QtPlotReceiver::SetXyPlotTitle(const std::string &name,const std::string &title) { if (auto *entry=find(name)) entry->data->title=text(title); }
void QtPlotReceiver::ShowXyPlotLegend(const std::string &name) { if (auto *entry=find(name)) entry->data->legend=true; }
bool QtPlotReceiver::RefreshXyPlot(const std::string &name) { return RefreshGlPlot(name); }
bool QtPlotReceiver::UpdateXyPlot(const std::string &name,const std::string &,const Real &x,const Rvector &ys,const std::string &title,const std::string &xlabel,const std::string &ylabel,bool update,bool grid)
{
   auto *entry=find(name); if (!entry) return false;
   if (!title.empty()) entry->data->title=text(title);
   if (!xlabel.empty()) entry->data->xLabel=text(xlabel);
   if (!ylabel.empty()) entry->data->yLabel=text(ylabel);
   entry->data->grid=grid; ++entry->data->frame;
   for (Integer i=0;i<ys.GetSize();++i) entry->data->append(i,x,ys[i]);
   if (update) refresh(*entry,true); return true;
}
bool QtPlotReceiver::UpdateXyPlotData(const std::string &name,const Real &x,const Rvector &ys,const Rvector *highs,const Rvector *lows)
{
   auto *entry=find(name); if (!entry) return false;
   ++entry->data->frame;
   for (Integer i=0;i<ys.GetSize();++i) entry->data->append(i,x,ys[i],0,0,true,false,
         highs && i<highs->GetSize() ? (*highs)[i] : 0,lows && i<lows->GetSize() ? (*lows)[i] : 0);
   refresh(*entry); return true;
}
bool QtPlotReceiver::UpdateXyPlotCurve(const std::string &name,const Integer curve,const Real x,const Real y,const Real high,const Real low)
{
   auto *entry=find(name); if (!entry || curve<0) return false;
   ++entry->data->frame; entry->data->append(curve,x,y,0,0,true,false,high,low); refresh(*entry); return true;
}
bool QtPlotReceiver::DeactivateXyPlot(const std::string &name)
{
   if (auto *entry=find(name)) { entry->xyRedrawSuspended=true; return true; }
   return false;
}
bool QtPlotReceiver::ActivateXyPlot(const std::string &name)
{
   if (auto *entry=find(name)) { entry->xyRedrawSuspended=false; refresh(*entry,true); return true; }
   return false;
}
bool QtPlotReceiver::TakeXYAction(const std::string &name,const std::string &action)
{
   if (action.compare(0,12,"SolverScope=")==0) { if (auto *entry=find(name)) { entry->solverScope=text(action.substr(12)); return true; } return false; }
   if (action=="AlwaysRedraw") { SetGlUpdateFrequency(name,1); return find(name)!=nullptr; }
   if (action=="RunModeRedraw") return find(name)!=nullptr;
   if (action=="Clear") { ClearXyPlotData(name); return find(name)!=nullptr; }
   return TakeGlAction(name,action);
}

bool QtPlotReceiver::CreateGroundTrackWindow(const std::string &name,const std::string &oldName,const std::string &title,Real x,Real y,Real w,Real h,bool maximized)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::GroundTrack,x,y,w,h,maximized);
   if (auto *configured=Moderator::Instance()->GetConfiguredObject(name)) {
      if (configured->IsOfType("GroundTrack")) {
         SetMaxGlDataPoints(name,configured->GetIntegerParameter("MaxPlotPoints"));
         entry.data->redrawPoints=std::max(0,static_cast<int>(configured->GetIntegerParameter("NumPointsToRedraw")));
         entry.data->defaultLineWidth=std::clamp(static_cast<int>(configured->GetIntegerParameter("LineWidth")),1,20);
         entry.data->footprints=configured->GetStringParameter("ShowFootPrints")=="All";
      }
   }
   entry.data->title=text(title.empty() ? name : title); entry.data->xLabel="Longitude (deg)"; entry.data->yLabel="Latitude (deg)"; return true;
}
void QtPlotReceiver::SetGroundTrackOption(const std::string &name,const std::string &option,const std::string &value)
{
   if (auto *entry=find(name)) {
      if (option=="TextureMap") {
         QString path=text(value);
         if (auto *configured=Moderator::Instance()->GetConfiguredObject(name)) {
            if (configured->IsOfType("GroundTrack") || configured->IsOfType("GroundTrackPlot")) {
               std::string file,resolved;
               if (FileManager::Instance()->GetTextureMapFile(value,configured->GetStringParameter("CentralBody"),name,file,resolved,false)) path=text(resolved);
            }
         }
         entry->data->map=QImage(path);
         if (!value.empty() && entry->data->map.isNull()) MessageInterface::ShowMessage("Qt GroundTrack '%s': could not load map '%s'; using coordinate grid.\n",name.c_str(),value.c_str());
      } else warn(name,option);
   }
}
bool QtPlotReceiver::UpdateGroundTrackData(const std::string &name,const double epoch,const double *points,const int count)
{
   auto *entry=find(name); if (!entry || count<0 || (count>0 && !points)) return false;
   auto &data=*entry->data; ++data.frame;
   if (data.frame>1 && epoch<data.lastEpoch) data.breakLines();
   for (int i=0;i<count;++i) data.append(i,points[2*i],points[2*i+1],0,epoch,true,entry->solverData);
   refresh(*entry); return true;
}
bool QtPlotReceiver::TakeGroundTrackAction(const std::string &name,const std::string &action)
{
   auto *entry=find(name); if (!entry) return false;
   const auto command=text(action).section('=',0,0), argument=text(action).section('=',1);
   if (command=="SolverScope") entry->solverScope=argument;
   else if (command=="SolverData") {
      entry->solverData=argument=="On";
      for (auto &curve:entry->data->curves)
         if (auto *point=Moderator::Instance()->GetSpacePoint(curve.name.toStdString()))
            curve.color=entry->solverData ? entry->targetColors.value(curve.name,rgb(point->GetCurrentTargetColor())) : entry->segmentColors.value(curve.name,entry->orbitColors.value(curve.name,rgb(point->GetCurrentOrbitColor())));
   } else if (command=="SpacecraftColors") {
      const auto fields=argument.split('|'); if (fields.size()%3!=0) return false;
      QMap<QString,QColor> orbit,target;
      for (int i=0;i<fields.size();i+=3) {
         bool orbitOk=false,targetOk=false; const auto orbitValue=fields[i+1].toUInt(&orbitOk),targetValue=fields[i+2].toUInt(&targetOk);
         if (fields[i].isEmpty() || orbit.contains(fields[i]) || !orbitOk || !targetOk) return false;
         orbit[fields[i]]=rgb(orbitValue); target[fields[i]]=rgb(targetValue);
      }
      entry->orbitColors=orbit; entry->targetColors=target;
   } else if (command=="SegmentColor") {
      const auto fields=argument.split('|'); if (fields.size()<2 || (fields[0]!="On" && fields[0]!="Off")) return false;
      bool ok=false; const auto value=fields[1].toUInt(&ok); if (!ok) return false;
      for (int i=2;i<fields.size();++i) {
         if (fields[0]=="On") entry->segmentColors[fields[i]]=rgb(value);
         else entry->segmentColors.remove(fields[i]);
      }
   } else if (command=="MarkBreak") XyPlotMarkBreak(name);
   else if (command=="ClearFromBreak") {
      bool ok=false; const auto index=argument.toInt(&ok);
      if (!ok) return false;
      XyPlotClearFromBreak(name,index); refresh(*entry,true);
   } else if (command=="Satellites") {
      const auto names=argument.split('|',Qt::SkipEmptyParts);
      for (int i=0;i<names.size();++i) {
         auto &curve=entry->data->curves[i]; curve.name=names[i];
         curve.width=entry->data->defaultLineWidth;
         if (auto *object=Moderator::Instance()->GetSpacePoint(names[i].toStdString())) curve.color=rgb(object->GetCurrentOrbitColor());
      }
   } else if (command=="SatColors") {
      // Satellites resolves GMAT's RGB colors directly, including named colors.
   } else if (command=="AddStation") {
      auto *point=dynamic_cast<BodyFixedPoint *>(Moderator::Instance()->GetSpacePoint(argument.toStdString()));
      if (!point) { MessageInterface::ShowMessage("Qt GroundTrack '%s': missing or invalid station '%s'.\n",name.c_str(),argument.toStdString().c_str()); return false; }
      const auto location=point->GetSphericalLocation(21545.0);
      double longitude=std::remainder(location[1]*degrees,360.0);
      entry->data->stations.append({argument,longitude,location[0]*degrees,rgb(point->GetCurrentOrbitColor())});
   } else if (command=="PlotUpdateFrequency") {
      bool ok=false; int value=argument.toInt(&ok); if (!ok || value<1) return false; entry->data->updateFrequency=value;
   } else if (command=="UseGrid") entry->data->grid=argument=="True";
   else if (command=="LatitudeLineCount" || command=="LongitudeLineCount") {
      bool ok=false; int value=argument.toInt(&ok); if (!ok || value<1 || value>1000) return false;
      if (command=="LatitudeLineCount") entry->data->latitudeLines=value; else entry->data->longitudeLines=value;
   } else if (command=="Reinitialize" || command=="ClearData" || command=="Reset") {
      entry->dataAvailability.reset();
      entry->data->clear(); entry->data->stations.clear(); entry->solverBreaks.clear(); entry->solverScope.clear();
      if (command=="Reinitialize") { entry->data->curves.clear(); entry->segmentColors.clear(); entry->orbitColors.clear(); entry->targetColors.clear(); }
   } else if (command=="Refresh" || command=="RunComplete") {
      if (command=="RunComplete") entry->data->endOfRun=true;
      refresh(*entry,true);
   }
   else return TakeGlAction(name,action);
   return true;
}
bool QtPlotReceiver::CreateDynamicDataDisplay(const std::string &name,const std::string &oldName,const std::string &title,Real x,Real y,Real w,Real h)
{
   if (oldName!=name && !oldName.empty()) remove(oldName);
   auto &entry=create(name,PlotModel::Kind::Table,x,y,w,h,false);
   if (!title.empty()) entry.window->setWindowTitle(text(title)); return true;
}
bool QtPlotReceiver::SetDynamicDataTableSize(const std::string &name,Integer rows,Integer columns)
{
   auto *entry=find(name); if (!entry || !entry->table || rows<0 || columns<0 || rows>10000 || columns>1000) return false;
   entry->table->setRowCount(rows); entry->table->setColumnCount(columns); entry->cells.resize(rows);
   for (auto &row:entry->cells) row.resize(columns); return true;
}
bool QtPlotReceiver::UpdateDynamicDataDisplay(const std::string &name,std::vector<std::vector<DDD>> rows)
{
   auto *entry=find(name); if (!entry) return false;
   int columns=0; for (const auto &row:rows) columns=std::max(columns,static_cast<int>(row.size()));
   entry->cells.resize(rows.size());
   if (entry->table) { entry->table->setRowCount(rows.size()); entry->table->setColumnCount(columns); entry->table->clearContents(); }
   for (int r=0;r<static_cast<int>(rows.size());++r) {
      entry->cells[r].clear(); entry->cells[r].resize(columns);
      for (int c=0;c<static_cast<int>(rows[r].size());++c) {
         const auto &cell=rows[r][c]; const QString value=text(cell.paramName.empty() ? cell.paramValue : cell.paramName+" = "+cell.paramValue);
         entry->cells[r][c]={value,rgb(cell.paramTextColor),rgb(cell.paramBackgroundColor)};
         if (entry->table) {
            auto *item=new QTableWidgetItem(value); item->setForeground(rgb(cell.paramTextColor)); item->setBackground(rgb(cell.paramBackgroundColor)); entry->table->setItem(r,c,item);
         }
      }
   }
   if (entry->table) fitTableColumns(entry->table); return true;
}
bool QtPlotReceiver::DeleteDynamicData(const std::string &name,const std::string &) { return remove(name); }
bool QtPlotReceiver::SetDynamicDataTextColor(const std::string &name,std::vector<std::vector<DDD>> rows)
{
   auto *entry=find(name); if (!entry) return false;
   for (int r=0;r<static_cast<int>(rows.size());++r) for (int c=0;c<static_cast<int>(rows[r].size());++c)
      if (r<entry->cells.size() && c<entry->cells[r].size()) {
         entry->cells[r][c].foreground=rgb(rows[r][c].paramTextColor);
         if (entry->table) if (auto *item=entry->table->item(r,c)) item->setForeground(rgb(rows[r][c].paramTextColor));
      }
   return true;
}
