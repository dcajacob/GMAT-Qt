#pragma once
#include <QDialog>
class SpaceTrackAccount;
class SpaceTrackAccountDialog final : public QDialog
{
public:
   explicit SpaceTrackAccountDialog(SpaceTrackAccount &account,QWidget *parent=nullptr);
};
