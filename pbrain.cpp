#include <iostream>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include <cstdint>
#include <climits>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <readline/readline.h> // TODO Remove
#include <readline/history.h>

// TODO
// ^C should interrrupt the running program and place the user at a fresh prompt rather than exiting

typedef uint8_t cell;
typedef long index_t;

bool gui = 0;
unsigned int gui_sleep = 40 * 1000;
int ionum = 0;

namespace util
{
	template <typename T> std::string t2s(T t)
	{
		std::stringstream ss{};
		ss << t;
		return ss.str();
	}

	template <typename T> T s2t(std::string s)
	{
		std::stringstream ss{s};
		T t;
		ss >> t;
		return t;
	}

	std::vector<std::string> split(const std::string &s, const char delim)
	{
		std::vector<std::string> ret;
		std::istringstream ss{s};
		std::string cur;
		while (std::getline(ss, cur, delim)) if (cur != "") ret.push_back(cur);
		return ret;
	}
}

namespace curses
{
	std::ostream &out = std::cout;

	struct termios ios_default, ios_raw;

	void init()
	{
		tcgetattr(STDOUT_FILENO, &ios_default);
		ios_raw = ios_default;
		ios_raw.c_lflag &= ~(ICANON | ECHO); // cbreak
	}

	void set_raw() { tcsetattr(STDOUT_FILENO, TCSANOW, &ios_raw); }

	void set_cooked() { tcsetattr(STDOUT_FILENO, TCSANOW, &ios_default); }

	void clear() { out << "\033[H\033[2J" << std::flush; }

	void scr_save()
	{
		out << "\033[?1049h\033[?25l";
		clear();
		set_raw();
	}

	void scr_restore()
	{
		out << "\033[?1049l\033[?25h" << std::flush;
		set_cooked();
	}

	char readchar()
	{
		if (gui) out << "\033[?25h" << std::flush;
		else set_raw();
		char ret = getc(stdin);
		if (gui) out << "\033[?25l" << std::flush;
		else set_cooked();
		return ret;
	}

	std::pair<int, int> termsize()
	{
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return std::pair<int, int>{w.ws_row, w.ws_col};
	}

	void move(int y, int x) { out << "\033[" << y << ";" << x << "H"; }

	void attr_on(int attr) { out << "\033[" << attr << "m"; }

	void attr_reset() { out << "\033[m"; }
	
	void rect(int y, int x, int h, int w)
	{
		move(y, x);
		out << "┌";
		for (int i = 1; i < w; i++) out << "─";
		out << "┐";
		for (int i = 1; i < h; i++)
		{
			move(y + i, x);
			out << "│";
			move(y + i, x + w);
			out << "│";
		}
		move(y + h, x);
		out << "└";
		for (int i = 1; i < w; i++) out << "─";
		out << "┘";
	}

	void table(int y, int x, int h, int w, int rowh, int nrows, std::vector<int> cols)
	{
		move(y, x);
		int colnum = 0, colstart = 0;
		out << "┌";
		for (int i = 1; i < w; i++)
		{
			if (cols.size() > colnum && cols[colnum] == i - colstart)
			{
				colnum++;
				colstart = i;
				out << "┬";
			}
			else out << "─";
		}
		out << "┐";
		for (int j = 1; j < h; j++)
		{
			bool endrow = (j % (rowh + 1) == 0 && (nrows == 0 || j / (rowh + 1) <= nrows));
			move(y + j, x);
			colnum = 0, colstart = 0;
			out << (endrow ? "├" : "│");
			for (int i = 1; i < w; i++)
			{
				if (cols.size() > colnum && cols[colnum] == i - colstart)
				{
					colnum++;
					colstart = i;
					out << (endrow ? "┼" : "│");
				}
				else out << (endrow ? "─" : " ");
			}
			out << (endrow ? "┤" : "│");
		}
		move(y + h, x);
		colnum = 0, colstart = 0;
		out << "└";
		for (int i = 1; i < w; i++)
		{
			if (cols.size() > colnum && cols[colnum] == i - colstart)
			{
				colnum++;
				colstart = i;
				out << "┴";
			}
			else out << "─";
		}
		out << "┘";
	}
	
