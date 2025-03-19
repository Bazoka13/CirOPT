#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>
#include <map>
#include <ctime>
#include <cassert>


using namespace std;


ostream &log() { return cerr << "c "; }
void abort() { log() << "aborted" << endl; exit(-1); }


// buffered io
const int BufferSize = 1 << 28;

struct BufferedInput {
	FILE *inf;
	char *buffer;
	int pos, n;
	bool eof;

	BufferedInput(FILE *f) : inf(f), pos(0), n(0), eof(0) {
		buffer = static_cast<char *>(malloc(BufferSize));
	}

	BufferedInput(const char *fn) : pos(0), n(0), eof(0) {
		inf = fopen(fn, "r");
		buffer = static_cast<char *>(malloc(BufferSize));
	}

	~BufferedInput() {
		flush();
		free(buffer);
	}

	void flush() {
		n = fread(buffer, sizeof(char), BufferSize, inf);
		pos = 0;
	}

	char get() {
		if (!eof && pos >= n) { flush(); }
		if (pos >= n) { eof = 1; }
		if (eof) { return EOF; }
		return buffer[pos++];
	}
};

struct BufferedOutput {
	FILE *ouf;
	char *buffer;
	int pos;

	BufferedOutput(FILE *f) : ouf(f), pos(0) {
		buffer = static_cast<char *>(malloc(BufferSize * sizeof(char)));
	}

	BufferedOutput(const char *fn) : pos(0) {
		ouf = fopen(fn, "w");
		buffer = static_cast<char *>(malloc(BufferSize * sizeof(char)));
	}

	~BufferedOutput() {
		flush();
		free(buffer);
	}

	void flush() {
		fwrite(buffer, sizeof(char), pos, ouf);
		pos = 0;
	}

	void _print(const char *s) {
		int n = strlen(s);
		int p = 0;
		while (p < n) {
			int len = min(n - p, BufferSize - pos);
			memcpy(buffer + pos, s + p, len);
			p += len;
			pos += len;
			if (pos == BufferSize) {
				flush();
			}
		}
	}

	void print(char c) {
		buffer[pos++] = c;
		if (pos == BufferSize) {
			flush();
		}
	}
	void print(const char *s) { _print(s); }
	void print(const string &s) { _print(s.c_str()); }
};


// object id
struct Hash {
	static int count;
	int id;

	Hash() { id = ++count; }
	
	friend bool operator == (const Hash &a, const Hash &b) {
		return a.id == b.id;
	}
};

int Hash::count = 0;


// type
struct Type {
	Hash id;
	string name;

	Type(const string &s) : name(s) {};

	friend bool operator == (const Type &a, const Type &b) {
		return a.id == b.id;
	}
};

shared_ptr<Type> select_from(const string &t, const vector<shared_ptr<Type>> &types) {
	for (auto type : types) {
		if (t == type->name) {
			return type;
		}
	}
	log() << "type " << t << " not matched" << endl;
	abort();
	return nullptr;
}


// llvm ir
class Var;
class Inst;
class Func;
class Module;
class Node;
class CFG;

class Base {
public:
	Hash id;

	virtual string get_name() = 0;
	virtual shared_ptr<Type> get_type() = 0;
};


// assume that all variables are of type i1 or void
// and each variable is associated with its def inst
shared_ptr<Type> I1   = make_shared<Type>("i1");
shared_ptr<Type> Void = make_shared<Type>("void");

class Var : public Base {
public:
	string name;
	shared_ptr<Type> type;
	shared_ptr<Inst> def_inst;
	string relabel;
	bool forwarded;
	shared_ptr<Node> node;

	Var(
		const string &s,
		shared_ptr<Inst> inst,
		shared_ptr<Type> t,
		const string &r = string()
	) : name(s), def_inst(inst), type(t), relabel(r), forwarded(false), node(nullptr) {}

	string get_name() { return name; }

	shared_ptr<Type> get_type() { return type; }
};

shared_ptr<Var> True  = make_shared<Var>("true", nullptr, nullptr, "vdd");
shared_ptr<Var> False = make_shared<Var>("false", nullptr, nullptr, "gnd");


