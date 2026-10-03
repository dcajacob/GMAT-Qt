#include "PropagationForm.hpp"
#include "PropagationOptions.hpp"
#include "ReportParameterDialog.hpp"
#include "Propagate.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
QPair<int,int> backwardSpan(const QString &statement)
{
   const auto prefix=QRegularExpression("^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?").match(statement);
   const int start=prefix.capturedEnd();
   const auto keyword=QRegularExpression("^BackProp\\s+").match(statement.mid(start));
   return {start,keyword.hasMatch() ? keyword.capturedLength() : 0};
}
bool eventStop(const QString &parameter) { return parameter.endsWith(".Periapsis") || parameter.endsWith(".Apoapsis"); }
struct Propagation {
   bool valid=false;
   QRegularExpressionMatch match;
   QString stop,goal,tolerance;
   QStringList entries;
   int stopIndex=-1,toleranceIndex=-1;
};
Propagation parse(const QString &statement)
{
   Propagation result;
   result.match=QRegularExpression(
      "^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?(?:BackProp\\s+)?([A-Za-z][A-Za-z0-9_]*)\\s*\\(\\s*([A-Za-z][A-Za-z0-9_]*)\\s*\\)"
      "(\\s*\\{([^{};,%]*(?:,[^{};,%]*)*)\\})?\\s*;[ \\t]*(?:%[^\\n]*)?\\s*$").match(statement);
   if (!result.match.hasMatch()) return result;
   if (result.match.capturedStart(3)>=0) {
      result.entries=result.match.captured(4).split(',');
      if (result.entries.size()>1 && result.entries.first().trimmed().isEmpty()) {
         for (int i=1;i<result.entries.size();++i) if (!QRegularExpression("^\\s*(?:StopTolerance|OrbitColor)\\s*=").match(result.entries[i]).hasMatch()) return result;
         result.entries.removeFirst();
      }
      bool color=false;
      for (int i=0;i<result.entries.size();++i) {
         const auto entry=result.entries[i].trimmed(); const int equal=entry.indexOf('=');
         const auto name=(equal<0 ? entry : entry.left(equal)).trimmed();
         const auto value=equal<0 ? QString() : entry.mid(equal+1).trimmed();
         if (name=="StopTolerance") {
            if (result.toleranceIndex>=0 || value.isEmpty()) return result;
            result.toleranceIndex=i; result.tolerance=value;
         } else if (name=="OrbitColor") {
            if (color || value.isEmpty()) return result; color=true;
         } else {
            if (result.stopIndex>=0 || !QRegularExpression("^[A-Za-z][A-Za-z0-9_.]*$").match(name).hasMatch() || (eventStop(name) ? equal>=0 : value.isEmpty())) return result;
            result.stopIndex=i; result.stop=name; result.goal=value;
         }
      }
   }
   result.valid=true; return result;
}
}
PropagationForm::PropagationForm(const QStringList &propagators,const QStringList &satellites,
      std::function<void(const QString &)> changed,QWidget *parent) : QGroupBox("Propagation",parent)
{
   setObjectName("propagationForm"); auto *layout=new QFormLayout(this);
   propagator=new QComboBox(this); propagator->setObjectName("propagationPropagator"); propagator->addItems(propagators);
   spacecraft=new QComboBox(this); spacecraft->setObjectName("propagationSpacecraft"); spacecraft->addItems(satellites);
   singleStep=new QCheckBox("Single step",this); singleStep->setObjectName("propagationSingleStep");
   singleStep->setToolTip("Advance once using the propagator's current integration step, without a stopping condition.");
   units=new QComboBox(this); units->setObjectName("propagationUnits");
   units->addItem("Seconds","ElapsedSecs"); units->addItem("Days","ElapsedDays"); units->addItem("Other parameter","");
   duration=new QLineEdit(this); duration->setObjectName("propagationDuration"); duration->setPlaceholderText("Stop value or parameter expression");
   stopParameter=new QLineEdit(this); stopParameter->setObjectName("propagationStopParameter");
   layout->addRow("Propagator",propagator); layout->addRow("Spacecraft",spacecraft); layout->addRow(singleStep); layout->addRow("Stop type",units);
   auto picker=[&](const QString &label,QLineEdit *input,const QString &name) {
      auto *row=new QWidget(this); auto *box=new QHBoxLayout(row); box->setContentsMargins(0,0,0,0); box->addWidget(input);
      auto *choose=new QPushButton("Choose…",row); choose->setObjectName(name); box->addWidget(choose); layout->addRow(label,row);
      connect(choose,&QPushButton::clicked,this,[this,input] {
         ReportParameterDialog dialog({input->text()},this,input==stopParameter ? ReportParameterDialog::Mode::StopParameter : ReportParameterDialog::Mode::Single); dialog.setObjectName("propagationParameterDialog");
         if (dialog.exec()==QDialog::Accepted && !dialog.selection().isEmpty()) input->setText(dialog.selection().first());
      });
   };
   picker("Stop parameter",stopParameter,"propagationChooseStop"); picker("Stop value",duration,"propagationChooseGoal");
   backward=new QCheckBox("Propagate backwards",this); backward->setObjectName("propagationBackwards"); layout->addRow(backward);
   tolerance=new QLineEdit(this); tolerance->setObjectName("propagationTolerance");
   Propagate defaults; tolerance->setPlaceholderText("Default: "+QString::number(defaults.GetRealParameter("StopTolerance"),'g',10));
   tolerance->setToolTip("Positive stop tolerance. Leave blank to use GMAT's default."); layout->addRow("Stop tolerance",tolerance);
   auto refresh=[this] {
      const bool stopping=!singleStep->isChecked(),event=eventStop(stopParameter->text());
      units->setEnabled(stopping); stopParameter->setEnabled(stopping); tolerance->setEnabled(stopping);
      findChild<QPushButton *>("propagationChooseStop")->setEnabled(stopping);
      duration->setEnabled(stopping && !event); findChild<QPushButton *>("propagationChooseGoal")->setEnabled(stopping && !event);
   };
   auto update=[this,changed,refresh] {
      if (synchronizing) return;
      const auto parsed=parse(original); if (!parsed.valid) return; const auto &match=parsed.match;
      refresh(); QStringList entries;
      for (int i=0;i<parsed.entries.size();++i) {
         const auto entry=parsed.entries[i];
         if (i==parsed.stopIndex) {
            if (singleStep->isChecked()) continue;
            const bool event=eventStop(stopParameter->text());
            entries.append(stopParameter->text()==parsed.stop && (event || duration->text()==parsed.goal) ? entry : stopParameter->text()+(event ? QString() : " = "+duration->text()));
         } else if (i==parsed.toleranceIndex) {
            if (tolerance->text().trimmed().isEmpty()) continue;
            entries.append(tolerance->text().trimmed()==parsed.tolerance ? entry : "StopTolerance = "+tolerance->text().trimmed());
         } else entries.append(entry);
      }
      if (!singleStep->isChecked() && parsed.stopIndex<0) entries.prepend(stopParameter->text()+(eventStop(stopParameter->text()) ? QString() : " = "+duration->text()));
      if (parsed.toleranceIndex<0 && !singleStep->isChecked() && !tolerance->text().trimmed().isEmpty()) entries.append("StopTolerance = "+tolerance->text().trimmed());
      if (singleStep->isChecked()) entries=singleStepOptions(entries);
      QString result=original;
      if (entries!=parsed.entries) {
         for (auto &entry:entries) entry=entry.trimmed();
         if (match.capturedStart(3)>=0) {
            if (entries.isEmpty()) result.remove(match.capturedStart(3),match.capturedLength(3));
            else result.replace(match.capturedStart(4),match.capturedLength(4),entries.join(", "));
         } else if (!entries.isEmpty()) result.insert(original.indexOf(')',match.capturedEnd(2))+1," {"+entries.join(", ")+"}");
      }
      result.replace(match.capturedStart(2),match.capturedLength(2),spacecraft->currentText());
      result.replace(match.capturedStart(1),match.capturedLength(1),propagator->currentText());
      const auto span=backwardSpan(original);
      result.replace(span.first,span.second,backward->isChecked() ? (span.second ? original.mid(span.first,span.second) : QString("BackProp ")) : QString());
      synchronizing=true; changed(result); synchronizing=false;
   };
   connect(propagator,&QComboBox::currentTextChanged,this,update);
   connect(spacecraft,&QComboBox::currentTextChanged,this,[this,update] {
      if (synchronizing) return;
      if (!units->currentData().toString().isEmpty()) { const QSignalBlocker block(stopParameter); stopParameter->setText(spacecraft->currentText()+"."+units->currentData().toString()); }
      update();
   });
   connect(units,&QComboBox::currentIndexChanged,this,[this,update] {
      if (synchronizing || units->currentData().toString().isEmpty()) return;
      { const QSignalBlocker block(stopParameter); stopParameter->setText(spacecraft->currentText()+"."+units->currentData().toString()); }
      update();
   });
   connect(stopParameter,&QLineEdit::textChanged,this,[this,update] {
      const QSignalBlocker block(units); const auto suffix=stopParameter->text().section('.',-1);
      units->setCurrentIndex(suffix=="ElapsedSecs" ? 0 : suffix=="ElapsedDays" ? 1 : 2); update();
   });
   connect(duration,&QLineEdit::textChanged,this,update); connect(tolerance,&QLineEdit::textChanged,this,update);
   connect(backward,&QCheckBox::toggled,this,update); connect(singleStep,&QCheckBox::toggled,this,update);
}
void PropagationForm::setStatement(const QString &statement)
{
   if (synchronizing) return;
   // The group and stopping-condition dialogs handle multiple propagators or stops.
   const auto parsed=parse(statement); const auto &match=parsed.match;
   if (!parsed.valid || propagator->findText(match.captured(1))<0 || spacecraft->findText(match.captured(2))<0) { hide(); return; }
   synchronizing=true; original=statement;
   propagator->setCurrentText(match.captured(1)); spacecraft->setCurrentText(match.captured(2));
   backward->setChecked(backwardSpan(statement).second>0); tolerance->setText(parsed.tolerance);
   singleStep->setChecked(parsed.stopIndex<0); stopParameter->setText(parsed.stopIndex<0 ? spacecraft->currentText()+".ElapsedSecs" : parsed.stop);
   const auto suffix=stopParameter->text().section('.',-1); units->setCurrentIndex(suffix=="ElapsedSecs" ? 0 : suffix=="ElapsedDays" ? 1 : 2);
   duration->setText(parsed.stopIndex<0 ? "60" : parsed.goal);
   const bool stopping=!singleStep->isChecked(),event=eventStop(stopParameter->text());
   units->setEnabled(stopping); stopParameter->setEnabled(stopping); tolerance->setEnabled(stopping);
   findChild<QPushButton *>("propagationChooseStop")->setEnabled(stopping); duration->setEnabled(stopping && !event);
   findChild<QPushButton *>("propagationChooseGoal")->setEnabled(stopping && !event);
   synchronizing=false; show();
}
