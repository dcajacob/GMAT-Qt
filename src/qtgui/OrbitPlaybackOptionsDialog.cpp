#include "OrbitPlaybackOptionsDialog.hpp"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>
#include <stdexcept>

OrbitPlaybackOptionsDialog::OrbitPlaybackOptionsDialog(const std::optional<QtPlaybackSetting> &initial,const QStringList &parents,QWidget *parent)
   : QDialog(parent)
{
   setObjectName("orbitPlaybackOptionsDialog"); setWindowTitle("Playback options");
   const auto setting=initial.value_or(QtPlaybackSetting());
   auto *layout=new QVBoxLayout(this);
   auto *help=new QLabel("Save the playback clock and toolbar preference for this OrbitView. OK keeps changes pending until Apply.",this);
   help->setWordWrap(true); layout->addWidget(help);
   auto *form=new QFormLayout; layout->addLayout(form);
   clockMode=new QComboBox(this); clockMode->setObjectName("playbackClockMode");
   clockMode->addItems({"Qt history sweep","Simulation clock"});
   clockMode->setCurrentIndex(setting.timeScale || !setting.syncParent.isEmpty() ? 1 : 0); form->addRow("Playback clock",clockMode);
   syncParent=new QComboBox(this); syncParent->setObjectName("playbackSyncParent");
   syncParent->addItem("Independent",QString()); for (const auto &name:parents) syncParent->addItem(name,name);
   if (!setting.syncParent.isEmpty() && syncParent->findData(setting.syncParent)<0) syncParent->addItem(setting.syncParent+" (unavailable)",setting.syncParent);
   syncParent->setCurrentIndex(std::max(0,syncParent->findData(setting.syncParent))); form->addRow("Synchronize with",syncParent);
   timeScale=new QLineEdit(setting.timeScale ? QString::number(*setting.timeScale,'g',17) : QString(),this); timeScale->setObjectName("playbackTimeScale");
   timeScale->setPlaceholderText("3600"); form->addRow("Simulation seconds per real second",timeScale);
   auto *rateHelp=new QLabel("A positive rate plays forward; a negative rate plays backward; zero holds the clock. A synchronized view follows its parent's clock and retains its own rate for later independent playback. Leave the rate empty to use the default clock rate. Propagation steps shows recorded samples; Interpolated steps interpolates between connected samples.",this);
   rateHelp->setWordWrap(true); layout->addWidget(rateHelp);
   toolbar=new QComboBox(this); toolbar->setObjectName("playbackToolbar"); toolbar->addItems({"Qt default (shown)","Show","Hide"}); toolbar->setCurrentIndex(setting.showToolbar ? (*setting.showToolbar ? 1 : 2) : 0); form->addRow("Plot toolbar",toolbar);
   auto *toolbarHelp=new QLabel("When hidden, the viewer's Show toolbar control restores the plot controls. The shared playback controls remain available in the main window.",this); toolbarHelp->setWordWrap(true); layout->addWidget(toolbarHelp);
   auto *status=new QLabel(this); status->setObjectName("playbackOptionsStatus"); status->setWordWrap(true); layout->addWidget(status);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); buttons->setObjectName("playbackOptionsButtons"); layout->addWidget(buttons);
   auto *defaults=buttons->addButton("Qt defaults",QDialogButtonBox::ResetRole); defaults->setObjectName("playbackDefaults");
   connect(defaults,&QPushButton::clicked,this,[this] { clockMode->setCurrentIndex(0); syncParent->setCurrentIndex(0); timeScale->clear(); toolbar->setCurrentIndex(0); });
   connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
   connect(buttons,&QDialogButtonBox::accepted,this,[this,status] { try { settings(); accept(); } catch (const std::exception &error) { status->setText(QString::fromUtf8(error.what())); } });
   const auto dependencies=[this] { const bool clock=clockMode->currentIndex()==1; syncParent->setEnabled(clock); timeScale->setEnabled(clock); };
   connect(clockMode,&QComboBox::currentIndexChanged,this,[dependencies](int) { dependencies(); }); dependencies(); resize(620,390);
}
std::optional<QtPlaybackSetting> OrbitPlaybackOptionsDialog::settings() const
{
   QtPlaybackSetting setting;
   if (clockMode->currentIndex()==1) {
      setting.syncParent=syncParent->currentData().toString();
      if (!timeScale->text().trimmed().isEmpty()) {
         bool valid=false; const auto value=timeScale->text().toDouble(&valid);
         if (!valid || !std::isfinite(value)) throw std::runtime_error("Enter a finite playback rate, or leave it empty for the default rate.");
         setting.timeScale=value;
      } else setting.timeScale=3600;
   }
   if (toolbar->currentIndex()!=0) setting.showToolbar=toolbar->currentIndex()==1;
   if (!setting.timeScale && setting.syncParent.isEmpty() && !setting.showToolbar) return std::nullopt;
   return qtPlayback(qtPlaybackJson(setting));
}