// instruction
shared_ptr<Type> Ret      = make_shared<Type>("ret");
shared_ptr<Type> Load     = make_shared<Type>("load");
shared_ptr<Type> Store    = make_shared<Type>("store");
shared_ptr<Type> Call     = make_shared<Type>("call");
shared_ptr<Type> And      = make_shared<Type>("and");
shared_ptr<Type> Or       = make_shared<Type>("or");
shared_ptr<Type> Xor      = make_shared<Type>("xor");
shared_ptr<Type> Select   = make_shared<Type>("select");
// for cfg
shared_ptr<Type> Global   = make_shared<Type>("global");
shared_ptr<Type> Local    = make_shared<Type>("local");
shared_ptr<Type> CtrlFlow = make_shared<Type>("ctrlflow");

class Inst : public Base {
public:
	Hash id;
	shared_ptr<Type> type;
	shared_ptr<Var> def;
	vector<shared_ptr<Var>> ops;
	shared_ptr<Func> callee;

	Inst(
		shared_ptr<Type> t,
		shared_ptr<Var> op_d,
		shared_ptr<Func> op_callee = nullptr
	) : type(t), def(op_d), callee(op_callee) {}

	Inst(
		shared_ptr<Type> t,
		shared_ptr<Var> op_d,
		const vector<shared_ptr<Var>> &op_list,
		shared_ptr<Func> op_callee = nullptr
	) : type(t), def(op_d), ops(op_list), callee(op_callee) {}

	void append(shared_ptr<Var> op) {
		ops.emplace_back(op);
	}

	string get_name() { return type ? type->name : "unknown"; }
	
	shared_ptr<Type> get_type() {
		return def ? def->type : Void;
	}
};


// assume that function only contains one single block
class Func : public Base {
public:
	Hash id;
	string name;
	shared_ptr<Type> type;
	vector<shared_ptr<Var>> args;
	vector<shared_ptr<Inst>> insts;
	shared_ptr<CFG> cfg;

	Func(
		const string &s,
		shared_ptr<Type> t
	) : name(s), type(t), cfg(nullptr) {}

	void append_arg(shared_ptr<Var> arg) {
		args.emplace_back(arg);
	}

	void append_inst(shared_ptr<Inst> inst) {
		insts.emplace_back(inst);
	}

	string get_name() { return name; }

	shared_ptr<Type> get_type() { return type; }
};


// llvm ir module
using Scope = map<string, shared_ptr<Base>>;

class Module {
public:
	Hash id;
	vector<shared_ptr<Func>> funcs;
	vector<shared_ptr<Inst>> global_defs;
	shared_ptr<Scope> scope;

	Module() {
		scope = make_shared<Scope>();
		(*scope)["true"] = static_pointer_cast<Base>(True);
		(*scope)["false"] = static_pointer_cast<Base>(False);
		global_defs.emplace_back(make_shared<Inst>(Global, True));
		global_defs.emplace_back(make_shared<Inst>(Global, False));
	}

	void append_func(shared_ptr<Func> func) {
		funcs.emplace_back(func);
	}

	void append_global_def(shared_ptr<Inst> def) {
		global_defs.emplace_back(def);
	}
};


// llvm ir parser
namespace Parser {
	vector<vector<string>> lines;
	vector<string> tokens;
	int line, pos;
	vector<shared_ptr<Scope>> scopes;
	string Eof = {EOF, EOF};
	string Eol = {EOF};

	void get_tokens(BufferedInput &inf) {
		lines.clear();
		tokens.clear();
		char cur = inf.get();
		while (cur != EOF) {
			if (cur <= 32) {
				if (cur == '\n') {
					lines.emplace_back(tokens);
					tokens.clear();
				}
				cur = inf.get();
				continue;
			}
			if (cur == ';') {
				while (cur != EOF && cur != '\n') {
					cur = inf.get();
				}
				continue;
			}
			if (isalnum(cur) || string("@#%._").find(cur) != -1) {
				string token;
				token += cur;
				while ((cur = inf.get()) != EOF) {
					if (!isalnum(cur) && string("@#%._").find(cur) == -1) {
						break;
					}
					token += cur;
				}
				tokens.emplace_back(token);
				continue;
			}
			tokens.emplace_back(string() + cur);
			cur = inf.get();
		}
		lines.emplace_back(tokens);
	}

	string &next_token(bool local = false) {
		if (line >= lines.size()) {
			return Eof;
		}
		if (pos >= lines[line].size()) {
			if (local) {
				return Eol;
			}
			line++;
			pos = 0;
			return next_token();
		}
		return lines[line][pos];
	}

