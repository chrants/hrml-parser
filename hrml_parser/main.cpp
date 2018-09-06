//
//  main.cpp
//  hrml_parser
//
//  Created by Christian Tschoepe on 13/07/2018.
//  Copyright © 2018 Christian Tschoepe. All rights reserved.
//

#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <ctype.h>
#include <assert.h>

using std::string;

enum TOKENS {
    OPEN_TAG, // <
    CLOSE_TAG, // >
    END_TAG_SYMBOL, // `/` as in </tagname>
    TAG_NAME,
    ATTR_NAME,
    ATTR_VAL,
    NONE, // anything else or no rule
//    INVALID,
    
    LQUOTE,
    RQUOTE,
} ;

struct TokenValuePair {
    TOKENS token;
    string value;
} ;

std::ostream& operator<<(std::ostream& os, const TokenValuePair& tok) {
    return os << "(Token: " << tok.token << ", Value: " << tok.value << ")";
}

bool isValidNameChar (char c) { return isalnum(c) || c == '-' || c == '_'; }

std::vector<TokenValuePair> tokenize_hrml_string(string line) {
    std::vector<TokenValuePair> tokens;
    TokenValuePair next = { TOKENS::NONE, "" };
    
    for (char c : line) {
        switch(next.token) {
            case NONE:
                if (c == '<') { // NONE -> OPEN_TAG
                    next = { TOKENS::OPEN_TAG, "" };
                    tokens.push_back(next);
                }
                break;
            case OPEN_TAG:
                if (isalpha(c)) { // OPEN_TAG -> TAG_NAME
                    next = { TOKENS::TAG_NAME, "" };
                    next.value += c;
                } else if (c == '/') {
                    next = { TOKENS::END_TAG_SYMBOL, "" };
                }
                break;
            case END_TAG_SYMBOL:
                if (isValidNameChar(c)) {
                    next.value += c;
                } else if (c == '>') { // END_TAG_SYMBOL -> CLOSE_TAG
                    tokens.push_back(next);
                    TokenValuePair close = { TOKENS::CLOSE_TAG, "" };
                    tokens.push_back(close);
                    next = { TOKENS::NONE, "" };
                }
                break;
            case TAG_NAME:
                if (isValidNameChar(c)) {
                    next.value += c;
                } else if (isspace(c)) { // TAG_NAME -> ATTR_NAME
                    tokens.push_back(next);
                    next = { TOKENS::ATTR_NAME, "" };
                } else if (c == '>') {
                    tokens.push_back(next);
                    TokenValuePair close = { TOKENS::CLOSE_TAG, "" };
                    tokens.push_back(close);
                    next = { TOKENS::NONE, "" };
                }
                break;
            case ATTR_NAME:
                if (isValidNameChar(c)) {
                    next.value += c;
                } else if (c == '=') { // ATTR_NAME -> ATTR_VAL.
                    tokens.push_back(next);
                    next = { TOKENS::ATTR_VAL, "" };
                } else if (c == '>') {
                    TokenValuePair close = { TOKENS::CLOSE_TAG, "" };
                    tokens.push_back(close);
                    next = { TOKENS::NONE, "" };
                }
                break;
            case ATTR_VAL: {
                TOKENS lastToken = tokens[tokens.size() - 1].token;
                if (c == '"') {
                    if (lastToken == TOKENS::ATTR_NAME) {
                        TokenValuePair lquote = { TOKENS::LQUOTE, "" };
                        tokens.push_back(lquote);
                    } else { // last token is lquote (should be)
                        tokens.push_back(next);
                        TokenValuePair rquote = { TOKENS::RQUOTE, "" };
                        tokens.push_back(rquote);
                    }
                } else if (lastToken == TOKENS::RQUOTE && c == '>') { // ATTR_VAL -> CLOSE_TAG
                    TokenValuePair close = { TOKENS::CLOSE_TAG, "" };
                    tokens.push_back(close);
                    next = { TOKENS::NONE, "" };
                    
                } else if (lastToken == TOKENS::RQUOTE && isalpha(c)) { // ATTR_VAL -> ATTR_NAME
                    next = { TOKENS::ATTR_NAME, "" };
                    next.value += c;
                } else if (lastToken == TOKENS::ATTR_NAME && isspace(c)) { // skip preceding space
                    ;
                } else { // continue ATTR_VAL
                    next.value += c;
                }
                break; }
                
//            case INVALID:
            default:
                ;
        }
    }
    
    return tokens;
}

struct HrmlElement {
    string tagName;
    string innerText;
    std::map<string, string> attributes;
    std::vector<HrmlElement*> children;
} ;

std::ostream& operator<<(std::ostream& os, const HrmlElement& element) {
    os << "[BEGIN " << element.tagName;
    for (std::pair<string, string> attr : element.attributes) {
        os << " ATTR(" << attr.first << "=\"" << attr.second << "\")";
    }
    os << "]" << std::endl;
    for (HrmlElement * elem : element.children) {
        os << *elem;
    }
    os << "[END " << element.tagName << "]" << std::endl;
    return os;
}

