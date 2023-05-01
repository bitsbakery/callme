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

#include <cassert>
#include <stdexcept>
#include <vector>
#include <concepts>
#include <ostream>
#include <cstdint>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"
#endif

export module pretty:impl;

import unicode;

namespace pretty
{
	export enum class FrameStyle
	{
		Basic,
		Line,
		LineRounded,
		ThickLine,
		DoubleLine,
		Minimal
	};

	export template<typename T, T max, const char *ExceededMessage>
	struct Index
	{
		Index() = default;

		Index(std::size_t v) :
			_val(static_cast<T>(v))
		{
			if (v > max)
				throw std::range_error(ExceededMessage);
		}

		void operator +=(T rhs)
		{
			if(_val > max - rhs)
				throw std::range_error(ExceededMessage);
			_val += rhs;
		}

		[[maybe_unused]]T operator++()
		{
			++_val;
			return _val;
		}

		[[maybe_unused]]T operator--()
		{
			--_val;
			return _val;
		}

		[[maybe_unused]]T checkedInc()
		{
			if(_val >= max)
				throw std::range_error(ExceededMessage);
			++_val;
			return _val;
		}

		operator T() const { return _val; }

		T val() { return _val; }
	private:
		T _val = 0;
	};

	constexpr auto MaxRows = 1024;
	constexpr char ErrMaxRows[] = "Max row number exceeded";
	export using RowN = Index<uint16_t, MaxRows, ErrMaxRows>;

	constexpr auto MaxColumns = 255;
	constexpr char ErrMaxCols[] = "Max column number exceeded";
	export using ColN = Index<uint8_t, MaxColumns, ErrMaxCols>;

	constexpr auto MaxCellTextSize = 255;
	constexpr char ErrMaxCellText[] = "Max cell text size exceeded";
	export using CellTextSize = Index<uint8_t, MaxCellTextSize, ErrMaxCellText>;

	using Rows = std::vector<std::vector<std::string>>;
	using ColumnSizes = std::vector<CellTextSize>;

	constexpr char SpaceChar = ' ';

	export template<typename...Vs>
		concept RowValues =
		(std::convertible_to<Vs, std::string> && ...) &&
		sizeof...(Vs) <= MaxColumns &&
		sizeof...(Vs) > 0;

	struct PrinterImpl;

	struct TableImpl
	{
		void reserveRows(RowN rowsToReserve);
		void reserveColumns(ColN colsToReserve);

		template<typename...Vs> requires RowValues<Vs...>
		void addRow(Vs&&...rowValues);
		void addRow(std::vector<std::string>&& rowValues);
		void addRows(RowN numRows);

		void addCol(std::vector<std::string>&& colValues);
		void addColumns(ColN numCols);

		void setText(RowN row,
					 ColN col,
					 std::convertible_to<std::string> auto&& text);

		void title(std::convertible_to<std::string> auto&& t)
		{
			_title = std::forward<decltype(t)>(t);
		}

		[[nodiscard]]std::string title() const
		{
			return _title;
		}

		[[nodiscard]]ColN numColumns() const { return _numColumns; }
		[[nodiscard]]RowN numRows() const { return _numRows; } 

	private:
		Rows _rows;

		ColN _numColumns;
		RowN _numRows;

		std::string _title;

		friend PrinterImpl;
	};

	struct FrameGlyphsRow
	{
		std::string left;
		std::string intersect;
		std::string right;
	};

	struct FrameGlyphs
	{
		std::string horizontal;
		std::string vertical;
		FrameGlyphsRow top;
		FrameGlyphsRow middle;
		FrameGlyphsRow bottom;
	};

	const FrameGlyphs Basic			= { "-", "|", {"+", "+", "+"}, {"+", "+", "+"}, {"+", "+", "+"} };
	const FrameGlyphs LineRounded	= { "─", "│", {"╭", "┬", "╮"}, {"├", "┼", "┤"}, {"╰", "┴", "╯"} };
	const FrameGlyphs Line			= { "─", "│", {"┌", "┬", "┐"}, {"├", "┼", "┤"}, {"└", "┴", "┘"} };
	const FrameGlyphs ThickLine		= { "━", "┃", {"┏", "┳", "┓"}, {"┣", "╋", "┫"}, {"┗", "┻", "┛"} };
	const FrameGlyphs DoubleLine	= { "═", "║", {"╔", "╦", "╗"}, {"╠", "╬", "╣"}, {"╚", "╩", "╝"} };
	const FrameGlyphs Minimal		= { "-", " ", {" ", " ", " "}, {" ", " ", " "}, {" ", " ", " "} };

