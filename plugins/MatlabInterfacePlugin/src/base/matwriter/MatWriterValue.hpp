#pragma once
#include "MatlabValue.hpp"
#include "WriterData.hpp"
#include "UtilityException.hpp"
#include <limits>

// Native owned values for MAT output. These helpers call no MATLAB C API.
namespace MatWriterValue
{
inline Integer Dimension(size_t count)
{
   if (count > static_cast<size_t>(std::numeric_limits<Integer>::max()))
      throw UtilityException("MATLAB writer value exceeds native dimension capacity");
   return static_cast<Integer>(count);
}
inline size_t Count(size_t rows, size_t columns)
{
   Dimension(rows); Dimension(columns);
   if (columns && rows > static_cast<size_t>(std::numeric_limits<Integer>::max()) / columns)
      throw UtilityException("MATLAB writer value exceeds native element capacity");
   return rows * columns;
}
inline MatlabValue Empty()
{
   MatlabValue value; value.type = MatlabValue::Type::NUMERIC; value.className = "double";
   return value;
}
inline MatlabValue Character(const std::string &text)
{
   MatlabValue value; value.type = MatlabValue::Type::CHARACTER; value.className = "char";
   value.rows = 1; value.columns = Dimension(text.size()); value.text = text;
   return value;
}
inline MatlabValue Cell(size_t rows, size_t columns)
{
   MatlabValue value; value.type = MatlabValue::Type::CELL; value.className = "cell";
   value.rows = Dimension(rows); value.columns = Dimension(columns);
   value.children.resize(Count(rows, columns));
   return value;
}
inline MatlabValue Numbers(const Matrix &matrix)
{
   const size_t rows = matrix.size(), columns = rows ? matrix.front().size() : 0;
   for (const auto &row : matrix)
      if (row.size() != columns) throw UtilityException("MATLAB writer matrix has unequal row lengths");
   auto value = Empty(); value.rows = Dimension(rows); value.columns = Dimension(columns);
   value.numbers.reserve(Count(rows, columns));
   for (size_t column = 0; column < columns; ++column)
      for (size_t row = 0; row < rows; ++row) value.numbers.push_back(matrix[row][column]);
   return value;
}
inline MatlabValue Strings(const StringMatrix &matrix)
{
   const size_t rows = matrix.size(), columns = rows ? matrix.front().size() : 0;
   for (const auto &row : matrix)
      if (row.size() != columns) throw UtilityException("MATLAB writer string matrix has unequal row lengths");
   auto value = Cell(rows, columns);
   for (size_t column = 0; column < columns; ++column)
      for (size_t row = 0; row < rows; ++row)
         value.children[row + rows * column] = Character(matrix[row][column]);
   return value;
}
}
