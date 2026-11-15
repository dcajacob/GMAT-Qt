#include "ScriptEditor.hpp"
#include <QApplication>
#include <QWidget>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv)
{
   QApplication app(argc, argv);
   ScriptEditor editor;
   editor.resize(800, 600);
   editor.show();
   for (const int lines : {1, 9, 10, 99, 100, 999, 1000}) {
      editor.setPlainText(QString("% Unicode Δ\n").repeated(lines));
      for (const int width : {160, 480, 1200, 240}) {
         editor.resize(width, 400);
         app.processEvents();
         const auto text = editor.toPlainText();
         const int margin = editor.viewport()->geometry().left();
         editor.setLineNumbersVisible(false);
         app.processEvents();
         if (editor.viewport()->geometry().left() >= margin)
            throw std::runtime_error("Hiding line numbers failed to reclaim margin");
         editor.setLineNumbersVisible(true);
         app.processEvents();
         if (editor.viewport()->geometry().left() != margin || editor.toPlainText() != text)
            throw std::runtime_error("Line-number layout did not restore exactly");
      }
   }
   std::cout << "Script editor margin transitions passed\n";
}
