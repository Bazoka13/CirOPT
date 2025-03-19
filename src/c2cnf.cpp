#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cassert>

class Bench {
public:
    class Var {
    public:
        std::string name;
        std::string op;
        std::vector<std::string> operands;
        bool is_input = false;
        bool is_output = false;

        explicit Var(std::string  name)
                : name(std::move(name)) {}

        void set_inst(const std::string& opp, const std::vector<std::string>& operandss) {
            this->op = opp;
            this->operands = operandss;
        }

        void mark_input() {
            is_input = true;
        }

        void mark_output() {
            is_output = true;
        }

        std::string str() const {
            return name;
        }
    };

    std::vector<std::shared_ptr<Var>> inputs;
    std::vector<std::shared_ptr<Var>> outputs;
    std::vector<std::shared_ptr<Var>> insts;
    std::vector<int> vals;
    std::unordered_map<std::string, std::shared_ptr<Var>> vars;

    std::shared_ptr<Var> operator[](const std::string& name) {
        if (vars.find(name) == vars.end()) {
            vars[name] = std::make_shared<Var>(name);
        }
        return vars[name];
    }

    void mark_input(const std::shared_ptr<Var>& var) {
        inputs.push_back(var);
    }

    void mark_output(const std::shared_ptr<Var>& var) {
        outputs.push_back(var);
    }

    void append(const std::shared_ptr<Var>& var) {
        insts.push_back(var);
    }

    std::string str() const {
        std::stringstream ss;
        ss << "# input" << std::endl;
        for (const auto& var : inputs) {
            if (var->is_input) {
                ss << "INPUT(" << var->str() << ")" << std::endl;
            }
        }
        ss << "# output" << std::endl;
        for (const auto& var : outputs) {
            if (var->is_output) {
                ss << "OUTPUT(" << var->str() << ")" << std::endl;
            }
        }
        ss << "#" << std::endl;
        for (const auto& var : insts) {
            if (!var->op.empty()) {
                ss << var->str() << " = " << var->op << "(";
                for (size_t i = 0; i < var->operands.size(); ++i) {
                    ss << var->operands[i];
                    if (i != var->operands.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ")" << std::endl;
            }
        }
        return ss.str();
    }
};

class Parser {
public:
    Bench bench;

    explicit Parser(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("Could not open file");
        }

        std::string line;
        std::vector<std::string> lines;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        work(lines);
    }

    void work(const std::vector<std::string>& lines) {
        for (auto line : lines) {
            auto comment_pos = line.find("#");
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }
            line = trim(line);
            if (line.empty()) {
                continue;
            }

            std::string lval;
            if (line.find('=') != std::string::npos) {
                lval = trim(line.substr(0, line.find('=')));
                line = trim(line.substr(line.find('=') + 1));
            }

            std::string op = trim(line.substr(0, line.find('(')));
            std::transform(op.begin(), op.end(), op.begin(), ::toupper);
            std::string operands_str = line.substr(line.find('(') + 1, line.find(')') - line.find('(') - 1);
            std::vector<std::string> operands = split(operands_str, ',');

            for (auto& operand : operands) {
                operand = trim(operand);
                bench[operand];
            }

            if (!lval.empty()) {
                auto var = bench[lval];
                var->set_inst(op, operands);
                bench.append(var);
            } else {
                if (op == "INPUT"||op=="input") {
                    for (auto& operand : operands) {
                        auto var = bench[operand];
                        var->mark_input();
                        bench.mark_input(var);
                    }
                } else if (op == "OUTPUT"||op=="output") {
                    for (auto& operand : operands) {
                        auto var = bench[operand];
                        var->mark_output();
                        bench.mark_output(var);
                    }
                }
            }
        }
    }

private:
    static std::string trim(const std::string& str) {
        const auto str_begin = str.find_first_not_of(" \t");
        if (str_begin == std::string::npos)
            return "";

        const auto str_end = str.find_last_not_of(" \t");
        const auto str_range = str_end - str_begin + 1;

        return str.substr(str_begin, str_range);
    }