	int tape(int y, int x, int w, int boxpos, int size, int oldboxpos = -1)
	{
		int n = (w - 1) / (size + 1);
		int diff = w - ((size + 1) * n + 1);
		int ldiff = diff / 2, rdiff = diff - ldiff;
		if (oldboxpos >= 0)
		{
			if (boxpos == oldboxpos) return ldiff;
			int oldboxx = x + ldiff + (size + 1) * oldboxpos;
			int newboxx = x + ldiff + (size + 1) * boxpos;
			move(y, oldboxx);
			out << "┬";
			for (int j = 0; j < size; j++) out << "─";
			out << "┬";
			move(y + 1, oldboxx);
			out << "│";
			move(y + 1, oldboxx + size + 1);
			out << "│";
			move(y + 2, oldboxx);
			out << "┴";
			for (int j = 0; j < size; j++) out << "─";
			out << "┴";
			move(y, newboxx);
			out << "┲";
			for (int j = 0; j < size; j++) out << "━";
			out << "┱";
			move(y + 1, newboxx);
			out << "┃";
			move(y + 1, newboxx + size + 1);
			out << "┃";
			move(y + 2, newboxx);
			out << "┺";
			for (int j = 0; j < size; j++) out << "━";
			out << "┹";
			return ldiff;
		}
		move(y, x);
		for (int i = 0; i < ldiff; i++) out << "─";
		for (int i = 0; i < n; i++)
		{
			std::string tee, bar;
			if (i == boxpos) { tee = "┲"; bar = "━"; }
			else if (i == boxpos + 1) { tee = "┱"; bar = "─"; }
			else { tee = "┬"; bar = "─"; }
			out << tee;
			for (int j = 0; j < size; j++) out << bar;
		}
		out << "┬";
		for (int i = 0; i < rdiff; i++) out << "─";
		move(y + 1, x);
		for (int i = 0; i < ldiff; i++) out << " ";
		for (int i = 0; i < n; i++)
		{
			std::string bar;
			if (i == boxpos || i == boxpos + 1) bar = "┃";
			else bar = "│";
			out << bar;
			out << "\033[" << size << "C";
		}
		out << "│";
		for (int i = 0; i < rdiff; i++) out << " ";
		move(y + 2, x);
		for (int i = 0; i < ldiff; i++) out << "─";
		for (int i = 0; i < n; i++)
		{
			std::string tee, bar;
			if (i == boxpos) { tee = "┺"; bar = "━"; }
			else if (i == boxpos + 1) { tee = "┹"; bar = "─"; }
			else { tee = "┴"; bar = "─"; }
			out << tee;
			for (int j = 0; j < size; j++) out << bar;
		}
		out << "┴";
		for (int i = 0; i < rdiff; i++) out << "─";
		return ldiff;
	}

	struct iobox
	{
		int x, y, w, h, pos, off;
		std::string buf;
		bool input;

		iobox() : x{0}, y{0}, w{0}, h{0}, pos{0}, off{0}, buf{}, input{1} { }

		void setsize(int newy, int newx, int newh, int neww)
		{
			x = newx;
			y = newy;
			w = neww;
			h = newh;
		}

		void clear()
		{
			for (int i = 0; i < h; i++)
			{
				move(y + i, x);
				for (int j = 0; j < w; j++) putc(' ', stdout);
			}
		}

		void putcursor(int i = -1)
		{
			if (i < 0) i = pos;
			i -= off;
			if (w == 0) move(y, x);
			else move(y + i / w, x + i % w);
		}

		void reset(std::string content = "")
		{
			buf = content;
			redraw();
			pos = content.size();
		}

		void redraw()
		{
			//attr_on(7);
			clear();
			//attr_reset();
			for (int i = off; i < off + w * h; i++)
			{
				putcursor(i);
				if (i >= buf.size()) return;
				putc(buf[i], stdout);
			}
		}

		void redraw_right()
		{
			for (int i = (pos == off ? pos : pos - 1); i < off + w * h; i++)
			{
				putcursor(i);
				if (i >= buf.size())
				{
					putc(' ', stdout);
					return;
				}
				putc(buf[i], stdout);
			}
		}