	void next_line() {
		if (line >= lines.size()) {
			return;
		}
		line++;
	}

	string match(const string &expect = string(), bool local = false) {
		string token = next_token(local);
		if (expect.size()) {
			if (token == Eof) {
				log() << "parser: tokens expected but reached end of file" << endl;
				abort();
			}
			if (token != expect) {
				log() << "parser: expected " << expect << " but " << token << " found" << endl;
			}
		}
		pos++;
		return token;
	}

	string match_inline(const string &expect = string()) {
		return match(expect, true);
	}

	void scope_push(shared_ptr<Scope> scope = nullptr) {
		if (scope) {
			scopes.emplace_back(scope);
		} else {
			scopes.emplace_back(make_shared<Scope>());
		}
	}

	void scope_pop() {
		scopes.pop_back();
	}

	void define_var(shared_ptr<Base> var) {
		if (scopes.back()->count(var->get_name())) {
			log() << "parser: duplicate variable " << var->get_name() << " in same scope" << endl;
			abort();
		}
		(*scopes.back())[var->get_name()] = var;
	}

	shared_ptr<Base> get_var(const string &name) {
		for (int depth = scopes.size() - 1; depth >= 0; depth--) {
			if (scopes[depth]->count(name)) {
				return (*scopes[depth])[name];
			}
		}
		log() << "parser: reference of undefined variable " << name << endl;
		abort();
	}

	shared_ptr<Inst> parse_ret_inst() {
		auto inst_type = select_from(match("ret"), {Ret});
		auto def_type = select_from(match_inline(), {I1, Void});
		auto inst = make_shared<Inst>(inst_type, nullptr);
		if (def_type != Void) {
			inst->def = static_pointer_cast<Var>(get_var(match_inline()));
		}
		return inst;
	}

	shared_ptr<Inst> parse_store_inst() {
		auto inst_type = select_from(match("store"), {Store});
		auto def_type = select_from(match_inline(), {I1, Void});
		auto inst = make_shared<Inst>(inst_type, nullptr);
		inst->append(static_pointer_cast<Var>(get_var(match_inline())));
		match_inline(",");
		match_inline();
		if (next_token(true) == "*") {
			match_inline("*");
		}
		inst->def = static_pointer_cast<Var>(get_var(match_inline()));
		while (next_token(true) != Eol && next_token(true) != "}") {
			match_inline();
		}
		return inst;
	}

	shared_ptr<Inst> parse_select_inst(const string &def_name) {
		auto inst_type = select_from(match("select"), {Select});
		auto def_type = select_from(match_inline(), {I1, Void});
		auto var = make_shared<Var>(def_name, nullptr, def_type);
		define_var(var);
		auto cond = static_pointer_cast<Var>(get_var(match_inline()));
		match_inline(",");
		match_inline();
		auto true_value = static_pointer_cast<Var>(get_var(match_inline()));
		match_inline(",");
		match_inline();
		auto false_value = static_pointer_cast<Var>(get_var(match_inline()));
		auto inst = make_shared<Inst>(inst_type, var, vector<shared_ptr<Var>>{cond, true_value, false_value});
		var->def_inst = inst;
		return inst;
	}

	shared_ptr<Inst> parse_inst() {
		if (next_token() == "ret") {
			return parse_ret_inst();
		}
		if (next_token() == "store") {
			return parse_store_inst();
		}
		string def_name = match();
		match_inline("=");
		if (next_token() == "select") {
			return parse_select_inst(def_name);
		}
		auto inst_type = select_from(match_inline(), {
			Ret, Load, Store, Call, And, Or, Xor, Select
		});
		// ignore or disjoint flag
		if (next_token() == "disjoint") {
			match_inline("disjoint");
		}
		auto def_type = select_from(match_inline(), {I1, Void});
		auto inst = make_shared<Inst>(inst_type, nullptr);
		auto var = make_shared<Var>(def_name, inst, def_type);
		inst->def = var;
		inst->append(static_pointer_cast<Var>(get_var(match_inline())));
		while (next_token() == ",") {
			match_inline(",");
			inst->append(static_pointer_cast<Var>(get_var(match_inline())));
		}
		define_var(var);
		return inst;
	}