	struct PrinterImpl
	{
		PrinterImpl();

		void padding(CellTextSize n);
		void headerSeparator(bool show) { _headerSeparator = show; }
		void frame(FrameStyle style);
		[[nodiscard]]std::string toString(const TableImpl& t) const;
		void print(const TableImpl& t, std::ostream& os) const;
	
	private:
#ifdef _WIN32
		void setUtf8() const;
#endif

		bool _headerSeparator = true;

		FrameGlyphs _styleSymbols;
		FrameStyle _style = FrameStyle::Line;

		CellTextSize _padding = 1;

		std::string _paddingStr;

		[[nodiscard]]ColumnSizes measure(const TableImpl& table) const;

		void makeHorizontalLine(const TableImpl& table,
						   const ColumnSizes& columnSizes,
						   const FrameGlyphsRow& rowType,
						   std::string& out) const;

		void makeRow(const TableImpl& table,
					const ColumnSizes& columnSizes,
					const std::vector<std::string>& row,
					std::string& out) const;
	};

	void validateText(const std::string& str)
	{
		if (str.size() > MaxCellTextSize)
			throw std::out_of_range(str);

		if (str.find_first_of("\a\b\t\n\v\f\r") != std::string::npos)
			throw std::invalid_argument("Escape sequences are not supported");
	}

	[[nodiscard]]CellTextSize TextSize(const std::string& str)
	{
		return unicode::columnWidth(str);
	}

	void TableImpl::addRows(RowN numRows)
	{
		_numRows += numRows;

		_rows.reserve(_rows.size() + numRows);
		for (; numRows; --numRows)
			_rows.push_back(std::vector<std::string>(_numColumns));
	}

	void TableImpl::addColumns(ColN numCols)
	{
		_numColumns += numCols;

		for (std::vector<std::string>& row : _rows)
		{
			row.reserve(row.size() + numCols);
			for (ColN c{}; c < numCols; ++c)
				row.push_back({});
		}
	}

	void TableImpl::reserveRows(RowN rowsToReserve)
	{
		_rows.reserve(rowsToReserve);
	}

	void TableImpl::reserveColumns(ColN colsToReserve)
	{
		if (_rows.empty())
			throw std::logic_error("Create rows first, reserveColumns() affects only existing rows");

		for (auto& row : _rows)
			row.reserve(colsToReserve);
	}

	template<typename...Vs> requires RowValues<Vs...>
	void TableImpl::addRow(Vs&&...rowValues)
	{
		addRow({ std::forward<Vs>(rowValues)... });
	}

	void TableImpl::addRow(std::vector<std::string>&& rowValues)
	{
		if (_numColumns == 0)
			addColumns(rowValues.size());
		else if (rowValues.size() != _numColumns)
			throw std::invalid_argument{ "Number of columns/headers mismatch" };

		for (const std::string& v : rowValues)
			validateText(v);

		_numRows.checkedInc();
		_rows.push_back(std::move(rowValues));
	}

	void TableImpl::addCol(std::vector<std::string>&& colValues)
	{
		if (_numRows == 0)
			addRows(colValues.size());
		else if (colValues.size() != _numRows)
			throw std::invalid_argument{ "Number of rows mismatch" };

		for (const std::string& v : colValues)
			validateText(v);

		_numColumns.checkedInc();

		for (RowN row{}; row < _numRows; ++row)
			_rows[row].push_back(std::move(colValues[row]));
	}

	void TableImpl::setText(RowN row,ColN col,
							std::convertible_to<std::string> auto&& text)
	{
		if (row + 1 > _numRows)
			throw std::out_of_range{ "Row out of range." };
		if (col + 1 > _numColumns)
			throw std::out_of_range{ "Column out of range." };

		validateText(text);

		_rows[row][col] = std::forward<decltype(text)>(text);
	}

	// Printer
	//////////////////////////////////////////////////////////////

	std::string dup(const std::string& s, CellTextSize repeats)
	{
		std::string ret;
		ret.reserve(s.size() * repeats);
		for (; repeats; --repeats)
			ret.append(s);
		return ret;
	}

	std::string dup(char ch, CellTextSize repeats)
	{
		return std::string(repeats, ch);
	}

	PrinterImpl::PrinterImpl():
		_styleSymbols(Line),
		_paddingStr(dup(SpaceChar, _padding))
	{
	}

