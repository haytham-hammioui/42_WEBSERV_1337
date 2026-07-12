
#include "Resp.hpp"
#include "Req.hpp"

#include <cstdlib>
#include <sstream>

Resp::Resp()
	: _status_codes(200), _headers(), _body(), _cookies() {}

Resp::~Resp() {}

Resp::Resp(const Resp &other)
	: _status_codes(other._status_codes), _headers(other._headers), _body(other._body), _cookies(other._cookies) {}

Resp &Resp::operator=(Resp &other)
{
	if (this != &other)
	{
		_status_codes = other._status_codes;
		_headers = other._headers;
		_body = other._body;
		_cookies = other._cookies;
	}
	return *this;
}

Resp &Resp::override_status(int status)
{
	_status_codes = status;
	return *this;
}

Resp &Resp::set_body(const std::string &body)
{
	_body = body;
	return *this;
}

Resp &Resp::set_headers(const std::string &key, const std::string &value)
{
	_headers[key] = value;
	return *this;
}

std::string Resp::reason_phrase(int status)
{
	if (status == 200)
		return "OK";
	if (status == 201)
		return "Created";
	if (status == 204)
		return "No Content";
	if (status == 301)
		return "Moved Permanently";
	if (status == 302)
		return "Found";
	if (status == 400)
		return "Bad Request";
	if (status == 403)
		return "Forbidden";
	if (status == 404)
		return "Not Found";
	if (status == 405)
		return "Method Not Allowed";
	if (status == 413)
		return "Payload Too Large";
	if (status == 500)
		return "Internal Server Error";
	if (status == 505)
		return "HTTP Version Not Supported";
	return "OK";
}

Resp &Resp::set_cookie(const std::string &name, const std::string &value,
                       int max_age, const std::string &path,
                       bool http_only, bool secure,
                       const std::string &same_site)
{
	std::ostringstream oss;
	oss << name << "=" << value;
	if (max_age >= 0)
		oss << "; Max-Age=" << max_age;
	if (!path.empty())
		oss << "; Path=" << path;
	if (secure)
		oss << "; Secure";
	if (http_only)
		oss << "; HttpOnly";
	if (!same_site.empty())
		oss << "; SameSite=" << same_site;
	_cookies.push_back(oss.str());
	return *this;
}

std::string Resp::build(const Req &req) const
{
	(void)req;
	std::ostringstream out;
	out << "HTTP/1.1 " << _status_codes << " " << reason_phrase(_status_codes) << "\r\n";
	if (_headers.find("Content-Length") == _headers.end())
		out << "Content-Length: " << _body.size() << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
		out << it->first << ": " << it->second << "\r\n";
	for (std::vector<std::string>::const_iterator it = _cookies.begin(); it != _cookies.end(); ++it)
		out << "Set-Cookie: " << *it << "\r\n";
	out << "\r\n";
	out << _body;
	return out.str();
}