	shared_ptr<Func> parse_func() {
		scope_push();
		match("define");
		auto func_type = select_from(match_inline(), {I1, Void});
		string func_name = match_inline();
		auto func = make_shared<Func>(func_name, func_type);
		match_inline("(");
		bool first = true;
		while (next_token() != ")") {
			if (!first) {
				match_inline(",");
			}
			auto arg_type = select_from(match_inline(), {I1, Void});
			string arg_name;
			while (next_token() != "," && next_token() != ")") {
				arg_name = match_inline();
			}
			auto arg = make_shared<Var>(arg_name, nullptr, arg_type);
			func->append_arg(arg);
			define_var(static_pointer_cast<Base>(arg));
			first = false;
		}
		match_inline(")");
		while (next_token() != "{") {
			match();
		}
		match("{");
		while (next_token() != "}") {
			auto inst = parse_inst();
			if (inst) {
				func->append_inst(inst);
			}
		}
		match("}");
		scope_pop();
		define_var(static_pointer_cast<Base>(func));
		return func;
	}

	shared_ptr<Inst> parse_global_def() {
		string def_name = match();
		match_inline("=");
		while (next_token(true) != Eol and next_token(true) != "global") {
			match_inline();
		}
		if (next_token() != "global") {
			return nullptr;
		}
		match_inline("global");
		auto def_type = select_from(match_inline(), {I1, Void});
		auto value = static_pointer_cast<Var>(get_var(match_inline()));
		auto var = make_shared<Var>(def_name, nullptr, def_type);
		auto global_def = make_shared<Inst>(Global, var, vector<shared_ptr<Var>>{value});
		var->def_inst = global_def;
		define_var(var);
		return global_def;
	}

	shared_ptr<Module> parse_module() {
		auto module = make_shared<Module>();
		scope_push(module->scope);
		while (next_token() != Eof) {
			if (next_token() == "define") {
				auto func = parse_func();
				if (func) {
					module->append_func(func);
				}
			} else if (next_token() == "attributes") {
				next_line();
			} else {
				auto global_def = parse_global_def();
				if (global_def) {
					module->append_global_def(global_def);
				}
			}
		}
		scope_pop();
		return module;
	}

	shared_ptr<Module> work(BufferedInput &inf) {
		log() << "parser: start parsing module" << endl;
		get_tokens(inf);
		line = pos = 0;
		scopes.clear();
		auto module = parse_module();
		log() << "parser: available functions:" << endl;
		ostream &out = (log() << "  ");
		bool first = true;
		for (auto func : module->funcs) {
			if (!first) {
				out << ", ";
			}
			out << func->name;
			first = false;
		}
		out << endl;
		return module;
	}
};


// control flow graph node
class Node : public enable_shared_from_this<Node> {
public:
	Hash id;
	shared_ptr<Type> op;
	shared_ptr<Inst> inst;
	shared_ptr<Var> var;
	vector<shared_ptr<Node>> pred;
	int succ_cnt;

	Node(
		shared_ptr<Var> var,
		bool global_def = false
	) : inst(nullptr), var(var), succ_cnt(0) {
		op = global_def ? Global : Local;
	}

	Node(
		shared_ptr<Inst> node_inst
	) : op(node_inst->type), inst(node_inst), var(node_inst->def), succ_cnt(0) {
		for (auto op : inst->ops) {
			pred.emplace_back(op->node);
		}
	}

	Node(
		vector<shared_ptr<Node>> &node_pred,
		shared_ptr<Type> node_op = nullptr
	) : inst(nullptr), succ_cnt(0) {
		op = node_op ? node_op : CtrlFlow;
		var = make_shared<Var>(string("%temp.") + to_string(id.id), nullptr, nullptr);
		pred = vector<shared_ptr<Node>>(node_pred.begin(), node_pred.end());
	}

	void link() {
		var->node = shared_from_this();
	}

	bool merge() {
		bool merged = false, changed;
		static vector<shared_ptr<Node>> del_pred;
		static vector<shared_ptr<Node>> new_pred;
		do {
			changed = false;
			del_pred.clear();
			new_pred.clear();
			for (auto node : pred) {
				if (node->op != op || node->succ_cnt > 1) {
					continue;
				}
				del_pred.emplace_back(node);
				for (auto u : node->pred) {
					new_pred.emplace_back(u);
				}
				node->succ_cnt--;
				changed = merged = true;
			}
			int cur = 0, p = 0;
			for (auto node : del_pred) {
				while (pred[cur] != node) {
					pred[p++] = pred[cur++];
				}
				cur++;
			}
			if (p != cur) {
				while (cur < pred.size()) {
					pred[p++] = pred[cur++];
				}
				pred.resize(p);
			}
			for (auto node : new_pred) {
				pred.emplace_back(node);
			}
		} while (changed);
		return merged;
	}
};


