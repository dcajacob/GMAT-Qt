#pragma once
#include "ResourcePreview.hpp"
#include <QDialog>
#include <QMap>
#include <QStringList>
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;
class GravityDialog final : public QDialog
{
public:
   GravityDialog(GmatBase &forceModel,const QMap<QString,QString> &pending,QWidget *parent=nullptr);
   QMap<QString,QString> values() const { return acceptedValues; }
private:
   void saveBody();
   void loadBody(const QString &name);
   void refresh(bool adjustModel=false);
   void acceptChanges();
   GmatBase *force(const QString &name) const;
   QtResourcePreview original;
   QMap<QString,QMap<QString,QString>> baseline,states;
   QMap<QString,QString> sources,acceptedValues,presets;
   QMap<QString,QLineEdit *> edits;
   QComboBox *body,*potentialModel,*source,*tideModel;
   QLabel *limits,*error;
   QPushButton *tideBrowse;
   QString activeBody;
   bool updating=false;
};
