#include <bits/stdc++.h>
using namespace std;
string getVar(int idx){
        if(idx==0)return "false";
        if(idx==1)return "true";
        return to_string(idx>>1);
    }
static std::vector<std::string> m_split(const std::string& input, const std::string& pred){
  std::vector<std::string> result;
  std::string temp{""};
  unsigned count1 = input.size();
  unsigned count2 = pred.size();
  unsigned j;
  for (size_t i = 0; i < count1; i++)
  {
    for(j = 0; j < count2; j++)
    {
      if(input[i] == pred[j])
      {
        break;
      }
    }
    if(j == count2)
      temp += input[i];
    else
    {
      if(!temp.empty())
      {
        result.push_back(temp);
        temp.clear();
      }
    }
  }
  if(!temp.empty())
  {
    result.push_back(temp);
    temp.clear();
  }
  return result;
}
vector<string> parse(istream& is){
    vector<string> res;
    vector<string> fin;
	int line_ctr = 0;
  int inputs_num;
  int outputs_num;
  int num;
  int and_num;
  int fl = 0;
  map<int,int> mp;
	for (string line; getline(is, line, '\n');) {
		++line_ctr;
    if(line.find("aag") != string::npos){
      vector<string> temp = m_split(line, " ");
      and_num = stoi(temp[5]);
      if (and_num == 0) return {};
      num = stoi(temp[1]);
      inputs_num = stoi(temp[2]);
      outputs_num = stoi(temp[4]);
      continue;
    }
    if(line_ctr > 1 && line_ctr <= inputs_num + 1)
    {
      int i = stoi(line)>>1;
      string s="INPUT(";
        s+=to_string(i);
        s+=")";
        res.push_back(s);
      continue;
    }
    if(line_ctr > inputs_num + 1 && line_ctr <= inputs_num + outputs_num + 1)
    {
        int i = stoi(line);
        fl=i%2;
        string s="OUTPUT(";
        if(fl==0)
            s+=to_string(i>>1);
        else{
            s+=to_string(i>>1);
            s+="_not";
        }
        s+=")";
        res.push_back(s);
        if(fl){
            string t=to_string(i>>1);
            t+="_not = not( ";
            t+=to_string(i>>1);
            t+=")";
            fin.push_back(t);
        }
        continue;
    }
    
    if(line_ctr <= num + outputs_num + 1){
        vector<string> gate = m_split(line, " ");
        int output = stoi(gate[0]);
        int i1 = stoi(gate[1]);
        int i2 = stoi(gate[2]);
        string ii1 = getVar(i1);
        string ii2 = getVar(i2);
        if(i1>1&&i1%2){
            if(!mp[i1>>1]){
                mp[i1>>1]=1;
                string s=ii1;
                s+="_not = not(";
                s+=ii1;
                s+=")";
                res.push_back(s);
            }
            ii1+="_not";
        }
        if(i2>1&&i2%2){
            if(!mp[i2>>1]){
                mp[i2>>1]=1;
                string s=ii2;
                s+="_not = not(";
                s+=ii2;
                s+=")";
                res.push_back(s);
            }
            ii2+="_not";
        }
        string s=to_string(output>>1);
        s+=" = and(";
        s+=ii1;
        s+=", ";
        s+=ii2;
        s+=")";
        res.push_back(s);
        continue;
    }
	}
    if(fl){
        res.push_back(fin[0]);
    }
	return res;
}


void replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

class BenchTranslation {
private:
    struct Gate {
        std::string func;
        std::vector<std::string> input;
        std::string output;
    };
    vector<string> circuit;
    std::string circuitPath;
    std::string outPath;
    std::unordered_map<std::string, bool> outputs;
    std::unordered_map<std::string, std::string> sameNode;
    std::vector<std::string> inputs;
    std::vector<std::string> optsList;
    std::vector<Gate> gates;
public:
    BenchTranslation(const vector<string>& circuit, const std::string& outPath)
            : circuit(circuit), outPath(outPath) {}

    void readOut(const std::string& line) {
        std::string modifiedLine = line;
        replace_all(modifiedLine, "@", "DAC");
        replace_all(modifiedLine, "%", "DAC");

        std::string opt = modifiedLine.substr(modifiedLine.find('(') + 1, modifiedLine.find(')') - modifiedLine.find('(') - 1);
        if (std::find(inputs.begin(), inputs.end(), "%L" + opt) != inputs.end()) {
            sameNode["@L" + opt + "_o"] = "%L" + opt;
            opt += "_o";
        }
        outputs["@L" + opt] = true;
        optsList.push_back("@L" + opt + "= global i1 false");
    }

    void readIn(const std::string& line) {
        std::string modifiedLine = line;
        replace_all(modifiedLine, "@", "DAC");
        replace_all(modifiedLine, "%", "DAC");

        std::string opt = modifiedLine.substr(modifiedLine.find('(') + 1, modifiedLine.find(')') - modifiedLine.find('(') - 1);
        inputs.push_back("%L" + opt);
    }

