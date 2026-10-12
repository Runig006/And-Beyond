// WinHelp 4 .HLP to one HTML page (see hlp.h). Text is cp1252, stored here as UTF-8.
#include "hlp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

namespace {

typedef std::vector<uint8_t> Bytes;

const char kNbsp[] = "\xC2\xA0";  // U+00A0 in UTF-8

struct HlpError {};

[[noreturn]] void Fail()
{
	throw HlpError();
}

// ---- index and number rules the format needs ----

// a[i]: a negative index counts from the end
size_t PyIndex(int64_t i, size_t len)
{
	if (i < 0) {
		i += (int64_t)len;
	}
	if (i < 0 || i >= (int64_t)len) {
		Fail();
	}
	return (size_t)i;
}

// a[start:end]
Bytes Slice(const Bytes &b, int64_t start, int64_t end)
{
	int64_t n = (int64_t)b.size();
	start = start < 0 ? std::max<int64_t>(start + n, 0) : std::min(start, n);
	end = end < 0 ? std::max<int64_t>(end + n, 0) : std::min(end, n);
	if (end <= start) {
		return Bytes();
	}
	return Bytes(b.begin() + (ptrdiff_t)start, b.begin() + (ptrdiff_t)end);
}

// struct.unpack_from's offset: a negative one counts from the end, and the value must fit
size_t At(const Bytes &b, int64_t off, size_t size)
{
	int64_t n = (int64_t)b.size();
	if (off < 0) {
		if (off + n < 0) {
			Fail();
		}
		off += n;
	}
	if (n - off < (int64_t)size) {
		Fail();
	}
	return (size_t)off;
}

int U16At(const Bytes &b, int64_t off)
{
	size_t p = At(b, off, 2);
	return b[p] | b[p + 1] << 8;
}

int I16At(const Bytes &b, int64_t off)
{
	return (int16_t)U16At(b, off);
}

uint32_t U32At(const Bytes &b, int64_t off)
{
	size_t p = At(b, off, 4);
	return (uint32_t)b[p] | (uint32_t)b[p + 1] << 8 | (uint32_t)b[p + 2] << 16 | (uint32_t)b[p + 3] << 24;
}

int32_t I32At(const Bytes &b, int64_t off)
{
	return (int32_t)U32At(b, off);
}

int64_t FloorDiv(int64_t a, int64_t b)
{
	if (b == 0) {
		Fail();
	}
	int64_t q = a / b;
	if ((a % b != 0) && ((a < 0) != (b < 0))) {
		q--;
	}
	return q;
}

int64_t FloorMod(int64_t a, int64_t b)
{
	return a - FloorDiv(a, b) * b;
}

// b.split(b"\0")
std::vector<Bytes> SplitNul(const Bytes &b)
{
	std::vector<Bytes> out(1);
	for (uint8_t c : b) {
		if (c == 0) {
			out.emplace_back();
		} else {
			out.back().push_back(c);
		}
	}
	return out;
}

// b.split(b"\0")[0]
Bytes FirstPart(const Bytes &b)
{
	return Bytes(b.begin(), std::find(b.begin(), b.end(), (uint8_t)0));
}

void PutUtf8(std::string &s, uint32_t cp)
{
	if (cp < 0x80) {
		s += (char)cp;
	} else if (cp < 0x800) {
		s += (char)(0xC0 | cp >> 6);
		s += (char)(0x80 | (cp & 0x3F));
	} else {
		s += (char)(0xE0 | cp >> 12);
		s += (char)(0x80 | (cp >> 6 & 0x3F));
		s += (char)(0x80 | (cp & 0x3F));
	}
}

// Code point at s[i] (UTF-8 made by this file, so valid); n = its length in bytes
uint32_t GetUtf8(const std::string &s, size_t i, size_t &n)
{
	uint8_t c = (uint8_t)s[i];
	if (c < 0x80) {
		n = 1;
		return c;
	}
	if (c < 0xE0) {
		n = 2;
		return (c & 0x1F) << 6 | ((uint8_t)s[i + 1] & 0x3F);
	}
	n = 3;
	return (c & 0x0F) << 12 | ((uint8_t)s[i + 1] & 0x3F) << 6 | ((uint8_t)s[i + 2] & 0x3F);
}

std::string Latin1(const Bytes &b)
{
	std::string s;
	for (uint8_t c : b) {
		PutUtf8(s, c);
	}
	return s;
}

// bytes.decode("cp1252"): 0x81, 0x8D, 0x8F, 0x90 and 0x9D are an error
std::string Cp1252(const Bytes &b)
{
	static const uint16_t high[32] = {
		0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
		0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E,
		0x0178};
	std::string s;
	for (uint8_t c : b) {
		uint32_t cp = c;
		if (c >= 0x80 && c < 0xA0) {
			cp = high[c - 0x80];
			if (cp == 0) {
				Fail();
			}
		}
		PutUtf8(s, cp);
	}
	return s;
}

// str.isspace()
bool PySpace(uint32_t cp)
{
	return (cp >= 9 && cp <= 13) || (cp >= 0x1C && cp <= 0x20) || cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
		(cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 || cp == 0x2029 || cp == 0x202F || cp == 0x205F ||
		cp == 0x3000;
}

// str.strip()
std::string PyStrip(const std::string &s)
{
	size_t a = 0, b = s.size(), n;
	while (a < b && PySpace(GetUtf8(s, a, n))) {
		a += n;
	}
	while (b > a) {
		size_t start = b - 1;
		while (start > a && ((uint8_t)s[start] & 0xC0) == 0x80) {
			start--;
		}
		if (!PySpace(GetUtf8(s, start, n))) {
			break;
		}
		b = start;
	}
	return s.substr(a, b - a);
}

// int() of a latin-1 string: spaces around, a sign, digits with single "_" between them. Numbers that cannot name
// a picture (outside 0-65535) come back as -1.
int64_t PyInt(const std::string &s)
{
	bool ascii = std::all_of(s.begin(), s.end(), [](char c) { return (uint8_t)c < 0x80; });
	auto space = [&](uint8_t c) {
		return c == ' ' || (c >= 9 && c <= 13) || (!ascii && (c == 0x85 || c == 0xA0));
	};
	size_t a = 0, b = s.size();
	while (a < b && space((uint8_t)s[a])) {
		a++;
	}
	while (b > a && space((uint8_t)s[b - 1])) {
		b--;
	}
	bool neg = false;
	if (a < b && (s[a] == '+' || s[a] == '-')) {
		neg = s[a] == '-';
		a++;
	}
	int64_t v = 0;
	bool digit = false;
	for (size_t i = a; i < b; i++) {
		if (s[i] >= '0' && s[i] <= '9') {
			v = std::min<int64_t>(v * 10 + (s[i] - '0'), 1 << 20);
			digit = true;
		} else if (s[i] == '_' && digit) {
			digit = false;
		} else {
			Fail();
		}
	}
	if (!digit) {
		Fail();
	}
	if (neg) {
		v = -v;
	}
	return v >= 0 && v <= 0xFFFF ? v : -1;
}

// html.escape(s)
std::string Escape(const std::string &s)
{
	std::string out;
	for (char c : s) {
		switch (c) {
		case '&':
			out += "&amp;";
			break;
		case '<':
			out += "&lt;";
			break;
		case '>':
			out += "&gt;";
			break;
		case '"':
			out += "&quot;";
			break;
		case '\'':
			out += "&#x27;";
			break;
		default:
			out += c;
		}
	}
	return out;
}

std::string Base64(const Bytes &b)
{
	static const char abc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve((b.size() + 2) / 3 * 4);
	size_t i = 0;
	for (; i + 3 <= b.size(); i += 3) {
		uint32_t v = b[i] << 16 | b[i + 1] << 8 | b[i + 2];
		out += abc[v >> 18];
		out += abc[v >> 12 & 63];
		out += abc[v >> 6 & 63];
		out += abc[v & 63];
	}
	if (i + 1 == b.size()) {
		uint32_t v = b[i] << 16;
		out += abc[v >> 18];
		out += abc[v >> 12 & 63];
		out += "==";
	} else if (i + 2 == b.size()) {
		uint32_t v = b[i] << 16 | b[i + 1] << 8;
		out += abc[v >> 18];
		out += abc[v >> 12 & 63];
		out += abc[v >> 6 & 63];
		out += '=';
	}
	return out;
}

// "%g" (the values here are whole numbers or halves)
std::string FmtG(double v)
{
	char buf[64];
	snprintf(buf, sizeof buf, "%g", v);
	return buf;
}

// "%.1f": the exact value of the double rounded half to even
std::string Fixed1(double d)
{
	bool neg = std::signbit(d);
	int exp;
	double m = std::frexp(std::fabs(d), &exp);
	uint64_t mant = (uint64_t)std::ldexp(m, 53);
	int shift = 53 - exp;  // |d| = mant / 2^shift
	if (shift <= 0) {
		char buf[400];
		snprintf(buf, sizeof buf, "%.1f", d);  // a whole number: printed exactly
		return buf;
	}
	uint64_t tenths = 0;
	if (shift < 59) {
		uint64_t num = mant * 10;
		tenths = num >> shift;
		uint64_t rem = num & ((1ull << shift) - 1), half = 1ull << (shift - 1);
		if (rem > half || (rem == half && (tenths & 1))) {
			tenths++;
		}
	}
	return (neg ? "-" : "") + std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
}

void PutU16(Bytes &b, int64_t v)
{
	if (v < 0 || v > 0xFFFF) {
		Fail();
	}
	b.push_back((uint8_t)v);
	b.push_back((uint8_t)(v >> 8));
}

void PutU32(Bytes &b, int64_t v)
{
	if (v < 0 || v > 0xFFFFFFFFll) {
		Fail();
	}
	for (int i = 0; i < 4; i++) {
		b.push_back((uint8_t)(v >> (8 * i)));
	}
}

void PutI32(Bytes &b, int64_t v)
{
	if (v < INT32_MIN || v > INT32_MAX) {
		Fail();
	}
	PutU32(b, v & 0xFFFFFFFF);
}

// ---- the converter ----

struct Reader {
	const Bytes &b;
	int64_t p;

	Reader(const Bytes &data, int64_t pos = 0) : b(data), p(pos) {}

	int U8()
	{
		int v = b[PyIndex(p, b.size())];
		p++;
		return v;
	}

	int U16()
	{
		int v = U16At(b, p);
		p += 2;
		return v;
	}

	int I16()
	{
		int v = I16At(b, p);
		p += 2;
		return v;
	}

	int32_t I32()
	{
		int32_t v = I32At(b, p);
		p += 4;
		return v;
	}

	uint32_t U32()
	{
		uint32_t v = U32At(b, p);
		p += 4;
		return v;
	}

	Bytes Cstr()
	{
		int64_t n = (int64_t)b.size();
		int64_t e = p < 0 ? std::max<int64_t>(p + n, 0) : p;
		while (e < n && b[(size_t)e] != 0) {
			e++;
		}
		if (e >= n) {
			Fail();
		}
		Bytes s = Slice(b, p, e);
		p = e + 1;
		return s;
	}

	int Cshort()
	{
		if (b[PyIndex(p, b.size())] & 1) {
			return U16() >> 1;
		}
		return U8() >> 1;
	}

	int64_t Clong()
	{
		if (b[PyIndex(p, b.size())] & 1) {
			return U32() >> 1;
		}
		return U16() >> 1;
	}

	int Sshort()
	{
		if (b[PyIndex(p, b.size())] & 1) {
			return (U16() >> 1) - 0x4000;
		}
		return (U8() >> 1) - 0x40;
	}

	int64_t Slong()
	{
		if (b[PyIndex(p, b.size())] & 1) {
			return (int64_t)(U32() >> 1) - 0x40000000;
		}
		return (U16() >> 1) - 0x4000;
	}
};

// Entries of the leaf pages of a B-tree; entry(Reader &) reads one
template <class F> void Btree(const Bytes &b, F entry)
{
	int64_t pagesize = U16At(b, 4), root = I16At(b, 26), levels = I16At(b, 32);
	Bytes pages = Slice(b, 38, (int64_t)b.size());
	int64_t page = root;
	for (int64_t i = 0; i < levels - 1; i++) {
		page = I16At(pages, page * pagesize + 4);
	}
	std::set<int64_t> seen;
	while (page != -1) {
		if (!seen.insert(page).second) {
			Fail();  // pages in a loop: it would never end
		}
		Reader r(pages, page * pagesize + 2);
		int n = r.I16();
		r.I16();
		int nxt = r.I16();
		for (int i = 0; i < n; i++) {
			entry(r);
		}
		page = nxt;
	}
}

struct Hlp {
	const Bytes &d;
	std::vector<std::pair<std::string, int32_t>> files;  // name (latin-1 bytes) -> offset, in dict order

	explicit Hlp(const Bytes &data) : d(data)
	{
		uint32_t magic = U32At(d, 0);
		int32_t dirstart = I32At(d, 4);
		if (magic != 0x35F3F) {
			Fail();
		}
		std::map<std::string, size_t> where;
		Bytes dir = File(dirstart);
		Btree(dir, [&](Reader &r) {
			Bytes raw = r.Cstr();
			std::string name(raw.begin(), raw.end());
			int32_t off = r.I32();
			auto it = where.find(name);
			if (it != where.end()) {
				files[it->second].second = off;
			} else {
				where[name] = files.size();
				files.push_back({name, off});
			}
		});
	}

	Bytes File(int64_t off) const
	{
		int32_t used = I32At(d, (int64_t)At(d, off, 9) + 4);
		return Slice(d, off + 9, off + 9 + used);
	}

	bool Has(const std::string &name) const
	{
		for (const auto &f : files) {
			if (f.first == name) {
				return true;
			}
		}
		return false;
	}

	Bytes Internal(const std::string &name) const
	{
		for (const auto &f : files) {
			if (f.first == name) {
				return File(f.second);
			}
		}
		Fail();
	}

	std::string Title() const
	{
		Bytes s = Internal("|SYSTEM");
		int64_t r = 12;
		while (r + 4 <= (int64_t)s.size()) {
			int rtype = U16At(s, r), size = U16At(s, r + 2);
			if (rtype == 1) {
				return Cp1252(FirstPart(Slice(s, r + 4, r + 4 + size)));
			}
			r += 4 + size;
		}
		return "Help";
	}

	// Link hash -> title of the topic
	std::map<uint32_t, std::string> LinkTitles() const
	{
		std::vector<std::pair<int32_t, std::string>> titles;
		Bytes ttl = Internal("|TTLBTREE");
		Btree(ttl, [&](Reader &r) {
			int32_t pos = r.I32();
			titles.push_back({pos, Cp1252(r.Cstr())});
		});
		std::sort(titles.begin(), titles.end());
		std::map<uint32_t, std::string> out;
		Bytes ctx = Internal("|CONTEXT");
		Btree(ctx, [&](Reader &r) {
			uint32_t h = r.U32();
			int32_t pos = r.I32();
			auto it = std::partition_point(titles.begin(), titles.end(),
				[&](const std::pair<int32_t, std::string> &t) { return t.first <= pos; });
			if (it != titles.begin()) {
				out[h] = (it - 1)->second;
			}
		});
		return out;
	}
};

enum ChunkKind { kText, kFont, kImg, kBr, kTab, kLinkTopic, kLinkMacro, kEnd };

struct Chunk {
	ChunkKind kind;
	std::string text;     // kText; kLinkMacro: the macro
	uint32_t n = 0;       // kFont: font; kImg: picture; kLinkTopic: hash
	const char *align = "";  // kImg: inline, left or right
};

struct Fmt {
	int above = 0, below = 0, lines = 0, left = 0, right = 0, first = 0;
	std::vector<int> tabs;
	const char *align = "left";
};

typedef std::vector<Chunk> Chunks;
typedef std::vector<std::pair<Fmt, Chunks>> Cell;
typedef std::vector<Cell> Row;
typedef std::vector<std::pair<int, int>> Widths;

struct Item {
	bool table = false;
	Fmt fmt;          // paragraph
	Chunks chunks;    // paragraph
	Widths widths;    // table
	std::vector<Row> rows;  // table
};

Chunk MakeChunk(ChunkKind kind, const std::string &text = "", uint32_t n = 0)
{
	Chunk c;
	c.kind = kind;
	c.text = text;
	c.n = n;
	return c;
}

// Content of a record: paragraphs, or one table with one row
std::vector<Item> Paragraphs(const Bytes &link1, const Bytes &link2, int rtype)
{
	Reader r(link1);
	const int64_t len = (int64_t)link1.size();
	const bool table = rtype == 0x23;
	r.Slong();   // size of the topic
	r.Cshort();  // length of the text
	Widths widths;
	if (table) {
		int cols = r.U8();
		int kind = r.U8();
		if (kind == 0 || kind == 2) {
			r.U16();
		}
		for (int i = 0; i < cols; i++) {
			int w = r.I16();
			int gap = r.I16();
			widths.push_back({w, gap});
		}
	}
	std::vector<Bytes> texts = SplitNul(link2);
	size_t ti = 0;
	std::vector<Item> out;
	Row row;
	Chunks cur;
	int64_t steps = 0;
	while (r.p < len) {
		int64_t col = 0;
		if (table) {
			// One byte more between the table header and the first cell
			if (r.p + 3 < len && Slice(link1, r.p, r.p + 2) != Bytes{0xFF, 0xFF} &&
				link1[PyIndex(r.p + 2, link1.size())] != 1 && link1[PyIndex(r.p + 3, link1.size())] == 1) {
				r.p++;
			}
			col = r.I16();
			if (col == -1) {
				break;
			}
			while ((int64_t)row.size() <= col) {
				row.emplace_back();
			}
			r.U8();
			r.Slong();
		}
		r.U8();
		r.U8();
		r.U16();
		int bits = r.U16();
		Fmt fmt;
		if (bits & 0x0001) {
			r.Clong();
		}
		int *keys[6] = {&fmt.above, &fmt.below, &fmt.lines, &fmt.left, &fmt.right, &fmt.first};
		for (int i = 0; i < 6; i++) {
			if (bits & (2 << i)) {
				*keys[i] = r.Sshort();
			}
		}
		if (bits & 0x0100) {  // border
			r.U8();
			r.U16();
		}
		if (bits & 0x0200) {
			int count = r.Sshort();
			for (int i = 0; i < count; i++) {
				int stop = r.Cshort();
				if (stop & 0x4000) {
					r.Cshort();
				}
				fmt.tabs.push_back(stop & 0x3FFF);
			}
		}
		fmt.align = bits & 0x0400 ? "right" : bits & 0x0800 ? "center" : "left";
		Cell cell;
		auto flush = [&]() {
			if (table) {
				cell.push_back({fmt, cur});
			} else {
				Item item;
				item.fmt = fmt;
				item.chunks = cur;
				out.push_back(item);
			}
			cur.clear();
		};
		for (;;) {
			if (ti < texts.size() && !texts[ti].empty()) {
				cur.push_back(MakeChunk(kText, Cp1252(texts[ti])));
			}
			ti++;
			if (r.p >= len) {
				break;
			}
			if (++steps > 16 * len + 1024) {
				Fail();  // pictures jumping back in a loop: it would never end
			}
			int c = r.U8();
			if (c == 0xFF) {
				break;
			}
			if (c == 0x20) {
				r.I32();
			} else if (c == 0x21) {
				r.U16();
			} else if (c == 0x80) {
				cur.push_back(MakeChunk(kFont, "", r.U16()));
			} else if (c == 0x81) {
				cur.push_back(MakeChunk(kBr));
			} else if (c == 0x82) {
				flush();
			} else if (c == 0x83) {
				cur.push_back(MakeChunk(kTab));
			} else if (c == 0x86 || c == 0x87 || c == 0x88) {
				int ptype = r.U8();
				int64_t size = r.Slong();
				if (ptype == 0x22) {
					r.Cshort();
				}
				int64_t start = r.p;
				if (ptype == 0x03 || ptype == 0x22) {
					r.U16();  // 0: picture of a |bmN file
					Chunk img = MakeChunk(kImg, "", r.U16());
					img.align = c == 0x86 ? "inline" : c == 0x87 ? "left" : "right";
					cur.push_back(img);
				}
				r.p = start + size;
			} else if (c == 0x89) {
				cur.push_back(MakeChunk(kEnd));
			} else if (c == 0x8B) {
				cur.push_back(MakeChunk(kText, kNbsp));
			} else if (c == 0x8C) {
				cur.push_back(MakeChunk(kText, "-"));
			} else if (c == 0xC8 || c == 0xCC) {
				int n = r.U16();
				cur.push_back(MakeChunk(kLinkMacro, Cp1252(FirstPart(Slice(link1, r.p, r.p + n)))));
				r.p += n;
			} else if (c >= 0xE0 && c <= 0xE7) {
				cur.push_back(MakeChunk(kLinkTopic, "", r.U32()));
			} else if (c >= 0xEA && c <= 0xEF) {  // u16 size, u8 type, u32 hash, then window or file names
				int n = r.U16();
				cur.push_back(MakeChunk(kLinkTopic, "", U32At(link1, r.p + 1)));
				r.p += n;
			} else {
				Fail();  // unknown formatting command
			}
		}
		if (!cur.empty()) {
			flush();
		}
		if (table) {
			Cell &dst = row[PyIndex(col, row.size())];
			dst.insert(dst.end(), cell.begin(), cell.end());
		} else {
			break;
		}
	}
	if (table) {
		Item item;
		item.table = true;
		item.widths = widths;
		item.rows.push_back(row);
		out.push_back(item);
	}
	return out;
}

struct Topic {
	std::string title;
	std::vector<Item> items;
};

Bytes Lz77(const Bytes &b);

// |SYSTEM flag 4: each |TOPIC block is LZ77-compressed (the Portuguese file)
bool Compressed(const Hlp &hlp)
{
	return (U16At(hlp.Internal("|SYSTEM"), 10) & 4) != 0;
}

// Phrases of the "Hall" compression (|PhrIndex, |PhrImage); empty if the file has none
std::vector<Bytes> Phrases(const Hlp &hlp)
{
	std::vector<Bytes> out;
	if (!hlp.Has("|PhrIndex")) {
		return out;
	}
	Bytes index = hlp.Internal("|PhrIndex"), image = hlp.Internal("|PhrImage");
	int32_t count = I32At(index, 4), size = I32At(index, 12), packed = I32At(index, 16);
	int bits = U16At(index, 24) & 15;
	if (size != packed) {
		image = Slice(Lz77(image), 0, size);
	}
	Bytes stream = Slice(index, 28, (int64_t)index.size());
	int64_t at = 0;
	// Phrase lengths: bits from 32-bit words, low bit first
	auto bit = [&]() {
		uint32_t word = U32At(stream, (at >> 5) * 4);
		int b = (word >> (at & 31)) & 1;
		at++;
		return b;
	};
	std::vector<int64_t> offsets = {0};
	for (int32_t i = 0; i < count; i++) {
		int64_t n = 1;
		while (bit()) {
			n += (int64_t)1 << bits;
		}
		for (int k = 0; k < bits; k++) {
			if (bit()) {
				n += (int64_t)1 << k;
			}
		}
		offsets.push_back(offsets.back() + n);
	}
	for (int32_t i = 0; i < count; i++) {
		out.push_back(Slice(image, offsets[i], offsets[i + 1]));
	}
	return out;
}

// The text of a record with phrase compression
Bytes Unphrase(const Bytes &b, const std::vector<Bytes> &phrases)
{
	Bytes out;
	size_t i = 0;
	auto phrase = [&](size_t n) {
		if (n >= phrases.size()) {
			Fail();
		}
		out.insert(out.end(), phrases[n].begin(), phrases[n].end());
	};
	while (i < b.size()) {
		int c = b[i++];
		if ((c & 1) == 0) {
			phrase(c >> 1);
		} else if ((c & 3) == 1) {
			if (i >= b.size()) {
				Fail();
			}
			phrase((size_t)(c >> 2) * 256 + b[i++] + 128);
		} else if ((c & 7) == 3) {
			size_t n = (c >> 3) + 1, end = std::min(i + n, b.size());
			out.insert(out.end(), b.begin() + i, b.begin() + end);
			i = end;
		} else if ((c & 15) == 7) {
			out.insert(out.end(), (size_t)(c >> 4) + 1, ' ');
		} else {
			out.insert(out.end(), (size_t)(c >> 4) + 1, 0);
		}
	}
	return out;
}

// Topics in order; the rows of a table come in records one after another: they are joined in one
std::vector<Topic> Topics(const Hlp &hlp)
{
	const int64_t block = 4096;
	bool compressed = Compressed(hlp);
	const int64_t blockData = compressed ? 0x4000 - 12 : block - 12;
	Bytes t = hlp.Internal("|TOPIC");
	Bytes data;
	for (int64_t i = 0; i < (int64_t)t.size(); i += block) {
		Bytes part = Slice(t, i + 12, i + block);
		if (compressed) {
			part = Lz77(part);
			part.resize((size_t)blockData, 0);
		}
		data.insert(data.end(), part.begin(), part.end());
	}
	auto idx = [&](int64_t pos) {
		return FloorDiv(pos, 0x4000) * blockData + FloorMod(pos, 0x4000) - 12;
	};
	std::vector<Bytes> phrases = Phrases(hlp);
	int64_t pos = I32At(t, 4);  // first record of block 0
	std::vector<Topic> result;
	std::set<int64_t> seen;
	while (pos != -1 && !seen.count(pos)) {
		seen.insert(pos);
		int64_t p = idx(pos);
		if (p < 0 || p + 21 > (int64_t)data.size()) {
			break;
		}
		int32_t size = I32At(data, p), len2 = I32At(data, p + 4), nxt = I32At(data, p + 12), len1 = I32At(data, p + 16);
		int rtype = data[(size_t)p + 20];
		Bytes link1 = Slice(data, p + 21, p + len1), link2 = Slice(data, p + len1, p + size);
		if (!phrases.empty() && len2 > size - len1) {
			link2 = Unphrase(link2, phrases);
		}
		if (rtype == 2) {
			Topic topic;
			topic.title = Cp1252(FirstPart(link2));
			result.push_back(topic);
		} else if ((rtype == 0x20 || rtype == 0x23) && !result.empty()) {
			std::vector<Item> &items = result.back().items;
			for (Item &item : Paragraphs(link1, link2, rtype)) {
				if (item.table && !items.empty() && items.back().table && items.back().widths == item.widths) {
					items.back().rows.insert(items.back().rows.end(), item.rows.begin(), item.rows.end());
				} else {
					items.push_back(item);
				}
			}
		}
		pos = nxt;
	}
	return result;
}

Bytes Lz77(const Bytes &b)
{
	Bytes out;
	size_t i = 0, n = b.size();
	while (i < n) {
		int bits = b[i++];
		for (int k = 0; k < 8; k++) {
			if (i >= n) {
				break;
			}
			if (bits & (1 << k)) {
				if (i + 1 >= n) {
					Fail();
				}
				int v = b[i] | b[i + 1] << 8;
				i += 2;
				size_t length = (v >> 12) + 3, dist = (v & 0xFFF) + 1;
				for (size_t j = 0; j < length; j++) {
					uint8_t c = dist <= out.size() ? out[out.size() - dist] : 0;
					out.push_back(c);
				}
			} else {
				out.push_back(b[i++]);
			}
		}
	}
	return out;
}

Bytes Rle(const Bytes &b)
{
	Bytes out;
	size_t i = 0, n = b.size();
	while (i < n) {
		int c = b[i++];
		if (c & 0x80) {
			size_t k = c & 0x7F;
			out.insert(out.end(), b.begin() + i, b.begin() + std::min(i + k, n));
			i += k;
		} else {
			if (i >= n) {
				Fail();
			}
			out.insert(out.end(), (size_t)c, b[i]);
			i++;
		}
	}
	return out;
}

// BMP file of a DIB (BITMAPINFOHEADER, palette and pixels one after another)
Bytes Bmp(const Bytes &info, int64_t colors)
{
	int64_t hsize = U32At(info, 0);
	Bytes out = {'B', 'M'};
	PutU32(out, 14 + (int64_t)info.size());
	PutU16(out, 0);
	PutU16(out, 0);
	PutU32(out, 14 + hsize + colors * 4);
	out.insert(out.end(), info.begin(), info.end());
	return out;
}

// The largest bitmap of a Windows metafile (META_STRETCHDIB 0x0F43 or META_DIBSTRETCHBLT 0x0B41)
bool MetafileBitmap(const Bytes &wmf, Bytes &result)
{
	int64_t p = (int64_t)U16At(wmf, 2) * 2, bestStart = 0, bestLen = -1;
	while (p + 6 <= (int64_t)wmf.size()) {
		int64_t size = U32At(wmf, p);
		int func = U16At(wmf, p + 4);
		if (size < 3) {
			break;
		}
		// params = wmf[p + 6:p + size * 2]; the DIB starts 22 or 20 bytes into them
		int64_t end = std::min<int64_t>(p + size * 2, (int64_t)wmf.size());
		int64_t skip = func == 0x0F43 ? 22 : func == 0x0B41 ? 20 : -1;
		if (skip >= 0) {
			int64_t start = p + 6 + skip, dlen = end - start;
			if (dlen > 0 && (bestLen < 0 || dlen > bestLen)) {
				bestStart = start;
				bestLen = dlen;
			}
		}
		p += size * 2;
	}
	if (bestLen <= 0) {
		return false;
	}
	Bytes best(wmf.begin() + (ptrdiff_t)bestStart, wmf.begin() + (ptrdiff_t)(bestStart + bestLen));
	int bitcount = U16At(best, 14);
	int64_t used = U32At(best, 32);
	result = Bmp(best, used ? used : (bitcount <= 8 ? 1 << bitcount : 0));
	return true;
}

Bytes Unpack(Bytes raw, int packing)
{
	if (packing == 2 || packing == 3) {
		raw = Lz77(raw);
	}
	if (packing == 1 || packing == 3) {
		raw = Rle(raw);
	}
	return raw;
}

// The first picture of a |bmN file as BMP; false if it is not a bitmap
bool Bitmap(const Bytes &b, Bytes &result)
{
	int magic = U16At(b, 0), count = U16At(b, 2);
	if ((magic != 0x506C && magic != 0x706C) || !count) {
		return false;
	}
	int64_t start = U32At(b, 4);
	Reader r(b, start);
	int ptype = r.U8(), packing = r.U8();
	if (ptype == 8) {  // metafile
		r.Cshort();  // mapping mode, width, height
		r.U16();
		r.U16();
		r.Clong();
		int64_t csize = r.Clong();
		r.Clong();
		int64_t coff = U32At(b, r.p);
		return MetafileBitmap(Unpack(Slice(b, start + coff, start + coff + csize), packing), result);
	}
	if (ptype != 6) {
		return false;
	}
	int64_t xdpi = r.Clong(), ydpi = r.Clong();
	int planes = r.Cshort(), bitcount = r.Cshort();
	int64_t width = r.Clong(), height = r.Clong();
	int64_t colors = r.Clong();
	r.Clong();
	int64_t csize = r.Clong();
	r.Clong();
	int64_t coff = U32At(b, r.p);
	r.p += 8;
	int64_t ncolors = colors ? colors : (bitcount <= 8 ? 1 << bitcount : 0);
	Bytes palette = Slice(b, r.p, r.p + ncolors * 4);
	Bytes pixels = Unpack(Slice(b, start + coff, start + coff + csize), packing);
	Bytes info;
	PutU32(info, 40);
	PutI32(info, width);
	PutI32(info, height);
	PutU16(info, planes ? planes : 1);
	PutU16(info, bitcount);
	PutU32(info, 0);
	PutU32(info, (int64_t)pixels.size());
	PutI32(info, (int64_t)((double)xdpi * 39.37));
	PutI32(info, (int64_t)((double)ydpi * 39.37));
	PutU32(info, ncolors);
	PutU32(info, 0);
	info.insert(info.end(), palette.begin(), palette.end());
	info.insert(info.end(), pixels.begin(), pixels.end());
	result = Bmp(info, ncolors);
	return true;
}

enum FontKind { FONT_NORMAL, FONT_SYMBOL, FONT_WINGDINGS };

// CSS of each font of |FONT (11-byte format), and whether it is Symbol or Wingdings (the browser may not have them:
// their characters go as Unicode in Arial)
std::vector<std::pair<std::string, FontKind>> Fonts(const Hlp &hlp)
{
	Bytes f = hlp.Internal("|FONT");
	int nfaces = U16At(f, 0), ndesc = U16At(f, 2), faceoff = U16At(f, 4), descoff = U16At(f, 6);
	int64_t size = FloorDiv(descoff - faceoff, nfaces);
	std::vector<std::string> faces;
	for (int64_t i = 0; i < nfaces; i++) {
		faces.push_back(Latin1(FirstPart(Slice(f, faceoff + i * size, faceoff + (i + 1) * size))));
	}
	std::vector<std::pair<std::string, FontKind>> styles;
	for (int64_t i = 0; i < ndesc; i++) {
		int64_t at = descoff + i * 11;
		At(f, at, 5);
		int attr = f[(size_t)at], half = f[(size_t)at + 1], family = f[(size_t)at + 2], face = U16At(f, at + 3);
		Bytes rgb = Slice(f, at + 5, at + 8);
		std::string name = face < (int)faces.size() ? faces[face] : "Arial";
		FontKind kind = name == "Symbol" ? FONT_SYMBOL : name.rfind("Wingdings", 0) == 0 ? FONT_WINGDINGS : FONT_NORMAL;
		std::string css = "font-size:" + FmtG(half / 2.0) + "pt;font-family:" +
			(kind != FONT_NORMAL ? std::string("Arial") : "'" + name + "'");
		css += family == 2 ? ",serif" : ",sans-serif";
		if (attr & 1) {
			css += ";font-weight:bold";
		}
		if (attr & 2) {
			css += ";font-style:italic";
		}
		if (attr & 4) {
			css += ";text-decoration:underline";
		}
		if (rgb.empty()) {
			Fail();
		}
		if (*std::max_element(rgb.begin(), rgb.end()) > 8) {
			if (rgb.size() != 3) {
				Fail();
			}
			css += ";color:rgb(" + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," +
				std::to_string(rgb[2]) + ")";
		}
		styles.push_back({css, kind});
	}
	return styles;
}

// Writes the paragraphs with their format: margins and indents in points, and the tabs as fixed-width boxes up to
// the next stop (the help's; if none, every half inch), as WinHelp placed them
class Page {
public:
	std::vector<std::pair<std::string, FontKind>> styles;
	std::map<int64_t, std::string> images;
	std::map<uint32_t, std::string> targets;
	uint32_t font = 0;
	bool hasLink = false;
	Chunk link;

	std::string Span(const std::string &text) const
	{
		std::string css;
		FontKind kind = FONT_NORMAL;
		if (font < styles.size()) {
			css = styles[font].first;
			kind = styles[font].second;
		}
		std::string t = kind == FONT_SYMBOL ? Symbol(text) : kind == FONT_WINGDINGS ? Bullets(text) : text;
		if (hasLink) {  // WinHelp's look of a link: green, underlined
			css += ";color:#008000;text-decoration:underline";
		}
		return "<span style=\"" + css + "\">" + Escape(t) + "</span>";
	}

	std::string Paragraph(const Fmt &fmt, const Chunks &chunks)
	{
		std::vector<std::pair<std::string, int64_t>> segments;  // width -1: none
		std::string line;
		int64_t x = fmt.left + fmt.first;  // where the current piece starts
		for (const Chunk &c : chunks) {
			if (c.kind == kText) {
				line += Span(c.text);
			} else if (c.kind == kFont) {
				font = c.n;
			} else if (c.kind == kBr) {
				segments.push_back({line, -1});
				segments.push_back({"<br>", -1});
				line.clear();
				x = fmt.left;
			} else if (c.kind == kTab) {
				std::vector<int64_t> stops;
				for (int s : fmt.tabs) {
					if (s > x) {
						stops.push_back(s);
					}
				}
				if (fmt.left > x) {
					stops.push_back(fmt.left);
				}
				int64_t stop = stops.empty() ? (FloorDiv(x, 36) + 1) * 36 : *std::min_element(stops.begin(), stops.end());
				segments.push_back({line, stop - x});
				line.clear();
				x = stop;
			} else if (c.kind == kImg && images.count(c.n)) {
				std::string align = c.align;
				std::string css = align == "left" ? "float:left;margin:0 8pt 4pt 0" :
					align == "right" ? "float:right;margin:0 0 4pt 8pt" : "";
				line += "<img src=\"" + images[c.n] + "\" style=\"" + css + "\" alt=\"\">";
			} else if (c.kind == kLinkTopic || c.kind == kLinkMacro) {
				std::string target = c.kind == kLinkTopic && targets.count(c.n) ? targets[c.n] : "";
				hasLink = true;
				link = c;
				if (!target.empty()) {
					line += "<a href=\"#" + target + "\">";
				}
			} else if (c.kind == kEnd && hasLink) {
				if (link.kind == kLinkTopic && targets.count(link.n) && !targets[link.n].empty()) {
					line += "</a>";
				}
				hasLink = false;
			}
		}
		segments.push_back({line, -1});
		std::string body;
		for (const auto &s : segments) {
			if (s.second != -1) {
				body += "<span class=\"tab\" style=\"min-width:" + FmtG((double)s.second) + "pt\">" + s.first + "</span>";
			} else {
				body += s.first;
			}
		}
		std::string style = "margin:" + FmtG(fmt.above) + "pt " + FmtG(fmt.right) + "pt " + FmtG(fmt.below) + "pt " +
			FmtG(fmt.left) + "pt;text-indent:" + FmtG(fmt.first) + "pt;text-align:" + fmt.align;
		return "<p style=\"" + style + "\">" + (body.empty() ? Span(kNbsp) : body) + "</p>";
	}

	// The column widths are shares of the table, which takes the page width
	std::string Table(const Widths &widths, const std::vector<Row> &rows)
	{
		int64_t total = 0;
		for (const auto &w : widths) {
			total += w.first;
		}
		if (total == 0) {
			total = 1;
		}
		std::string out = "<table>";
		for (const auto &w : widths) {
			out += "<col style=\"width:" + Fixed1((double)(100 * (int64_t)w.first) / (double)total) + "%\">";
		}
		for (const Row &row : rows) {
			out += "<tr>";
			for (const Cell &cell : row) {
				out += "<td>";
				for (const auto &p : cell) {
					out += Paragraph(p.first, p.second);
				}
				out += "</td>";
			}
			out += "</tr>";
		}
		return out + "</table>";
	}

private:
	// Symbol font characters as Unicode (the browser has no Symbol font): the bullet and a few more
	static std::string Symbol(const std::string &text)
	{
		static const std::map<uint32_t, uint32_t> map = {{0xB7, 0x2022}, {0xA7, 0x2663}, {0xA8, 0x2666},
			{0xA9, 0x2665}, {0xAA, 0x2660}, {0xAE, 0x2192}, {0xAC, 0x2190}, {0xAD, 0x2191}, {0xAF, 0x2193}};
		std::string out;
		size_t n;
		for (size_t i = 0; i < text.size(); i += n) {
			uint32_t cp = GetUtf8(text, i, n);
			auto it = map.find(cp);
			PutUtf8(out, it != map.end() ? it->second : cp);
		}
		return out;
	}

	// Wingdings: only list bullets (Italian and Portuguese help, "Wingdings 2")
	static std::string Bullets(const std::string &text)
	{
		std::string out;
		size_t n;
		for (size_t i = 0; i < text.size(); i += n) {
			uint32_t cp = GetUtf8(text, i, n);
			PutUtf8(out, PySpace(cp) ? cp : 0x2022);
		}
		return out;
	}
};

// The link and its text if the paragraph's only text is one link
bool LinkOnly(const Chunks &chunks, Chunk &link, std::string &text)
{
	bool have = false, inside = false;
	std::string outside;
	text.clear();
	for (const Chunk &c : chunks) {
		if (c.kind == kLinkTopic || c.kind == kLinkMacro) {
			if (have) {
				return false;
			}
			link = c;
			have = true;
			inside = true;
		} else if (c.kind == kEnd) {
			inside = false;
		} else if (c.kind == kText) {
			(inside ? text : outside) += c.text;
		}
	}
	if (!have || !PyStrip(outside).empty()) {
		return false;
	}
	text = PyStrip(text);
	return true;
}

bool IsBack(const Chunk &link)
{
	if (link.kind != kLinkMacro) {
		return false;
	}
	std::string s;
	for (char c : link.text) {
		if (c != ' ') {
			s += c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
		}
	}
	return s == "back()";
}

// Labels of WinHelp's "back" buttons (paragraphs that are only a Back() link): those paragraphs, and the ones that
// are only a link with the same label, are navigation of the help window, not text
std::set<std::string> Navigation(const std::vector<Topic> &parts)
{
	std::set<std::string> labels;
	for (const Topic &topic : parts) {
		for (const Item &item : topic.items) {
			Chunk link;
			std::string text;
			if (!item.table && LinkOnly(item.chunks, link, text) && IsBack(link)) {
				labels.insert(text);
			}
		}
	}
	return labels;
}

bool BackLink(const Chunks &chunks, const std::set<std::string> &labels)
{
	Chunk link;
	std::string text;
	return LinkOnly(chunks, link, text) && (IsBack(link) || labels.count(text));
}

std::string ToHtml(const Bytes &data)
{
	Hlp hlp(data);
	std::map<int64_t, std::string> images;
	for (const auto &f : hlp.files) {
		if (f.first.compare(0, 3, "|bm") == 0) {
			Bytes bmp;
			if (Bitmap(hlp.File(f.second), bmp)) {
				images[PyInt(f.first.substr(3))] = "data:image/bmp;base64," + Base64(bmp);
			}
		}
	}
	std::vector<Topic> parts = Topics(hlp);
	std::map<std::string, std::string> ids;  // a repeated title takes the last number
	for (size_t i = 0; i < parts.size(); i++) {
		ids[parts[i].title] = "topic" + std::to_string(i);
	}
	std::map<uint32_t, std::string> targets;
	for (const auto &t : hlp.LinkTitles()) {
		auto it = ids.find(t.second);
		if (it != ids.end()) {
			targets[t.first] = it->second;
		}
	}
	std::string title = hlp.Title();
	std::vector<std::string> out;
	out.push_back("<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
		"<meta name=\"viewport\" content=\"width=device-width\"><title>" + Escape(title) + "</title><style>"
		// Space around the page (CSS only: a purple night and two layers of stars); the text on a white sheet
		// edged in Buzz's suit colours. Printed, only the sheet.
		"html{background:#12082b}"
		"body{margin:0;min-height:100vh;color:#000;background:"
		"radial-gradient(1.5px 1.5px at 20px 30px,#fff,transparent),"
		"radial-gradient(1px 1px at 120px 80px,#ffe9a8,transparent),"
		"radial-gradient(1px 1px at 60px 150px,#fff,transparent),"
		"radial-gradient(2px 2px at 170px 170px,#cfe8ff,transparent),"
		"radial-gradient(1px 1px at 90px 10px,#fff,transparent),"
		"radial-gradient(1.5px 1.5px at 330px 260px,#fff,transparent),"
		"radial-gradient(1px 1px at 250px 120px,#d8c8ff,transparent),"
		"radial-gradient(1px 1px at 380px 40px,#fff,transparent),"
		"linear-gradient(160deg,#2b0f57 0%,#12082b 45%,#0b2a3a 100%);"
		"background-size:200px 200px,200px 200px,200px 200px,200px 200px,200px 200px,"
		"420px 300px,420px 300px,420px 300px,100% 100%;background-attachment:fixed}"
		"main{margin:0 auto;max-width:640pt;padding:20pt;background:#fff;"
		"border-left:6px solid #5bb534;border-right:6px solid #6b3fa0;box-shadow:0 0 40px rgba(120,200,90,.35)}"
		"section{padding-bottom:24pt;border-bottom:1px solid #ccc;margin-bottom:24pt}"
		"@media print{html,body{background:#fff}main{border:0;box-shadow:none;max-width:none;padding:0}"
		"section{page-break-before:always;border:0}section:first-child{page-break-before:auto}}"
		"p{margin:0;white-space:pre-wrap;overflow-wrap:break-word}"
		".tab{display:inline-block;text-indent:0;vertical-align:top}"
		"table{border-collapse:collapse;width:100%}td{vertical-align:top;padding:0}"
		"img{max-width:100%}a{color:inherit}</style></head><body><main>");
	Page page;
	page.styles = Fonts(hlp);
	page.images = images;
	page.targets = targets;
	std::set<std::string> labels = Navigation(parts);
	for (const Topic &topic : parts) {
		out.push_back("<section id=\"" + ids[topic.title] + "\">");
		for (const Item &item : topic.items) {
			if (!item.table) {
				if (!BackLink(item.chunks, labels)) {
					out.push_back(page.Paragraph(item.fmt, item.chunks));
				} else {
					// Dropped, but its font changes still count for what follows
					for (auto it = item.chunks.rbegin(); it != item.chunks.rend(); ++it) {
						if (it->kind == kFont) {
							page.font = it->n;
							break;
						}
					}
				}
			} else {
				std::vector<Row> rows;
				for (const Row &row : item.rows) {
					Row kept;
					for (const Cell &cell : row) {
						Cell c;
						for (const auto &p : cell) {
							if (!BackLink(p.second, labels)) {
								c.push_back(p);
							}
						}
						kept.push_back(c);
					}
					rows.push_back(kept);
				}
				out.push_back(page.Table(item.widths, rows));
			}
		}
		out.push_back("</section>");
	}
	out.push_back("</main></body></html>");
	// "\n".join(out), written in text mode: every "\n" becomes "\r\n"
	std::string html;
	for (size_t i = 0; i < out.size(); i++) {
		if (i) {
			html += '\n';
		}
		html += out[i];
	}
	std::string crlf;
	crlf.reserve(html.size() + out.size() * 2);
	for (char c : html) {
		if (c == '\n') {
			crlf += '\r';
		}
		crlf += c;
	}
	return crlf;
}

}  // namespace

bool HlpToHtml(const std::vector<uint8_t> &hlp, std::string &html)
{
	try {
		html = ToHtml(hlp);
		return true;
	} catch (...) {
		return false;
	}
}