// control flow graph of function
class CFG : public enable_shared_from_this<CFG> {
public:
	shared_ptr<Func> func;
	vector<shared_ptr<Node>> nodes;

	CFG(shared_ptr<Module> module) : func(nullptr) {
		for (auto global_def : module->global_defs) {
			make_shared<Node>(global_def)->link();
		}
		for (auto func : module->funcs) {
			make_shared<CFG>(func)->work();
		}
	}

	CFG(shared_ptr<Func> cfg_func) : func(cfg_func) {};

	void new_node(shared_ptr<Node> node) {
		node->link();
		nodes.emplace_back(node);
	}

	void update_succ_cnt() {
		for (auto node : nodes) {
			node->succ_cnt = 0;
		}
		for (auto node : nodes) {
			for (auto pred : node->pred) {
				pred->succ_cnt++;
			}
		}
	}

	bool remove_redundancy() {
		update_succ_cnt();
		static vector<shared_ptr<Node>> list;
		list.clear();
		for (int i = (int)nodes.size() - 1; i >= 0; i--) {
			auto node = nodes[i];
			if (node->succ_cnt == 0) {
				if (node->op != CtrlFlow && node->op != Local && node->op != Global) {
					for (auto pred : node->pred) {
						pred->succ_cnt--;
					}
					continue;
				}
			}
			list.emplace_back(node);
		}
		bool changed = nodes.size() != list.size();
		nodes = vector<shared_ptr<Node>>(list.rbegin(), list.rend());
		return changed;
	}

	void rebuild() {
		func->insts.clear();
		for (auto node : nodes) {
			if (node->op == Local || node->op == Global) {
				continue;
			}
			if (node->inst) {
				func->insts.emplace_back(node->inst);
				continue;
			}
			log() << "cfg rebuild: no instruction for node " << node->op->name << " #" << node->id.id << endl;
		}
	}

	void work() {
		for (auto arg : func->args) {
			new_node(make_shared<Node>(arg));
		}
		for (auto inst : func->insts) {
			if (inst->type == Ret || inst->type == Store) {
				static vector<shared_ptr<Node>> pred;
				pred.clear();
				for (auto op : inst->ops) {
					pred.emplace_back(op->node);
				}
				if (inst->def) {
					pred.emplace_back(inst->def->node);
				}
				auto node = make_shared<Node>(pred);
				node->inst = inst;
				new_node(node);
				continue;
			}
			new_node(make_shared<Node>(inst));
		}
		bool changed;
		do {
			changed = false;
			update_succ_cnt();
			for (int i = (int)nodes.size() - 1; i >= 0; i--) {
				auto node = nodes[i];
				if (node->succ_cnt && (node->op == And || node->op == Or)) {
					changed |= node->merge();
				}
			}
			changed |= remove_redundancy();
		} while (changed);
		func->cfg = shared_from_this();
		if (func->name == "@main") {
			log() << "cfg: " << func->name << " has " << nodes.size() << " nodes" << endl;
		}
	}
};


namespace Selector {
	vector<shared_ptr<CFG>> funcs;
	vector<shared_ptr<Node>> pending, handled;
	int func_order;

	int get_size(shared_ptr<CFG> func) {
		int size = 0;
		for (auto node : func->nodes) {
			if (node->op != Global && node->op != Local) {
				size += node->pred.size();
			}
		}
		return size;
	}

	shared_ptr<Node> get_retvalue(shared_ptr<CFG> func) {
		auto ret = func->nodes.back();
		if (ret->op != CtrlFlow || ret->pred.size() != 1) {
			log() << "cannot find return value of function " << func->func->get_name() << endl;
			abort();
		}
		return *ret->pred.begin();
	}

	void register_func(shared_ptr<CFG> func) {
		funcs.emplace_back(func);
		sort(funcs.begin(), funcs.end(), [&](shared_ptr<CFG> &x, shared_ptr<CFG> &y) {
			if (func_order) {
				return get_size(x) < get_size(y);
			}
			return get_size(x) > get_size(y);
		});
	}

