#include "Req.hpp"
#include <cstddef>
#include <cstdlib>

Req::Req(){
	status_code = 200;
	_state = 0;
	_bytes_consumed = 0;
};

Req::Req(std::string raw_data)
{
	_raw_data = raw_data;
	status_code = 0;
	_state = 0;
	index = 0;
	_bytes_consumed = 0;
	size_t first = raw_data.find("\r\n");
	if (first != std::string::npos)
		this->parse_first_line();
}

Req::~Req(){}; 
Req::Req(const Req &other):_method(other._method),_req_uri(other._req_uri),_http_vrs(other._http_vrs),_Headers(other._Headers),_body(other._body),_raw_data(other._raw_data),status_code(other.status_code),index(other.index),_state(other._state),_bytes_consumed(other._bytes_consumed){}; 

Req &Req::operator=(const Req &other)
{
	if (this != &other)
	{
		_method = other._method;
		_req_uri = other._req_uri;
		_http_vrs = other._http_vrs;
		_Headers = other._Headers;
		_body = other._body;
		_raw_data = other._raw_data;
		_state = other._state;
		status_code = other.status_code;
		_cookies = other._cookies;
		_bytes_consumed = other._bytes_consumed;
	}
	return *this;
};

std::string lower(std::string hi)
{
	for (size_t i = 0; i < hi.size(); i++)
	{
		if (hi[i] >= 'A' && hi[i] <= 'Z')
			hi[i] = hi[i] + 32;
	}
	return hi;
}

std::string Req::get_header(std::string header_name)
{
	return (_Headers[header_name]);
}


Req &Req::parse_headers()
{
    int reading_value = 0;
    std::string Header_key;
    std::string Header_value;

    size_t request_line_end = _raw_data.find("\r\n");
    if (request_line_end == std::string::npos)
        return *this;

    size_t pos = request_line_end + 2;

	for (size_t i = pos; i < _raw_data.length(); i++)
    {
		if ((_raw_data[i] == ' ' && Header_key.empty()) || _raw_data[i] == '\t') // skip spaces
            continue;
		else if (i + 1 < _raw_data.length() && _raw_data[i] == '\r' && _raw_data[i + 1] == '\n')
        {
            if (Header_key.empty())
			{  // blank line = end of headers
				_state = 2;
                break;
			}
            if (reading_value == 0)  // line had no colon
            {
                status_code = 400;
                return *this;
            }
            // trim trailing whitespace from value
            size_t end = Header_value.find_last_not_of(" \t"); // search until find another character not in set, return it's position
            if (end != std::string::npos)
                Header_value = Header_value.substr(0, end + 1);
            else
                Header_value.clear();
            Header_key = lower(Header_key);
            _Headers[Header_key] = Header_value;
            if (Header_key == "cookie")
                parse_cookies(Header_value);
            Header_key.clear();
            Header_value.clear();
            reading_value = 0;
            i += 1;
        }
		else if (_raw_data[i] == ':' && reading_value == 0 && !Header_key.empty())
            reading_value = 1;
		else if (reading_value == 1)
		{
			if (_raw_data[i] == ' ' && Header_value.empty())  // skip leading spaces
				continue;
			Header_value += _raw_data[i];
		}
		else
			Header_key += _raw_data[i];
    }

	// Only check Host requirement once all headers are fully parsed
	// (i.e. when the blank line has been seen, _state == 2).
	if (_state == 2 && status_code < 400)
	{
		if (_http_vrs == "HTTP/1.1" && _Headers.find("host") == _Headers.end())
			status_code = 400;
	}
    return *this;
}

void Req::parse()
{
    if (_state == 0)
        parse_first_line();   // sets _state = 1 if done
    if (_state == 1)
        parse_headers();      // sets _state = 2 if done
    if (_state == 2)
        parse_body();         // sets _state = 3 if done
}

void Req::feed(const std::string &data)
{
    _raw_data += data;
	parse();
}


std::string unchunk(std::string raw)
{
    std::string result;
	size_t i = 0;

    while (i < raw.length())
    {
        size_t crlf = raw.find("\r\n", i); // find end of size line
        if (crlf == std::string::npos)
            break;
        std::string size_str = raw.substr(i, crlf - i);
		long size = strtol(size_str.c_str(), NULL, 16);
        if (size == 0)
            break;
		size_t data_start = crlf + 2;
        if (data_start + size > raw.length())
            return result;  // incomplete chunk — wait for more bytes
		result += raw.substr(data_start , size);
        i = data_start + size + 2; // jump to next chunk: past data + \r\n
    }
    return result;
}

