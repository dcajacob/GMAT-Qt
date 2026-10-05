#pragma once
#include "ScriptCompatibility.hpp"
#include <QDialog>
class QComboBox;
class QLineEdit;
class QWidget;
class OrbitStarOptionsDialog final : public QDialog
{
public:
   OrbitStarOptionsDialog(const std::optional<QtStarSetting> &setting,QWidget *parent=nullptr,std::optional<int> pendingCount=std::nullopt);
   std::optional<QtStarSetting> settings() const;
private:
   QComboBox *preset;
   QLineEdit *catalog,*count,*minMagnitude,*maxMagnitude,*minPixels,*maxPixels,*minDimRatio;
   QWidget *customFields,*catalogFields;
};
