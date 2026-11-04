#include "VisualModelDialog.hpp"
#include "PlotWidget.hpp"
#include "OrbitRenderer.hpp"
#include "GmatDefaults.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QScrollArea>
#include <QScreen>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
double sliderScale(int position) { return std::pow(10.0,position/90-4)*(position%90+10); }
int scalePosition(double value) {
   int nearest=0; double difference=std::numeric_limits<double>::infinity();
   for (int i=0;i<=540;++i) if (std::abs(sliderScale(i)-value)<difference) { nearest=i; difference=std::abs(sliderScale(i)-value); }
   return nearest;
}
}

VisualModelDialog::VisualModelDialog(const QMap<QString,QString> &initial,Normalize normalize,QWidget *parent)
   : QDialog(parent),normalize(std::move(normalize)),model(std::make_shared<PlotModel>(PlotModel::Kind::Orbit))
{
   setObjectName("visualModelDialog"); setWindowTitle("Spacecraft visual model");
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Preview the model and adjust its orientation, offset and scale. OK keeps changes pending until you Apply the spacecraft properties.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *splitter=new QSplitter(this); splitter->setObjectName("visualModelSplitter"); layout->addWidget(splitter,1);
   auto *scroll=new QScrollArea(splitter); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
   auto *controls=new QWidget; scroll->setWidget(controls); auto *left=new QVBoxLayout(controls);
   auto makeField=[&](const QString &name,QWidget *parent) {
      auto *edit=new QLineEdit(initial.value(name),parent); edit->setObjectName("visual_"+name); fields.insert(name,edit); return edit;
   };
   auto *fileGroup=new QGroupBox("Model file",controls); auto *fileLayout=new QHBoxLayout(fileGroup);
   auto *file=makeField("ModelFile",fileGroup); auto *browse=new QPushButton("Browse…",fileGroup); browse->setObjectName("visualBrowseModel");
   fileLayout->addWidget(file); fileLayout->addWidget(browse); left->addWidget(fileGroup);
   connect(browse,&QPushButton::clicked,this,[this,file] {
      QFileDialog picker(this,"Choose spacecraft model",file->text()); picker.setObjectName("visualModelFileDialog");
      picker.setFileMode(QFileDialog::ExistingFile);
      picker.setNameFilters({"Spacecraft models (*.3ds *.3DS *.obj *.OBJ)","All files (*)"});
      if (picker.exec()==QDialog::Accepted && !picker.selectedFiles().isEmpty()) {
         file->setText(picker.selectedFiles().first());
         // Match wx's new-model selection: reset its pose, retaining colors.
         for (const auto &axis:QStringList{"X","Y","Z"}) {
            fields["ModelRotation"+axis]->setText("0"); fields["ModelOffset"+axis]->setText("0");
         }
         fields["ModelScale"]->setText("1");
      }
   });
   for (const auto &group:QStringList{"Rotation","Offset","Scale"}) {
      auto *box=new QGroupBox(group=="Offset" ? "Translation" : group,controls); auto *form=new QFormLayout(box);
      const QStringList axes=group=="Scale" ? QStringList{""} : QStringList{"X","Y","Z"};
      for (const auto &axis:axes) {
         const auto name="Model"+group+axis; auto *row=new QWidget(box); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
         auto *slider=new QSlider(Qt::Horizontal,row); slider->setObjectName("visualSlider_"+name);
         slider->setRange(group=="Rotation" ? -180 : 0,group=="Rotation" ? 180 : 540);
         if (group=="Offset") slider->setRange(-350,350);
         auto *edit=makeField(name,row); edit->setMaximumWidth(115);
         line->addWidget(slider,1); line->addWidget(edit); if (group=="Rotation") line->addWidget(new QLabel("deg",row));
         form->addRow(axis.isEmpty() ? "Factor" : axis,row);
         connect(slider,&QSlider::valueChanged,this,[edit,group](int value) {
            edit->setText(QString::number(group=="Scale" ? sliderScale(value) : group=="Offset" ? value/100.0 : value,'g',17));
         });
         auto sync=[slider,edit,group] {
            bool valid=false; const auto value=edit->text().toDouble(&valid); if (!valid || !std::isfinite(value)) return;
            const QSignalBlocker block(slider);
            const double position=group=="Scale" ? scalePosition(value) : group=="Offset" ? value*100 : value;
            slider->setValue(qRound(std::clamp(position,double(slider->minimum()),double(slider->maximum()))));
         };
         connect(edit,&QLineEdit::textChanged,this,sync); sync();
      }
      left->addWidget(box);
   }
   auto *resets=new QHBoxLayout; left->addLayout(resets);
   auto *recenter=new QPushButton("Recenter model",controls); recenter->setObjectName("visualRecenter"); resets->addWidget(recenter);
   auto *autoscale=new QPushButton("Autoscale model",controls); autoscale->setObjectName("visualAutoscale"); resets->addWidget(autoscale);
   connect(recenter,&QPushButton::clicked,this,[this] { for (const auto &axis:QStringList{"X","Y","Z"}) fields["ModelOffset"+axis]->setText("0"); });
   connect(autoscale,&QPushButton::clicked,this,[this] { fields["ModelScale"]->setText("1"); });
   // NASA's model panel enables transforms only after a model is selected.
   // Empty-model native properties are unavailable;their omitted values must
   // not leave active sliders next to blank inputs.
   auto availability=[this,file,recenter,autoscale] {
      const bool selected=!file->text().trimmed().isEmpty();
      for (auto it=fields.cbegin();it!=fields.cend();++it)
         if (it.key().startsWith("ModelRotation") || it.key().startsWith("ModelOffset") || it.key()=="ModelScale") {
            it.value()->setEnabled(selected);
            if (auto *slider=findChild<QSlider *>("visualSlider_"+it.key())) slider->setEnabled(selected);
         }
      recenter->setEnabled(selected); autoscale->setEnabled(selected);
   };
   connect(file,&QLineEdit::textChanged,this,availability); availability();
   auto *colors=new QGroupBox("Colors",controls); auto *colorForm=new QFormLayout(colors);
   for (const auto &name:QStringList{"OrbitColor","TargetColor"}) {
      auto *row=new QWidget(colors); auto *line=new QHBoxLayout(row); line->setContentsMargins(0,0,0,0);
      auto *edit=makeField(name,row); auto *choose=new QPushButton("Choose…",row); choose->setObjectName("visualChoose_"+name);
      line->addWidget(edit); line->addWidget(choose); colorForm->addRow(name=="OrbitColor" ? "Orbit" : "Target",row);
      connect(choose,&QPushButton::clicked,this,[this,edit] {
         QColor color; try { color=QColor::fromRgb(RgbColor::ToIntColor(edit->text().toStdString())&0xffffff); } catch (...) {}
         QColorDialog picker(color,this); picker.setObjectName("visualModelColorDialog");
         if (picker.exec()==QDialog::Accepted) { const auto selected=picker.selectedColor(); edit->setText(QString("[%1 %2 %3]").arg(selected.red()).arg(selected.green()).arg(selected.blue())); }
      });
   }
   left->addWidget(colors); left->addStretch();
   auto *display=new QWidget(splitter); auto *right=new QVBoxLayout(display);
   model->legend=false; model->labels=false; model->grid=false; model->sunlight=false; model->perspective=true;
   model->automaticTrajectory="CoordinateSystem"; model->automaticRadius=1300;
   model->curves[0].name="Model"; model->curves[0].lines=false; model->append(0,0,0,0);
   model->curves[1].name="Earth"; model->curves[1].radius=GmatSolarSystemDefaults::PLANET_EQUATORIAL_RADIUS[GmatSolarSystemDefaults::EARTH];
   model->curves[1].color=QColor(50,50,130); model->curves[1].visible=false; model->curves[1].wireframeObject=true; model->append(1,0,0,0);
   canvas=new PlotCanvas(model,display); canvas->setObjectName("visualModelPreview"); canvas->setMinimumSize(300,350); right->addWidget(canvas,1);
   auto *actions=new QHBoxLayout; right->addLayout(actions);
   auto *earth=new QPushButton("Show Earth",display); earth->setCheckable(true); earth->setObjectName("visualShowEarth"); actions->addWidget(earth);
   auto *fit=new QPushButton("Fit view",display); fit->setObjectName("visualFit"); actions->addWidget(fit);
   connect(earth,&QPushButton::toggled,this,[this,earth](bool shown) {
      model->curves[1].visible=shown; earth->setText(shown ? "Hide Earth" : "Show Earth");
      model->automaticRadius=shown ? model->curves[1].radius*1.1 : 1300; model->fitCamera=false; canvas->refresh();
   });
   connect(fit,&QPushButton::clicked,canvas,&PlotCanvas::fit);
   status=new QLabel(this); status->setObjectName("visualModelError"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   refreshTimer=new QTimer(this); refreshTimer->setSingleShot(true); refreshTimer->setInterval(150);
   connect(refreshTimer,&QTimer::timeout,this,[this] { refreshPreview(); });
   for (auto *edit:fields) connect(edit,&QLineEdit::textChanged,this,[this] { refreshTimer->start(); });
   for (auto it=fields.cbegin();it!=fields.cend();++it) if (it.key()!="ModelFile" && !it.key().endsWith("Color"))
      connect(it.value(),&QLineEdit::editingFinished,this,[this] {
         if (!refreshPreview()) return;
         for (auto it=acceptedValues.cbegin();it!=acceptedValues.cend();++it)
            if (it.key().startsWith("ModelRotation") || it.key().startsWith("ModelOffset") || it.key()=="ModelScale") fields[it.key()]->setText(it.value());
         refreshTimer->stop();
      });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this] { refreshTimer->stop(); if (refreshPreview()) accept(); });
   splitter->setStretchFactor(0,0); splitter->setStretchFactor(1,1);
   const auto available=screen()->availableGeometry().size();
   resize(std::min(1000,available.width()-40),std::min(720,available.height()-80)); refreshPreview();
}

