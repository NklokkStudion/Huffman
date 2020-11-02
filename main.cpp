#include <istream>
#include <ostream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <tuple>

using namespace Arch;

static const size_t BYTES = 256;

enum Error {
	OK = 0,
	WRONG_ARGUMENTS,
};

struct Node {
	int64_t l, r;
	size_t letter, idx, sum;
	Node(size_t _letter, size_t _idx, size_t _sum = 0, int64_t _l = -1, int64_t _r = -1)
		: letter(_letter), idx(_idx), sum(_sum), l(_l), r(_r) {}
	bool operator<(const Node& other) const {
		return std::tie(sum, idx) < std::tie(other.sum, other.idx);
	}
};

void dfs(const Node& v, std::vector<bool>& buf, const std::vector<Node>& tree, std::unordered_map<size_t, std::vector<bool>>& have) {
	if (v.l != -1) {
		buf.push_back(true);
		dfs(tree[v.l], buf, tree, have);
		buf.pop_back();
	}
	if (v.r != -1) {
		buf.push_back(false);
		dfs(tree[v.r], buf, tree, have);
		buf.pop_back();
	}
	if (v.l == -1 && v.r == -1) {
		have[v.letter] = buf;
	}
}

std::vector<Node> build_tree(const std::vector<size_t>& cnt) {
	std::vector<Node> tree;
	std::multiset<Node> q;
	size_t id = 0;
	for (size_t i = 0; i < BYTES; ++i) {
		if (cnt[i] != 0) {
			Node now(i, id++, cnt[i]);
			tree.push_back(now);
			q.insert(now);
		}
	}
	while (q.size() >= 2) {
		Node last = *q.begin();
		q.erase(q.begin());
		Node prelast = *q.begin();
		q.erase(q.begin());
		Node now(0, id++, last.sum + prelast.sum, last.idx, prelast.idx);
		tree.push_back(now);
		q.insert(now);
	}
	return tree;
}

std::vector<std::vector<bool>> get_codes(const std::vector<size_t>& cnt) {
	std::vector<Node> tree = build_tree(cnt);
	std::vector<bool> buf1;
	std::unordered_map<size_t, std::vector<bool>> code;
	if (tree.size()) {
		dfs(tree.back(), buf1, tree, code);
	} else {
		throw Error::OK;
	}
	std::vector<std::vector<bool>> codes(BYTES);
	if (tree.size() == 1) {
		for (size_t i = 0; i < BYTES; ++i) {
			if (cnt[i] > 0) {
				codes[i] = std::vector<bool>(1);
				break;
			}
		}
		return codes;
	}
	for (const auto& x : code) {
		codes[x.first] = x.second;
	}
	return codes;
}

int Arch::compress(std::istream* in, std::ostream* out) {
	if (in == nullptr || out == nullptr) {
		return Error::WRONG_ARGUMENTS;
	}
	std::vector<size_t> cnt(BYTES);
	in->seekg(0, in->end);
	size_t length = static_cast<size_t> (in->tellg());
	for (size_t i = 0; i < length; ++i) {
		in->seekg(i);
		++cnt[static_cast<unsigned char>(in->peek())];
	}
	in->seekg(0, in->beg);
	for (size_t i = 0; i < BYTES; ++i) {
		out->write(reinterpret_cast<char*>(&cnt[i]), sizeof(size_t));
	}
	try {
		std::vector<std::vector<bool>> codes = get_codes(cnt);
		unsigned char buf = 0;
		int cur = 7;
		for (size_t i = 0; i < length; ++i) {
			unsigned char c;
			in->read(reinterpret_cast<char*>(&c), sizeof(c));
			for (bool x : codes[c]) {
				if (x == true) {
					buf |= (1 << cur);
				}
				--cur;
				if (cur == -1) {
					cur = 7;
					out->write(reinterpret_cast<const char*>(&buf), sizeof(buf));
					buf = 0;
				}
			}
		}
		if (cur != 7) {
			out->write(reinterpret_cast<const char*>(&buf), sizeof(buf));
		}
	} catch(...) {
		return Error::OK;
	}
	return Error::OK;
}

int Arch::decompress(std::istream* in, std::ostream* out) {
	if (in == nullptr || out == nullptr) {
		return Error::WRONG_ARGUMENTS;
	}
	in->seekg(0, in->end);
	size_t length = static_cast<size_t> (in->tellg());
	in->seekg(0, in->beg);
	std::vector<size_t> cnt(BYTES);
	for (size_t i = 0; i < BYTES; ++i) {
		in->read(reinterpret_cast<char*>(&cnt[i]), sizeof(size_t));
	}
	size_t all = 0;
	size_t razn = 0;
	for (auto& x : cnt) {
		all += x;
		if (x > 0) ++razn;
	}
	std::vector<Node> tree = build_tree(cnt);
	size_t now = tree.size() - 1;
	if (BYTES * sizeof(size_t) == length) {
		return Error::OK;
	}
	for (size_t i = BYTES * sizeof(size_t); i < length; ++i) {
		unsigned char t2;
		in->read(reinterpret_cast<char*>(&t2), sizeof(t2));
		for (int j = 7; j >= 0; --j) {
			if (all == 0) {
				if (length == 1 + i) {
					return Error::OK;
				} else {
					return Error::WRONG_ARGUMENTS;
				}
			}
			if (tree[now].l == -1 && tree[now].r == -1) {
				unsigned char x = static_cast<unsigned char>(tree[now].letter);
				out->write(reinterpret_cast<const char*>(&x), sizeof(x));
				now = tree.size() - 1;
				--all;
				if (razn > 1) ++j;
				continue;
			}
			if ((t2 >> j) & 1) {
				now = tree[now].l;
			} else {
				now = tree[now].r;
			}
		}
	}
	if (all != 0) {
		return Error::WRONG_ARGUMENTS;
	}
	return Error::OK;
}
