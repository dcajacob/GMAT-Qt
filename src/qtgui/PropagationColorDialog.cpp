#include "PropagationColorDialog.hpp"
#include "UserParameter.hpp"
#include "ScriptStatements.hpp"
#include "Moderator.hpp"
#include "SpacePoint.hpp"
#include "RgbColor.hpp"
#include "BaseException.hpp"
#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>
namespace {
struct Parsed {
   bool valid=false,optionsOnly=true;
   ScriptStatement statement;
   int open=-1,close=-1,insert=0,color=-1,valueBegin=0,valueEnd=0;
   QVector<QPair<int,int>> entries;
};
Parsed parse(const QString &source)
{
   Parsed result;
   try {
      const auto statements=scriptStatements(source); if (statements.size()!=1) return result;
      result.statement=statements.first(); const auto &code=result.statement.code;
      if (!QRegularExpression("^\\s*Propagate\\s+(?:'[^'\\n]*'\\s+)?(?:BackProp\\s+)?(?:Synchronized\\s+)?[A-Za-z][A-Za-z0-9_]*\\s*\\(").match(code).hasMatch()) return result;
      bool quoted=false; int depth=0;
      for (int i=0;i<code.size();++i) {
         const auto c=code[i];
         if (c=='\'') { if (quoted && i+1<code.size() && code[i+1]=='\'') ++i; else quoted=!quoted; continue; }
         if (quoted) continue;
         if (c=='{' && depth==0) { if (result.open>=0) return result; result.open=i; }
         if (c=='(' || c=='[' || c=='{') ++depth;
         if (c==')' || c==']' || c=='}') { if (--depth<0) return result; if (c=='}' && depth==0) result.close=i; }
      }
      if (quoted || depth!=0 || (result.open>=0 && result.close<0)) return result;
      int end=code.size(); while (end>0 && code[end-1].isSpace()) --end;
      result.insert=end>0 && code[end-1]==';' ? end-1 : end;
      if (result.open>=0) {
         if (!QRegularExpression("^\\s*;?\\s*$").match(code.mid(result.close+1)).hasMatch()) return result;
         int begin=result.open+1; depth=0; quoted=false;
         for (int i=begin;i<=result.close;++i) {
            const auto c=code[i];
            if (c=='\'') { if (quoted && i+1<result.close && code[i+1]=='\'') ++i; else quoted=!quoted; }
            if (!quoted) {
               if (c=='(' || c=='[') ++depth;
               if (c==')' || c==']') --depth;
               if ((c==',' && depth==0) || i==result.close) { result.entries.append({begin,i}); begin=i+1; }
            }
         }
         for (int i=0;i<result.entries.size();++i) {
            const auto span=result.entries[i]; const auto entry=code.mid(span.first,span.second-span.first);
            if (entry.trimmed().isEmpty()) return result;
            if (!QRegularExpression("^\\s*(?:StopTolerance|OrbitColor)\\s*=").match(entry).hasMatch()) result.optionsOnly=false;
            const auto match=QRegularExpression("^\\s*OrbitColor\\s*=\\s*(.*?)\\s*$",QRegularExpression::DotMatchesEverythingOption).match(entry);
            if (!match.hasMatch()) continue;
            if (result.color>=0 || match.captured(1).isEmpty()) return result;
            result.color=i; result.valueBegin=span.first+match.capturedStart(1); result.valueEnd=result.valueBegin+match.capturedLength(1);
         }
      }
      result.valid=true;
   } catch (const std::exception &) {}
   return result;
}
QString edit(const QString &source,const Parsed &parsed,bool enabled,const QString &value)
{
   if (!parsed.valid) throw std::runtime_error("Use the source editor to correct this propagation command first.");
   const auto &statement=parsed.statement; const auto &code=statement.code;
   if (enabled && parsed.color>=0 && value==code.mid(parsed.valueBegin,parsed.valueEnd-parsed.valueBegin)) return source;
   if (!enabled && parsed.color<0) return source;
   if (enabled && parsed.color<0) {
      const auto index=parsed.open<0 ? parsed.insert : parsed.close;
      const auto position=index<statement.positions.size() ? statement.positions[index] : statement.positions.last()+1;
      auto result=source; result.insert(position,parsed.open<0 ? " {OrbitColor = "+value+"}" : ", OrbitColor = "+value); return result;
   }
   int begin=parsed.valueBegin,end=parsed.valueEnd;
   if (!enabled) {
      const auto span=parsed.entries[parsed.color]; begin=span.first; end=span.second;
      if (parsed.entries.size()==1 || parsed.optionsOnly) { begin=parsed.open; end=parsed.close+1; }
      else if (parsed.color>0) --begin; else ++end;
   }
   QVector<qsizetype> remove;
   for (int i=begin;i<end;++i) {
      const auto position=statement.positions[i]; const auto c=source[position];
      const bool beforeComment=c.isSpace() && position+1<source.size() && source[position+1]=='%';
      if (c!='\n' && c!='\r' && !beforeComment) remove.append(position);
   }
   for (auto position:statement.continuations) if (position>=statement.positions[begin] && position<=statement.positions[end-1]) remove.append(position);
   std::sort(remove.begin(),remove.end(),std::greater<qsizetype>());
   auto result=source; for (auto position:remove) result.remove(position,1);
   if (enabled) result.insert(statement.positions[begin],value);
   return result;
}
}
bool PropagationColorDialog::supports(const QString &source) { return parse(source).valid; }
PropagationColorDialog::PropagationColorDialog(const QString &source,QWidget *parent) : QDialog(parent),source(source)
{
   setObjectName("propagationColorDialog"); setProperty("helpTopic","Propagate"); setWindowTitle("Propagation segment color");
   const auto parsed=parse(source); auto *layout=new QVBoxLayout(this); layout->setSizeConstraint(QLayout::SetMinimumSize);
   auto *note=new QLabel("Override the trajectory color for this propagation segment. Turn it off to use each spacecraft's orbit color. Changes stay pending until you Apply the command.",this); note->setObjectName("propagationColorNote"); note->setWordWrap(true); note->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum); layout->addWidget(note);
   overrideColor=new QCheckBox("Override segment color",this); overrideColor->setObjectName("propagationColorOverride"); overrideColor->setChecked(parsed.color>=0); layout->addWidget(overrideColor);
   QString initial="Red";
   if (parsed.color>=0) initial=parsed.statement.code.mid(parsed.valueBegin,parsed.valueEnd-parsed.valueBegin);
   else if (const auto match=QRegularExpression("\\(\\s*([A-Za-z][A-Za-z0-9_]*)").match(parsed.statement.code);match.hasMatch()) if (auto *point=Moderator::Instance()->GetSpacePoint(match.captured(1).toStdString())) initial=QString::fromStdString(RgbColor::ToRgbString(point->GetCurrentOrbitColor()));
   auto *row=new QHBoxLayout; color=new QLineEdit(initial,this); color->setObjectName("propagationColorValue"); color->setToolTip("GMAT color name or integer RGB components [red green blue], each 0–255."); row->addWidget(color);
   auto *choose=new QPushButton("Choose color…",this); choose->setObjectName("propagationColorChoose"); row->addWidget(choose); layout->addLayout(row);
   auto *error=new QLabel(this); error->setObjectName("propagationColorError"); error->setWordWrap(true); layout->addWidget(error);
   auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this); layout->addWidget(buttons);
   const auto refresh=[=] {
      color->setEnabled(overrideColor->isChecked()); choose->setEnabled(overrideColor->isChecked());
      QString message;
      try { if (overrideColor->isChecked()) RgbColor::ToIntColor(color->text().trimmed().toStdString()); }
      catch (BaseException &failure) { message=QString::fromStdString(failure.GetFullMessage()); }
      catch (const std::exception &failure) { message=QString::fromUtf8(failure.what()); }
      error->setText(message); buttons->button(QDialogButtonBox::Ok)->setEnabled(parsed.valid && message.isEmpty());
   };
   connect(overrideColor,&QCheckBox::toggled,this,refresh); connect(color,&QLineEdit::textChanged,this,refresh);
   connect(choose,&QPushButton::clicked,this,[=] {
      QColor initial;
      try { initial=QColor::fromRgb(RgbColor::ToIntColor(color->text().trimmed().toStdString())&0xffffff); }
      catch (BaseException &) { initial=Qt::red; }
      QColorDialog dialog(initial,this); dialog.setObjectName("propagationColorPicker");
      if (dialog.exec()==QDialog::Accepted) { const auto value=dialog.currentColor(); color->setText(QString("[%1 %2 %3]").arg(value.red()).arg(value.green()).arg(value.blue())); }
   });
   connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject); refresh();
}
QString PropagationColorDialog::statement() const { return edit(source,parse(source),overrideColor->isChecked(),color->text().trimmed()); }