		void adjoff()
		{
			if (pos < off)
			{
				off = (pos / w) * w;
				redraw();
			}
			else if (pos >= off + w * h - 1)
			{
				off = (pos / w - h + 1) * w;
				redraw();
			}
			else redraw_right();
		}

		void resize(int newy, int newx, int newh, int neww)
		{
			clear();
			setsize(newy, newx, newh, neww);
			adjoff();
			redraw();
		}

		void addchar(char c)
		{
			if (c < 32 || c > 126) return;
			buf.insert(pos++, 1, c);
			adjoff();
		}

		char in()
		{
			putcursor();
			char ret = readchar();
			if (ret >= 32 && ret <= 126 && input) addchar(ret);
			return ret;
		}

		void out(char c)
		{
			putcursor();
			if (input) addchar(c);
		}

		void back()
		{
			if (pos > 0) pos--;
			adjoff();
		}

		void forward()
		{
			if (pos < buf.size()) pos++;
			adjoff();
		}

		void home()
		{
			pos = 0;
			adjoff();
		}

		void end()
		{
			pos = buf.size();
			off = pos - w * h;
			if (off < 0) off = 0;
			adjoff();
		}

		void backspace()
		{
			if (pos == 0) return;
			buf.erase(pos - 1, 1);
			pos--;
			adjoff();
		}
	};

	struct readline
	{
		int x, y, w, h;
		std::string prompt;
		iobox io;
		std::vector<std::string> history;
		int histidx;

		readline() : x{0}, y{0}, w{0}, h{0}, prompt{"> "}, io{}, history{""}, histidx{0} { }

		void redraw()
		{
			move(y, x);
			out << prompt;
			io.redraw();
		}

		void resize(int yloc, int xloc, int height, int width)
		{
			y = yloc;
			x = xloc;
			h = height;
			w = width;
			io.setsize(y, x + prompt.size(), h, w - prompt.size());
		}

		void histmove(int offset)
		{
			histidx += offset;
			if (histidx < 0) histidx = 0;
			else if (histidx >= history.size()) histidx = history.size() - 1;
			io.reset(history[histidx]);
		}

		void histset(const std::string &content, bool force = false)
		{
			if ((histidx == history.size() - 1 || force) && (history.size() < 2 || history[history.size() - 2] != content)) history.back() = content; // TODO Cleanup!  This sets the last line in history if we're currently on the last line or we're re-executing an older statement, but only if it's not a duplicate of the previous line.
		}

		void histcopy()
		{
			if (histidx == history.size() - 1) return;
			history.back() = history[histidx];
			histidx = history.size() - 1;
		}

		void histnew()
		{
			if (history.back() != "") history.push_back("");
			histidx = history.size() - 1;
		}

		bool read(std::string &dest)
		{
			redraw();
			io.reset();
			io.redraw();
			histidx = history.size() - 1;
			while (1)
			{
				io.input = 1;
				char c = io.in();
				if (c == '\n')
				{
					dest = io.buf;
					histset(io.buf, true);
					histnew();
					return true;
				}
				else if (c == 4) return 0;
				else if (c == 27)
				{
					io.input = 0;
					if (io.in() != '[') continue;
					char d = io.in();
					if (d == 'C') io.forward();
					else if (d == 'D') io.back();
					else if (d == 'A' || d == 'B')
					{
						histset(io.buf);
						histmove(d == 'A' ? -1 : 1);
					}
				}
				else if (c == 127 or c == 8) { io.backspace(); histcopy(); }
				else if (c == 1) io.home();
				else if (c == 5) io.end();
				else { dest += c; histcopy(); }
			}
			return false;
		}

		
	};
}

struct tape
{
	index_t p, offset;
	bool numchange;
	std::vector<cell> pos, neg;

	tape() : p{0}, offset{-1}, numchange{true}, pos{0}, neg{0} { }

	cell *resolve(index_t idx)
	{
		std::vector<cell> *arr = &pos;
		if (idx < 0) { arr = &neg; idx *= -1; }
		if (arr->size() <= idx) arr->resize(idx + 1);
		return &(*arr)[idx];
	}