Req &Req::parse_body()
{
	std::map<std::string,std::string>::const_iterator it = _Headers.find("content-length");
	std::map<std::string,std::string>::const_iterator it1 = _Headers.find("transfer-encoding");
	if (it == _Headers.end() && it1 == _Headers.end())
	{
		size_t hdr = _raw_data.find("\r\n\r\n");
		if (hdr != std::string::npos)
			_bytes_consumed = hdr + 4;
		_state = 3;
		return *this;
	}

	// RFC 7230 §3.3.3: Transfer-Encoding takes precedence over Content-Length
	if (it1 != _Headers.end())
	{
		std::string te = lower(_Headers["transfer-encoding"]);
		if (te == "chunked")
		{
			size_t sep = _raw_data.find("\r\n\r\n");
			if (sep == std::string::npos)
				return *this;
			size_t body_start = sep + 4;
			std::string raw_chunked = _raw_data.substr(body_start);
			_body = unchunk(raw_chunked);
			size_t terminator = raw_chunked.find("0\r\n\r\n");
			if (terminator == std::string::npos)
				return *this;
			_bytes_consumed = body_start + terminator + 5;
			_state = 3;
			return *this;
		}
	}

	// Fallback to Content-Length if TE is absent or not chunked
	if (it != _Headers.end())
	{
		std::string sum = _Headers["content-length"];
		long number = strtol(sum.c_str(), NULL, 10);
		if (number == 0)
		{
			size_t hdr = _raw_data.find("\r\n\r\n");
			if (hdr != std::string::npos)
				_bytes_consumed = hdr + 4;
			_state = 3;
			return *this;
		}
		size_t body_line = _raw_data.find("\r\n\r\n");
		if (body_line == std::string::npos)
			return *this;
		body_line += 4;
		if (_raw_data.length() < body_line + (size_t)number)
			return *this;
		_body = _raw_data.substr(body_line, number);
		_bytes_consumed = body_line + number;
		_state = 3;
	}

	return *this;
}

Req &Req::parse_first_line()
{
	std::string methods[] = {"GET","POST","DELETE","HEAD"};
	std::string invalid_uri_chars = "<>\"{}|\\^`";
	int cmp[3] = {0,0,0};
	int j = 0;

	// fill in methode + req uri + http version
	for(size_t i = 0; i < _raw_data.length() ;i++)
	{
		if (j == 0)
		{
			_method += _raw_data[i];
			index = i;
			if (i + 1 < _raw_data.length() && _raw_data[i] && _raw_data[i + 1] == ' ')
				j++;
		}
		else if (_raw_data[i] != ' ' && j == 1)
		{
			_req_uri += _raw_data[i];
			index = i;
			if (i + 1 < _raw_data.length() && _raw_data[i] && _raw_data[i + 1] == ' ')
				j++;
		}
		else if (j == 2)
		{
			if (i + 1 < _raw_data.length() && _raw_data[i] == '\r' && _raw_data[i+1] == '\n')
				break;
			if (_raw_data[i] && _raw_data[i] != ' ')
			{	
				_http_vrs += _raw_data[i];
				index = i+1;
				if (i + 1 < _raw_data.length() && _raw_data[i] && _raw_data[i + 1] == ' ')
					j++;
			}
		}
	}
	// compare methods
	for (int i = 0; i < 4; i++)
	{
		if (_method.compare(methods[i]) == 0)
			cmp[0] = 1;
	}
	// see if uri is valid
	if (_req_uri.empty() || _req_uri[0] != '/')
		cmp[1] = 0;
	else
	{
		cmp[1] = 1;
		for (size_t i = 0; i < _req_uri.length(); i++)
		{
			if (invalid_uri_chars.find(_req_uri[i]) != std::string::npos)
			{
				cmp[1] = 0;
				break;
			}
		}
	}
	// check the http version ?
	// http 1.1-1.0 valid else not
	
	if (!_http_vrs.compare("HTTP/1.1") || !_http_vrs.compare("HTTP/1.0"))
		cmp[2] = 1;
	else
	{
		if (_http_vrs.find("HTTP/") == 0)
		{
			for (size_t i = _http_vrs.find("HTTP/") ; i < _http_vrs.size(); i++)
			{
				if (std::isalnum((unsigned char)_http_vrs[i]) || _http_vrs[i] == '.')
					status_code = 505;
			}
		}
		else	
			status_code = 400;
	}
	if (_req_uri.size() > 8000)
		status_code = 414;
	else if (cmp[0] && cmp[1] && cmp[2])
		status_code = 200;
	else if (cmp[2])
		status_code = 400;
	_state = 1;
	return *this;
}

void Req::clear()
{
	std::string leftover;
	if (_state == 3 && _bytes_consumed > 0 && _bytes_consumed < _raw_data.size())
		leftover = _raw_data.substr(_bytes_consumed);

	_method.clear();
	_req_uri.clear();
	_http_vrs.clear();
	_Headers.clear();
	_body.clear();
	_raw_data.clear();
	_cookies.clear();
	status_code = 200;
	_state = 0;
	index = 0;
	_bytes_consumed = 0;

	if (!leftover.empty())
	{
		_raw_data = leftover;
		parse();
	}
}

bool Req::hasPendingData() const
{
	return !_raw_data.empty() && _state > 0;
}

int Req::isComplete() const
{
    return _state;
}

std::string Req::get_body()
{
	return _body;
}

int Req::get_header_count()
{
	return _Headers.size();
}

int Req::get_status_code()
{
	return status_code;
}

std::string Req::get_cookie(const std::string &name) const
{
	std::map<std::string, std::string>::const_iterator it = _cookies.find(name);
	if (it == _cookies.end())
		return "";
	return it->second;
}

void Req::parse_cookies(const std::string &raw)
{
	size_t pos = 0;
	while (pos < raw.size())
	{
		while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t'))
			++pos;
		size_t eq = raw.find('=', pos);
		if (eq == std::string::npos)
			break;
		std::string name = raw.substr(pos, eq - pos);
		pos = eq + 1;
		size_t semi = raw.find(';', pos);
		std::string value = raw.substr(pos, semi - pos);
		if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
			value = value.substr(1, value.size() - 2);
		_cookies[name] = value;
		if (semi == std::string::npos)
			break;
		pos = semi + 1;
	}
}

std::string Req::getMethod() const
{
	return _method;
}

std::string Req::getUri() const{
	return _req_uri;
}