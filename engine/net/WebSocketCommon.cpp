#include "net/WebSocketCommon.h"

#include "utils/Base64.h"

#include <sha1.h>

#include <cctype>
#include <stdexcept>

HTTPResponse::HTTPResponse(StatusLine status_line, HTTPHeaders headers) : status_line(status_line), headers(headers) {
}

HTTPResponse::StatusLine HTTPResponse::get_status() const {
    return status_line;
}

std::string HTTPResponse::get_header(std::string header) const {
    auto it = headers.find(header);
    if (it != headers.end())
        return it->second;
    return "";
}

namespace ws {

std::string trim(const std::string& str) {
    if (str.empty())
        return str;

    std::size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
        ++start;

    std::size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
        --end;

    return str.substr(start, end - start);
}

std::string collapse_lws(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    bool in_whitespace = false;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_whitespace = true;
        }
        else {
            if (in_whitespace) {
                result.push_back(' ');
                in_whitespace = false;
            }
            result.push_back(c);
        }
    }

    return result;
}

std::pair<std::string, std::string> parse_http_header(const std::string& header) {
    auto colonPos = header.find(':');
    if (colonPos == std::string::npos)
        return std::make_pair(trim(header), std::string{});

    std::string fieldName = trim(header.substr(0, colonPos));
    std::string fieldValue;
    if (colonPos + 1 < header.size())
        fieldValue = header.substr(colonPos + 1);
    fieldValue = trim(fieldValue);
    fieldValue = collapse_lws(fieldValue);

    return std::make_pair(fieldName, fieldValue);
}

HTTPResponse::StatusLine parse_status_line(const std::string& line) {
    std::size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        throw std::runtime_error("Invalid status line: missing space after HTTP version.");

    std::string httpVersion = line.substr(0, firstSpace);

    std::size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        throw std::runtime_error("Invalid status line: missing space after status code.");

    std::string statusCodeStr = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));
    std::string reasonPhrase = line.substr(secondSpace + 1);

    if (httpVersion.size() < 5 || httpVersion.compare(0, 5, "HTTP/") != 0)
        throw std::runtime_error("Invalid HTTP version: must start with 'HTTP/'.");

    int statusCode;
    try {
        statusCode = std::stoi(statusCodeStr);
    }
    catch (...) {
        throw std::runtime_error("Invalid status code: not an integer.");
    }

    if (statusCode < 100 || statusCode > 599)
        throw std::runtime_error("Invalid status code: out of HTTP standard range 100-599.");

    if (reasonPhrase.empty())
        throw std::runtime_error("Empty reason phrase.");

    return HTTPResponse::StatusLine{httpVersion, statusCode, reasonPhrase};
}

HTTPRequestLine parse_request_line(const std::string& line) {
    std::size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos)
        throw std::runtime_error("Invalid request line: missing space after method.");

    std::size_t secondSpace = line.find(' ', firstSpace + 1);
    if (secondSpace == std::string::npos)
        throw std::runtime_error("Invalid request line: missing space after URI.");

    std::string method = line.substr(0, firstSpace);
    std::string uri = line.substr(firstSpace + 1, secondSpace - (firstSpace + 1));
    std::string httpVersion = line.substr(secondSpace + 1);

    return HTTPRequestLine{method, uri, httpVersion};
}

bool case_insensitive_equal(const std::string& a, const std::string& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

std::vector<std::string> get_lines(std::span<uint8_t> input, std::vector<uint8_t>& remainder) {
    // Prepend any leftover bytes from the previous call
    std::vector<uint8_t> buf;
    buf.reserve(remainder.size() + input.size());
    buf.insert(buf.end(), remainder.begin(), remainder.end());
    buf.insert(buf.end(), input.begin(), input.end());
    remainder.clear();

    std::vector<std::string> lines;
    static const uint8_t DELIMITER[] = {'\r', '\n'};

    size_t start = 0;
    while (start < buf.size()) {
        auto it = std::search(buf.begin() + static_cast<std::ptrdiff_t>(start), buf.end(), std::begin(DELIMITER),
                              std::end(DELIMITER));

        if (it == buf.end()) {
            remainder.assign(buf.begin() + static_cast<std::ptrdiff_t>(start), buf.end());
            break;
        }
        else {
            size_t foundPos = static_cast<size_t>(std::distance(buf.begin(), it));
            lines.emplace_back(buf.begin() + static_cast<std::ptrdiff_t>(start),
                               buf.begin() + static_cast<std::ptrdiff_t>(foundPos));
            start = foundPos + 2; // skip CRLF
        }
    }

    return lines;
}

std::string compute_accept_key(const std::string& client_key_b64) {
    static const std::string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concat = client_key_b64 + magic_string;

    SHA1 sha;
    sha.update(concat);
    const std::string hashstr = sha.final();
    const std::vector<uint8_t> hash(hashstr.begin(), hashstr.end());

    return Base64::encode(hash);
}

} // namespace ws