    void readGate(const std::string& line) {
        std::string modifiedLine = line;
        replace_all(modifiedLine, "@", "DAC");
        replace_all(modifiedLine, "%", "DAC");
        replace_all(modifiedLine, "\t", "");

        std::vector<std::string> opts;
        std::istringstream iss(modifiedLine);
        std::string word;
        while (iss >> word) {
            opts.push_back(word);
        }

        std::string output = "%L" + opts[0];
        if (outputs.find("@L" + opts[0]) != outputs.end()) {
            output = "%L" + opts[0] + "_1";
        }

        if (modifiedLine.find('(') == std::string::npos) {
            std::string opt = output + "= alloca i1\n";
            if (modifiedLine.find("gnd") != std::string::npos) {
                opt = output + "= call i1 @not(i1 true)";
            } else if (modifiedLine.find("vdd") != std::string::npos) {
                opt = output + "= call i1 @not(i1 false)";
            } else {
                std::cerr << "unexpected gate found!" << std::endl;
            }
            optsList.push_back(opt);
            return;
        }

        std::string funcName = opts[2].substr(0, opts[2].find('(')).substr(0, opts[2].find('('));
        std::string inputStr = modifiedLine.substr(modifiedLine.find('(') + 1, modifiedLine.find(')') - modifiedLine.find('(') - 1);
        std::vector<std::string> inputsList;
        std::istringstream issInputs(inputStr);
        while (std::getline(issInputs, word, ',')) {
            word.erase(std::remove_if(word.begin(), word.end(), ::isspace), word.end());
            if (outputs.find("@L" + word) != outputs.end()) {
                inputsList.push_back("%L" + word + "_1");
            } else {
                if(word =="false"||word=="true"){
                    inputsList.push_back(word);
                }
                else inputsList.push_back("%L" + word);
            }
        }

        if (funcName == "buf" || funcName == "buff") {
            funcName = "buffer";
        }

        Gate gate;
        transform(funcName.begin(),funcName.end(),funcName.begin(),::tolower);
        gate.func = funcName;
        gate.input = inputsList;
        gate.output = output;
        gates.push_back(gate);
    }

    void printGate(const Gate& gate) {
        std::string opt = "";
        if (outputs.find("@L" + gate.output.substr(2)) != outputs.end()) {
            opt = gate.output + "_1";
        }
        opt = gate.output + "  = call i1 @" + gate.func + "( ";

        for (size_t i = 0; i < gate.input.size(); ++i) {
            opt += "i1 " + gate.input[i];
            if (i != gate.input.size() - 1) {
                opt += ", ";
            }
        }
        opt += ")";
        optsList.push_back(opt);
    }

    void printOut() {
        for (const auto& kv : outputs) {
            if (sameNode.find(kv.first) != sameNode.end()) {
                continue;
            }
            std::string opt = "store i1 %L" + kv.first.substr(2) + "_1, i1* " + kv.first;
            optsList.push_back(opt);
        }
    }

    void read() {
        // std::ifstream file(circuitPath);
        std::string line;
        for(auto line:circuit) {
            if (line.empty() || line[0] == '#') {
                continue;
            }
            replace_all(line, "[", "_");
            replace_all(line, "]", "_");
            if (line.find("INPUT(") != std::string::npos) {
                readIn(line);
            } else if (line.find("OUTPUT(") != std::string::npos) {
                readOut(line);
            }
        }

        std::string delc = "define void @main(";
        for (size_t i = 0; i < inputs.size(); ++i) {
            delc += "i1 " + inputs[i];
            if (i != inputs.size() - 1) {
                delc += ",";
            }
        }
        delc += "){";
        optsList.push_back(delc);

        for (const auto& kv : sameNode) {
            optsList.push_back("store i1 " + kv.second + ", i1* " + kv.first);
        }

        // file.clear();
        // file.seekg(0, std::ios::beg);
        // while (std::getline(file, line)) {
        for(auto line:circuit) {
            if (line.empty() || line[0] == '#') {
                continue;
            }
            replace_all(line, "[", "_");
            replace_all(line, "]", "_");
            if (line.find("INPUT(") == std::string::npos && line.find("OUTPUT(") == std::string::npos) {
                readGate(line);
            }
        }

        for (const auto& gate : gates) {
            printGate(gate);
        }
        printOut();
        optsList.push_back("ret void\n}");
    }

    void printOpts() {
        std::ifstream funcFile("func.ll");
        std::ofstream outFile(outPath);
        std::string line;

        while (std::getline(funcFile, line)) {
            outFile << line << std::endl;
        }
        outFile << std::endl;

        for (const auto& opt : optsList) {
            outFile << opt << std::endl;
        }
    }
};

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(0);cout.tie(0);
    ifstream ifs(argv[1]);
    auto res = parse(ifs);
    BenchTranslation bt(res, argv[2]);
    bt.read();
    bt.printOpts();
    return 0;
}