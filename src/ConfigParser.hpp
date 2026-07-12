#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include "Config.hpp"

enum TokenType {
    TOKEN_WORD,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_EOF
};

struct Token {
    TokenType   type;
    std::string value;
    int         line;

    Token(TokenType t, const std::string& v, int l)
        : type(t), value(v), line(l) {}
};

class ConfigParser {
public:
    ConfigParser();
    ConfigParser(const ConfigParser& other);
    ConfigParser& operator=(const ConfigParser& other);
    ~ConfigParser();

    std::vector<ServerConfig> parse(const std::string& filename);

private:
    std::vector<Token>      _tokens;
    size_t                  _pos;
    std::string             _filename;
    std::string             _configDir;

    // tokenizer
    void                    tokenize(const std::string& content);
    bool                    isSpecial(char c);

    // parser helpers
    Token&                  current();
    Token&                  expect(TokenType type);
    void                    advance();
    bool                    atEnd();

    // parsers
    ServerConfig            parseServer();
    LocationConfig          parseLocation();
    std::string             parseValue();
    std::vector<std::string> parseValues();
    size_t                  parseSize(const std::string& value);

    // path resolution
    std::string             resolvePath(const std::string& path) const;

    // validation
    void                    validateServer(const ServerConfig& srv);
    void                    validateLocation(const LocationConfig& loc);
};

#endif