	void draw(int y, int x, int w, bool redraw = true)
	{
		static int ldiff = -1;
		static index_t old_p = -1, old_offset = 0;
		std::ostream &out = std::cout;
		int n = (w - 1) / 6;
		if (offset < 0) offset = n / 2;
		else if (offset < 2) offset = 2;
		else if (offset >= n - 2) offset = n - 3;
		bool rdr_tape = false, rdr_num = false;
		if (numchange) rdr_num = true;
		numchange = false;
		if (redraw) rdr_tape = rdr_num = true;
		//else if (p - old_p == offset - old_offset) rdr_tape = true;
		else if (offset == old_offset) rdr_num = true;
		else rdr_num = true;
		if (rdr_tape) ldiff = curses::tape(y, x, w, offset, 5);
		else ldiff = curses::tape(y, x, w, offset, 5, old_offset);
		if (rdr_num) for (int i = 0; i < n; i++)
		{
			cell val = *resolve(p - offset + i);
			curses::move(y + 1, x + 2 + ldiff + i * 6);
			if (val < 100) out << ' ';
			out << (int) val << ' ';
		}
		old_p = p;
		old_offset = offset;
	}

	void reset()
	{
		neg.clear();
		neg.resize(1);
		pos.clear();
		pos.resize(1);
		p = 0;
	}

	cell get() { return *resolve(p); }

	index_t posn() { return p; }

	void set(cell val) { *resolve(p) = val; }

	char out() { return (char) *resolve(p); } // .

	void in(char val) { *resolve(p) = val; } // ,

	void l() { p--; offset--; } // <

	void r() { p++; offset++; } // >

	void dec() { (*resolve(p))--; numchange = true; } // -

	void inc() { (*resolve(p))++; numchange = true; } // +
};

struct ptable
{
	struct pinfo
	{
		std::size_t start, length;
		std::string preview;

		pinfo(std::size_t st, const std::string &content) : start{st}, length{content.size()}, preview{content.substr(0, 100)} { }
		pinfo(pinfo &&orig) = default;
		pinfo() = default;
		pinfo &operator =(const pinfo &other) = default;
	};

	struct stackp
	{
		cell id;
		std::size_t ret;
		
		stackp(cell i, std::size_t r) : id{i}, ret{r} { }
		stackp(stackp &&orig) = default;
		stackp() = default;
		stackp &operator =(const stackp &other) = default;
	};

	std::stack<stackp> callstack;
	std::map<cell, pinfo> table;
	bool dirty = true;
	int cur = -1;
	
	void draw(int y, int x, int h, int w, bool redraw = true)
	{
		if (! redraw && ! dirty) return;
		dirty = false;
		int rowh = 1;
		int nrows = h / (rowh + 1);
		std::ostream &out = std::cout;
		curses::table(y, x, h, w, rowh, table.size() + 1, std::vector<int>{6, 12, 11});
		curses::move(y + 1, x + 2);
		out << "#";
		curses::move(y + 1, x + 8);
		out << "Start";
		curses::move(y + 1, x + 20);
		out << "Length";
		curses::move(y + 1, x + 31);
		int i = 1;
		for (const std::pair<const cell, pinfo> &p : table)
		{
			if (i >= nrows) break;
			int ypos = y + i * (rowh + 1) + 1;
			curses::move(ypos, x + 2);
			out << (int) p.first;
			curses::move(ypos, x + 8);
			out << (int) p.second.start;
			curses::move(ypos, x + 20);
			out << (int) p.second.length;
			curses::move(ypos, x + 31);
			out << p.second.preview.substr(0, w - 32);
			i++;
		}
	}

	int size()
	{
		return table.size();
	}

	void add(cell id, std::size_t addr, const std::string &content)
	{
		dirty = true;
		table[id] = pinfo{addr, content};
	}

	std::size_t push(cell id, std::size_t p)
	{
		if (! table.count(id)) return p; // Don't jump if absent entry
		callstack.push(stackp{id, p});
		cur = id;
		return table[id].start;
	}

	std::size_t pop()
	{
		if (callstack.empty()) throw std::runtime_error{"Tried to pop empty callstack"};
		std::size_t addr = callstack.top().ret;
		callstack.pop();
		if (callstack.empty()) cur = -1;
		else cur = callstack.top().id;
		return addr;
	}

