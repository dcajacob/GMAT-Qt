#pragma once
#include "ScriptCompatibility.hpp"
#include <QDialog>
class QComboBox;
class QLineEdit;
class OrbitPlaybackOptionsDialog final : public QDialog
{
public:
   OrbitPlaybackOptionsDialog(const std::optional<QtPlaybackSetting> &setting,const QStringList &parents,QWidget *parent=nullptr);
   std::optional<QtPlaybackSetting> settings() const;
private:
   QComboBox *clockMode,*syncParent,*toolbar;
   QLineEdit *timeScale;
};
