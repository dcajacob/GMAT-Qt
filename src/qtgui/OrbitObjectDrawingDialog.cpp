#include "OrbitObjectDrawingDialog.hpp"
#include "ScriptCompatibility.hpp"
#include "TableColumns.hpp"
#include "Moderator.hpp"
#include "GmatBase.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QTabWidget>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QScreen>
#include <QRegularExpression>
#include <algorithm>
OrbitObjectDrawingDialog::OrbitObjectDrawingDialog(const QStringList &names,const QMap<QString,bool> &labels,const QMap<QString,bool> &trajectories,QWidget *parent,const QtCameraSetting *drawing) : QDialog(parent)
{
   setObjectName("orbitObjectDrawingDialog"); setWindowTitle("Object drawing");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Choose each object's paths, labels, markers and body guides. Velocity shows a segment at every recorded object sample, using a 1000-second scale in the display coordinate system; Default hides these segments. Axes, latitude/longitude grid and local XY plane follow recorded position and attitude; Default hides them. The local XY plane extends to 15 object radii. These are separate from the plot-wide Grid and XY plane settings. Default uses normal viewer choices: width 1 pixel and marker size 10 pixels. Show labels is the label master switch. Orbit-view setup controls models/bodies. Label style uses pixel sizes (0 hides the label); Default uses the viewer font and placement. Segment labels identify every regular propagation segment and are separate from the object-name Label choice; Default hides segment labels. OK keeps edits pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   objects=new QTableWidget(names.size(),5,this); objects->setObjectName("orbitObjectDrawing");
   objects->setHorizontalHeaderLabels({"Object","Trajectory","Label","Width (px)","Velocity"}); objects->verticalHeader()->hide();
   configureTableColumns(objects,{10,8,8,6,8});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); objects->setItem(row,0,item);
      for (int column=1;column<3;++column) {
         const auto &values=column==1 ? trajectories : labels;
         auto *choice=new QComboBox(objects); choice->setObjectName(QString("orbitDrawing_%1_%2").arg(column==1 ? "trajectory" : "label",names[row]));
         choice->addItems({"Default","On","Off"}); choice->setCurrentIndex(values.contains(names[row]) ? (values.value(names[row]) ? 1 : 2) : 0);
         objects->setCellWidget(row,column,choice);
      }
      auto *width=new QLineEdit(objects); width->setObjectName("orbitDrawing_width_"+names[row]); width->setPlaceholderText("Default");
      if (drawing && drawing->objectLineWidths.contains(names[row])) width->setText(QString::number(drawing->objectLineWidths.value(names[row]),'g',17));
      objects->setCellWidget(row,3,width);
      auto *velocity=new QComboBox(objects); velocity->setObjectName("orbitDrawing_velocity_"+names[row]); velocity->addItems({"Default","On","Off"});
      if (drawing && drawing->objectVelocities.contains(names[row])) velocity->setCurrentIndex(drawing->objectVelocities.value(names[row]) ? 1 : 2);
      auto *moderator=Moderator::Instance();
      auto *object=moderator->IsInitialized() ? moderator->GetConfiguredObject(names[row].toStdString()) : nullptr;
      velocity->setEnabled(object && object->IsOfType(Gmat::SPACE_POINT));
      velocity->setToolTip(velocity->isEnabled() ? "Segments use recorded velocity in this display frame, with a 1000-second scale." : "Velocity segments are available for space-point trajectories.");
      objects->setCellWidget(row,4,velocity);
   }
   markers=new QTableWidget(names.size(),4,this); markers->setObjectName("orbitObjectMarkers");
   markers->setHorizontalHeaderLabels({"Object","Center","Endpoints","Size (px)"}); markers->verticalHeader()->hide(); configureTableColumns(markers,{10,8,8,6});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); markers->setItem(row,0,item);
      for (int column=1;column<3;++column) {
         const auto values=drawing ? (column==1 ? drawing->objectCenters : drawing->objectEndpoints) : QMap<QString,bool>();
         auto *choice=new QComboBox(markers); choice->setObjectName(QString("orbitDrawing_%1_%2").arg(column==1 ? "center" : "endpoints",names[row]));
         choice->addItems({"Default","On","Off"}); choice->setCurrentIndex(values.contains(names[row]) ? (values.value(names[row]) ? 1 : 2) : 0); markers->setCellWidget(row,column,choice);
      }
      auto *size=new QLineEdit(markers); size->setObjectName("orbitDrawing_size_"+names[row]); size->setPlaceholderText("Default");
      if (drawing && drawing->objectMarkerSizes.contains(names[row])) size->setText(QString::number(drawing->objectMarkerSizes.value(names[row])));
      markers->setCellWidget(row,3,size);
   }
   fonts=new QTableWidget(names.size(),4,this); fonts->setObjectName("orbitObjectFonts");
   fonts->setHorizontalHeaderLabels({"Object","Font (px)","Position","Segment labels"}); fonts->verticalHeader()->hide(); configureTableColumns(fonts,{10,6,12,10});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); fonts->setItem(row,0,item);
      auto *size=new QLineEdit(fonts); size->setObjectName("orbitDrawing_fontSize_"+names[row]); size->setPlaceholderText("Default");
      if (drawing && drawing->objectFontSizes.contains(names[row])) size->setText(QString::number(drawing->objectFontSizes.value(names[row])));
      fonts->setCellWidget(row,1,size);
      auto *position=new QComboBox(fonts); position->setObjectName("orbitDrawing_fontPosition_"+names[row]); position->addItems({"Default","Top-Right","Top-Left","Bottom-Right","Bottom-Left"});
      if (drawing && drawing->objectFontPositions.contains(names[row])) position->setCurrentText(drawing->objectFontPositions.value(names[row]));
      fonts->setCellWidget(row,2,position);
      auto *segments=new QComboBox(fonts); segments->setObjectName("orbitDrawing_segmentLabel_"+names[row]); segments->addItems({"Default","On","Off"});
      if (drawing && drawing->objectSegmentLabels.contains(names[row])) segments->setCurrentIndex(drawing->objectSegmentLabels.value(names[row]) ? 1 : 2);
      auto *moderator=Moderator::Instance();
      auto *object=moderator->IsInitialized() ? moderator->GetConfiguredObject(names[row].toStdString()) : nullptr;
      segments->setEnabled(object && object->IsOfType(Gmat::SPACE_POINT));
      segments->setToolTip(segments->isEnabled() ? "Label every regular propagation segment. Default is Off; this is independent of the object-name Label choice." : "Segment labels are available for space-point trajectories. Imported settings for other objects are retained.");
      fonts->setCellWidget(row,3,segments);
   }
   guides=new QTableWidget(names.size(),4,this); guides->setObjectName("orbitObjectGuides");
   guides->setHorizontalHeaderLabels({"Object","Body axes","Lat/lon grid","Local XY plane"}); guides->verticalHeader()->hide(); configureTableColumns(guides,{10,8,9,10});
   for (int row=0;row<names.size();++row) {
      auto *item=new QTableWidgetItem(names[row]); item->setFlags(item->flags()&~Qt::ItemIsEditable); guides->setItem(row,0,item);
      for (int column=1;column<4;++column) {
         auto *choice=new QComboBox(guides);
         const QString field=column==1 ? "axes" : column==2 ? "grid" : "xyPlane";
         choice->setObjectName("orbitDrawing_"+field+"_"+names[row]); choice->addItems({"Default","On","Off"});
         const auto values=drawing ? (column==1 ? drawing->objectAxes : column==2 ? drawing->objectGrids : drawing->objectXYPlanes) : QMap<QString,bool>();
         if (values.contains(names[row])) choice->setCurrentIndex(values.value(names[row]) ? 1 : 2);
         guides->setCellWidget(row,column,choice);
      }
   }
   auto *framePage=new QWidget(this); auto *frameLayout=new QFormLayout(framePage);
   auto *frameHelp=new QLabel("These labels describe the display coordinate system. Axes labels show X, Y and Z when Axes is On; Default is On. Reference frame labels the origin, even when Axes is Off; Default is Off. Both are independent of the object Show labels switch.",framePage);
   frameHelp->setWordWrap(true); frameLayout->addRow(frameHelp);
   axesLabels=new QComboBox(framePage); axesLabels->setObjectName("orbitDrawing_axesLabels"); axesLabels->addItems({"Default","On","Off"});
   frameLabel=new QComboBox(framePage); frameLabel->setObjectName("orbitDrawing_frameLabel"); frameLabel->addItems({"Default","On","Off"});
   axesLabels->setCurrentText(qtDrawingFlagText(drawing ? drawing->axesLabels : std::nullopt));
   frameLabel->setCurrentText(qtDrawingFlagText(drawing ? drawing->frameLabel : std::nullopt));
   frameLayout->addRow("Axes labels",axesLabels); frameLayout->addRow("Reference frame",frameLabel);
   auto *tabs=new QTabWidget(this); tabs->setObjectName("orbitDrawingTabs"); tabs->addTab(objects,"Paths and labels"); tabs->addTab(markers,"Markers"); tabs->addTab(fonts,"Label style"); tabs->addTab(guides,"Body guides"); tabs->addTab(framePage,"Frame labels"); layout->addWidget(tabs,1);
   auto *status=new QLabel(this); status->setObjectName("orbitDrawingStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,status] { try { settings(); accept(); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); } }); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   resize(560,std::min(380,QGuiApplication::primaryScreen()->availableGeometry().height()-80)); fitTableColumns(objects); fitTableColumns(markers); fitTableColumns(fonts); fitTableColumns(guides);
}
QMap<QString,QString> OrbitObjectDrawingDialog::settings() const
{
   QMap<QString,bool> labels,trajectories,velocities;
   for (int row=0;row<objects->rowCount();++row) for (int column=1;column<3;++column) {
      const auto *choice=qobject_cast<QComboBox *>(objects->cellWidget(row,column));
      if (choice->currentIndex()) (column==1 ? trajectories : labels).insert(objects->item(row,0)->text(),choice->currentIndex()==1);
   }
   for (int row=0;row<objects->rowCount();++row) {
      const auto *choice=qobject_cast<QComboBox *>(objects->cellWidget(row,4));
      if (choice->currentIndex()) velocities.insert(objects->item(row,0)->text(),choice->currentIndex()==1);
   }
   QMap<QString,double> widths;
   for (int row=0;row<objects->rowCount();++row) {
      const auto text=qobject_cast<QLineEdit *>(objects->cellWidget(row,3))->text().trimmed();
      if (text.isEmpty()) continue;
      bool ok=false; const auto width=text.toDouble(&ok);
      if (!ok || !std::isfinite(width) || width<1 || width>std::numeric_limits<float>::max()) throw std::runtime_error((objects->item(row,0)->text()+": enter a finite line width >= 1 within the renderer's range, or leave it empty for Default.").toStdString());
      widths[objects->item(row,0)->text()]=width;
   }
   QMap<QString,bool> centers,endpoints; QMap<QString,quint32> sizes;
   for (int row=0;row<markers->rowCount();++row) {
      const auto name=markers->item(row,0)->text();
      for (int column=1;column<3;++column) {
         const auto *choice=qobject_cast<QComboBox *>(markers->cellWidget(row,column));
         if (choice->currentIndex()) (column==1 ? centers : endpoints).insert(name,choice->currentIndex()==1);
      }
      const auto text=qobject_cast<QLineEdit *>(markers->cellWidget(row,3))->text().trimmed();
      if (!text.isEmpty()) {
         bool ok=false; const auto size=text.toULongLong(&ok);
         if (!ok || size>4294967295ULL || !QRegularExpression("^[0-9]+$").match(text).hasMatch()) throw std::runtime_error((name+": enter an unsigned integer marker size or leave it empty for Default.").toStdString());
         sizes[name]=static_cast<quint32>(size);
      }
   }
   QMap<QString,quint32> fontSizes; QMap<QString,QString> fontPositions; QMap<QString,bool> segmentLabels;
   for (int row=0;row<fonts->rowCount();++row) {
      const auto name=fonts->item(row,0)->text(),text=qobject_cast<QLineEdit *>(fonts->cellWidget(row,1))->text().trimmed();
      if (!text.isEmpty()) {
         bool ok=false; const auto size=text.toULongLong(&ok);
         if (!ok || size>10000 || !QRegularExpression("^[0-9]+$").match(text).hasMatch()) throw std::runtime_error((name+": enter an integer label size from 0 to 10000 pixels, or leave it empty for Default.").toStdString());
         fontSizes[name]=static_cast<quint32>(size);
      }
      const auto *position=qobject_cast<QComboBox *>(fonts->cellWidget(row,2)); if (position->currentIndex()) fontPositions[name]=position->currentText();
      const auto *segments=qobject_cast<QComboBox *>(fonts->cellWidget(row,3)); if (segments->currentIndex()) segmentLabels[name]=segments->currentIndex()==1;
   }
   QMap<QString,bool> axes,grids,planes;
   for (int row=0;row<guides->rowCount();++row) for (int column=1;column<4;++column) {
      const auto *choice=qobject_cast<QComboBox *>(guides->cellWidget(row,column));
      if (choice->currentIndex()) (column==1 ? axes : column==2 ? grids : planes).insert(guides->item(row,0)->text(),choice->currentIndex()==1);
   }
   return {{"@QtAxesLabels",axesLabels->currentText()},{"@QtFrameLabel",frameLabel->currentText()},{"@QtObjectSegmentLabels",qtObjectFlagsJson(segmentLabels)},{"@QtObjectVelocities",qtObjectFlagsJson(velocities)},{"@QtObjectAxes",qtObjectFlagsJson(axes)},{"@QtObjectGrids",qtObjectFlagsJson(grids)},{"@QtObjectXYPlanes",qtObjectFlagsJson(planes)},{"@QtObjectLabels",qtObjectFlagsJson(labels)},{"@QtObjectTrajectories",qtObjectFlagsJson(trajectories)},{"@QtObjectCenters",qtObjectFlagsJson(centers)},{"@QtObjectEndpoints",qtObjectFlagsJson(endpoints)},{"@QtObjectMarkerSizes",qtObjectSizesJson(sizes)},{"@QtObjectLineWidths",qtObjectWidthsJson(widths)},{"@QtObjectFontSizes",qtObjectSizesJson(fontSizes)},{"@QtObjectFontPositions",qtObjectFontPositionsJson(fontPositions)}};
}