	void PrinterImpl::padding(CellTextSize n)
	{
		if (n == _padding)
			return;

		_padding = n;
		_paddingStr = dup(SpaceChar, _padding);
	}

	void PrinterImpl::frame(FrameStyle style)
	{
		if (style == _style)
			return;

		_style = style;
		switch (style)
		{
		case FrameStyle::Basic:
			_styleSymbols = Basic;
			break;
		case FrameStyle::ThickLine:
			_styleSymbols = ThickLine;
			break;
		case FrameStyle::LineRounded:
			_styleSymbols = LineRounded;
			break;
		case FrameStyle::Line:
			_styleSymbols = Line;
			break;
		case FrameStyle::DoubleLine:
			_styleSymbols = DoubleLine;
			break;
		case FrameStyle::Minimal:
			_styleSymbols = Minimal;
			break;
		default:
			_styleSymbols = Basic;
		}
	}

	void PrinterImpl::makeHorizontalLine(
		const TableImpl& table,
		const ColumnSizes& columnSizes,
		const FrameGlyphsRow& rowType,
		std::string& out) const
	{
		out.append(rowType.left);
		for (ColN col{}; col < table._numColumns; ++col)
		{
			out.append(dup(_styleSymbols.horizontal, 
						   columnSizes[col] + _padding + _padding));
			out.append(col == table._numColumns - 1 ? 
					   rowType.right : rowType.intersect);
		}
		out.append("\n");
	}

	void PrinterImpl::makeRow(const TableImpl& table,
							 const ColumnSizes& columnSizes,
							 const std::vector<std::string>& row,
							 std::string& out) const
	{
		out.append(_styleSymbols.vertical);
		for (ColN col{}; col < table._numColumns; ++col)
		{
			const std::string& text = row[col];
			out.append(_paddingStr).
				append(text).
				append(dup(SpaceChar, columnSizes[col] - TextSize(text))).
				append(_paddingStr).
				append(_styleSymbols.vertical);
		}
		out.append("\n");
	}

	ColumnSizes PrinterImpl::measure(const TableImpl& table) const
	{
		ColumnSizes sizes(table.numColumns());
		
		for (const auto& row : table._rows)
			for (ColN col{};col<table._numColumns;++col)
				if (auto size = TextSize(row[col]); size > sizes[col])
					sizes[col] = size;

		return sizes;
	}

	std::string PrinterImpl::toString(const TableImpl& t) const
	{
		if (t._rows.empty())
			return {};

		assert(t._numRows == t._rows.size());
		assert(t._numColumns == t._rows[0].size());

		const auto columnsSizes = measure(t);

		std::string out;

		makeHorizontalLine(t, 
						   columnsSizes, 
						  _styleSymbols.top, 
						   out);

		/* The top line is composed of UTF8 multi-byte codepoints,
		so the number of bytes cbTop is greater than the number of
		visible characters in the line.
		By using cbTop as an estimate of number of bytes in other lines,
		we generally overestimate the expected number of bytes for
		the whole table, but this is ok.
		*/
		const auto approxExpectedTableSize =
			t._rows.size() * out.size() +
			3 * out.size();//top+bottom+header separator

		out.reserve(approxExpectedTableSize);

		if (_style == FrameStyle::Minimal)
			out.clear();

		RowN row;//header
		makeRow(t, columnsSizes, t._rows[row], out);
		++row;

		//header separator
		if (_headerSeparator)
		{
			makeHorizontalLine(t, 
							   columnsSizes, 
							   _styleSymbols.middle, 
							   out);
		}

		//data
		for (; row < t._numRows; ++row)
			makeRow(t, columnsSizes, t._rows[row], out);

		if (_style != FrameStyle::Minimal)
		{
			makeHorizontalLine(t, 
							   columnsSizes, 
			                   _styleSymbols.bottom, 
							   out);
		}

		return out;
	}

#ifdef _WIN32
	void PrinterImpl::setUtf8() const
	{
		//non-unicode styles
		if (_style == FrameStyle::Basic || _style == FrameStyle::Minimal)
			return;

		static bool _Utf8Activated = false;
		if (!_Utf8Activated)
		{
			_Utf8Activated = true;
			SetConsoleOutputCP(CP_UTF8);
		}
	}
#endif

	void PrinterImpl::print(const TableImpl& t, std::ostream& os) const
	{
#ifdef _WIN32
		setUtf8();
#endif

		os << toString(t);
	}
}
