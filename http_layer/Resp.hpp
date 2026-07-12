#pragma once

#include <string>
#include <map>
#include <vector>

class Req;

class Resp{
	private:
		int	_status_codes;
		std::map<std::string,std::string> _headers;
		std::string _body;
		std::vector<std::string> _cookies;
	public:
		static std::string reason_phrase(int status);
		Resp();
		~Resp();
		Resp(const Resp &other);
		Resp &operator=(Resp &other);
		Resp &override_status(int status);
		Resp &set_body(const std::string &body);
		Resp &set_headers(const std::string &key, const std::string &value);
		Resp &set_cookie(const std::string &name, const std::string &value,
		                 int max_age = -1, const std::string &path = "/",
		                 bool http_only = false, bool secure = false,
		                 const std::string &same_site = "");
		std::string build(const Req &req) const;
};