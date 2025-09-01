#include "StartScreen.hpp"
#include "GameState.hpp"
#include <optional>

bool StartScreen::loadFont(){

    if(!font_.getInfo().family.empty()) return true;
    return font_.openFromFile("AmericanCaptain.otf");

}

bool StartScreen::show(sf::RenderWindow& window){

    if(!loadFont()) return false;

    sf::Text header(font_, "Welcome to Elliot Dash!", 64);
    header.setFillColor(sf::Color::White);
    header.setPosition({120.f, 120.f});

    sf::Text msg(font_, "\n\nPress SPACE to start", 28);
    msg.setFillColor(sf::Color::White);
    msg.setPosition({120.f, 220.f});

    while(window.isOpen()){

        while (const std::optional<sf::Event> e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                window.close();
                return false;
            } else if (e->is<sf::Event::KeyPressed>()) {
                const auto* k = e->getIf<sf::Event::KeyPressed>();
                if (k) {
                    if (k->code == sf::Keyboard::Key::Space){
                        return true;
                    }   // restart
                }
            }
        }

        window.clear();   // dark background
        window.draw(header);
        window.draw(msg);
        window.display();

    }
    return false;
}