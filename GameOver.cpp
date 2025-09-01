#include "GameOver.hpp"
#include "GameState.hpp"
#include <optional>

bool GameOver::loadFont(){

    if(!font_.getInfo().family.empty()) return true;
    return font_.openFromFile("AmericanCaptain.otf");

}

GameState GameOver::show(sf::RenderWindow& window){

    if(!loadFont()) return GameState::Start;

    sf::Text header(font_, "GAME OVER", 64);
    header.setFillColor(sf::Color::Red);
    header.setPosition({120.f, 120.f});

    sf::Text msg(font_, "\n\nPress R to restart\nB to go back to menu", 28);
    msg.setFillColor(sf::Color::Red);
    msg.setPosition({120.f, 220.f});

    while(window.isOpen()){

        while (const std::optional<sf::Event> e = window.pollEvent()) {
            if (e->is<sf::Event::Closed>()) {
                window.close();                   
            } else if (e->is<sf::Event::KeyPressed>()) {
                const auto* k = e->getIf<sf::Event::KeyPressed>();
                if (k) {
                    if (k->code == sf::Keyboard::Key::R){
                    return GameState::Playing;}   
                    if (k->code == sf::Keyboard::Key::B){
                        return GameState::Start;
                    }  
                }
            }
        }

        window.clear();  
        window.draw(header);
        window.draw(msg);
        window.display();

    }
    return GameState::Start;
}