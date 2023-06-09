/*
	https://github.com/bitsbakery/pretty

	pretty.h and pretty.cpp will not be supported. They exist
	as a temporary workaround until C++20 modules are well
	supported by GCC and cmake.

	pretty.ixx, pretty-impl, unicode.ixx constitute the implementation
	that is and will be supported.	
*/

#include "pretty.h"

namespace unicode
{
    int columnWidth(const std::string& u8str)
    {
        return mk_wcswidth(toUtf32(u8str).c_str(), u8str.size());
    }

    int columnWidth(const std::u32string& str)
    {
        return mk_wcswidth(str.c_str(), str.size());
    }
}

std::u32string toUtf32(const char * u8str)
{
    std::u32string out;
    out.reserve(std::strlen(u8str));
    unsigned int codepoint = 0;
    while (*u8str)
    {
        const unsigned char ch = *u8str;
        if (ch <= 0x7f)
            codepoint = ch;
        else if (ch <= 0xbf)
            codepoint = (codepoint << 6) | (ch & 0x3f);
        else if (ch <= 0xdf)
            codepoint = ch & 0x1f;
        else if (ch <= 0xef)
            codepoint = ch & 0x0f;
        else
            codepoint = ch & 0x07;
        ++u8str;
        if (((*u8str & 0xc0) != 0x80) && (codepoint <= 0x10ffff))
        {
            out.append(1, codepoint);
        }
    }
    return out;
}

std::u32string toUtf32(const std::string& u8str)
{
    return toUtf32(u8str.c_str());
}

/* auxiliary function for binary search in interval table */
int bisearch(char32_t ucs, const struct interval* table, int max)
{
    int min = 0;

    if (ucs < table[0].first || ucs > table[max].last)
        return 0;
    while (max >= min) {
        const int mid = (min + max) / 2;
        if (ucs > table[mid].last)
            min = mid + 1;
        else if (ucs < table[mid].first)
            max = mid - 1;
        else
            return 1;
    }

    return 0;
}

