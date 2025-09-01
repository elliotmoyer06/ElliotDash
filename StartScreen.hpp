#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

class StartScreen{

public:
    bool show(sf::RenderWindow& window);

private:
    bool loadFont();
    sf::Font font_;

};