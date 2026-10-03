#include "ReportViewer.hpp"
#include "FindReplaceDialog.hpp"
#include <QPlainTextEdit>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFontDatabase>
#include <QSignalBlocker>
#include <QLineEdit>
#include <QCheckBox>
#include <QTimer>
#include <QTextCursor>
#include <algorithm>
#include <limits>

ReportViewer::ReportViewer(const QString &path,const QString &name,QWidget *parent,const std::function<void()> &compare)
   : QWidget(parent),path(path)
{
   auto *layout=new QVBoxLayout(this); layout->setContentsMargins(4,4,4,4);
   auto *controls=new QHBoxLayout; layout->addLayout(controls);
   auto button=[&](const QString &label,const QString &id) { auto *b=new QPushButton(label,this); b->setObjectName(id); controls->addWidget(b); return b; };
   first=button("First","reportFirst"); previous=button("Previous","reportPrevious");
   pageNumber=new QSpinBox(this); pageNumber->setObjectName("reportPage"); pageNumber->setPrefix("Page "); pageNumber->setKeyboardTracking(false); pageNumber->setRange(1,1); controls->addWidget(pageNumber);
   next=button("Next","reportNext"); last=button("Last","reportLast");
   auto *reload=button("Reload","reportReload"),*find=button("Find…","reportFind"); controls->addStretch();
   if (compare) connect(button("Compare…","reportCompare"),&QPushButton::clicked,this,compare);
   auto *fileSearch=new QHBoxLayout; layout->addLayout(fileSearch);
   searchText=new QLineEdit(this); searchText->setObjectName("reportFileSearchText"); searchText->setMaxLength(32768);
   searchText->setPlaceholderText("Find in complete file…"); fileSearch->addWidget(searchText,1);
   auto *searchAll=new QPushButton("Search file",this); searchAll->setObjectName("reportSearchFile"); fileSearch->addWidget(searchAll);
   nextMatch=new QPushButton("Next match",this); nextMatch->setObjectName("reportNextMatch"); nextMatch->setEnabled(false); fileSearch->addWidget(nextMatch);
   stop=new QPushButton("Stop",this); stop->setObjectName("reportStopSearch"); stop->setEnabled(false); fileSearch->addWidget(stop);
   auto *options=new QHBoxLayout; layout->addLayout(options);
   matchCase=new QCheckBox("Match case",this); matchCase->setObjectName("reportFileMatchCase"); matchCase->setChecked(true); options->addWidget(matchCase);
   wholeWords=new QCheckBox("Whole words",this); wholeWords->setObjectName("reportFileWholeWords"); options->addWidget(wholeWords); options->addStretch();
   searchInput=new QFile(path,this); searchTimer=new QTimer(this); searchTimer->setInterval(0);
   connect(searchAll,&QPushButton::clicked,this,[this] { searchFile(0); });
   connect(searchText,&QLineEdit::returnPressed,this,[this] { searchFile(0); });
   connect(nextMatch,&QPushButton::clicked,this,[this] { searchFile(nextOffset); });
   connect(stop,&QPushButton::clicked,this,[this] { stopSearch(); status->setText("File search stopped. Displayed content is unchanged."); });
   connect(searchText,&QLineEdit::textChanged,this,[this] { stopSearch(); nextMatch->setEnabled(false); });
   for (auto *option:{matchCase,wholeWords}) connect(option,&QCheckBox::toggled,this,[this] {
      stopSearch(); nextMatch->setEnabled(false);
      status->setText("Search options changed. Search file starts from the beginning.");
   });
   connect(searchTimer,&QTimer::timeout,this,[this] {
      qint64 start=searchInput->pos()-overlap.size();
      const auto block=searchInput->read(1024*1024);
      if (searchInput->error()!=QFileDevice::NoError) { stopSearch(); status->setText("File search failed: "+searchInput->errorString()); return; }
      auto buffer=overlap+block;
      // Next match starts with up to four preceding bytes for word context.
      // Drop an initial partial UTF-8 character before decoding the window.
      qsizetype leading=0;
      while (leading<buffer.size() && (static_cast<unsigned char>(buffer[leading]) & 0xc0)==0x80) ++leading;
      buffer.remove(0,leading); start+=leading;
      const auto decoded=QString::fromUtf8(buffer); const bool atEnd=searchInput->atEnd();
      auto wordAt=[&](qsizetype index) {
         if (index<0 || index>=decoded.size()) return false;
         char32_t character=decoded[index].unicode();
         if (decoded[index].isLowSurrogate() && index>0 && decoded[index-1].isHighSurrogate())
            character=QChar::surrogateToUcs4(decoded[index-1],decoded[index]);
         else if (decoded[index].isHighSurrogate() && index+1<decoded.size() && decoded[index+1].isLowSurrogate())
            character=QChar::surrogateToUcs4(decoded[index],decoded[index+1]);
         const auto category=QChar::category(character);
         return QChar::isLetterOrNumber(character) || category==QChar::Punctuation_Connector ||
               category==QChar::Mark_NonSpacing || category==QChar::Mark_SpacingCombining || category==QChar::Mark_Enclosing;
      };
      qsizetype from=0;
      while (true) {
         const auto match=decoded.indexOf(needle,from,searchCase);
         if (match<0) break;
         // Defer tail candidates until their complete text and right word
         // context are available. Four bytes per UTF-16 unit bound overlap.
         if (!atEnd && match>decoded.size()-needle.size()-4) break;
         from=match+1;
         // The first retained character has no left context. It was already
         // evaluated in the preceding window, so cannot start a new word here.
         if (searchWholeWords && ((start>0 && match==0) || wordAt(match-1) || wordAt(match+needle.size()))) continue;
         const qint64 offset=start+decoded.left(match).toUtf8().size();
         if (offset<searchFrom) continue;
         const auto matched=decoded.mid(match,needle.size());
         nextOffset=offset+matched.toUtf8().size(); stopSearch();
         if (!loadPage(offset/(1024*1024))) return;
         if (offset>=visibleStart+visibleBytes.size() && !loadPage(currentPage+1)) return;
         const auto prefix=QString::fromUtf8(visibleBytes.left(offset-visibleStart)).replace("\r\n","\n");
         auto cursor=text->textCursor(); cursor.setPosition(prefix.size());
         const auto selected=QString(matched).replace("\r\n","\n");
         cursor.setPosition(std::min<qsizetype>(text->toPlainText().size(),prefix.size()+selected.size()),QTextCursor::KeepAnchor);
         text->setTextCursor(cursor); text->ensureCursorVisible(); nextMatch->setEnabled(true);
         status->setText(QString("Match at byte %1.%2").arg(offset+1).arg(nextOffset>visibleStart+visibleBytes.size() ? " Match continues on the next page." : "")); return;
      }
      if (block.isEmpty() || atEnd) { stopSearch(); status->setText("No further matches. Search file starts again from the beginning."); return; }
      qsizetype retain=std::max<qsizetype>(0,buffer.size()-(needle.size()*4+20));
      while (retain>0 && (static_cast<unsigned char>(buffer[retain]) & 0xc0)==0x80) --retain;
      overlap=buffer.mid(retain);
      status->setText(QString("Searching complete file… %1 of %2 bytes").arg(searchInput->pos()).arg(searchInput->size()));
   });
   status=new QLabel(this); status->setObjectName("reportPageStatus"); status->setWordWrap(true); layout->addWidget(status);
   text=new QPlainTextEdit(this); text->setObjectName("report:"+name); text->setReadOnly(true);
   text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); text->setLineWrapMode(QPlainTextEdit::NoWrap); text->setToolTip(path); layout->addWidget(text);
   search=new FindReplaceDialog(text,this);
   connect(find,&QPushButton::clicked,search,&FindReplaceDialog::openSearch);
   // The main Edit actions own the workspace shortcuts and route them here.
   // Registering another Ctrl+F in this child makes both shortcuts ambiguous.
   connect(first,&QPushButton::clicked,this,[this] { loadPage(0); });
   connect(previous,&QPushButton::clicked,this,[this] { loadPage(currentPage-1); });
   connect(next,&QPushButton::clicked,this,[this] { loadPage(currentPage+1); });
   connect(last,&QPushButton::clicked,this,[this] { loadPage(std::numeric_limits<qint64>::max()); });
   connect(reload,&QPushButton::clicked,this,[this] { loadPage(currentPage); });
   connect(pageNumber,&QSpinBox::valueChanged,this,[this](int value) { loadPage(value-1); });
   first->setEnabled(false); previous->setEnabled(false); next->setEnabled(false); last->setEnabled(false);
   loadPage(0);
}
bool ReportViewer::loadPage(qint64 requested)
{
   stopSearch();
   QFile file(path);
   auto fail=[&](const QString &message) { status->setText(message+" Previous displayed content is retained."); const QSignalBlocker block(pageNumber); pageNumber->setValue(int(currentPage+1)); };
   if (!file.open(QIODevice::ReadOnly)) { fail("Report unavailable: "+file.errorString()); return false; }
   constexpr qint64 chunk=1024*1024;
   const qint64 size=file.size(),pages=(size==0 ? 1 : (size-1)/chunk+1);
   if (pages>std::numeric_limits<int>::max()) { fail("Report exceeds the supported page count."); return false; }
   const qint64 page=std::clamp<qint64>(requested,0,pages-1),start=page*chunk,end=std::min(size,start+chunk);
   // Extend both edges by up to three bytes to keep valid UTF-8 characters
   // intact. The same boundary rule partitions adjacent pages without gaps.
   const qint64 readStart=std::max<qint64>(0,start-3);
   if (!file.seek(readStart)) { fail("Report seek failed: "+file.errorString()); return false; }
   const auto bytes=file.read(end-readStart);
   if (file.error()!=QFileDevice::NoError || bytes.size()!=end-readStart) { fail("Report changed or could not be read. Reload to retry."); return false; }
   auto boundary=[&](qint64 offset) {
      while (offset>0 && offset<bytes.size() && (static_cast<unsigned char>(bytes[offset]) & 0xc0)==0x80) --offset;
      return offset;
   };
   qint64 begin=boundary(start-readStart);
   if (begin>0 && begin<bytes.size() && bytes[begin]=='\n' && bytes[begin-1]=='\r') --begin;
   qint64 finish=bytes.size();
   if (end<size) {
      // Inspect the byte immediately after this page to detect a split codepoint.
      const auto following=file.read(1);
      if (following.size()!=1 || file.error()!=QFileDevice::NoError) { fail("Report changed or could not be read. Reload to retry."); return false; }
      if (!following.isEmpty() && !bytes.isEmpty() && following[0]=='\n' && bytes.back()=='\r') --finish;
      else if (!following.isEmpty() && (static_cast<unsigned char>(following[0]) & 0xc0)==0x80) {
         finish=bytes.size()-1;
         while (finish>0 && (static_cast<unsigned char>(bytes[finish]) & 0xc0)==0x80) --finish;
      }
   }
   visibleBytes=bytes.mid(begin,finish-begin); visibleStart=readStart+begin;
   text->setPlainText(QString::fromUtf8(visibleBytes)); currentPage=page;
   { const QSignalBlocker block(pageNumber); pageNumber->setRange(1,int(std::min<qint64>(pages,std::numeric_limits<int>::max()))); pageNumber->setValue(int(page+1)); }
   first->setEnabled(page>0); previous->setEnabled(page>0); next->setEnabled(page+1<pages); last->setEnabled(page+1<pages);
   status->setText(QString("Page %1 of %2 · %3 bytes total. Rows may continue on adjacent pages. Find searches this page.").arg(page+1).arg(pages).arg(size));
   return true;
}

void ReportViewer::stopSearch()
{
   searchTimer->stop(); searchInput->close(); overlap.clear(); stop->setEnabled(false);
}
void ReportViewer::searchFile(qint64 start)
{
   stopSearch(); nextMatch->setEnabled(false); needle=searchText->text();
   searchFrom=start; searchCase=matchCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive; searchWholeWords=wholeWords->isChecked();
   if (needle.isEmpty()) { status->setText("Enter text to search for."); return; }
   if (!searchInput->open(QIODevice::ReadOnly) || !searchInput->seek(std::max<qint64>(0,start-4))) {
      status->setText("Cannot search report: "+searchInput->errorString()); searchInput->close(); return;
   }
   stop->setEnabled(true); status->setText("Searching complete file…"); searchTimer->start();
}