	using MatchResult = map<int, shared_ptr<Node>>;

	pair<bool, MatchResult> match(
		shared_ptr<Node> f_node,
		shared_ptr<Node> t_node,
		int depth
	);

	pair<bool, MatchResult> match_pred(
		shared_ptr<Node> t_node,
		vector<shared_ptr<Node>> &f_pred,
		vector<shared_ptr<Node>> &t_pred,
		int depth,
		bool swappable = false,
		int last = 0
	) {
		if (f_pred.size() == 0) {
			return make_pair(t_pred.size() == 0, MatchResult({}));
		}
		auto cur = f_pred[0];
		MatchResult new_node;
		if (f_pred.size() == 1 && t_pred.size() > 1) {
			auto new_pred = make_shared<Node>(t_pred, t_node->op);
			new_pred->link();
			t_pred = {new_pred};
			new_node[-new_pred->id.id] = new_pred;
			last = 0;
		}
		for (int i = last; i < t_pred.size(); i++) {
			auto r_node = match(cur, t_pred[i], depth + 1);
			if (!r_node.first) {
				continue;
			}
			auto tmp_f_pred = vector<shared_ptr<Node>>(f_pred.begin() + 1, f_pred.end());
			auto tmp_t_pred = vector<shared_ptr<Node>>(t_pred.begin(), t_pred.end());
			tmp_t_pred.erase(tmp_t_pred.begin() + i);
			auto r_pred = match_pred(t_node, tmp_f_pred, tmp_t_pred, depth, swappable, swappable ? i : 0);
			if (!r_pred.first) {
				continue;
			}
			for (auto item : r_node.second) {
				r_pred.second[item.first] = item.second;
			}
			for (auto item : new_node) {
				r_pred.second[item.first] = item.second;
			}
			return make_pair(true, r_pred.second);
		}
		return make_pair(false, MatchResult({}));
	}

	pair<bool, MatchResult> match(
		shared_ptr<Node> f_node,
		shared_ptr<Node> t_node,
		int depth = 0
	) {
		if (f_node->op == Local) {
			return make_pair(
				true,
				MatchResult({{f_node->id.id, t_node}})
			);
		}
		if (f_node->op != t_node->op) {
			return make_pair(false, MatchResult({}));
		}
		if (f_node->op == Global) {
			if (f_node->id == t_node->id) {
				return make_pair(
					true,
					MatchResult({{f_node->id.id, t_node}})
				);
			}
			return make_pair(false, MatchResult({}));
		}
		bool swappable = true;
		auto op_type = (*f_node->pred.begin())->op;
		for (auto pred : f_node->pred) {
			if (pred->op != op_type) {
				swappable = false;
				break;
			}
		}
		auto f_pred = vector<shared_ptr<Node>>(f_node->pred.begin(), f_node->pred.end());
		auto t_pred = vector<shared_ptr<Node>>(t_node->pred.begin(), t_node->pred.end());
		auto r_pred = match_pred(t_node, f_pred, t_pred, depth, swappable);
		if (!r_pred.first) {
			return make_pair(false, MatchResult({}));
		}
		r_pred.second[f_node->id.id] = t_node;
		return make_pair(true, r_pred.second);
	}

	void replace(shared_ptr<Node> cur, shared_ptr<CFG> func, MatchResult &info) {
		auto inst = make_shared<Inst>(Call, cur->var, func->func);
		for (auto node : func->nodes) {
			if (node->op != Local) {
				continue;
			}
			auto rep_node = info[node->id.id];
			if (info.count(-rep_node->id.id)) {
				if (rep_node->op == And || rep_node->op == Or) {
					for (auto pred : rep_node->pred) {
						inst->append(pred->var);
					}
					info.erase(-node->id.id);
					continue;
				}
			}
			inst->append(rep_node->var);
		}
		cur->var->def_inst = inst;
		cur->op = Call;
		cur->inst = inst;
		cur->pred.clear();
		static map<shared_ptr<Node>, bool> is_pred;
		is_pred.clear();
		for (auto op : inst->ops) {
			cur->pred.emplace_back(op->node);
			is_pred[op->node] = true;
		}
		for (auto item : info) {
			if (item.first < 0 && is_pred.count(item.second)) {
				pending.emplace_back(item.second);
			}
		}
	}