std::vector<HrmlElement> parse_hrml_tokens(std::vector<TokenValuePair> * toks) {
    std::vector<TokenValuePair> tokens = *toks;
    std::vector<HrmlElement> nonTreedElements;
    
    HrmlElement currentElement = { };
    string currentAttribute = "";
    for (TokenValuePair tok : tokens) {
        switch (tok.token) {
            case OPEN_TAG:
                break;
            case END_TAG_SYMBOL:
                currentElement.tagName = "/" + tok.value;
                break;
            case TAG_NAME:
                currentElement.tagName = tok.value;
                break;
            case ATTR_NAME:
                currentAttribute = tok.value;
                break;
            case LQUOTE:
                break;
            case ATTR_VAL:
                currentElement.attributes[currentAttribute] = tok.value;
                currentAttribute = "";
                break;
            case RQUOTE:
                break;
            case CLOSE_TAG:
                // if nested...
                // currentElement.children.push_back();
                // if not nested...
                nonTreedElements.push_back(currentElement);
                currentElement = { };
                break;
                
            default:
                ;
        }
    }
    
    std::stack<HrmlElement*> tagStack;
    std::vector<HrmlElement*> elements_p;
    for (HrmlElement el : nonTreedElements) {
        HrmlElement * elem = new HrmlElement(el);
        if(el.tagName[0] != '/') {
            if (tagStack.size() > 0) {
                tagStack.top()->children.push_back(elem);
            } else {
                elements_p.push_back(elem);
            }
            tagStack.push(elem);
        } else {
            tagStack.pop();
        }
    }
    std::vector<HrmlElement> elements;
    for (HrmlElement * elem : elements_p) {
        elements.push_back(*elem);
    }
    
    return elements;
}

void test_tokenize_hrml_line();
void test_parse_hrml_tokens();

string query_hrml(string query, std::vector<HrmlElement> hrmlElements) {
    HrmlElement * element = nullptr;
    int attrLoc = query.find("~");
    string attrName = query.substr(attrLoc + 1);
    string subquery = query.substr(0, attrLoc);

    // split string into tagNames by delimiter '.'
    std::vector<string> tagNames;
    std::stringstream ss(subquery);
    std::string token;
    while (std::getline(ss, token, '.')) {
        tagNames.push_back(token);
    }
    
    auto it = std::find_if(hrmlElements.begin(), hrmlElements.end(), [=] (auto& el) {
        return el.tagName == tagNames[0];
    });
    if(it == hrmlElements.end()) { return "Not Found!"; }
    element = &*it;
    
    if (tagNames.size() > 1) {
        for (int idx = 1; idx < tagNames.size(); idx++) {
            auto it = std::find_if(element->children.begin(), element->children.end(), ([=] (auto& el) {
                return el->tagName == tagNames[idx];
            }));
            if(it == element->children.end()) { return "Not Found!"; }
            element = *it;
        }
    }
    
    if (element->attributes.find(attrName) != element->attributes.end()) {
        return element->attributes[attrName];
    } else {
        return "Not Found!";
    }
}

int main(int argc, const char * argv[]) {
    test_tokenize_hrml_line();
    test_parse_hrml_tokens();
//    std::cout << "Done testing! Congrats :)" << std::endl;
    
    int n, q;
    std::cin >> n >> q;
    std::cin.ignore();
    
    string lines = "";
    for (int idx = 0; idx < n; idx++) {
        string line;
        getline(std::cin, line);
        lines += line + '\n';
    }
    std::cout << lines;
    
    std::vector<TokenValuePair> tokens = tokenize_hrml_string(lines);
    std::vector<HrmlElement> elements = parse_hrml_tokens(&tokens);
    
//    for (auto token : tokens) {
//        std::cout << token;
//    }
//    for (auto element : elements) {
//        std::cout << element;
//    }

    if (q > 0) {
        for (int idx = 0; idx < q; idx++) {
            string query;
            getline(std::cin, query);
            string attrContent = query_hrml(query, elements);
            std::cout << attrContent << std::endl;
        }
    }
    
    return 0;
}