QMap<QString,QString> VisualModelDialog::settings() const
{
   QMap<QString,QString> result;
   for (auto it=fields.cbegin();it!=fields.cend();++it) result[it.key()]=it.value()->text();
   return result;
}
bool VisualModelDialog::refreshPreview()
{
   try {
      QMap<QString,QString> pending; for (auto it=fields.cbegin();it!=fields.cend();++it) pending.insert(it.key(),it.value()->text());
      const auto normalized=normalize(pending); const auto path=normalized.value("@ResolvedModelFile");
      if (!pending.value("ModelFile").isEmpty()) {
         if (path!=checkedPath || checkedPath.isEmpty()) { checkedPath=path; checkedReadable=orbitModelFileReadable(path); }
         if (!checkedReadable) throw std::runtime_error("The model file could not be loaded. Choose a supported model file or clear the path.");
      }
      auto &curve=model->curves[0]; curve.modelPath=path; curve.modelScale=normalized.value("ModelScale").toDouble();
      for (int i=0;i<3;++i) {
         const auto axis=QString(QChar('X'+i)); curve.modelRotation[i]=normalized.value("ModelRotation"+axis).toDouble();
         curve.modelOffset[i]=normalized.value("ModelOffset"+axis).toDouble();
      }
      curve.color=QColor::fromRgb(RgbColor::ToIntColor(normalized.value("OrbitColor").toStdString())&0xffffff);
      curve.points.back().color=curve.color;
      acceptedValues=normalized; acceptedValues.remove("@ResolvedModelFile");
      status->clear(); canvas->refresh(); return true;
   } catch (BaseException &error) { status->setText(QString::fromStdString(error.GetFullMessage())); return false; }
   catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); return false; }
}