    static std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
};
class Literal{
public:
    int rel;
    long long idx;
    explicit Literal(int rell,long long idxx) {
        this->rel = rell;
        this->idx = idxx;
    }
    std::string str() const {
        return std::to_string(idx*rel);
    }
    bool operator==(const Literal &lit) const{
        return rel==lit.rel&&idx==lit.idx;
    }
    bool operator<(const Literal &lit) const{
        return idx<lit.idx||(idx==lit.idx&&rel<lit.rel);
    }
};
class Clause{
public:
    std::vector<Literal> literals;
    explicit Clause(std::vector<Literal> literalss) {
        this->literals=literalss;
    }
    std::string str() const {
        std::string res;
        for(auto i:this->literals){
            res+=i.str();
//            res+=std::to_string(i.idx);
            res+=" ";
        }
        res+="0";
        return res;
    }

};
class Cnf{
public:
    long long vars;
    std::map<std::string,long long> name2idx;
    Bench bench;
    std::vector<Literal> literal_pool;
    std::vector<Clause> clauses;
    std::vector<int> vals;
    std::vector<std::vector<long long>> edges[2];//0 in 1 out
    std::vector<int> val_sure;
    std::vector<int> father;
    int find_father(int x){
        if(father[x]==x)return x;
        return father[x]= find_father(father[x]);
    }
    void merge_father(int a,int b){
        int fa= find_father(a),fb= find_father(b);
        father[fa]=fb;
    }
    explicit Cnf(Bench bench1) {
        this->vars=0;
        this->bench=std::move(bench1);
    }
    bool go_forward(long long idx){
        int v = this->val_sure[idx];
        int tar_val=-1;
        for(auto edge:edges[0][idx]){
            auto gate = this->bench.insts[edge];
            std::string type = gate->op;
            if(type=="AND"){
                if(v==0)tar_val=0;
                else{
                    int fl = 0 ;
                    for(const auto& in_node:gate->operands){
                        long long node_idx = this->name2idx[in_node];
                        if(this->val_sure[node_idx]!=1){
                            fl=1;
                            break;
                        }
                    }
                    if(!fl)tar_val=1;
                }
            }else if(type=="NAND"){
                if(v==0)tar_val=1;
                else{
                    int fl = 0 ;
                    for(const auto& in_node:gate->operands){
                        long long node_idx = this->name2idx[in_node];
                        if(this->val_sure[node_idx]!=1){
                            fl=1;
                            break;
                        }
                    }
                    if(!fl)tar_val=0;
                }
            }else if(type=="OR"){
                if(v==1)tar_val=1;
                else{
                    int fl = 0 ;
                    for(const auto& in_node:gate->operands){
                        long long node_idx = this->name2idx[in_node];
                        if(this->val_sure[node_idx]!=0){
                            fl=1;
                            break;
                        }
                    }
                    if(!fl)tar_val=0;
                }
            }else if(type=="NOR"){
                if(v==1)tar_val=0;
                else{
                    int fl = 0 ;
                    for(const auto& in_node:gate->operands){
                        long long node_idx = this->name2idx[in_node];
                        if(this->val_sure[node_idx]!=0){
                            fl=1;
                            break;
                        }
                    }
                    if(!fl)tar_val=1;
                }
            }else if(type=="XOR"){
                int fl =-1;
                for(const auto& in_node:gate->operands){
                    long long node_idx = this->name2idx[in_node];
                    if(node_idx!=idx){
                        fl=this->val_sure[node_idx];
                    }
                }
                tar_val=v^fl;
            }else if(type=="NOT"){
                tar_val=!v;
            }else if(type=="BUF"||type=="BUFF"){
                tar_val=v;
            }else{
                std::cerr<<"The type is "<<type<<". Weird type found!\n";
                assert(0);
            }
            auto out_idx = this->name2idx[gate->name];
            if(tar_val>=0&&this->val_sure[out_idx]<0){
                this->val_sure[out_idx]=tar_val;
                auto res = go_forward(out_idx);
                if(!res)return false;
                res = go_back(out_idx);
                if(!res)return false;
            }else if(tar_val>=0&&this->val_sure[out_idx]!=tar_val){
                return false;
            }
        }
        return true;
    }
    bool go_back(long long idx){
        int v = this->val_sure[idx];
        for(auto edge:edges[1][idx]){
            auto gate = this->bench.insts[edge];
            std::string type = gate->op;
            if(type=="AND"){
                if(v==0){
                    int cnt=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==1){
                            cnt++;
                        }
                    }
                    if(cnt==gate->operands.size())return false;
                    else if(cnt==(int)gate->operands.size()-1){
                        for(const auto& in_node:gate->operands){
                            auto in_idx = this->name2idx[in_node];
                            if(this->val_sure[in_idx]<0){
                                this->val_sure[in_idx]=0;
                                auto res = go_forward(in_idx);
                                if(!res)return false;
                                res = go_back(in_idx);
                                if(!res)return false;
                            }
                        }
                    }
                }else{
                    int fl=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==0){
                            fl=1;
                            break;
                        }
                    }
                    if(fl)return false;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]<0){
                            this->val_sure[in_idx]=1;
                            auto res = go_forward(in_idx);
                            if(!res)return false;
                            res = go_back(in_idx);
                            if(!res)return false;
                        }
                    }
                }
            }else if(type=="NAND"){
                if(v==1){
                    int cnt=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==1){
                            cnt++;
                        }
                    }
                    if(cnt==gate->operands.size())return false;
                    else if(cnt==(int)gate->operands.size()-1){
                        for(const auto& in_node:gate->operands){
                            auto in_idx = this->name2idx[in_node];
                            if(this->val_sure[in_idx]<0){
                                this->val_sure[in_idx]=0;
                                auto res = go_forward(in_idx);
                                if(!res)return false;
                                res = go_back(in_idx);
                                if(!res)return false;
                            }
                        }
                    }
                }else{
                    int fl=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==0){
                            fl=1;
                            break;
                        }
                    }
                    if(fl)return false;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]<0){
                            this->val_sure[in_idx]=1;
                            auto res = go_forward(in_idx);
                            if(!res)return false;
                            res = go_back(in_idx);
                            if(!res)return false;
                        }
                    }
                }
            }else if(type=="OR"){
                if(v==1){
                    int cnt=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==0){
                            cnt++;
                        }
                    }
                    if(cnt==gate->operands.size())return false;
                    else{
                        if(cnt==(int)gate->operands.size()-1){
                            for(const auto& in_node:gate->operands){
                                auto in_idx = this->name2idx[in_node];
                                if(this->val_sure[in_idx]<0){
                                    this->val_sure[in_idx]=1;
                                    auto res = go_forward(in_idx);
                                    if(!res)return false;
                                    res = go_back(in_idx);
                                    if(!res)return false;
                                }
                            }
                        }
                    }
                }else{
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==1){
                            return false;
                        }
                    }
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]<0){
                            this->val_sure[in_idx]=0;
                            auto res = go_forward(in_idx);
                            if(!res)return false;
                            res = go_back(in_idx);
                            if(!res)return false;
                        }
                    }
                }
            }else if(type=="NOR"){
                if(v==0){
                    int cnt=0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==0){
                            cnt++;
                        }
                    }
                    if(cnt==gate->operands.size())return false;
                    else{
                        if(cnt==(int)gate->operands.size()-1){
                            for(const auto& in_node:gate->operands){
                                auto in_idx = this->name2idx[in_node];
                                if(this->val_sure[in_idx]<0){
                                    this->val_sure[in_idx]=1;
                                    auto res = go_forward(in_idx);
                                    if(!res)return false;
                                    res = go_back(in_idx);
                                    if(!res)return false;
                                }
                            }
                        }
                    }
                }else{
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]==1){
                            return false;
                        }
                    }
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[in_idx]<0){
                            this->val_sure[in_idx]=0;
                            auto res = go_forward(in_idx);
                            if(!res)return false;
                            res = go_back(in_idx);
                            if(!res)return false;
                        }
                    }
                }
            }else if(type=="XOR"){
                int fl = 0;
                for(const auto& in_node:gate->operands){
                    auto in_idx = this->name2idx[in_node];
                    if(this->val_sure[in_idx]<0){
                        fl++;
                    }
                }
                if(fl==1){
                    int tar_val = 0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[idx]>=0)tar_val=this->val_sure[in_idx];
                    }
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        if(this->val_sure[idx]<0){
                            this->val_sure[idx]=v^tar_val;
                            auto res = go_forward(in_idx);
                            if(!res)return false;
                            res = go_back(in_idx);
                            if(!res)return false;
                        }
                    }
                }else if(fl==0){
                    int tar_val = 0;
                    for(const auto& in_node:gate->operands){
                        auto in_idx = this->name2idx[in_node];
                        tar_val^=this->val_sure[in_idx];
                    }
                    if(tar_val!=v)return false;
                }
            }else if(type=="NOT"){
                for(const auto& in_node:gate->operands){
                    auto in_idx = this->name2idx[in_node];
                    if(this->val_sure[in_idx]<0){
                        this->val_sure[in_idx]=!v;
                        auto res = go_forward(in_idx);
                        if(!res)return false;
                        res = go_back(in_idx);
                        if(!res)return false;
                    }else if(this->val_sure[in_idx]==v){
                        return false;
                    }
                }
            }else if(type=="BUF"||type=="BUFF"){
                for(const auto& in_node:gate->operands){
                    auto in_idx = this->name2idx[in_node];
                    if(this->val_sure[in_idx]<0){
                        this->val_sure[in_idx]=v;
                        auto res = go_forward(in_idx);
                        if(!res)return false;
                        res = go_back(in_idx);
                        if(!res)return false;
                    }else if(this->val_sure[in_idx]!=v){
                        return false;
                    }
                }
            }else{
                std::cerr<<"The type is "<<type<<". Weird type found!\n";
                assert(0);
            }
        }
        return true;
    }
    void pre_build(){
        this->name2idx.clear();
        for(const auto& gate: this->bench.insts) {
            std::string type = gate->op;
            std::string input_line_name = gate->str();
            auto tmp = gate->operands;
            for (const auto &s: tmp) {
                if (!this->name2idx[s]){
                    this->name2idx[s] = ++vars;
                }
            }
            if (!this->name2idx[input_line_name]) {
                this->name2idx[input_line_name] = ++vars;
            }
        }
        this->father.resize(this->vars+10);
        for(int i=0;i<this->vars+10;i++) {
            this->father[i]=i;
            this->literal_pool.emplace_back(1, i);
        }
//        return ;
        for(const auto& gate: this->bench.insts) {
            std::string type = gate->op;
            if(type!="NOT"&&type!="BUF"&&type!="BUFF")continue;
            std::string input_line_name = gate->str();
            auto tmp = gate->operands;
            std::vector<long long> output_lines;
            long long input_line = this->name2idx[input_line_name];
            for(const auto& s:tmp){
                output_lines.push_back(this->name2idx[s]);
            }
            if(type=="NOT"){
                if(output_lines.size()!=1){
                    std::cerr<<"Wrong NOT size! The input is "<<gate->str()<<"\n";
                    assert(0);
                }
                auto out_f = find_father(output_lines[0]);
                this->literal_pool[input_line].idx=out_f;
                this->literal_pool[input_line].rel= this->literal_pool[output_lines[0]].rel*(-1);
                merge_father(input_line,output_lines[0]);
            }else{
                if(output_lines.size()!=1){
                    std::cerr<<"Wrong BUFF size! The input is "<<gate->str()<<"\n";
                    assert(0);
                }
                auto out_f = find_father(output_lines[0]);
                this->literal_pool[input_line].idx=out_f;
                this->literal_pool[input_line].rel= this->literal_pool[output_lines[0]].rel;
                merge_father(input_line,output_lines[0]);
            }
        }

    }
    void build(){
        for(const auto& gate: this->bench.insts){
            std::string type = gate->op;
            std::string input_line_name = gate->str();
            auto tmp = gate->operands;
            std::vector<long long> output_lines;
            long long input_line;
            for(const auto& s:tmp){
                if(!this->name2idx[s]){
                    std::cerr<<"?";
                    this->name2idx[s] = ++vars;
                }
                output_lines.push_back(this->name2idx[s]);
            }

            if(!this->name2idx[input_line_name]){
                std::cerr<<"?";
                this->name2idx[input_line_name]=++vars;
                input_line=vars;
            }else input_line = this->name2idx[input_line_name];

            if(type=="AND"){
                std::vector<Literal> tot_literals;
                tot_literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                for(const auto& output_line:output_lines){
                    // -i ∨ o
                    std::vector<Literal> literals;
                    literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(this->literal_pool[output_line].rel, this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                    tot_literals.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                }
                // -o1 ∨ -o2 ... ∨ -on ∨ i
                this->clauses.emplace_back(tot_literals);
            }else if(type =="NOT"){
                continue;
                if(output_lines.size()!=1){
                    std::cerr<<"Wrong NOT size! The input is "<<gate->str()<<"\n";
                    assert(0);
                }
                for(const auto& output_line:output_lines){
                    std::vector<Literal> literals1,literals2;
                    // i ∨ o
                    literals1.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals1.emplace_back(this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals1);
                    // -i ∨ -o
                    literals2.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals2.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals2);
                }
            }else if(type =="XOR"){
                if(output_lines.size()!=2){
                    std::cerr<<"Wrong XOR size! The input is "<<gate->str()<<"\n";
                    assert(0);
                }
                long long a = output_lines[0];
                long long b = output_lines[1];
                std::string clause;
                //-a ∨ -b ∨ -i
                std::vector<Literal> literals;
                literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                literals.emplace_back(-1* this->literal_pool[a].rel,this->literal_pool[a].idx);
                literals.emplace_back(-1* this->literal_pool[b].rel,this->literal_pool[b].idx);
                this->clauses.emplace_back(literals);
                //a ∨ b ∨ -i
                literals.clear();
                literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                literals.emplace_back(this->literal_pool[a].rel,this->literal_pool[a].idx);
                literals.emplace_back(this->literal_pool[b].rel,this->literal_pool[b].idx);
                this->clauses.emplace_back(literals);
                //a ∨ -b ∨ i
                literals.clear();
                literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                literals.emplace_back(this->literal_pool[a].rel,this->literal_pool[a].idx);
                literals.emplace_back(-1*this->literal_pool[b].rel,this->literal_pool[b].idx);
                this->clauses.emplace_back(literals);
                //-a ∨ b ∨ i
                literals.clear();
                literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                literals.emplace_back(-1*this->literal_pool[a].rel,this->literal_pool[a].idx);
                literals.emplace_back(this->literal_pool[b].rel,this->literal_pool[b].idx);
                this->clauses.emplace_back(literals);
            }else if(type == "OR"){
                std::vector<Literal> tot_literals;
                tot_literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                for(const auto& output_line:output_lines){
                    // i ∨ -o
                    std::vector<Literal> literals;
                    literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                    tot_literals.emplace_back(1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                }
                // o1 ∨ o2 ... ∨ on ∨ -i
                this->clauses.emplace_back(tot_literals);
            }else if(type == "BUFF"||type == "BUF"){
                continue;
                if(output_lines.size()!=1){
                    std::cerr<<"Wrong BUFF size! The input is "<<gate->str()<<"\n";
                    assert(0);
                }
                for(const auto& output_line:output_lines){
                    // i ∨ -o
                    std::vector<Literal> literals;
                    literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                    // -i ∨ o
                    literals.clear();
                    literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                }
            }else if(type=="NAND"){
                std::vector<Literal> tot_literals;
                tot_literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                for(const auto& output_line:output_lines){
                    // i ∨ o
                    std::vector<Literal> literals;
                    literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                    tot_literals.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                }
                // -o1 ∨ -o2 ... ∨ -on ∨ -i
                this->clauses.emplace_back(tot_literals);
            }else if(type == "NOR"){
                std::vector<Literal> tot_literals;
                tot_literals.emplace_back(this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                for(const auto& output_line:output_lines){
                    // -i ∨ -o
                    std::vector<Literal> literals;
                    literals.emplace_back(-1*this->literal_pool[input_line].rel,this->literal_pool[input_line].idx);
                    literals.emplace_back(-1*this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                    this->clauses.emplace_back(literals);
                    tot_literals.emplace_back(this->literal_pool[output_line].rel,this->literal_pool[output_line].idx);
                }
                // o1 ∨ o2 ... ∨ on ∨ i
                this->clauses.emplace_back(tot_literals);
            }else if(type == "ITE"){
                auto l = input_line;
                auto c = output_lines[0];
                auto t = output_lines[1];
                auto e = output_lines[2];
                // -c v -l v t
                std::vector<Literal> literals;
                literals.emplace_back(-1*this->literal_pool[c].rel,this->literal_pool[c].idx);
                literals.emplace_back(-1*this->literal_pool[l].rel,this->literal_pool[l].idx);
                literals.emplace_back(this->literal_pool[t].rel,this->literal_pool[t].idx);
                this->clauses.emplace_back(literals);
                // -c v l v -t
                literals.clear();
                literals.emplace_back(-1*this->literal_pool[c].rel,this->literal_pool[c].idx);
                literals.emplace_back(this->literal_pool[l].rel,this->literal_pool[l].idx);
                literals.emplace_back(-1*this->literal_pool[t].rel,this->literal_pool[t].idx);
                this->clauses.emplace_back(literals);
                // c v -l v e
                literals.clear();
                literals.emplace_back(this->literal_pool[c].rel,this->literal_pool[c].idx);
                literals.emplace_back(-1*this->literal_pool[l].rel,this->literal_pool[l].idx);
                literals.emplace_back(this->literal_pool[e].rel,this->literal_pool[e].idx);
                this->clauses.emplace_back(literals);
                // c v l v -e
                literals.clear();
                literals.emplace_back(this->literal_pool[c].rel,this->literal_pool[c].idx);
                literals.emplace_back(this->literal_pool[l].rel,this->literal_pool[l].idx);
                literals.emplace_back(-1*this->literal_pool[e].rel,this->literal_pool[e].idx);
                this->clauses.emplace_back(literals);
            }else{
                std::cerr<<"The type is "<<type<<". Weird type found!\n";
                assert(0);
            }
        }

        for(const auto& output: this->bench.outputs){
            std::vector<Literal> literals;
            long long idx = this->name2idx[output->str()];
            //output is set to 1
            literals.emplace_back(this->literal_pool[idx].rel,this->literal_pool[idx].idx);
            this->clauses.emplace_back(literals);
        }

        if(this->name2idx["gnd"]){
            std::vector<Literal> literals;
            auto idx = this->name2idx["gnd"];
            literals.emplace_back(-1* this->literal_pool[idx].rel,this->literal_pool[idx].idx);
            this->clauses.emplace_back(literals);
        }
        if(this->name2idx["vdd"]){
            std::vector<Literal> literals;
            auto idx=this->name2idx["vdd"];
            literals.emplace_back( this->literal_pool[idx].rel,this->literal_pool[idx].idx);
            this->clauses.emplace_back(literals);
        }
    }
    int calc(){
        std::vector<Clause> tmp_clauses;
        for(const auto& clause: this->clauses){
            auto tmp=clause.literals;
            std::vector<Literal> tmp_lits;
            std::sort(tmp.begin(),tmp.end());
            int sz = tmp.size();
            int fl = 0;
            for(int i=0;i<sz;i++){
                if(i<sz-1&&tmp[i].idx==tmp[i+1].idx){
                    if(tmp[i].rel+tmp[i+1].rel==0){
                        fl=1;
                        break;
                    }else{
                        continue;
                    }
                }
                tmp_lits.push_back(tmp[i]);
            }
            if(fl)continue;
            tmp_clauses.emplace_back(tmp_lits);
        }
        this->clauses=tmp_clauses;
        std::set<long long> st;
        for(const auto& clause: this->clauses){
            for(auto literal: clause.literals){
                st.insert(literal.idx);
            }
        }
        std::map<long long,int> mp;
        int cnt=0;
        for(auto i:st){
            mp[i]=++cnt;
        }
        for(auto & clause : this->clauses){
            for(auto & literal : clause.literals){
                literal.idx=(long long)mp[literal.idx];
            }
        }
        return (int)st.size();
    }
    std::string str(){
        int num = calc();
        std::stringstream ss;
        ss<<"p cnf "<<std::to_string(num)<<" "<<std::to_string(this->clauses.size())<<"\n";
        for(const auto& clause: this->clauses){
//            ss<<clause.literals.size()<<" ";
            ss<<clause.str()<<"\n";
        }
        return ss.str();
    }
    void run(const std::vector<int>& input_vals){
        this->vals.resize(this->vars+10);
        for(int i=1;i<= this->vars;i++){
            this->vals[i]=-1;
        }
        for(auto & input : this->bench.inputs){
            long long idx = this->name2idx[input->str()];
            std::cout<<idx<<" "<<input_vals[idx]<<"\n";
            assert(abs(idx)==abs(input_vals[idx]));

            this->vals[idx]=input_vals[idx]>0?1:0;
        }
        for(int i=0;i< this->bench.insts.size();i++){
            auto inst = this->bench.insts[i];
            std::vector<long long> inputs;
            for(const auto& opr:inst->operands){
                long long idx = this->name2idx[opr];
                inputs.push_back(idx);
                assert(this->vals[idx]==0||this->vals[idx]==1);
            }
            std::string gate = inst->op;
            if(gate=="AND"){
                int res = 1;
                for(auto idx:inputs){
                    res&= this->vals[idx];
                }
                this->vals[this->name2idx[inst->str()]]=res;
            }else if(gate=="NAND"){
                int res = 1;
                for(auto idx:inputs){
                    res&= this->vals[idx];
                }
                this->vals[this->name2idx[inst->str()]]=!res;
            }else if(gate=="NOR"){
                int res = 0;
                for(auto idx:inputs){
                    res|= this->vals[idx];
                }
                this->vals[this->name2idx[inst->str()]]=!res;
            }else if(gate=="OR"){
                int res = 0;
                for(auto idx:inputs){
                    res|= this->vals[idx];
                }
                this->vals[this->name2idx[inst->str()]]=res;
            }else if(gate=="XOR"){
                int res = this->vals[inputs[0]]^this->vals[inputs[1]];
                assert(inputs.size()==2);
                this->vals[this->name2idx[inst->str()]]=res;
            }else if(gate=="BUF"||gate=="BUFF"){
                int res = this->vals[inputs[0]];
                assert(inputs.size()==1);
                this->vals[this->name2idx[inst->str()]]=res;
            }else if(gate=="NOT"){
                int res = this->vals[inputs[0]];
                assert(inputs.size()==1);
                this->vals[this->name2idx[inst->str()]]=!res;
            }else{
                std::cerr<<"The type is "<<gate<<". Weird type found!\n";
                assert(0);
            }
        }
    }
};

int main(int argc, char *argv[]){
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    std::cout.tie(0);
    std::string bench_name = argv[1];
    std::string result_name = argv[2];
    try {
        Parser parser(bench_name);
//        std::cout << parser.bench.str() << std::endl;
        Cnf cnf(parser.bench);
        cnf.pre_build();
        cnf.build();
        freopen(result_name.c_str(),"w",stdout);
        std::cout << cnf.str();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}