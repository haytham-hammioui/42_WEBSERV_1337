#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

ConfigParser::ConfigParser() : _pos(0) {}

ConfigParser::ConfigParser(const ConfigParser& other) { *this = other; }

ConfigParser& ConfigParser::operator=(const ConfigParser& other) {
    if (this == &other) return *this;
    _tokens   = other._tokens;
    _pos      = other._pos;
    _filename = other._filename;
    return *this;
}

ConfigParser::~ConfigParser() {}

static std::string itoa(int n) {
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

// ─── PUBLIC ───────────────────────────────────────────────────────────────────

std::vector<ServerConfig> ConfigParser::parse(const std::string& filename) {
    _filename = filename;
    _pos      = 0;
    _tokens.clear();

    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
        throw std::runtime_error("ConfigParser: cannot get CWD");
    _configDir = cwd;

    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("ConfigParser: cannot open file: " + filename);

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    tokenize(content);

    std::vector<ServerConfig> servers;
    while (!atEnd()) {
        Token& tok = current();
        if (tok.type == TOKEN_WORD && tok.value == "server") {
            advance(); 
            servers.push_back(parseServer());
        } else {
            throw std::runtime_error(
                "ConfigParser: expected 'server' block at line "
                + itoa(tok.line));
        }
    }

    if (servers.empty())
        throw std::runtime_error("ConfigParser: no server blocks found");

    for (size_t i = 0; i < servers.size(); i++)
        validateServer(servers[i]);

    return servers;
}

// ─── TOKENIZER ────────────────────────────────────────────────────────────────

bool ConfigParser::isSpecial(char c) {
    return c == '{' || c == '}' || c == ';';
}

void ConfigParser::tokenize(const std::string& content) {
    int line = 1;
    size_t i = 0;

    while (i < content.size()) {
        char c = content[i];
        if (c == '\n') {
            line++;
            i++;
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r') {
            i++;
            continue;
        }
        if (c == '#') {
            while (i < content.size() && content[i] != '\n')
                i++;
            continue;
        }
        if (c == '{') { _tokens.push_back(Token(TOKEN_OPEN_BRACE,  "{", line)); i++; continue; }
        if (c == '}') { _tokens.push_back(Token(TOKEN_CLOSE_BRACE, "}", line)); i++; continue; }
        if (c == ';') { _tokens.push_back(Token(TOKEN_SEMICOLON,   ";", line)); i++; continue; }
        if (!isSpecial(c) && c != '#') {
            std::string word;
            while (i < content.size()
                   && content[i] != ' '  && content[i] != '\t'
                   && content[i] != '\n' && content[i] != '\r'
                   && !isSpecial(content[i]) && content[i] != '#') {
                word += content[i++];
            }
            _tokens.push_back(Token(TOKEN_WORD, word, line));
            continue;
        }
        i++;
    }
    _tokens.push_back(Token(TOKEN_EOF, "", line));
}

// ─── PARSER HELPERS ───────────────────────────────────────────────────────────

Token& ConfigParser::current() {
    return _tokens[_pos];
}

void ConfigParser::advance() {
    if (_pos < _tokens.size() - 1)
        _pos++;
}

bool ConfigParser::atEnd() {
    return _tokens[_pos].type == TOKEN_EOF;
}

Token& ConfigParser::expect(TokenType type) {
    Token& tok = current();
    if (tok.type != type) {
        throw std::runtime_error(
            "ConfigParser: unexpected token '" + tok.value
            + "' at line " + itoa(tok.line));
    }
    advance();
    return tok;
}

std::string ConfigParser::parseValue() {
    Token& tok = current();
    if (tok.type != TOKEN_WORD)
        throw std::runtime_error(
            "ConfigParser: expected value at line "
            + itoa(tok.line));
    std::string val = tok.value;
    advance();
    return val;
}

std::vector<std::string> ConfigParser::parseValues() {
    std::vector<std::string> vals;
    while (current().type == TOKEN_WORD)
        vals.push_back(parseValue());
    return vals;
}

size_t ConfigParser::parseSize(const std::string& value) {
    if (value.empty())
        throw std::runtime_error("ConfigParser: empty size value");

    size_t multiplier = 1;
    std::string number = value;
    char suffix = value[value.size() - 1];

    if (suffix == 'b' || suffix == 'B') { multiplier = 1;                number = value.substr(0, value.size() - 1); }
    if (suffix == 'k' || suffix == 'K') { multiplier = 1024;            number = value.substr(0, value.size() - 1); }
    if (suffix == 'm' || suffix == 'M') { multiplier = 1024 * 1024;     number = value.substr(0, value.size() - 1); }
    if (suffix == 'g' || suffix == 'G') { multiplier = 1024 * 1024 * 1024; number = value.substr(0, value.size() - 1); }

    size_t n = static_cast<size_t>(std::atol(number.c_str()));
    return n * multiplier;
}

std::string ConfigParser::resolvePath(const std::string& path) const {
    if (path.empty() || path[0] == '/')
        return path;
    return _configDir + "/" + path;
}

// ─── SERVER PARSER ────────────────────────────────────────────────────────────

ServerConfig ConfigParser::parseServer() {
    ServerConfig srv;
    expect(TOKEN_OPEN_BRACE);

    while (current().type != TOKEN_CLOSE_BRACE && !atEnd()) {
        std::string key = parseValue();

        if (key == "listen") {
            srv.port = std::atoi(parseValue().c_str());
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "host") {
            srv.host = parseValue();
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "client_max_body_size") {
            srv.client_max_body_size = parseSize(parseValue());
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "error_page") {
            int code = std::atoi(parseValue().c_str());
            std::string path = resolvePath(parseValue());
            srv.error_pages[code] = path;
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "location") {
            std::string loc_path = parseValue();
            LocationConfig loc = parseLocation();
            loc.path = loc_path;
            srv.locations.push_back(loc);
        }
        else {
            throw std::runtime_error(
                "ConfigParser: unknown server directive '"
                + key + "' at line "
                + itoa(current().line));
        }
    }

    expect(TOKEN_CLOSE_BRACE);
    return srv;
}

// ─── LOCATION PARSER ──────────────────────────────────────────────────────────

LocationConfig ConfigParser::parseLocation() {
    LocationConfig loc;
    expect(TOKEN_OPEN_BRACE);

    while (current().type != TOKEN_CLOSE_BRACE && !atEnd()) {
        std::string key = parseValue();

        if (key == "methods") {
            loc.methods = parseValues();
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "root") {
            loc.root = resolvePath(parseValue());
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "index") {
            loc.index = parseValue();
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "directory_listing") {
            std::string val = parseValue();
            loc.directory_listing = (val == "on");
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "upload_store") {
            loc.upload_store = resolvePath(parseValue());
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "redirect") {
            loc.redirect = parseValue();
            expect(TOKEN_SEMICOLON);
        }
        else if (key == "cgi_pass") {
            std::string ext        = parseValue();
            std::string interpreter = parseValue();
            loc.cgi_pass[ext]      = interpreter;
            expect(TOKEN_SEMICOLON);
        }
        else {
            throw std::runtime_error(
                "ConfigParser: unknown location directive '"
                + key + "' at line "
                + itoa(current().line));
        }
    }

    expect(TOKEN_CLOSE_BRACE);
    return loc;
}

// ─── VALIDATION ───────────────────────────────────────────────────────────────

void ConfigParser::validateServer(const ServerConfig& srv) {
    if (srv.port < 1 || srv.port > 65535)
        throw std::runtime_error(
            "ConfigParser: invalid port " + itoa(srv.port));

    if (srv.locations.empty())
        throw std::runtime_error(
            "ConfigParser: server on port "
            + itoa(srv.port)
            + " has no location blocks");

    for (size_t i = 0; i < srv.locations.size(); i++)
        validateLocation(srv.locations[i]);
}

void ConfigParser::validateLocation(const LocationConfig& loc) {
    const std::string valid_methods[] = {"GET", "POST", "DELETE", "HEAD"};
    for (size_t i = 0; i < loc.methods.size(); i++) {
        bool found = false;
        for (int j = 0; j < 4; j++) {
            if (loc.methods[i] == valid_methods[j]) {
                found = true;
                break;
            }
        }
        if (!found)
            throw std::runtime_error(
                "ConfigParser: invalid method '"
                + loc.methods[i] + "' in location " + loc.path);
    }
}