void test_tokenize_hrml_line() {
    // simple open / close tag with attribute
    std::vector<TokenValuePair> tokens = tokenize_hrml_string("<hrml tag = \"hello\"></hrml>");
    assert(tokens[0].token == OPEN_TAG);
    assert(tokens[1].token == TAG_NAME && tokens[1].value == "hrml");
    assert(tokens[2].token == ATTR_NAME && tokens[2].value == "tag");
    assert(tokens[3].token == LQUOTE);
    assert(tokens[4].token == ATTR_VAL && tokens[4].value == "hello");
    assert(tokens[5].token == RQUOTE);
    assert(tokens[6].token == CLOSE_TAG);
    assert(tokens[7].token == OPEN_TAG);
    assert(tokens[8].token == END_TAG_SYMBOL && tokens[8].value == "hrml");
    assert(tokens[9].token == CLOSE_TAG);
    
    // open tags only
    tokens = tokenize_hrml_string("<hrml>");
    assert(tokens[0].token == OPEN_TAG);
    assert(tokens[1].token == TAG_NAME && tokens[1].value == "hrml");
    assert(tokens[2].token == CLOSE_TAG);
    
    // nested tags, properties with different types of characters
    tokens = tokenize_hrml_string(" <hrml tag2=\"I can be long>!\" help=\"\" > \n <div> </div>");
    assert(tokens[0].token == OPEN_TAG);
    assert(tokens[1].token == TAG_NAME && tokens[1].value == "hrml");
    assert(tokens[2].token == ATTR_NAME && tokens[2].value == "tag2");
    assert(tokens[3].token == LQUOTE);
    assert(tokens[4].token == ATTR_VAL && tokens[4].value == "I can be long>!");
    assert(tokens[5].token == RQUOTE);
    assert(tokens[6].token == ATTR_NAME && tokens[6].value == "help");
    assert(tokens[7].token == LQUOTE);
    assert(tokens[8].token == ATTR_VAL && tokens[8].value == "");
    assert(tokens[9].token == RQUOTE);
    assert(tokens[10].token == CLOSE_TAG);
    assert(tokens[11].token == OPEN_TAG);
    assert(tokens[12].token == TAG_NAME && tokens[12].value == "div");
    assert(tokens[13].token == CLOSE_TAG);
    assert(tokens[14].token == OPEN_TAG);
    assert(tokens[15].token == END_TAG_SYMBOL && tokens[15].value == "div");
    assert(tokens[16].token == CLOSE_TAG);
    
    // whitespace, newlines, etc
    tokens = tokenize_hrml_string("<hrml\ttag \r\n=\n\"hello\"></hrml\n>");
    assert(tokens[0].token == OPEN_TAG);
    assert(tokens[1].token == TAG_NAME && tokens[1].value == "hrml");
    assert(tokens[2].token == ATTR_NAME && tokens[2].value == "tag");
    assert(tokens[3].token == LQUOTE);
    assert(tokens[4].token == ATTR_VAL && tokens[4].value == "hello");
    assert(tokens[5].token == RQUOTE);
    assert(tokens[6].token == CLOSE_TAG);
    assert(tokens[7].token == OPEN_TAG);
    assert(tokens[8].token == END_TAG_SYMBOL && tokens[8].value == "hrml");
    assert(tokens[9].token == CLOSE_TAG);
}

void test_parse_hrml_tokens() {
    using std::vector;
    vector<TokenValuePair> tokens ({{OPEN_TAG, ""}, {TAG_NAME, "hrml"}, {CLOSE_TAG, ""}});
    vector<HrmlElement> elements = parse_hrml_tokens(&tokens);
    assert(elements[0].tagName == "hrml" && elements[0].attributes.empty() && elements[0].children.empty());
    
    
    tokens = vector<TokenValuePair> ({
        {OPEN_TAG, ""}, {TAG_NAME, "hrml"},
            {ATTR_NAME, "attr"}, {LQUOTE, ""}, {ATTR_VAL, "12 34"}, {RQUOTE, ""},
            {ATTR_NAME, "attr2"}, {LQUOTE, ""}, {ATTR_VAL, "56 78"}, {RQUOTE, ""},
        {CLOSE_TAG, ""},
            {OPEN_TAG, ""}, {TAG_NAME, "child"}, {CLOSE_TAG, ""},
                {OPEN_TAG, ""}, {TAG_NAME, "childchild"}, {CLOSE_TAG, ""},
                {OPEN_TAG, ""}, {END_TAG_SYMBOL, "childchild"}, {CLOSE_TAG, ""},
            {OPEN_TAG, ""}, {END_TAG_SYMBOL, "child"}, {CLOSE_TAG, ""},
        {OPEN_TAG, ""}, {END_TAG_SYMBOL, "hrml"}, {CLOSE_TAG, ""},
        {OPEN_TAG, ""}, {TAG_NAME, "sibling"}, {CLOSE_TAG, ""},
        {OPEN_TAG, ""}, {END_TAG_SYMBOL, "sibling"}, {CLOSE_TAG, ""},
    });
    elements = parse_hrml_tokens(&tokens);
    
    HrmlElement hrml = elements[0];
    assert(hrml.tagName == "hrml");
    assert(hrml.attributes["attr"] == "12 34");
    assert(hrml.attributes["attr2"] == "56 78");
    
    HrmlElement * child = hrml.children[0];
    assert(child->tagName == "child");
    
    HrmlElement * childchild = child->children[0];
    assert(childchild->tagName == "childchild");
    
    HrmlElement sibling = elements[1];
    assert(sibling.tagName == "sibling");
    assert(sibling.children.empty());
}
