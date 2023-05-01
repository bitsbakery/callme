/*
	https://github.com/bitsbakery/pretty

	pretty.h and pretty.cpp will not be supported. They exist
	as a temporary workaround until C++20 modules are well
	supported by GCC and cmake.

	pretty.ixx, pretty-impl, unicode.ixx constitute the implementation
	that is and will be supported.	
*/

#pragma once
#ifndef MAIN_PRETTY_H
#define MAIN_PRETTY_H

#include <string>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <concepts>
#include <ostream>
#include <cstdint>
#include <ostream>
#include <vector>


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"
#endif

int mk_wcswidth(const char32_t* pwcs, size_t n);
std::u32string toUtf32(const std::string& u8str);

namespace unicode
{
    int columnWidth(const std::string& u8str);
    int columnWidth(const std::u32string& str);
}

std::u32string toUtf32(const char* u8str);

std::u32string toUtf32(const std::string& u8str);

/*
* This is an implementation of wcwidth() and wcswidth() (defined in
* IEEE Std 1002.1-2001) for Unicode.
*
* http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
* http://www.opengroup.org/onlinepubs/007904975/functions/wcswidth.html
*
* In fixed-width output devices, Latin characters all occupy a single
* "cell" position of equal width, whereas ideographic CJK characters
* occupy two such cells. Interoperability between terminal-line
* applications and (teletype-style) character terminals using the
* UTF-8 encoding requires agreement on which character should advance
* the cursor by how many cell positions. No established formal
* standards exist at present on which Unicode character shall occupy
* how many cell positions on character terminals. These routines are
* a first attempt of defining such behavior based on simple rules
* applied to data provided by the Unicode Consortium.
*
* For some graphical characters, the Unicode standard explicitly
* defines a character-cell width via the definition of the East Asian
* FullWidth (F), Wide (W), Half-width (H), and Narrow (Na) classes.
* In all these cases, there is no ambiguity about which width a
* terminal shall use. For characters in the East Asian Ambiguous (A)
* class, the width choice depends purely on a preference of backward
* compatibility with either historic CJK or Western practice.
* Choosing single-width for these characters is easy to justify as
* the appropriate long-term solution, as the CJK practice of
* displaying these characters as double-width comes from historic
* implementation simplicity (8-bit encoded characters were displayed
* single-width and 16-bit ones double-width, even for Greek,
* Cyrillic, etc.) and not any typographic considerations.
*
* Much less clear is the choice of width for the Not East Asian
* (Neutral) class. Existing practice does not dictate a width for any
* of these characters. It would nevertheless make sense
* typographically to allocate two character cells to characters such
* as for instance EM SPACE or VOLUME INTEGRAL, which cannot be
* represented adequately with a single-width glyph. The following
* routines at present merely assign a single-cell width to all
* neutral characters, in the interest of simplicity. This is not
* entirely satisfactory and should be reconsidered before
* establishing a formal standard in this area. At the moment, the
* decision which Not East Asian (Neutral) characters should be
* represented by double-width glyphs cannot yet be answered by
* applying a simple rule from the Unicode database content. Setting
* up a proper standard for the behavior of UTF-8 character terminals
* will require a careful analysis not only of each Unicode character,
* but also of each presentation form, something the author of these
* routines has avoided to do so far.
*
* http://www.unicode.org/unicode/reports/tr11/
*
* Markus Kuhn -- 2007-05-26 (Unicode 5.0)
*
* Permission to use, copy, modify, and distribute this software
* for any purpose and without fee is hereby granted. The author
* disclaims all warranties with regard to this software.
*
* Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
*/

struct interval
{
    char32_t first;
    char32_t last;
};

/* auxiliary function for binary search in interval table */
int bisearch(char32_t ucs, const struct interval* table, int max);


/* The following two functions define the column width of an ISO 10646
* character as follows:
*
*    - The null character (U+0000) has a column width of 0.
*
*    - Other C0/C1 control characters and DEL will lead to a return
*      value of -1.
*
*    - Non-spacing and enclosing combining characters (general
*      category code Mn or Me in the Unicode database) have a
*      column width of 0.
*
*    - SOFT HYPHEN (U+00AD) has a column width of 1.
*
*    - Other format characters (general category code Cf in the Unicode
*      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
*
*    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
*      have a column width of 0.
*
*    - Spacing characters in the East Asian Wide (W) or East Asian
*      Full-width (F) category as defined in Unicode Technical
*      Report #11 have a column width of 2.
*
*    - All remaining characters (including all printable
*      ISO 8859-1 and WGL4 characters, Unicode control characters,
*      etc.) have a column width of 1.
*
* This implementation assumes that char32_t characters are encoded
* in ISO 10646.
*/

int mk_wcwidth(char32_t ucs);

int mk_wcswidth(const char32_t* pwcs, size_t n);

/*
* The following functions are the same as mk_wcwidth() and
* mk_wcswidth(), except that spacing characters in the East Asian
* Ambiguous (A) category as defined in Unicode Technical Report #11
* have a column width of 2. This variant might be useful for users of
* CJK legacy encodings who want to migrate to UCS without changing
* the traditional terminal character-width behaviour. It is not
* otherwise recommended for general use.
*/
int mk_wcwidth_cjk(char32_t ucs);

int mk_wcswidth_cjk(const char32_t* pwcs, size_t n);

namespace pretty
{
    enum class FrameStyle
    {
        Basic,
        Line,
        LineRounded,
        ThickLine,
        DoubleLine,
        Minimal
    };

    template<typename T, T max, const char *ExceededMessage>
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

    inline constexpr auto MaxRows = 1024;
    inline constexpr char ErrMaxRows[] = "Max row number exceeded";
    using RowN = Index<uint16_t, MaxRows, ErrMaxRows>;

    inline constexpr auto MaxColumns = 255;
    inline constexpr char ErrMaxCols[] = "Max column number exceeded";
    using ColN = Index<uint8_t, MaxColumns, ErrMaxCols>;

    inline constexpr auto MaxCellTextSize = 255;
    inline constexpr char ErrMaxCellText[] = "Max cell text size exceeded";
    using CellTextSize = Index<uint8_t, MaxCellTextSize, ErrMaxCellText>;

    using Rows = std::vector<std::vector<std::string>>;
    using ColumnSizes = std::vector<CellTextSize>;

    constexpr char SpaceChar = ' ';

    template<typename...Vs>
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

        [[nodiscard]]std::string title()
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

    void validateText(const std::string& str);

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

    template<typename...Vs> requires RowValues<Vs...>
    void TableImpl::addRow(Vs&&...rowValues)
    {
        addRow({ std::forward<Vs>(rowValues)... });
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

    std::string dup(const std::string& s, CellTextSize repeats);

    std::string dup(char ch, CellTextSize repeats);
}

// A very lightweight console pretty printer for tables
namespace pretty
{
    struct Printer;

    // Tabular data holder
    struct Table
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

        Table& title(std::convertible_to<std::string> auto&& t)
        {
            _impl.title(t);
            return *this;
        }

        [[nodiscard]]std::string title()
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
    struct Printer
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
}

#endif //MAIN_PRETTY_H