	void reset()
	{
		dirty = true;
		table.clear();
		while (! callstack.empty()) callstack.pop();
	}
};

struct machine
{
	std::string deck;
	tape t;
	ptable pt;
	std::size_t p, offset;
	unsigned long cnt;
	std::istream &in;
	std::ostream &out;
	curses::iobox inbox, outbox;

	machine(std::istream &i) : deck{}, t{}, pt{}, p{0}, offset{0}, cnt{0}, in{i}, out{std::cout}, inbox{}, outbox{} { }

	void drawdeck(int y, int x, int w, bool redraw = true)
	{
		static int ldiff = -1;
		static std::size_t old_p = 0, old_offset = 0;
		offset += p - old_p;
		int n = (w - 1) / 4;
		if (offset == 0) offset = n / 2;
		else if (offset < 2) offset = 2;
		else if (offset >= n - 2) offset = n - 3;
		if (redraw) ldiff = curses::tape(y, x, w, offset, 3);
		else ldiff = curses::tape(y, x, w, offset, 3, old_offset);
		if (old_p != p || redraw) for (int i = 0; i < n; i++)
		{
			int idx = ((int) p) - offset + i;
			char cmd = (idx < 0 || idx >= deck.size()) ? ' ' : deck[idx];
			curses::move(y + 1, x + 2 + ldiff + i * 4);
			out << cmd;
		}
		old_p = p;
		old_offset = offset;
	}

	void drawstat(int y, int x, int h, int w, bool redraw = false)
	{
		const int keyw = 26, valw = 8;
		static const std::vector<std::string> names{"Instructions executed", "Deck size", "Instruction pointer", "Tape position", "Procedures defined", "Current procedure"};
		std::vector<std::string> values{util::t2s(cnt), util::t2s(deck.size()), util::t2s(p), util::t2s(t.posn()), util::t2s(pt.size()), pt.cur == -1 ? "-" : util::t2s(pt.cur)};
		if (redraw)
		{
			curses::attr_on(36);
			for (int i = 0; i < names.size(); i++)
			{
				curses::move(y + i % h, x + (i / h) * (keyw + valw));
				out << names[i] << ":";
			}
			curses::attr_reset();
		}
		for (int i = 0; i < values.size(); i++)
		{
			curses::move(y + i % h, x + (i / h) * (keyw + valw) + keyw);
			for (int j = 0; j < valw; j++) out << " ";
			curses::move(y + i % h, x + (i / h) * (keyw + valw) + keyw);
			out << values[i];
		}
	}

	void reset()
	{
		t.reset();
		pt.reset();
		inbox.reset();
		outbox.reset();
		deck = "";
		cnt = 0;
		p = 0;
	}

	std::size_t match(std::size_t idx)
	{
		int inc, stack = 0;
		char cur = deck[idx], target;
		switch (cur)
		{
			case '[': target = ']'; inc = 1; break;
			case ']': target = '['; inc = -1; break;
			case '(': target = ')'; inc = 1; break;
			case ')': target = '('; inc = -1; break;
			case '{': target = '}'; inc = 1; break;
			case '}': target = '{'; inc = -1; break;
			default: throw std::runtime_error{"Internal error: non-matchable symbol passed to match"};
		}
		for (std::size_t i = idx + inc; i < deck.size() && i >= 0; i += inc)
		{
			if (deck[i] == target)
			{
				if (stack == 0) return i;
				else stack--;
			}
			else if (deck[i] == cur) stack++;
		}
		throw std::runtime_error{std::string{"Unmatched \""} + cur + "\""};
	}

