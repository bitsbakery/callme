/*
* MIT License
*
* Copyright (c) 2023 Dmitry Kim <https://github.com/bitsbakery/pretty>
*
* Permission is hereby  granted, free of charge, to any  person obtaining a copy
* of this software and associated  documentation files (the "Software"), to deal
* in the Software  without restriction, including without  limitation the rights
* to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell 
* copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
* furnished to do so, subject to the following conditions:
*  
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
* IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
* FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
* AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
* LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

module;

#include <ostream>
#include <vector>

export module pretty;

export import :impl;

// A very lightweight console pretty printer for tables
namespace pretty
{
	export struct Printer;

	// Tabular data holder
	export struct Table
	{
		Table() = default;
		
		// Optionally call this to reserve memory for
		// rowsToReserve rows
		void reserveRows(RowN rowsToReserve)
		{
			_impl.reserveRows(rowsToReserve);
		}

		// Optionally call this to reserve memory for
		// colsToReserve columns after calling reserveRows()
		// or after adding some rows that you need to append
		// new columns to
		void reserveColumns(ColN colsToReserve)
		{
			_impl.reserveColumns(colsToReserve);
		}

		// Add numCols empty columns. Each new column has
		// the number of rows equal to the current
		// number of rows in the table
		Table& addColumns(ColN numCols)
		{
			_impl.addColumns(numCols);
			return *this;
		}

		// Add a column filled with the given colValues.
		// If there are existing rows in the table,
		// the number of colValues must match the current
		// number of rows
		Table& addCol(std::vector<std::string>&& colValues)
		{
			_impl.addCol(std::move(colValues));
			return *this;
		}

		// Add a row filled with the given rowValues.
		// If there are existing columns in the table,
		// the number of rowValues must match the current
		// number of columns
		template<typename...Vs> requires RowValues<Vs...>
		Table& addRow(Vs&&...rowValues)
		{
			_impl.addRow(std::forward<Vs>(rowValues)...);
			return *this;
		}

		// Add a row filled with the given rowValues.
		// If there are existing columns in the table,
		// the number of rowValues must match the current
		// number of columns
		Table& addRow(std::vector<std::string>&& rowValues)
		{
			_impl.addRow(std::move(rowValues));
			return *this;
		}

		// Add numRows empty rows. Each new row has
		// the number of columns equal to the current
		// number of columns in the table
		Table& addRows(RowN numRows)
		{
			_impl.addRows(numRows);
			return *this;
		}

		// Set text in the specific cell
		Table& setText(RowN row,
					   ColN col,
					   std::convertible_to<std::string> auto&& text)
		{
			_impl.setText(row, col, std::forward<decltype(text)>(text));
			return *this;
		}
		
		// Attach a title string to the table. The title is not 
		// printed with the table. You have to print it yourself.
		Table& title(std::convertible_to<std::string> auto&& t)
		{
			_impl.title(t);
			return *this;
		}
		
		// Get the title
		[[nodiscard]]std::string title() const
		{
			return _impl.title();
		}

		// The number of columns added so far
		[[nodiscard]]ColN numColumns() const
		{
			return _impl.numColumns();
		}

		// The number of rows added so far
		[[nodiscard]]RowN numRows() const
		{
			return _impl.numRows();
		}

		friend Printer;

	private:
		TableImpl _impl;
	};

	// Printing with formatting and styles
	export struct Printer
	{
		// Whether to separate the first row
		// (header row) from the rest with a
		// horizontal separator line
		Printer& headerSeparator(bool show)
		{
			_impl.headerSeparator(show);
			return *this;
		}

		// The style of frame lines 
		Printer& frame(FrameStyle style)
		{
			_impl.frame(style);
			return *this;
		}

		// The distance from the text to the cell border
		Printer& padding(CellTextSize n)
		{
			_impl.padding(n);
			return *this;
		}

		// Pretty-print the currently loaded table to os
		Printer& print(std::ostream& os) const;

		// Pretty-formatted string representation
		// of the table
		[[nodiscard]]std::string toString(const Table& t) const
		{
			return _impl.toString(t._impl);
		}

		// Load data into the printer
		Printer& load(const Table& data)
		{
			_data = &data;
			return *this;
		}

		// Load data into the printer
		Printer& operator()(const Table& t)
		{
			return load(t);
		}

		friend std::ostream& operator<<(std::ostream& os, const Printer& p);

	private:
		PrinterImpl _impl;
		const Table* _data = nullptr;
	};

	Printer& Printer::print(std::ostream& os) const
	{
		if (_data == nullptr)
			throw std::logic_error("Printer: no data loaded");
		_impl.print(_data->_impl,os);
		return const_cast<Printer&>(*this);
	}

	export std::ostream& operator<<(std::ostream& os, const Printer& p)
	{
		p.print(os);
		return os;
	}
}