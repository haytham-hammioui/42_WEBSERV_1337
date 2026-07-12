#pragma once

#include <iostream>
#include <string>
#include <map>
#include <exception>
#include <cctype>

class Req{
	private:
		std::string _method;
		std::string _req_uri;
		std::string _http_vrs;
		std::map<std::string,std::string> _Headers;
		std::string _body;
		std::string _raw_data;
		int			status_code;
		int			index;
		int			_state;
		std::map<std::string, std::string> _cookies;
		size_t _bytes_consumed;
		void parse_cookies(const std::string &raw);
	public:
		std::string first_line;
		std::string headers;
		std::string body;
		Req();
		~Req();
		Req(const Req &other);
		Req(std::string raw_data);
		Req &operator=(const Req &other);
		Req &parse_first_line();
		Req &parse_headers();
		Req &parse_body();

		int isComplete() const;
		int get_status_code();
		std::string get_header(std::string header_name);
		int	get_header_count();
		std::string	get_body();
		std::string get_cookie(const std::string &name) const;

		bool hasPendingData() const;
		void feed(const std::string &data);
		std::string getMethod() const;
		std::string getUri() const;
		void parse();
		void clear();
};