	int step()
	{
		if (p >= deck.size()) return 1;
		cnt++;
		switch(deck[p])
		{
			// Standard BF
			case '+': t.inc(); break;
			case '-': t.dec(); break;
			case '<': t.l(); break;
			case '>': t.r(); break;
			case ',': if (&in != &std::cin) t.in(in.get());
				  else if (gui) { ionum = 2; t.in(inbox.in()); }
				  else t.in(curses::readchar()); break;
			case '.': if (gui) outbox.out(t.out());
				  else putc(t.out(), stdout); break;
			case '[': if (! t.get()) p = match(p); break;
			case ']': if (t.get()) p = match(p); break;
			// Pbrain functions
			case '(': pt.add(t.get(), p, deck.substr(p + 1, match(p) - p - 1)); p = match(p); break;
			case ')': try { p = pt.pop(); }
				  catch (std::runtime_error e) { } break;
			case ':': p = pt.push(t.get(), p); break;
			// Debugging extensions
			case '!': p++; return 1;
			case '?': out << t.posn(); break;
			case '=': out << (int) t.out(); break;
			case '%': out << "\n"; break;
			default: break;
		}
		p++;
		return 0;
	}

	void load(const std::string &program)
	{
		deck.insert(p, program);
		//p = 0;
	}
};

struct runner
{
	machine m;
	curses::readline read;
	bool exts, pbrain;
	int rdln_x, rdln_y, rdln_w, rdln_h;
	std::ostream &out = std::cout;

	const static int redraw_frames = 0x01;
	const static int redraw_stats = 0x02;
	const static int redraw_tape = 0x04;
	const static int redraw_deck = 0x08;
	const static int redraw_procs = 0x10;
	const static int redraw_all = 0x1f;

	runner(std::istream &in) : m{in}, read{}
	{
		m.inbox.setsize(30, 69, 7, 40);
		m.outbox.setsize(42, 69, 7, 40);
	}

	std::string clean(const std::string &str)
	{
		std::string ret;
		for (char c : str)
		{
			switch (c)
			{
				case '+': case '-': case '<': case '>': case '[': case ']': case ',': case '.':
					ret += c;
					break;
				case ':': case '(': case ')':
					if (pbrain) ret += c;
					break;
				case '!': case '?': case '=': case '%':
					if (exts) ret += c;
					break;
			}
		}
		return ret;
	}

	void draw(int redraw = 0)
	{
		int minh = 46, minw = 78;
		std::pair<int, int> tsize = curses::termsize();
		int rows = tsize.first, cols = tsize.second;
		if (rows < minh || cols < minw) return; // Better way to handle small windows?
		int h_proc = (rows - 32) * 3 / 4;
		if (h_proc % 2) h_proc += 1;
		int h_cons = rows - 32 - h_proc;
		int w_proc = cols * 2 / 5 - 5;
		if (w_proc < 48) w_proc = 48;
		if (redraw & redraw_frames)
		{
			curses::clear();
			m.inbox.resize(30, w_proc + 10, h_proc / 2 - 4, cols - w_proc - 15);
			m.outbox.resize(28 + h_proc / 2 + 3, w_proc + 10, h_proc / 2 - 4, cols - w_proc - 15);
			curses::attr_on(32);
			curses::rect(1, 1, rows - 1, cols - 1);
			curses::rect(3, 4, 7, cols - 7);
			curses::rect(12, 4, 6, cols - 7);
			curses::rect(20, 4, 6, cols - 7);
			curses::rect(28, 4, h_proc, w_proc);
			curses::rect(28, w_proc + 7, h_proc / 2 - 1, cols - w_proc - 10);
			curses::rect(28 + h_proc / 2 + 1, w_proc + 7, h_proc / 2 - 1, cols - w_proc - 10);
			curses::rect(28 + h_proc + 2, 4, h_cons, cols - 7);
			curses::attr_reset();
			curses::move(3, 6);
			out << "[ Statistics ]";
			curses::move(12, 6);
			out << "[ Tape ]";
			curses::move(20, 6);
			out << "[ Instruction Deck ]";
			curses::move(28, 6);
			out << "[ Procedures ]";
			curses::move(28, w_proc + 9);
			out << "[ Input ]";
			curses::move(h_proc / 2 + 29, w_proc + 9);
			out << "[ Output ]";
			curses::move(h_proc + 30, 6);
			out << "[ Console ]";
			read.resize(h_proc + 32, 7, h_cons - 3, cols - 12);
			read.redraw();
		}
		if (redraw & redraw_stats) m.drawstat(5, 7, 4, cols - 12, redraw & redraw_frames);
		if (redraw & redraw_tape) m.t.draw(14, 7, cols - 12, redraw & redraw_frames);
		if (redraw & redraw_deck) m.drawdeck(22, 7, cols - 12, redraw & redraw_frames);
		if (redraw & redraw_tape) m.pt.draw(30, 7, h_proc - 4, w_proc - 6, redraw & redraw_frames);
		if (ionum == 1) read.io.putcursor();
		else if (ionum == 2) m.inbox.putcursor();
		out << std::flush;
	}