int mk_wcwidth(char32_t ucs)
{
    /* sorted list of non-overlapping intervals of non-spacing characters */
    /* generated by "uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B c" */
    static const struct interval combining[] =
    {
        { 0x0300, 0x036F }, { 0x0483, 0x0486 }, { 0x0488, 0x0489 },
        { 0x0591, 0x05BD }, { 0x05BF, 0x05BF }, { 0x05C1, 0x05C2 },
        { 0x05C4, 0x05C5 }, { 0x05C7, 0x05C7 }, { 0x0600, 0x0603 },
        { 0x0610, 0x0615 }, { 0x064B, 0x065E }, { 0x0670, 0x0670 },
        { 0x06D6, 0x06E4 }, { 0x06E7, 0x06E8 }, { 0x06EA, 0x06ED },
        { 0x070F, 0x070F }, { 0x0711, 0x0711 }, { 0x0730, 0x074A },
        { 0x07A6, 0x07B0 }, { 0x07EB, 0x07F3 }, { 0x0901, 0x0902 },
        { 0x093C, 0x093C }, { 0x0941, 0x0948 }, { 0x094D, 0x094D },
        { 0x0951, 0x0954 }, { 0x0962, 0x0963 }, { 0x0981, 0x0981 },
        { 0x09BC, 0x09BC }, { 0x09C1, 0x09C4 }, { 0x09CD, 0x09CD },
        { 0x09E2, 0x09E3 }, { 0x0A01, 0x0A02 }, { 0x0A3C, 0x0A3C },
        { 0x0A41, 0x0A42 }, { 0x0A47, 0x0A48 }, { 0x0A4B, 0x0A4D },
        { 0x0A70, 0x0A71 }, { 0x0A81, 0x0A82 }, { 0x0ABC, 0x0ABC },
        { 0x0AC1, 0x0AC5 }, { 0x0AC7, 0x0AC8 }, { 0x0ACD, 0x0ACD },
        { 0x0AE2, 0x0AE3 }, { 0x0B01, 0x0B01 }, { 0x0B3C, 0x0B3C },
        { 0x0B3F, 0x0B3F }, { 0x0B41, 0x0B43 }, { 0x0B4D, 0x0B4D },
        { 0x0B56, 0x0B56 }, { 0x0B82, 0x0B82 }, { 0x0BC0, 0x0BC0 },
        { 0x0BCD, 0x0BCD }, { 0x0C3E, 0x0C40 }, { 0x0C46, 0x0C48 },
        { 0x0C4A, 0x0C4D }, { 0x0C55, 0x0C56 }, { 0x0CBC, 0x0CBC },
        { 0x0CBF, 0x0CBF }, { 0x0CC6, 0x0CC6 }, { 0x0CCC, 0x0CCD },
        { 0x0CE2, 0x0CE3 }, { 0x0D41, 0x0D43 }, { 0x0D4D, 0x0D4D },
        { 0x0DCA, 0x0DCA }, { 0x0DD2, 0x0DD4 }, { 0x0DD6, 0x0DD6 },
        { 0x0E31, 0x0E31 }, { 0x0E34, 0x0E3A }, { 0x0E47, 0x0E4E },
        { 0x0EB1, 0x0EB1 }, { 0x0EB4, 0x0EB9 }, { 0x0EBB, 0x0EBC },
        { 0x0EC8, 0x0ECD }, { 0x0F18, 0x0F19 }, { 0x0F35, 0x0F35 },
        { 0x0F37, 0x0F37 }, { 0x0F39, 0x0F39 }, { 0x0F71, 0x0F7E },
        { 0x0F80, 0x0F84 }, { 0x0F86, 0x0F87 }, { 0x0F90, 0x0F97 },
        { 0x0F99, 0x0FBC }, { 0x0FC6, 0x0FC6 }, { 0x102D, 0x1030 },
        { 0x1032, 0x1032 }, { 0x1036, 0x1037 }, { 0x1039, 0x1039 },
        { 0x1058, 0x1059 }, { 0x1160, 0x11FF }, { 0x135F, 0x135F },
        { 0x1712, 0x1714 }, { 0x1732, 0x1734 }, { 0x1752, 0x1753 },
        { 0x1772, 0x1773 }, { 0x17B4, 0x17B5 }, { 0x17B7, 0x17BD },
        { 0x17C6, 0x17C6 }, { 0x17C9, 0x17D3 }, { 0x17DD, 0x17DD },
        { 0x180B, 0x180D }, { 0x18A9, 0x18A9 }, { 0x1920, 0x1922 },
        { 0x1927, 0x1928 }, { 0x1932, 0x1932 }, { 0x1939, 0x193B },
        { 0x1A17, 0x1A18 }, { 0x1B00, 0x1B03 }, { 0x1B34, 0x1B34 },
        { 0x1B36, 0x1B3A }, { 0x1B3C, 0x1B3C }, { 0x1B42, 0x1B42 },
        { 0x1B6B, 0x1B73 }, { 0x1DC0, 0x1DCA }, { 0x1DFE, 0x1DFF },
        { 0x200B, 0x200F }, { 0x202A, 0x202E }, { 0x2060, 0x2063 },
        { 0x206A, 0x206F }, { 0x20D0, 0x20EF }, { 0x302A, 0x302F },
        { 0x3099, 0x309A }, { 0xA806, 0xA806 }, { 0xA80B, 0xA80B },
        { 0xA825, 0xA826 }, { 0xFB1E, 0xFB1E }, { 0xFE00, 0xFE0F },
        { 0xFE20, 0xFE23 }, { 0xFEFF, 0xFEFF }, { 0xFFF9, 0xFFFB },
        { 0x10A01, 0x10A03 }, { 0x10A05, 0x10A06 }, { 0x10A0C, 0x10A0F },
        { 0x10A38, 0x10A3A }, { 0x10A3F, 0x10A3F }, { 0x1D167, 0x1D169 },
        { 0x1D173, 0x1D182 }, { 0x1D185, 0x1D18B }, { 0x1D1AA, 0x1D1AD },
        { 0x1D242, 0x1D244 }, { 0xE0001, 0xE0001 }, { 0xE0020, 0xE007F },
        { 0xE0100, 0xE01EF }
    };

    /* test for 8-bit control characters */
    if (ucs == 0)
        return 0;

    /* detect ansi escape sequence */
    if (ucs == 0x1b)
        return -1;

    /* detect zero-width-joiner (ZWJ) e.g. used to combine emojis like flags */
    if (ucs == 0x200D)
        return -2;

    if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
        return -10;

    /* binary search in table of non-spacing characters */
    if (bisearch(ucs, combining,
                 sizeof(combining) / sizeof(struct interval) - 1))
        return 0;

    /* if we arrive here, ucs is not a combining or C0/C1 control character */
    return 1 +
        (ucs >= 0x1100 &&
         (ucs <= 0x115f ||                    /* Hangul Jamo init. consonants */
          ucs == 0x2329 || ucs == 0x232a ||
          (ucs >= 0x2e80 && ucs <= 0xa4cf &&
           ucs != 0x303f) ||                  /* CJK ... Yi */
          (ucs >= 0xac00 && ucs <= 0xd7a3) || /* Hangul Syllables */
          (ucs >= 0xf900 && ucs <= 0xfaff) || /* CJK Compatibility Ideographs */
          (ucs >= 0xfe10 && ucs <= 0xfe19) || /* Vertical forms */
          (ucs >= 0xfe30 && ucs <= 0xfe6f) || /* CJK Compatibility Forms */
          (ucs >= 0xff00 && ucs <= 0xff60) || /* Fullwidth Forms */
          (ucs >= 0xffe0 && ucs <= 0xffe6) ||
          (ucs >= 0x20000 && ucs <= 0x2fffd) || // CJK
          (ucs >= 0x30000 && ucs <= 0x3fffd) ||
          // Miscellaneous Symbols and Pictographs + Emoticons:
          (ucs >= 0x1f300 && ucs <= 0x1f64f) ||
          // Supplemental Symbols and Pictographs:
          (ucs >= 0x1f900 && ucs <= 0x1f9ff)));
}

