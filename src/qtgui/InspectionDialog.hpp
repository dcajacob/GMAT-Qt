#pragma once
#include <QDialog>
#include <QStringList>
#include <functional>
class QPlainTextEdit;
class QLabel;
class InspectionDialog : public QDialog
{
public:
   using ProtectedPaths=std::function<QStringList()>;
   InspectionDialog(const QString &title,const QString &text,const QString &context,
                    QWidget *parent=nullptr,ProtectedPaths protectedPaths={});
   QString exportText(const QString &path) const;
   void setText(const QString &text);
private:
   QPlainTextEdit *text;
   QLabel *status;
   ProtectedPaths protectedPaths;
};
class SummaryDialog final : public InspectionDialog
{
public:
   using Loader=std::function<QString(const QString &,bool,bool)>;
   SummaryDialog(const QString &title,const QStringList &frames,bool mission,
                 Loader loader,const QString &context,QWidget *parent,ProtectedPaths protectedPaths);
};