	int base_run()
	{
		int ret = 0;
		try
		{
			while (! m.step()) if (gui)
			{
				draw(runner::redraw_stats | runner::redraw_tape | runner::redraw_deck);
				usleep(gui_sleep);
			}
		}
		catch (std::runtime_error e) { std::cerr << e.what(); ret = 1; }
		std::cout << std::endl;
		return ret;
	}

	int run(const std::string &deck)
	{
		m.load(clean(deck));
		if (gui) draw(runner::redraw_deck);
		return base_run();
	}

	int resume()
	{
		return base_run();
	}

	int prompt()
	{
		std::string line{};
		if (gui)
		{
			//draw(1); // TODO Is this necessary at all?
			ionum = 1;
			if (! read.read(line)) return 1;
		}
		else
		{
			char *cline = readline("> ");
			if (! cline) return 1;
			line = cline;
			if (line != "") add_history(line.c_str());
		}
		if (line[0] == '/')
		{
			std::vector<std::string> cmd{util::split(line.substr(1), ' ')};
			if (cmd[0] == "") return 0;
			else if (cmd[0] == "q" || cmd[0] == "quit") return 1;
			else if (cmd[0] == "r" || cmd[0] == "reset") m.reset();
			else if (cmd[0] == "c" || cmd[0] == "continue") resume();
		}
		else run(line);
		return 0;
	}
};

runner *r = 0;

void resize(int num) // TODO Rate-limit calls to this
{
	if (r) r->draw(runner::redraw_all);
}

void sig(int num)
{
	if (r) delete r;
	if (gui) curses::scr_restore();
	else
	{
		curses::set_cooked();
		std::cout << "\n";
	}
	exit(1);
}

int main(int argc, char **argv) try
{
	bool exitflag = 1, pbflag = 1, extflag = 1;
	int opt;
	while ((opt = getopt(argc, argv, "degps:")) > 0)
	{
		if (opt == 'g') gui = 1;
		else if (opt == 'e') exitflag = 0;
		else if (opt == 'd') extflag = 0;
		else if (opt == 'p') pbflag = 0;
		else if (opt == 's') gui_sleep = 1000 * util::s2t<unsigned int>(std::string{optarg});
		else return 1;
	}
	signal(2, sig);
	signal(15, sig);
	if (gui) signal(28, resize);
	curses::init();
	if (gui) curses::scr_save();
	bool deckflag = 0, inputflag = 0;
	std::ifstream input{};
	std::string deck;
	for (int arg = optind; arg < argc; arg++)
	{
		if (arg - optind == 0)
		{
			deckflag = 1;
			std::ifstream deckfile{};
			deckfile.open(argv[arg]);
			if (deckfile.fail()) throw std::runtime_error{std::string{"Couldn't open instruction deck "} + argv[arg]};
			deck = std::string{std::istreambuf_iterator<char>{deckfile}, std::istreambuf_iterator<char>{}};
		}
		else if (arg - optind == 1)
		{
			inputflag = 1;
			input.open(argv[arg]);
			if (input.fail()) throw std::runtime_error{std::string{"Couldn't open input file "} + argv[arg]};
		}
		else throw std::runtime_error{"Too many arguments"};
	}
	if (inputflag) r = new runner{input};
	else r = new runner{std::cin};
	r->exts = extflag;
	r->pbrain = pbflag;
	if (gui) r->draw(runner::redraw_all);
	if (deckflag) r->run(deck);
	if (! deckflag || ! exitflag)  while(! r->prompt());
	if (gui) curses::scr_restore();
	else std::cout << "\n";
	if (r) delete(r);
	return 0;
}
catch (std::runtime_error e)
{
	curses::set_cooked();
	std::cerr << e.what() << "\n";
	return 1;
}
