#ifndef SHARED_H
#define SHARED_H

#include <SFML/Network.hpp>
#include <unordered_map>

#ifdef WIN32
#include <Windows.h>
#endif

enum class Type { Message, ServerMessage, ServerIsFull, ServerConnected, ServerPasswordNeeded, Kick, Connection, Disconnection, Password, Promotion,
                  SomebodyPromotion, ServerExit};

enum class ClientType { Normie = 0, Administrator };

enum class Color {Red, Green, Blue, Yellow, White, Default};

struct ClientData{
    ClientData() : m_type(ClientType::Normie) {}
    sf::TcpSocket m_socket;
    std::string m_name;
    ClientType m_type;
};

template <class T>
sf::Packet& operator <<(sf::Packet& packet, const T& m)
{
    return packet << static_cast<sf::Uint16>(m);
}

template <class T>
sf::Packet& operator >>(sf::Packet& packet, T& m)
{
    sf::Uint16 t;
    packet >> t;
    m = static_cast<T>(t);
    return packet;
}

class ColorChanger{
public:
    ColorChanger() : m_color(Color::Default) {}
#ifdef WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    void setConsoleTextColor(const Color &l_color)
    {
        m_color = l_color;
    #ifdef WIN32
        if(l_color == Color::Red)
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        else if(l_color == Color::Green)
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if(l_color == Color::Default)
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
        else if (l_color == Color::Yellow)
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (l_color == Color::Blue)
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (l_color == Color::White)
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    #endif
    }

    Color m_color;
};


class Shared{
public:
    Shared()
    {
        m_names[ClientType::Normie] = "User";
        m_names[ClientType::Administrator] = "Administrator";
    }
    std::string getNameFor(const ClientType& l_type)
    {
        auto itr = m_names.find(l_type);
        if(itr == m_names.end()){
            return "";
        }
        return itr->second;
    }

    std::unordered_map<ClientType, std::string> m_names;
};
#endif // SHARED_H
