#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <regex>
#include <utility>

struct element {
	std::string_view tag;
	std::string id;
	std::vector<std::pair<std::string_view, std::string_view>> attributes;
	std::vector<std::pair<std::string_view, std::string_view>> onevent;
	// bindings whenever
	std::string_view content;
	bool self;
};
struct bajodef {
	std::string_view name;
	std::string_view value;
};
struct closing {};

int i2ln(std::string_view str, size_t index){
	if(index > str.length()) index = str.length();
	int lc = std::count(str.begin(), str.begin() + index, '\n');
	return lc+1;
}
bool isvalidtag(std::string_view str){
	if(str.empty()) return false;
	return std::all_of(str.begin(), str.end(), [](unsigned char c){
		return std::isalpha(c) || (c > 47 && c < 58);
	});
}
int closingquote(std::string_view str, size_t index, char opening){
	size_t n = index + 1;
	while(!(str[n] == opening && str[n-1] != '\\')){
		n++;
		if(n >= str.length()){
			std::cerr << "error: unclosed quotation mark on line " << i2ln(str, index) << "\n";
			exit(1);
		}
	}
	return n;
}
int closingchar(std::string_view str, char opening, char closing, size_t start){
	size_t n = start;
	int c = 1;
	while(c){
		n++;
		if(n >= str.length()){
			std::cerr << "error: unclosed '" << opening << "' on line " << i2ln(str, start) << "\n";
		}
		if(str[n] == '\'' || str[n] == '"') n = closingquote(str, n, str[n]);

		else if(str[n] == opening) c++;
		else if(str[n] == closing) c--;
	}	
	return n;
}
std::string expbajodes(std::string_view str){
	if(!str.contains('$')) return std::string(str);

	size_t start = 0;
	std::string out;
	while(start < str.size()){
		size_t i = str.find('$', start);

		if(i == std::string::npos){
			out += str.substr(start);
			break;
		}

		out += str.substr(start, i - start);

		size_t bend = str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890$_", i+1);
		if(bend == std::string::npos){
			bend = str.size();
		}

		std::string bname = std::string(str.substr(i, bend - i));

		if(bname == "$state"){
			out += bname;
			start = bend;
			continue;
		} else {
			out += bname;
			out += ".value";
		}

		start = bend;
	}	
	return out;
}
void attr(std::string_view attrs, element& elem){
	size_t i = 0;
	size_t nextclosing = attrs.length() - 1;
	while(true){
		size_t nextitem = attrs.find_first_not_of(" \t\n\r", i);
		if (nextitem == std::string::npos) break;
		
		if(nextitem >= nextclosing || nextitem == std::string::npos) break;
		i = nextitem;

		size_t nextequal = attrs.find_first_of("=", i);
		if (nextequal == std::string::npos) break;

		std::string_view key = attrs.substr(i, nextequal - i);
		i = nextequal + 1;
		
		if(i >= attrs.length()) break;
		if(attrs[i] == '\''){
			i = closingquote(attrs, i, '\'');
		} else if (attrs[i] == '"'){
			i = closingquote(attrs, i, '"');
		} else if (attrs[i] == '{'){
			i = closingchar(attrs, '{', '}', i);
		} else {
			size_t end = attrs.find_first_of(" \t\n\r", i) - 1;
			if(end != std::string::npos){
				i = end;
			} else i = nextclosing;
		}
					
		std::string_view value = attrs.substr(nextequal + 1, i - (nextequal + 1) + 1);

		std::pair kv{key, value};
		
		if(key.substr(0, 2) == "on") elem.onevent.push_back(kv);
		else elem.attributes.push_back(kv);

		i++;		
	};
}
std::string_view genelement(element e){
	// $element(tagname, id, onevent, attributes, content);	

	std::string str = "$element(";
	str += "\"";
	str += e.tag;
	str += "\",";
	str += "\"";
	str += e.id;
	str += "\",";
	str += "{";
		for(std::pair<std::string_view, std::string_view> oe : e.onevent){
			std::string_view v = oe.second;

			bool type = v[0] != '{';
			str += oe.first;
			str += ": () => ";
			if(type){
				str += "{";
				str += expbajodes(v.substr(1, v.length() - 2));
				str += "}";
			} else {
				str += expbajodes(v);
			}
			str += ",";
		}; 
	str += "},";
	str += "{";
		for(std::pair<std::string_view, std::string_view> attr : e.attributes){
			std::string_view v = attr.second;

			bool type = v[0] == '{';
			str += attr.first;
			str += ": ";
			if(v.contains('$')) str += "$mut(";
			if(type){
				str += "\"{";
				str += expbajodes(v.substr(1, v.length() - 2));
				str += "}\"";
			} else {
				str += expbajodes(v);
			}
			if(v.contains('$')){
				str += ",\"";
				str += e.id;
				str += ".";
				str += attr.first;
				str += "\"";	
				str += ")";
			}
			str += ",";
		}; 
	str += "},[";
	str += expbajodes(e.content);
	// C++ reference material shows += to be faster than = x + y + x 

	return str;
}
int main(int argc, char* argv[]){
	if(argc < 2){
		std::cerr << "error: no input provided\n";
		return 1;
	}

	char* inputpath = argv[1];
	char* outputpath;
	
	if(argc > 2){
		outputpath = argv[2];
	} else {
		outputpath = "a.html";
	};

	std::ifstream inputfile(inputpath);
	std::ofstream outputfile(outputpath);
	if(!inputfile.is_open() || !outputfile.is_open()){
		std::cerr << "error: failed to open file\n";
		return 1;
	}
	
	constexpr char setupc[] = {
		#embed "bajodingsetup"
	}; std::string_view setup(setupc, sizeof(setupc));

	std::stringstream buffer;
	buffer << inputfile.rdbuf();
	std::string inputr = buffer.str();

	std::string_view input(inputr.data(), inputr.size());
	std::string output;

	using stackobj = std::variant<element, bajodef, closing, std::string_view>;
	std::vector<stackobj> stack;
	
	size_t token = 0;
	int idv = 0;
	
	for(size_t i = 0; i < input.length(); i++){
		char c = input[i];

		if(c == '"' || c == '\''){
			size_t cq = closingquote(input, i, c);
			i = cq;
			continue;
		}
		if(c == '\\' && input[i+1] == '\\'){
			i = input.find_first_of("\n");
			continue;
		}
		if(c == '<'){
			if(input[i+1] == '/'){
				if(i > token){
					stack.emplace_back(input.substr(token, i - token));
				}

				i += 2;
				int next = input.find(">", i);
				if(next == std::string::npos){
					std::cerr << "error: unclosed tag on line " << i2ln(input, i) << "\n";
					exit(1);
				}
				std::string_view tag = input.substr(i, next - i);

				stack.emplace_back(closing{});
				i = next;
				token = i + 1;
			} else if(input[i+1] != ' '){
				size_t scan = i + 1;
				size_t nextspace = input.find_first_of(" \t\n\r", scan);
				size_t nextclosing = closingchar(input, '<', '>', scan - 1);
				bool hasattr = (nextspace < nextclosing);
				size_t next = hasattr ? nextspace : nextclosing;
				std::string_view tagname = input.substr(scan, next - scan);
				if(!isvalidtag(tagname)){ 
					continue;
				}
				if(i > token){
					stack.emplace_back(input.substr(token, i - token));
				}
				
				element elem;
				elem.tag = tagname;
				i = next;
					
				elem.onevent = {};
				elem.attributes = {};

				attr(input.substr(i, nextclosing - i), elem);

				elem.content = "";
				elem.self = false;

				elem.id = "$b" + std::to_string(idv);
				idv++;

				stack.push_back(elem);

				i = nextclosing;
				token = i + 1;
			}
		}
		if(input.substr(i, 4) == "let "){
			size_t scan = i + 4;	
			size_t equals = input.find_first_of("=", scan);
			if(input.substr(input.find_first_not_of(" \t\n\r", equals + 1), 7) != "$state("){
				continue;		
			};
			size_t vend = input.find_first_of(" \t\n\r", scan);

			std::string_view var = input.substr(scan, vend - scan);

			size_t opening = input.find_first_of("(", scan);
			size_t closing = input.find_first_of(")", scan);
				
			std::string_view value = input.substr(opening + 1, closing - opening - 1);

			stack.emplace_back(bajodef{var, value});

			token = input.find_first_of("\n;", closing) + 1;
		}
	}
	if(token < input.length()){
		stack.emplace_back(input.substr(token));
	}

	std::vector<std::string_view> idstack;

	bool openvar = false;
	for(auto& si : stack){
		if(std::holds_alternative<element>(si)){
			element e = std::get<element>(si);
			std::string_view g = genelement(e);

			output += g;
			openvar = true;
			idstack.push_back(e.id);
		} else if(std::holds_alternative<std::string_view>(si)){
			std::string ts;
			std::string_view t = std::get<std::string_view>(si);
			if(openvar){
				if(t.contains('$')) ts += "$mut(";
				ts += "\"";
				ts += expbajodes(t);
				ts += "\"";
				if(t.contains('$')){
					ts += ",\"";
					ts += idstack.back();
					ts += ".content\")";
				};
				ts += ",";
			} else {
				ts += t;
			}

			output += ts;
		} else if(std::holds_alternative<closing>(si)){
			// use only one element until nesting support is added			
			output += "][0]);";
			idstack.pop_back();
			openvar = false;
		} else if(std::holds_alternative<bajodef>(si)){
			bajodef d = std::get<bajodef>(si);

			std::string ds = "let ";
			ds += d.name;
			ds += " = $state(";
			ds += d.value;
			ds += ", \"";
			ds += d.name;
			ds += "\");";

			output += ds;
		}

	}
	if(openvar){
		std::cout << "error: unclosed tag\n";
	}
	output.insert(0, setup);
	output += "</script></body>";

	outputfile << output;
	outputfile.close();

	return 0;
}

