#pragma once
#include <string_view>
#include <iostream>

#include "stc.hpp"

class IConsoleMessage {
public:
    struct rgb { int r, g, b; };
    IConsoleMessage() = default;
    IConsoleMessage(rgb col) : Color(col) { }
    virtual ~IConsoleMessage() = default;
    virtual void operator()(std::string_view format, ...);
    rgb Color {255, 255, 255};
protected:
    virtual void _AcceptOstreamManips(std::ostream& (*f)(std::ostream &));
    virtual void _AcceptIosManips    (std::ostream& (*f)(std::ios &));
    virtual void _AcceptIosBaseManips(std::ostream& (*f)(std::ios_base &));
    friend IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ostream &));
    friend IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ios &));
    friend IConsoleMessage& operator<< (IConsoleMessage &s, std::ostream& (*f)(std::ios_base &));
};

template <typename T>
IConsoleMessage& operator<< (IConsoleMessage &s, const T &x) {
  std::cout << x;
  return s;
}

struct CConsoleInfoMsg : public IConsoleMessage {
    using IConsoleMessage::IConsoleMessage;
    void operator()(std::string_view format, ...) override;
};

template <typename T>
CConsoleInfoMsg& operator<< (CConsoleInfoMsg &s, const T &x) {
  std::cout << stc::rgb_fg(s.Color.r, s.Color.g, s.Color.b) << x << stc::reset;
  return s;
}

struct CConsoleErrorMsg : public IConsoleMessage {
    using IConsoleMessage::IConsoleMessage;
    void operator()(std::string_view format, ...) override;
};

template <typename T>
CConsoleErrorMsg& operator<< (CConsoleErrorMsg &s, const T &x) {
  std::cout << stc::rgb_fg(s.Color.r, s.Color.g, s.Color.b) << x << stc::reset;
  return s;
}

extern CConsoleInfoMsg Msg;
extern CConsoleErrorMsg Error;