	void work(shared_ptr<CFG> target) {
		log() << "selector: start working on " << target->func->get_name() << endl;
		pending = move(target->nodes);
		handled.clear();
		log() << "selector: " << pending.size() << " nodes are pending to be selected" << endl;

		int start_t = clock(), last_t = start_t, cur_t = start_t;
		while (pending.size() > 0) {
			cur_t++;
			if (((cur_t - last_t) & 0x3fffff) == 0) {
				cur_t = clock();
			}
			if (cur_t >= last_t + CLOCKS_PER_SEC) {
				last_t = cur_t;
				int percent = 100.0 * handled.size() / (handled.size() + pending.size());
				int rem_t = 1.0 * (cur_t - start_t) / handled.size() * pending.size() / CLOCKS_PER_SEC;
				log() << "  " << percent << "%  eta " << rem_t << "s" << endl;
			}
			auto cur = pending.back();
			pending.pop_back();
			shared_ptr<CFG> rep_func = nullptr;
			pair<bool, MatchResult> rep_info;
			for (auto func : funcs) {
				rep_info = match(get_retvalue(func), cur);
				if (rep_info.first) {
					rep_func = func;
					break;
				}
			}
			if (!rep_func) {
				handled.emplace_back(cur);
				if (cur->op != Call && cur->op != Local
					&& cur->op != Global && cur->op != CtrlFlow) {
					log() << "selector: no available cfg matched node " << cur->op->name << " #" << cur->id.id << endl;
				}
				continue;
			}
			replace(cur, rep_func, rep_info.second);
			handled.emplace_back(cur);
		}

		target->nodes = vector<shared_ptr<Node>>(handled.rbegin(), handled.rend());
		while (target->remove_redundancy()) {}
	}
};


namespace Bench {
	shared_ptr<Type> Input  = make_shared<Type>("input");
	shared_ptr<Type> Output = make_shared<Type>("output");
	shared_ptr<Type> Gate   = make_shared<Type>("gate");

	map<string, shared_ptr<Var>> labels;

	string relabeller(const string &var, shared_ptr<Type> type) {
		string ret = "L";
		for (auto ch : var) {
			if (isdigit(ch)) {
				ret += ch;
			}
		}
		if (ret == "L") {
			return "";
		}
		if (type == Output) {
			ret = ret + "_o";
		}
		return ret;
	}

	void relabel(shared_ptr<Func> func) {
		labels.clear();
		for (auto arg : func->args) {
			string relabel = relabeller(arg->get_name(), Input);
			if (!relabel.size()) {
				continue;
			}
			if (labels.count(relabel)) {
				log() << "relabel: input id " << relabel << " duplicated" << endl;
				continue;
			}
			arg->relabel = relabel;
			labels[relabel] = arg;
		}
		bool direct_forward = true;
		for (auto inst : func->insts) {
			if (inst->type == Store) {
				auto def = inst->def;
				string relabel = relabeller(def->get_name(), Output);
				if (!relabel.size()) {
					continue;
				}
				if (inst->ops[0]->relabel.size()) {
					if (inst->ops[0]->node->op == Global) {
						if (labels.count(relabel) == 0) {
							def->relabel = relabel;
							labels[relabel] = def;
							continue;
						}
					}
					if (labels.count(relabel) && inst->ops[0]->relabel != relabel) {
						continue;
					}
				}
				def->relabel = relabel;
				if (!inst->ops[0]->forwarded && direct_forward) {
					inst->ops[0]->relabel = relabel;
					inst->ops[0]->forwarded = true;
				}
				labels[relabel] = def;
			} else {
				direct_forward = false;
			}
		}
		int cnt = 1;
		for (auto arg : func->args) {
			if (arg->relabel.size()) {
				continue;
			}
			while (labels.count(relabeller(to_string(cnt), Gate))) {
				cnt++;
			}
			arg->relabel = relabeller(to_string(cnt), Gate);
			labels[arg->relabel] = arg;
		}
		direct_forward = true;
		for (auto inst : func->insts) {
			if (inst->type == Store) {
				auto def = inst->def;
				if (def->relabel.size()) {
					continue;
				}
				if (inst->ops[0]->relabel.size()) {
					if (inst->ops[0]->node->op != Global) {
						def->relabel = inst->ops[0]->relabel;
						continue;
					}
				}
				string relabel = relabeller(def->get_name(), Output);
				if (relabel.size()) {
					log() << "relabel: output id " << relabel << " duplicated" << endl;
				}
				while (labels.count(relabeller(to_string(cnt), Output))) {
					cnt++;
				}
				def->relabel = relabeller(to_string(cnt), Output);
				if (inst->ops[0]->node->op != Global) {
					if (!inst->ops[0]->forwarded && direct_forward) {
						inst->ops[0]->relabel = def->relabel;
						inst->ops[0]->forwarded = true;
					}
				}
				labels[def->relabel] = def;
			} else {
				direct_forward = false;
			}
		}
		for (auto inst : func->insts) {
			if (inst->type == Call) {
				for (auto op : inst->ops) {
					if (op->relabel.size()) {
						continue;
					}
					log() << "relabel: " << op->type->name << " #" << op->id.id << " should be relabelled but not" << endl;
					while (labels.count(relabeller(to_string(cnt), Gate))) {
						cnt++;
					}
					op->relabel = relabeller(to_string(cnt), Gate);
					labels[op->relabel] = op;
				}
				auto op = inst->def;
				while (labels.count(relabeller(to_string(cnt), Gate))) {
					cnt++;
				}
				op->relabel = relabeller(to_string(cnt), Gate);
				labels[op->relabel] = op;
			}
		}
		log() << labels.size() << " i/o/gates in total" << endl;
	}