int mk_wcswidth(const char32_t* pwcs, size_t n)
{
    int w, width = 0;

    int last_code_point_width = 0;
    bool in_escape_sequence = false;
    for (; *pwcs && n-- > 0; pwcs++) {
        if ((w = mk_wcwidth(*pwcs)) < 0) {
            switch (w) {
            case -1: // detected ansi escape sequence
                in_escape_sequence = true;
                last_code_point_width = 0;
                continue;
            case -2: // detected zero-width-joiner
                if (last_code_point_width >= 0) {
                    width -= last_code_point_width;
                } else {
                    last_code_point_width = 0;
                }
                continue;
            default:
                return -1;
            }
        } else {
            if (in_escape_sequence) {
                if (isalpha(*pwcs)) { // N.B. usually an 'm'
                    in_escape_sequence = false;
                }
            } else {
                width += w;
                last_code_point_width = w;
            }
        }
    }

    return width;
}

/*
* The following functions are the same as mk_wcwidth() and
* mk_wcswidth(), except that spacing characters in the East Asian
* Ambiguous (A) category as defined in Unicode Technical Report #11
* have a column width of 2. This variant might be useful for users of
* CJK legacy encodings who want to migrate to UCS without changing
* the traditional terminal character-width behaviour. It is not
* otherwise recommended for general use.
*/
int mk_wcwidth_cjk(char32_t ucs)
{
    /* sorted list of non-overlapping intervals of East Asian Ambiguous
    * characters, generated by "uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c" */
    static const struct interval ambiguous[] =
    {
        { 0x00A1, 0x00A1 }, { 0x00A4, 0x00A4 }, { 0x00A7, 0x00A8 },
        { 0x00AA, 0x00AA }, { 0x00AE, 0x00AE }, { 0x00B0, 0x00B4 },
        { 0x00B6, 0x00BA }, { 0x00BC, 0x00BF }, { 0x00C6, 0x00C6 },
        { 0x00D0, 0x00D0 }, { 0x00D7, 0x00D8 }, { 0x00DE, 0x00E1 },
        { 0x00E6, 0x00E6 }, { 0x00E8, 0x00EA }, { 0x00EC, 0x00ED },
        { 0x00F0, 0x00F0 }, { 0x00F2, 0x00F3 }, { 0x00F7, 0x00FA },
        { 0x00FC, 0x00FC }, { 0x00FE, 0x00FE }, { 0x0101, 0x0101 },
        { 0x0111, 0x0111 }, { 0x0113, 0x0113 }, { 0x011B, 0x011B },
        { 0x0126, 0x0127 }, { 0x012B, 0x012B }, { 0x0131, 0x0133 },
        { 0x0138, 0x0138 }, { 0x013F, 0x0142 }, { 0x0144, 0x0144 },
        { 0x0148, 0x014B }, { 0x014D, 0x014D }, { 0x0152, 0x0153 },
        { 0x0166, 0x0167 }, { 0x016B, 0x016B }, { 0x01CE, 0x01CE },
        { 0x01D0, 0x01D0 }, { 0x01D2, 0x01D2 }, { 0x01D4, 0x01D4 },
        { 0x01D6, 0x01D6 }, { 0x01D8, 0x01D8 }, { 0x01DA, 0x01DA },
        { 0x01DC, 0x01DC }, { 0x0251, 0x0251 }, { 0x0261, 0x0261 },
        { 0x02C4, 0x02C4 }, { 0x02C7, 0x02C7 }, { 0x02C9, 0x02CB },
        { 0x02CD, 0x02CD }, { 0x02D0, 0x02D0 }, { 0x02D8, 0x02DB },
        { 0x02DD, 0x02DD }, { 0x02DF, 0x02DF }, { 0x0391, 0x03A1 },
        { 0x03A3, 0x03A9 }, { 0x03B1, 0x03C1 }, { 0x03C3, 0x03C9 },
        { 0x0401, 0x0401 }, { 0x0410, 0x044F }, { 0x0451, 0x0451 },
        { 0x2010, 0x2010 }, { 0x2013, 0x2016 }, { 0x2018, 0x2019 },
        { 0x201C, 0x201D }, { 0x2020, 0x2022 }, { 0x2024, 0x2027 },
        { 0x2030, 0x2030 }, { 0x2032, 0x2033 }, { 0x2035, 0x2035 },
        { 0x203B, 0x203B }, { 0x203E, 0x203E }, { 0x2074, 0x2074 },
        { 0x207F, 0x207F }, { 0x2081, 0x2084 }, { 0x20AC, 0x20AC },
        { 0x2103, 0x2103 }, { 0x2105, 0x2105 }, { 0x2109, 0x2109 },
        { 0x2113, 0x2113 }, { 0x2116, 0x2116 }, { 0x2121, 0x2122 },
        { 0x2126, 0x2126 }, { 0x212B, 0x212B }, { 0x2153, 0x2154 },
        { 0x215B, 0x215E }, { 0x2160, 0x216B }, { 0x2170, 0x2179 },
        { 0x2190, 0x2199 }, { 0x21B8, 0x21B9 }, { 0x21D2, 0x21D2 },
        { 0x21D4, 0x21D4 }, { 0x21E7, 0x21E7 }, { 0x2200, 0x2200 },
        { 0x2202, 0x2203 }, { 0x2207, 0x2208 }, { 0x220B, 0x220B },
        { 0x220F, 0x220F }, { 0x2211, 0x2211 }, { 0x2215, 0x2215 },
        { 0x221A, 0x221A }, { 0x221D, 0x2220 }, { 0x2223, 0x2223 },
        { 0x2225, 0x2225 }, { 0x2227, 0x222C }, { 0x222E, 0x222E },
        { 0x2234, 0x2237 }, { 0x223C, 0x223D }, { 0x2248, 0x2248 },
        { 0x224C, 0x224C }, { 0x2252, 0x2252 }, { 0x2260, 0x2261 },
        { 0x2264, 0x2267 }, { 0x226A, 0x226B }, { 0x226E, 0x226F },
        { 0x2282, 0x2283 }, { 0x2286, 0x2287 }, { 0x2295, 0x2295 },
        { 0x2299, 0x2299 }, { 0x22A5, 0x22A5 }, { 0x22BF, 0x22BF },
        { 0x2312, 0x2312 }, { 0x2460, 0x24E9 }, { 0x24EB, 0x254B },
        { 0x2550, 0x2573 }, { 0x2580, 0x258F }, { 0x2592, 0x2595 },
        { 0x25A0, 0x25A1 }, { 0x25A3, 0x25A9 }, { 0x25B2, 0x25B3 },
        { 0x25B6, 0x25B7 }, { 0x25BC, 0x25BD }, { 0x25C0, 0x25C1 },
        { 0x25C6, 0x25C8 }, { 0x25CB, 0x25CB }, { 0x25CE, 0x25D1 },
        { 0x25E2, 0x25E5 }, { 0x25EF, 0x25EF }, { 0x2605, 0x2606 },
        { 0x2609, 0x2609 }, { 0x260E, 0x260F }, { 0x2614, 0x2615 },
        { 0x261C, 0x261C }, { 0x261E, 0x261E }, { 0x2640, 0x2640 },
        { 0x2642, 0x2642 }, { 0x2660, 0x2661 }, { 0x2663, 0x2665 },
        { 0x2667, 0x266A }, { 0x266C, 0x266D }, { 0x266F, 0x266F },
        { 0x273D, 0x273D }, { 0x2776, 0x277F }, { 0xE000, 0xF8FF },
        { 0xFFFD, 0xFFFD }, { 0xF0000, 0xFFFFD }, { 0x100000, 0x10FFFD }
    };

    /* binary search in table of non-spacing characters */
    if (bisearch(ucs, ambiguous,
                 sizeof(ambiguous) / sizeof(struct interval) - 1))
        return 2;

    return mk_wcwidth(ucs);
}

int mk_wcswidth_cjk(const char32_t* pwcs, size_t n)
{
    int w, width = 0;

    for (; *pwcs && n-- > 0; pwcs++)
        if ((w = mk_wcwidth_cjk(*pwcs)) < 0)
            return -1;
        else
            width += w;

    return width;
}

namespace pretty
{
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

    Printer& Printer::print(std::ostream& os) const
    {
        if (_data == nullptr)
            throw std::logic_error("Printer: no data loaded");
        _impl.print(_data->_impl,os);
        return const_cast<Printer&>(*this);
    }

    std::ostream& operator<<(std::ostream& os, const Printer& p)
    {
        p.print(os);
        return os;
    }
}