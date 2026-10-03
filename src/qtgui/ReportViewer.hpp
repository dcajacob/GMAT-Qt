#pragma once
#include <QWidget>
#include <QString>
#include <QByteArray>
#include <functional>
class QPlainTextEdit;
class FindReplaceDialog;
class QLineEdit;
class QCheckBox;
class QFile;
class QTimer;
class QLabel;
class QSpinBox;
class QPushButton;
class ReportViewer final : public QWidget
{
public:
   ReportViewer(const QString &path,const QString &name,QWidget *parent=nullptr,
                const std::function<void()> &compare={});
   FindReplaceDialog *searchDialog() const { return search; }
private:
   FindReplaceDialog *search;
   bool loadPage(qint64 page);
   void searchFile(qint64 start);
   void stopSearch();
   QLineEdit *searchText;
   QCheckBox *matchCase,*wholeWords;
   QString needle;
   Qt::CaseSensitivity searchCase=Qt::CaseSensitive;
   bool searchWholeWords=false;
   QPushButton *nextMatch,*stop;
   QFile *searchInput;
   QTimer *searchTimer;
   QByteArray overlap,visibleBytes;
   qint64 nextOffset=0,visibleStart=0,searchFrom=0;
   QString path;
   qint64 currentPage=0;
   QPlainTextEdit *text;
   QLabel *status;
   QSpinBox *pageNumber;
   QPushButton *first,*previous,*next,*last;
};
