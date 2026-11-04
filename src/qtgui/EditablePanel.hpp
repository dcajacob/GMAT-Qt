#pragma once
#include <QWidget>
#include <QMdiSubWindow>
#include <functional>
class QDialogButtonBox;
class EditablePanel : public QWidget
{
public:
   using QWidget::QWidget;
   virtual bool hasChanges() const = 0;
   // Input may remain dirty after its value is restored to the snapshot.
   virtual bool hasUnappliedInput() const { return hasChanges(); }
   virtual void discardChanges() = 0;
   // Workspace-only controls; embedded creation dialogs retain their buttons.
   void addOkCancelActions(QDialogButtonBox &buttons);
   // The workspace refreshes an accepted snapshot in the same MDI window.
   // Standalone callers retain their existing close-on-success contract.
   std::function<void()> onApplied;
protected:
   void appliedSuccessfully();
private:
   bool closeAfterApply=false;
};
class EditorSubWindow final : public QMdiSubWindow
{
protected:
   void closeEvent(QCloseEvent *event) override;
};