	void work(shared_ptr<Func> func, BufferedOutput &ouf) {
		relabel(func);
		ouf.print("# input\n");
		int in_cnt = 0;
		for (auto arg : func->args) {
			ouf.print("INPUT(");
			ouf.print(arg->relabel);
			ouf.print(")\n");
			in_cnt++;
		}
		ouf.print("# output\n");
		int out_cnt = 0;
		for (auto inst : func->insts) {
			if (inst->type == Store) {
				ouf.print("OUTPUT(");
				ouf.print(inst->def->relabel);
				ouf.print(")\n");
				out_cnt++;
			}
		}
		ouf.print("###\n");
		int gate_cnt = 0;
		for (auto inst : func->insts) {
			if (inst->type != Call) {
				continue;
			}
			ouf.print(inst->def->relabel);
			ouf.print(" = ");
			for (auto ch : inst->callee->get_name()) {
				if (isalpha(ch)) {
					ouf.print(ch);
				}
			}
			ouf.print("(");
			bool first = true;
			for (auto op : inst->ops) {
				if (!first) {
					ouf.print(", ");
				}
				ouf.print(op->relabel);
				first = false;
			}
			ouf.print(")\n");
			gate_cnt++;
		}
		for (auto inst :func->insts) {
			if (inst->type == Store) {
				if (inst->ops[0]->node->op == Global) {
					ouf.print(inst->def->relabel);
					ouf.print(" = buf(");
					ouf.print(inst->ops[0]->relabel);
					ouf.print(")\n");
				} else if (inst->ops[0]->relabel != inst->def->relabel) {
					ouf.print(inst->def->relabel);
					ouf.print(" = buf(");
					ouf.print(inst->ops[0]->relabel);
					ouf.print(")\n");
				}
			}
		}
		log() << "done, " << in_cnt << " inputs, " << out_cnt << " outputs, " << gate_cnt << " gates" << endl;
	}
};


int main(int argc, char **argv)
{
	int start_t = clock();
	BufferedInput inf(argv[1]);
	BufferedOutput ouf(argv[2]);
	if (argv[3]) {
		Selector::func_order = atoi(argv[3]);
	}
	auto module = Parser::work(inf);
	make_shared<CFG>(module);
	vector<string> funcs = {
		"@and", "@nand", "@or", "@nor",
		"@inv", "@not", "@xor", "@ite",
	};
	for (auto fn : funcs) {
		if (module->scope->count(fn)) {
			auto func = static_pointer_cast<Func>((*module->scope)[fn]);
			Selector::register_func(func->cfg);
		}
	}
	auto main = static_pointer_cast<Func>((*module->scope)["@main"]);
	Selector::work(main->cfg);
	main->cfg->rebuild();
	Bench::work(main, ouf);
	int end_t = clock();
	log() << "finished in " << (end_t - start_t) * 1.0 / CLOCKS_PER_SEC << "s" << endl;
	return 0;
}