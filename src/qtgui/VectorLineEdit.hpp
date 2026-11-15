#pragma once
#include <QLineEdit>
#include <array>
#include <functional>
#include <memory>

struct VectorPasteGroup;

class VectorLineEdit final : public QLineEdit
{
   Q_OBJECT
public:
   using QLineEdit::QLineEdit;
   static void link(const std::array<QLineEdit *,3> &fields,
                    std::function<bool()> enabled={});
public slots:
   void paste();
   void undo();
   void redo();
protected:
   void keyPressEvent(QKeyEvent *event) override;
   void contextMenuEvent(QContextMenuEvent *event) override;
private:
   std::shared_ptr<VectorPasteGroup> vectorGroup;
};
